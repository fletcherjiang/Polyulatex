/**
* @file queue_process_ccpu.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "queue_process_ccpu.h"
#include "log_inner.h"
#include "runtime/mem.h"
#include "runtime/rt_mem_queue.h"
#include "runtime/dev.h"
#include "aicpu/queue_schedule/qs_client.h"

namespace acl {
    aclError QueueProcessorCcpu::acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *qid)
    {
        ACL_LOG_INFO("Start to acltdtCreateQueue");
        ACL_REQUIRES_NOT_NULL(qid);
        ACL_REQUIRES_NOT_NULL(attr);
        int32_t deviceId = 0;
        static bool isQueueIint = false;
        std::lock_guard<std::recursive_mutex> lock(muForQueueCtrl_);
        if (!isQueueIint) {
            ACL_LOG_INFO("need to init queue once");
            ACL_REQUIRES_CALL_RTS_OK(rtMemQueueInit(deviceId), rtMemQueueInit);
            isQueueIint = true;
        }
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueCreate(deviceId, attr, qid), rtMemQueueCreate);
        ACL_LOG_INFO("Successfully to execute acltdtCreateQueue, qid is %u", *qid);
        return ACL_SUCCESS;
    }

    aclError QueueProcessorCcpu::acltdtDestroyQueue(uint32_t qid)
    {
        return acltdtDestroyQueueOndevice(qid);
    }

    aclError QueueProcessorCcpu::acltdtEnqueue(uint32_t qid, acltdtBuf buf, int32_t timeout)
    {
        return acltdtEnqueueOnDevice(qid, buf, timeout);
    }

    aclError QueueProcessorCcpu::acltdtDequeue(uint32_t qid, acltdtBuf *buf, int32_t timeout)
    {
        return acltdtDequeueOnDevice(qid, buf, timeout);
    }

    aclError QueueProcessorCcpu::acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t permission, int32_t timeout)
    {
        ACL_LOG_INFO("start to acltdtGrantQueue, qid is %u, pid is %d, permisiion is %u, timeout is %d",
                     qid, pid, permission, timeout);
        int32_t deviceId = 0;
        // check self group
        size_t selfGrpNum = 0;
        std::string selfGrpName;
        int32_t selfPid = mmGetPid();
        if (grpName_.empty()) {
            ACL_LOG_INFO("self create grp name is empty, need to query");
            ACL_REQUIRES_OK(QueryGroup(selfPid, selfGrpNum, selfGrpName));
        }
        ACL_LOG_INFO("query self pid %d success, num is %zu, grp name is %s", selfPid, selfGrpNum, selfGrpName.c_str());
        // check dst group status
        size_t dstGrpNum = 0;
        std::string dstGrpName;
        int32_t dstPid = pid;
        ACL_REQUIRES_OK(QueryGroup(dstPid, dstGrpNum, dstGrpName));
        ACL_LOG_INFO("query dst pid %d success, num is %zu, grp name is %s", dstPid, dstGrpNum, dstGrpName.c_str());
        if (selfGrpNum == 0) {
            if (dstGrpNum > 0) {
                ACL_LOG_INNER_ERROR("Dst pid [%d] already has a group %s", pid, dstGrpName.c_str());
                return ACL_ERROR_FAILURE;
            } else {
                ACL_LOG_INFO("need to create group");
                rtMemGrpConfig_t grpConfig = {0};
                grpName_ = "acltdt" + std::to_string(pid);
                ACL_REQUIRES_CALL_RTS_OK(rtMemGrpCreate(grpName_.c_str(), &grpConfig), rtMemGrpCreate);
                rtMemGrpShareAttr_t attr = {0};
                attr.admin = 1;
                attr.read = 1;
                attr.write = 1;
                attr.alloc =1;
                ACL_REQUIRES_CALL_RTS_OK(rtMemGrpAddProc(grpName_.c_str(), pid, &attr), rtMemGrpAddProc);
            }
        } else {
            if (dstGrpNum == 0) {
                rtMemGrpShareAttr_t attr = {0};
                attr.admin = 1;
                attr.read = 1;
                attr.write = 1;
                attr.alloc =1;
                ACL_REQUIRES_CALL_RTS_OK(rtMemGrpAddProc(selfGrpName.c_str(), pid, &attr), rtMemGrpAddProc);
            } else {
                if (dstGrpName != selfGrpName) {
                    ACL_LOG_INNER_ERROR("Dst pid [%d] already has a group %s, is not same as self group name %s",
                                        pid, dstGrpName.c_str(), selfGrpName.c_str());
                    return ACL_ERROR_FAILURE;
                }
            }
        }
        rtMemQueueShareAttr_t attr = {0};
        attr.manage = permission & ACL_TDT_QUEUE_PERMISSION_MANAGE;
        attr.read = permission & ACL_TDT_QUEUE_PERMISSION_DEQUEUE;
        attr.write = permission & ACL_TDT_QUEUE_PERMISSION_ENQUEUE;
        std::lock_guard<std::recursive_mutex> lock(muForQueueCtrl_);
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueGrant(deviceId, qid, pid, &attr), rtMemQueueGrant);
        ACL_LOG_INFO("successfully execute acltdtGrantQueue, qid is %u, pid is %d, permisiion is %u, timeout is %d",
                     qid, pid, permission, timeout);
        return ACL_SUCCESS;
    }

    aclError QueueProcessorCcpu::acltdtAttachQueue(uint32_t qid, int32_t timeout, uint32_t *permission)
    {
        ACL_REQUIRES_NOT_NULL(permission);
        int32_t deviceId = 0;

        if (!grpName_.empty()) {
            ACL_LOG_INNER_ERROR("This process has already create group, master process can not be attached");
            return ACL_ERROR_FAILURE;
        }
        std::lock_guard<std::recursive_mutex> lock(muForQueueCtrl_);
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueAttach(deviceId, qid, timeout * MSEC_TO_USEC), rtMemQueueAttach);
        ACL_LOG_INFO("start to query qid %u permisiion", qid);
        rtMemQueueShareAttr_t attr = {0};
        ACL_REQUIRES_CALL_RTS_OK(GetQueuePermission(deviceId, qid, attr), GetQueuePermission);
        uint32_t tmp = 0;
        tmp = attr.manage ? (tmp | ACL_TDT_QUEUE_PERMISSION_MANAGE) : tmp;
        tmp = attr.read ? (tmp | ACL_TDT_QUEUE_PERMISSION_DEQUEUE) : tmp;
        tmp = attr.write ? (tmp | ACL_TDT_QUEUE_PERMISSION_ENQUEUE) : tmp;
        *permission = tmp;
        ACL_LOG_INFO("successfully execute to get queue %u permission %u", qid, *permission);
        return ACL_SUCCESS;
    }

    aclError QueueProcessorCcpu::acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList)
    {
        ACL_REQUIRES_NOT_NULL(qRouteList);
        ACL_LOG_INFO("Start to acltdtBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        // qs is thread mode, so no need to grant queue to qs
        int32_t deviceId = 0;
        ACL_REQUIRES_OK(InitQueueSchedule(deviceId));
        // get dst id
        int32_t dstPid = 0;
        ACL_REQUIRES_OK(GetDstInfo(deviceId, QS_PID, dstPid));
        rtEschedEventSummary_t eventSum = {0};
        rtEschedEventReply_t ack = {0};
        bqs::QsProcMsgRsp qsRsp = {0};
        eventSum.pid = dstPid;
        eventSum.grpId = bqs::BINDQUEUEGRPID;
        eventSum.eventId = RT_MQ_SCHED_EVENT_QS_MSG;
        eventSum.dstEngine = RT_MQ_DST_ENGINE_CCPU_DEVICE;
        ack.buf = reinterpret_cast<char *>(&qsRsp);
        ack.bufLen = sizeof(qsRsp);
        std::lock_guard<std::recursive_mutex> lock(muForQueueCtrl_);
        if (!isQsInit_) {
            ACL_REQUIRES_OK(SendConnectQsMsg(deviceId, eventSum, ack));
            isQsInit_ = true;
        }
        ACL_REQUIRES_OK(SendBindUnbindMsg(qRouteList, deviceId, true, true, eventSum, ack));
        ACL_LOG_INFO("Successfully to execute acltdtBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        return ACL_SUCCESS;
    }

    aclError QueueProcessorCcpu::acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList)
    {
        ACL_REQUIRES_NOT_NULL(qRouteList);
        ACL_LOG_INFO("Start to acltdtUnBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        int32_t deviceId = 0;
        // get dst id
        int32_t dstPid = 0;
        ACL_REQUIRES_OK(GetDstInfo(deviceId, QS_PID, dstPid));
        rtEschedEventSummary_t eventSum = {0};
        rtEschedEventReply_t ack = {0};
        bqs::QsProcMsgRsp qsRsp = {0};
        eventSum.pid = dstPid;
        eventSum.grpId = bqs::BINDQUEUEGRPID;
        eventSum.eventId = RT_MQ_SCHED_EVENT_QS_MSG;
        eventSum.dstEngine = RT_MQ_DST_ENGINE_CCPU_DEVICE;
        ack.buf = reinterpret_cast<char *>(&qsRsp);
        ack.bufLen = sizeof(qsRsp);
        std::lock_guard<std::recursive_mutex> lock(muForQueueCtrl_);
        ACL_REQUIRES_OK(SendBindUnbindMsg(qRouteList, deviceId, false, true, eventSum, ack));
        ACL_LOG_INFO("Successfully to execute acltdtUnBindQueueRoutes, queue route is %zu",
                     qRouteList->routeList.size());
        return ACL_SUCCESS;
    }

    aclError QueueProcessorCcpu::acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo,
                                                        acltdtQueueRouteList *qRouteList)
    {
        ACL_REQUIRES_NOT_NULL(queryInfo);
        ACL_REQUIRES_NOT_NULL(qRouteList);
        ACL_LOG_INFO("Start to acltdtQueryQueueRoutes");
        int32_t deviceId = 0;
        // get dst id
        int32_t dstPid = 0;
        ACL_REQUIRES_OK(GetDstInfo(deviceId, QS_PID, dstPid));
        rtEschedEventSummary_t eventSum = {0};
        rtEschedEventReply_t ack = {0};
        bqs::QsProcMsgRsp qsRsp = {0};
        eventSum.pid = dstPid;
        eventSum.grpId = bqs::BINDQUEUEGRPID;
        eventSum.eventId = RT_MQ_SCHED_EVENT_QS_MSG;
        eventSum.dstEngine = RT_MQ_DST_ENGINE_CCPU_DEVICE;
        ack.buf = reinterpret_cast<char *>(&qsRsp);
        ack.bufLen = sizeof(qsRsp);
        std::lock_guard<std::recursive_mutex> lock(muForQueueCtrl_);
        size_t routeNum = 0;
        ACL_REQUIRES_OK(GetQueueRouteNum(queryInfo, deviceId, eventSum, ack, routeNum));
        ACL_REQUIRES_OK(QueryQueueRoutes(queryInfo, deviceId, true, routeNum, eventSum, ack, qRouteList));
        return ACL_SUCCESS;
    }

    aclError QueueProcessorCcpu::QueryGroup(int32_t pid, size_t &grpNum, std::string &grpName) 
    {
        rtMemGrpQueryInput_t input = {0};
        input.grpQueryByProc.pid = pid;
        rtMemGrpQueryOutput_t output = {0};
        rtMemGrpOfProc_t outputInfo = {0};
        output.groupsOfProc = &outputInfo;
        output.maxNum = 1;
        int32_t cmd = RT_MEM_GRP_QUERY_GROUPS_OF_PROCESS;
        ACL_REQUIRES_CALL_RTS_OK(rtMemGrpQuery(cmd, &input, &output), rtMemGrpQuery);
        grpNum = output.resultNum;
        if (grpNum > 0) {
            grpName = std::string(output.groupsOfProc->groupName);
        }
        ACL_LOG_INFO("This proc [%d] has [%zu] group, name is %s", input.grpQueryByProc.pid, grpNum, grpName.c_str());
        return ACL_SUCCESS;
    }

    aclError QueueProcessorCcpu::CreateGroupIfNoGroup()
    {
        size_t grpNum = 0;
        std::string grpName;
        int32_t pid = mmGetPid();
        ACL_REQUIRES_OK(QueryGroup(pid, grpNum, grpName));
        if (grpNum == 0) {
            ACL_LOG_INFO("need to create group");
            rtMemGrpConfig_t grpConfig = {0};
            grpName_ = "acltdt" + std::to_string(pid);
            ACL_REQUIRES_CALL_RTS_OK(rtMemGrpCreate(grpName_.c_str(), &grpConfig), rtMemGrpCreate);
        }
        return ACL_SUCCESS;
    }

    aclError QueueProcessorCcpu::acltdtAllocBuf(size_t size, acltdtBuf *buf)
    {
        ACL_REQUIRES_OK(CreateGroupIfNoGroup());
        if (isMbufInit_) {
            ACL_REQUIRES_CALL_RTS_OK(rtMbufInit(nullptr), rtMbufInit);
            isMbufInit_ = true;
        }
        return acltdtAllocBufOnDevice(size, buf);
    }

    aclError QueueProcessorCcpu::acltdtFreeBuf(acltdtBuf buf)
    {
        return acltdtFreeBufOnDevice(buf);
    }

    aclError QueueProcessorCcpu::acltdtGetBufData(const acltdtBuf buf, void **dataPtr, size_t *size)
    {
        return acltdtGetBufDataOnDevice(buf, dataPtr, size);
    }
}