/**
* @file queue_process_mdc.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "queue_process_mdc.h"
#include "log_inner.h"
#include "runtime/mem.h"
#include "runtime/rt_mem_queue.h"
#include "runtime/dev.h"
#include "aicpu/queue_schedule/qs_client.h"

namespace acl {

    aclError QueueProcessorMdc::GrantQueue2Cp(int32_t deviceId, uint32_t qid)
    {
        int32_t cpPid;
        // if cp is found
        if (GetDstInfo(deviceId, CP_PID, cpPid) == ACL_SUCCESS) {
            rtMemQueueShareAttr_t permission = {0};
            ACL_REQUIRES_CALL_RTS_OK(GetQueuePermission(deviceId, qid, permission), GetQueuePermission);
            if (permission.manage) {
                ACL_REQUIRES_CALL_RTS_OK(rtMemQueueGrant(deviceId, qid, cpPid, &permission), rtMemQueueGrant);
            } else {
                ACL_LOG_INNER_ERROR("current process has no manage permission on qid %u", qid);
                return ACL_ERROR_FAILURE;// new ret code
            }
        }
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *qid)
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
        int32_t cpPid;
        if (GetDstInfo(deviceId, CP_PID, cpPid) == ACL_SUCCESS) {
            ACL_LOG_INFO("get cp pid %d", cpPid);
            rtMemQueueShareAttr_t attr = {0};
            attr.read = 1;
            attr.manage = 1;
            attr.write = 1;
            ACL_REQUIRES_CALL_RTS_OK(rtMemQueueGrant(deviceId, *qid, cpPid, &attr), rtMemQueueGrant);
        }
        ACL_LOG_INFO("Successfully to execute acltdtCreateQueue, qid is %u", *qid);
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtDestroyQueue(uint32_t qid)
    {
        return acltdtDestroyQueueOndevice(qid);
    }

    aclError QueueProcessorMdc::acltdtEnqueue(uint32_t qid, acltdtBuf buf, int32_t timeout)
    {
        return acltdtEnqueueOnDevice(qid, buf, timeout);
    }

    aclError QueueProcessorMdc::acltdtDequeue(uint32_t qid, acltdtBuf *buf, int32_t timeout)
    {
        return acltdtDequeueOnDevice(qid, buf, timeout);
    }

    aclError QueueProcessorMdc::acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t permission, int32_t timeout)
    {
        ACL_LOG_INFO("start to acltdtGrantQueue, qid is %u, pid is %d, permisiion is %u, timeout is %d",
                     qid, pid, permission, timeout);
        int32_t deviceId = 0;
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

    aclError QueueProcessorMdc::acltdtAttachQueue(uint32_t qid, int32_t timeout, uint32_t *permission)
    {
        ACL_LOG_INFO("start to acltdtGrantQueue, qid is %u, permisiion is %u, timeout is %d",
                     qid, *permission, timeout);
        ACL_REQUIRES_NOT_NULL(permission);
        int32_t deviceId = 0;
        std::lock_guard<std::recursive_mutex> lock(muForQueueCtrl_);
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueAttach(deviceId, qid, timeout), rtMemQueueAttach);
        (void)GrantQueue2Cp(deviceId, qid);
        rtMemQueueShareAttr_t attr = {0};
        ACL_REQUIRES_CALL_RTS_OK(GetQueuePermission(deviceId, qid, attr), GetQueuePermission);
        uint32_t tmp = 0;
        tmp = attr.manage ? (tmp | ACL_TDT_QUEUE_PERMISSION_MANAGE) : tmp;
        tmp = attr.read ? (tmp | ACL_TDT_QUEUE_PERMISSION_DEQUEUE) : tmp;
        tmp = attr.write ? (tmp | ACL_TDT_QUEUE_PERMISSION_ENQUEUE) : tmp;
        *permission = tmp;
        ACL_LOG_INFO("successfully execute acltdtGrantQueue, qid is %u, permisiion is %u, timeout is %d",
                     qid, *permission, timeout);
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList)
    {
        ACL_REQUIRES_NOT_NULL(qRouteList);
        ACL_LOG_INFO("Start to acltdtBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        int32_t deviceId = 0;
        // get dst id
        ACL_REQUIRES_OK(InitQueueSchedule(deviceId));
        int32_t dstPid;
        ACL_REQUIRES_OK(GetDstInfo(deviceId, QS_PID, dstPid));
        std::lock_guard<std::recursive_mutex> lock(muForQueueCtrl_);
        for (size_t i = 0; i < qRouteList->routeList.size(); ++i) {
            rtMemQueueShareAttr_t attrSrc = {0};
            attrSrc.read = 1;
            rtMemQueueShareAttr_t attrDst = {0};
            attrSrc.write = 1;
            ACL_REQUIRES_CALL_RTS_OK(rtMemQueueGrant(deviceId, qRouteList->routeList[i].srcId, dstPid, &attrSrc),
                                     rtMemQueueGrant);
            ACL_REQUIRES_CALL_RTS_OK(rtMemQueueGrant(deviceId, qRouteList->routeList[i].dstId, dstPid, &attrDst),
                                     rtMemQueueGrant);
        }
        rtEschedEventSummary_t eventSum = {0};
        rtEschedEventReply_t ack = {0};
        bqs::QsProcMsgRsp qsRsp = {0};
        eventSum.pid = dstPid;
        eventSum.grpId = bqs::BINDQUEUEGRPID;
        eventSum.eventId = 25; //qs EVENT_ID
        eventSum.dstEngine = RT_MQ_DST_ENGINE_CCPU_DEVICE;
        ack.buf = reinterpret_cast<char *>(&qsRsp);
        ack.bufLen = sizeof(qsRsp);
        if (!isQsInit_) {
            ACL_REQUIRES_OK(SendConnectQsMsg(deviceId, eventSum, ack));
            isQsInit_ = true;
        }
        ACL_REQUIRES_OK(SendBindUnbindMsg(qRouteList, deviceId, true, true, eventSum, ack));
        ACL_LOG_INFO("Successfully to execute acltdtBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList)
    {
        ACL_REQUIRES_NOT_NULL(qRouteList);
        ACL_LOG_INFO("Start to acltdtUnBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        int32_t deviceId = 0;
        // get dst id
        int32_t dstPid;
        ACL_REQUIRES_OK(GetDstInfo(deviceId, QS_PID, dstPid));
        rtEschedEventSummary_t eventSum = {0};
        rtEschedEventReply_t ack = {0};
        bqs::QsProcMsgRsp qsRsp = {0};
        eventSum.pid = dstPid;
        eventSum.grpId = bqs::BINDQUEUEGRPID;
        eventSum.eventId = 25; //drv EVENT_ID
        eventSum.dstEngine = RT_MQ_DST_ENGINE_CCPU_DEVICE;
        ack.buf = reinterpret_cast<char *>(&qsRsp);
        ack.bufLen = sizeof(qsRsp);
        std::lock_guard<std::recursive_mutex> lock(muForQueueCtrl_);
        ACL_REQUIRES_OK(SendBindUnbindMsg(qRouteList, deviceId, false, true, eventSum, ack));
        ACL_LOG_INFO("Successfully to execute acltdtUnBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo, acltdtQueueRouteList *qRouteList)
    {
        ACL_REQUIRES_NOT_NULL(queryInfo);
        ACL_REQUIRES_NOT_NULL(qRouteList);
        ACL_LOG_INFO("Start to acltdtQueryQueueRoutes");
        int32_t deviceId = 0;
        // get dst id
        int32_t dstPid;
        ACL_REQUIRES_OK(GetDstInfo(deviceId, QS_PID, dstPid));
        rtEschedEventSummary_t eventSum = {0};
        rtEschedEventReply_t ack = {0};
        bqs::QsProcMsgRsp qsRsp = {0};
        eventSum.pid = dstPid;
        eventSum.grpId = bqs::BINDQUEUEGRPID;
        eventSum.eventId = 25; //qs EVENT_ID
        eventSum.dstEngine = RT_MQ_DST_ENGINE_CCPU_DEVICE;
        ack.buf = reinterpret_cast<char *>(&qsRsp);
        ack.bufLen = sizeof(qsRsp);
        std::lock_guard<std::recursive_mutex> lock(muForQueueCtrl_);
        ACL_REQUIRES_OK(GetQueueRouteNum(queryInfo, deviceId, eventSum, ack));
        size_t routeNum = reinterpret_cast<bqs::QsProcMsgRsp *>(ack.buf)->retValue;
        ACL_REQUIRES_OK(QueryQueueRoutes(queryInfo, deviceId, true, routeNum, eventSum, ack, qRouteList));
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtAllocBuf(size_t size, acltdtBuf *buf)
    {
        return acltdtAllocBufOnDevice(size, buf);
    }

    aclError QueueProcessorMdc::acltdtFreeBuf(acltdtBuf buf)
    {
        return acltdtFreeBufOnDevice(buf);
    }

    aclError QueueProcessorMdc::acltdtGetBufData(const acltdtBuf buf, void **dataPtr, size_t *size)
    {
        return acltdtGetBufDataOnDevice(buf, dataPtr, size);
    }
}