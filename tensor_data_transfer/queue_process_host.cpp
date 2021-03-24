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
        return ACL_SUCCESS;
    }

    aclError QueueProcessorHost::acltdtEnqueueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout)
    {
        return ACL_ERROR_API_NOT_SUPPORT;
    }

    aclError QueueProcessorHost::acltdtDequeueBuf(uint32_t qid, acltdtBuf **buf, int32_t timeout)
    {
        return ACL_ERROR_API_NOT_SUPPORT;
    }

    aclError QueueProcessorHost::acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t permission, int32_t timeout)
    {
        if (permission & 0x1) {
            ACL_LOG_ERROR("[CHECK][permission]permission manager is not allowed");
            return ACL_ERROR_INVALID_PARAM;
        };
        int32_t deviceId = 0;
        GET_CURRENT_DEVICE_ID(deviceId);
        uint64_t startTime = GetTimestamp();
        uint64_t endTime = 0;
        rtMemQueueShareAttr_t attr = {0};
        attr.manage = permission & ACL_TDTQUEUE_PERMISSION_MANAGER;
        attr.read = permission & ACL_TDTQUEUE_PERMISSION_READ;
        attr.write = permission & ACL_TDTQUEUE_PERMISSION_WRITE;
        do {
            auto ret = rtMemQueueGrant(deviceId, qid, pid, &attr);
            if (ret == RT_ERROR_NONE) {
                return ACL_SUCCESS;
            } else if (ret != 11111) {// 不需要重试
                return ret;
            }
            endTime = GetTimestamp();
        } while ((endTime - startTime >= (timeout * 10000)));
        return ACL_SUCCESS;
    }

    aclError QueueProcessorHost::acltdtAttachQueue(uint32_t qid, int32_t timeout, uint32_t *permission)
    {
        ACL_REQUIRES_NOT_NULL(permission);
        int32_t deviceId = 0;
        GET_CURRENT_DEVICE_ID(deviceId);
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueAttach(deviceId, qid, timeout), rtMemQueueAttach);
        // TODO查询权限返回
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
        ACL_REQUIRES_OK(GetDstInfo(deviceId, false, dstPid));
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
        ACL_REQUIRES_OK(GetDstInfo(deviceId, false, dstPid));
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
        ACL_REQUIRES_OK(GetDstInfo(deviceId, true, dstPid));
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