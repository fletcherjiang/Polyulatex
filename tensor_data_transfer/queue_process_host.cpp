/**
* @file queue_process_host.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "queue_process_host.h"
#include "log_inner.h"
#include "runtime/mem.h"
#include "runtime/rt_mem_queue.h"
#include "runtime/dev.h"
#include "aicpu/queue_schedule/qs_client.h"

namespace acl {
    aclError QueueProcessorHost::acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *qid)
    {
        ACL_LOG_INFO("Start to acltdtCreateQueue");
        ACL_REQUIRES_NOT_NULL(qid);
        ACL_REQUIRES_NOT_NULL(attr);
        int32_t deviceId = 0;
        rtError_t rtRet = RT_ERROR_NONE;
        rtRet = rtGetDevice(&deviceId);
        if (rtRet != ACL_SUCCESS) {
            ACL_LOG_CALL_ERROR("[Get][DeviceId]fail to get deviceId result = %d", rtRet);
            return rtRet;
        }
        static bool isQueueIint = false;
        if (!isQueueIint) {
            ACL_LOG_INFO("need to init queue once");
            ACL_REQUIRES_CALL_RTS_OK(rtMemQueueInit(deviceId), rtMemQueueInit);
        }
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueCreate(deviceId, attr, qid), rtMemQueueCreate);
        ACL_LOG_INFO("Successfully to execute acltdtCreateQueue, qid is %u", *qid);
        return ACL_SUCCESS;
    }

    aclError QueueProcessorHost::acltdtDestroyQueue(uint32_t qid)
    {
        std::lock_guard<std::recursive_mutex> lock(muForQueueCtrl);
        int32_t deviceId = 0;
        GET_CURRENT_DEVICE_ID(deviceId);
        // get qs id
        pid_t qsPid;
        size_t routeNum = 0;
        if (GetDstInfo(deviceId, QS_PID, qsPid) == RT_ERROR_NONE) {
            rtEschedEventSummary_t eventSum = {0};
            rtEschedEventReply_t ack = {0};
            bqs::QsProcMsgRsp qsRsp = {0};
            pid_t cpPid;
            ACL_REQUIRES_OK(GetDstInfo(deviceId, CP_PID, cpPid));
            eventSum.pid = cpPid;
            eventSum.grpId = 0;
            eventSum.eventId = 222222; //DRV EVENT_ID
            eventSum.dstEngine = RT_MQ_DST_ENGINE_CCPU_DEVICE;
            ack.buf = reinterpret_cast<char *>(&qsRsp);
            ack.bufLen = sizeof(qsRsp);
            acltdtQueueRouteQueryInfo queryInfo = {bqs::BQS_QUERY_TYPE_SRC_OR_DST, qid, qid};
            ACL_REQUIRES_OK(GetQueueRouteNum(&queryInfo, deviceId, eventSum, ack));
            routeNum = reinterpret_cast<bqs::QsProcMsgRsp *>(ack.buf)->retValue;
        }
        if (routeNum > 0) {
            ACL_LOG_ERROR("qid [%u] can not be destroyed, it need to be unbinded first.", qid);
            return ACL_ERROR_FAILURE;// 需要新增错误码
        }
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueDestroy(deviceId, qid), rtMemQueueDestroy);
        return ACL_SUCCESS;
    }

    aclError QueueProcessorHost::acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t permission, int32_t timeout)
    {
        ACL_LOG_INFO("start to acltdtGrantQueue, qid is %u, pid is %d, permisiion is %u, timeout is %d",
                     qid, pid, permission, timeout);
        if (permission & 0x1) {
            ACL_LOG_ERROR("[CHECK][permission]permission manager is not allowed");
            return ACL_ERROR_INVALID_PARAM;
        };
        int32_t deviceId = 0;
        GET_CURRENT_DEVICE_ID(deviceId);
        uint64_t startTime = GetTimestamp();
        uint64_t endTime = 0;
        pid_t cpPid;
        rtBindHostpidInfo_t info = {0};
        info.hostPid = mmGetPid();
        info.cpType = RT_DEV_PROCESS_CP1;
        info.chipId = deviceId;
        do {
            if (rtQueryDevpid(&info, &cpPid) != RT_ERROR_NONE) {
                ACL_LOG_WARN("can not acquire cp pid, try again");
            }
            // 是否需要sleep？
            endTime = GetTimestamp();
        } while ((endTime - startTime >= (timeout * 10000)));
        ACL_LOG_INFO("get cp pid %d", cpPid);
        rtMemQueueShareAttr_t attr = {0};
        attr.manage = permission & ACL_TDTQUEUE_PERMISSION_MANAGER;
        attr.read = permission & ACL_TDTQUEUE_PERMISSION_READ;
        attr.write = permission & ACL_TDTQUEUE_PERMISSION_WRITE;
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueGrant(deviceId, qid, pid, &attr), rtMemQueueGrant);
        ACL_LOG_INFO("successfully execute acltdtGrantQueue, qid is %u, pid is %d, permisiion is %u, timeout is %d",
                     qid, pid, permission, timeout);
        return ACL_SUCCESS;
    }

    aclError QueueProcessorHost::acltdtAttachQueue(uint32_t qid, int32_t timeout, uint32_t *permission)
    {
        ACL_LOG_INFO("start to acltdtGrantQueue, qid is %u, permisiion is %u, timeout is %d",
                     qid, *permission, timeout);
        ACL_REQUIRES_NOT_NULL(permission);
        int32_t deviceId = 0;
        GET_CURRENT_DEVICE_ID(deviceId);
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueAttach(deviceId, qid, timeout), rtMemQueueAttach);
        // TODO查询权限返回
        ACL_LOG_INFO("successfully execute acltdtGrantQueue, qid is %u, permisiion is %u, timeout is %d",
                     qid, *permission, timeout);
        return ACL_SUCCESS;
    }

    aclError QueueProcessorHost::acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList)
    {
        ACL_REQUIRES_NOT_NULL(qRouteList);
        ACL_LOG_INFO("Start to acltdtBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        int32_t deviceId = 0;
        GET_CURRENT_DEVICE_ID(deviceId);
        // get dst pid
        pid_t dstPid;
        ACL_REQUIRES_OK(GetDstInfo(deviceId, CP_PID, dstPid));
        rtEschedEventSummary_t eventSum = {0};
        rtEschedEventReply_t ack = {0};
        bqs::QsProcMsgRsp qsRsp = {0};
        eventSum.pid = dstPid;
        eventSum.grpId = 0;
        eventSum.eventId = 222222; //drv EVENT_ID
        eventSum.dstEngine = RT_MQ_DST_ENGINE_CCPU_DEVICE;
        ack.buf = reinterpret_cast<char *>(&qsRsp);
        ack.bufLen = sizeof(qsRsp);
        if (!isQsInit_) {
            // 需要调用rts接口拉起QS
            ACL_REQUIRES_OK(SendConnectQsMsg(deviceId, eventSum, ack));
            isQsInit_ = true;
        }
        ACL_REQUIRES_OK(SendBindUnbindMsg(qRouteList, deviceId, true, false, eventSum, ack));
        ACL_LOG_INFO("Successfully to execute acltdtBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        return ACL_SUCCESS;
    }

    aclError QueueProcessorHost::acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList)
    {
        ACL_REQUIRES_NOT_NULL(qRouteList);
        ACL_LOG_INFO("Start to acltdtUnBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
         int32_t deviceId = 0;
        GET_CURRENT_DEVICE_ID(deviceId);
        // get dst pid
        pid_t dstPid;
        ACL_REQUIRES_OK(GetDstInfo(deviceId, CP_PID, dstPid));
        rtEschedEventSummary_t eventSum = {0};
        rtEschedEventReply_t ack = {0};
        bqs::QsProcMsgRsp qsRsp = {0};
        eventSum.pid = dstPid;
        eventSum.grpId = 0;
        eventSum.eventId = 222222; //drv EVENT_ID
        eventSum.dstEngine = RT_MQ_DST_ENGINE_CCPU_DEVICE;
        ack.buf = reinterpret_cast<char *>(&qsRsp);
        ack.bufLen = sizeof(qsRsp);
        ACL_REQUIRES_OK(SendBindUnbindMsg(qRouteList, deviceId, false, false, eventSum, ack));
        ACL_LOG_INFO("Successfully to execute acltdtUnBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        return ACL_SUCCESS;
    }

    aclError QueueProcessorHost::acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo, acltdtQueueRouteList *qRouteList)
    {
        ACL_REQUIRES_NOT_NULL(queryInfo);
        ACL_REQUIRES_NOT_NULL(qRouteList);
        ACL_LOG_INFO("Start to acltdtQueryQueueRoutes");
        int32_t deviceId = 0;
        GET_CURRENT_DEVICE_ID(deviceId);
        // get dst id
        pid_t dstPid;
        ACL_REQUIRES_OK(GetDstInfo(deviceId, CP_PID, dstPid));
        rtEschedEventSummary_t eventSum = {0};
        rtEschedEventReply_t ack = {0};
        bqs::QsProcMsgRsp qsRsp = {0};
        eventSum.pid = dstPid;
        eventSum.grpId = 0;
        eventSum.eventId = 222222; //DRV EVENT_ID
        eventSum.dstEngine = RT_MQ_DST_ENGINE_CCPU_DEVICE;
        ack.buf = reinterpret_cast<char *>(&qsRsp);
        ack.bufLen = sizeof(qsRsp);
        ACL_REQUIRES_OK(GetQueueRouteNum(queryInfo, deviceId, eventSum, ack));
        size_t routeNum = reinterpret_cast<bqs::QsProcMsgRsp *>(ack.buf)->retValue;
        ACL_REQUIRES_OK(QueryQueueRoutes(queryInfo, deviceId, false, routeNum, eventSum, ack, qRouteList));
        return ACL_SUCCESS;
    }

}