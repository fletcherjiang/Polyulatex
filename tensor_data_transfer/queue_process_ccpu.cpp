/**
* @file queue_process_ccpu.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
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
        if (!isQueueIint) {
            ACL_LOG_INFO("need to init queue once");
            ACL_REQUIRES_CALL_RTS_OK(rtMemQueueInit(deviceId), rtMemQueueInit);
        }
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueCreate(deviceId, attr, qid), rtMemQueueCreate);
        ACL_LOG_INFO("Successfully to execute acltdtCreateQueue, qid is %u", *qid);
        return ACL_SUCCESS;
    }

    aclError QueueProcessorCcpu::acltdtDestroyQueue(uint32_t qid)
    {
        std::lock_guard<std::recursive_mutex> lock(muForQueueCtrl);
        int32_t deviceId = 0;
        // get qs id
        pid_t dstPid;
        size_t routeNum = 0;
        if (GetDstInfo(deviceId, QS_PID, dstPid) == ACL_SUCCESS) {
            rtEschedEventSummary_t eventSum = {0};
            rtEschedEventReply_t ack = {0};
            bqs::QsProcMsgRsp qsRsp = {0};
            eventSum.pid = dstPid;
            eventSum.grpId = bqs::BINDQUEUEGRPID;
            eventSum.eventId = 222222; //qs EVENT_ID
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

    aclError QueueProcessorCcpu::acltdtEnqueueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout)
    {
        ACL_REQUIRES_NOT_NULL(buf);
        int32_t deviceId = 0;
        rtError_t rtRet = rtMemQueueEnQueue(deviceId, qid, buf->mbuf);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Enqueue][Queue]fail to enqueue result = %d", rtRet);
            return rtRet;
        }
        return ACL_SUCCESS;
    }

    aclError QueueProcessorCcpu::acltdtDequeueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout)
    {
        ACL_REQUIRES_NOT_NULL(buf);
        int32_t deviceId = 0;
        rtError_t rtRet = rtMemQueueDeQueue(deviceId, qid, &buf->mbuf);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Dequeue][Queue]fail to enqueue result = %d", rtRet);
            return rtRet;
        }
        return ACL_SUCCESS;
    }

    aclError QueueProcessorCcpu::acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t permission, int32_t timeout)
    {
        int32_t deviceId = 0;
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
            } else if (ret != 11111) {// 不需要重试?
                return ret;
            }
            endTime = GetTimestamp();
        } while ((endTime - startTime >= (static_cast<uint64_t>(timeout) * 10000)));
        return ACL_SUCCESS;
    }

    aclError QueueProcessorCcpu::acltdtAttachQueue(uint32_t qid, int32_t timeout, uint32_t *permission)
    {
        ACL_REQUIRES_NOT_NULL(permission);
        int32_t deviceId = 0;
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueAttach(deviceId, qid, timeout), rtMemQueueAttach);
        // 查询queue权限
        // 查询有cp则授权Q权限
        // pid_t cpPid;
        // rtBindHostpidInfo_t info = {0};
        // info.hostPid = mmGetPid();
        // info.cpType = RT_DEV_PROCESS_CP1;
        // info.chipId = deviceId;
        // if (rtQueryDevpid(&info, &cpPid) == RT_ERROR_NONE) {

        // }
        return ACL_SUCCESS;
    }

    aclError QueueProcessorCcpu::acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList)
    {
        ACL_REQUIRES_NOT_NULL(qRouteList);
        ACL_LOG_INFO("Start to acltdtBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        // qs is thread mode, so no need to grant queue to qs
        int32_t deviceId = 0;
        // get dst id
        pid_t dstPid;
        ACL_REQUIRES_OK(GetDstInfo(deviceId, QS_PID, dstPid));
        rtEschedEventSummary_t eventSum = {0};
        rtEschedEventReply_t ack = {0};
        bqs::QsProcMsgRsp qsRsp = {0};
        eventSum.pid = dstPid;
        eventSum.grpId = bqs::BINDQUEUEGRPID;
        eventSum.eventId = 222222; //qs EVENT_ID
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

    aclError QueueProcessorCcpu::acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList)
    {
        ACL_REQUIRES_NOT_NULL(qRouteList);
        ACL_LOG_INFO("Start to acltdtUnBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        int32_t deviceId = 0;
        // get dst id
        pid_t dstPid;
        ACL_REQUIRES_OK(GetDstInfo(deviceId, QS_PID, dstPid));
        rtEschedEventSummary_t eventSum = {0};
        rtEschedEventReply_t ack = {0};
        bqs::QsProcMsgRsp qsRsp = {0};
        eventSum.pid = dstPid;
        eventSum.grpId = bqs::BINDQUEUEGRPID;
        eventSum.eventId = 222222; //drv EVENT_ID
        eventSum.dstEngine = RT_MQ_DST_ENGINE_CCPU_DEVICE;
        ack.buf = reinterpret_cast<char *>(&qsRsp);
        ack.bufLen = sizeof(qsRsp);
        ACL_REQUIRES_OK(SendBindUnbindMsg(qRouteList, deviceId, false, true, eventSum, ack));
        ACL_LOG_INFO("Successfully to execute acltdtUnBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        return ACL_SUCCESS;
    }

    aclError QueueProcessorCcpu::acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo, acltdtQueueRouteList *qRouteList)
    {
        ACL_REQUIRES_NOT_NULL(queryInfo);
        ACL_REQUIRES_NOT_NULL(qRouteList);
        ACL_LOG_INFO("Start to acltdtQueryQueueRoutes");
        int32_t deviceId = 0;
        // get dst id
        pid_t dstPid;
        ACL_REQUIRES_OK(GetDstInfo(deviceId, QS_PID, dstPid));
        rtEschedEventSummary_t eventSum = {0};
        rtEschedEventReply_t ack = {0};
        bqs::QsProcMsgRsp qsRsp = {0};
        eventSum.pid = dstPid;
        eventSum.grpId = bqs::BINDQUEUEGRPID;
        eventSum.eventId = 222222; //qs EVENT_ID
        eventSum.dstEngine = RT_MQ_DST_ENGINE_CCPU_DEVICE;
        ack.buf = reinterpret_cast<char *>(&qsRsp);
        ack.bufLen = sizeof(qsRsp);
        ACL_REQUIRES_OK(GetQueueRouteNum(queryInfo, deviceId, eventSum, ack));
        size_t routeNum = reinterpret_cast<bqs::QsProcMsgRsp *>(ack.buf)->retValue;
        ACL_REQUIRES_OK(QueryQueueRoutes(queryInfo, deviceId, true, routeNum, eventSum, ack, qRouteList));
        return ACL_SUCCESS;
    }

    acltdtBuf* QueueProcessorCcpu::acltdtCreateBuf(size_t size)
    {
        // if (!HasGroup) {
        //     creategrp;
        //     addgrp;
        // }
        rtMbufPtr_t buf = nullptr;
        rtError_t rtRet = rtMbufAlloc(&buf, size);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Alloc][mbuf]fail to alloc mbuf result = %d", rtRet);
            return nullptr;
        }
        return new(std::nothrow) acltdtBuf(buf);
    }

    aclError QueueProcessorCcpu::acltdtDestroyBuf(acltdtBuf *buf)
    {
        if (buf == nullptr) {
            return ACL_SUCCESS;
        }
        ACL_REQUIRES_NOT_NULL(buf->mbuf);
        rtError_t rtRet = rtMbufFree(buf->mbuf);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Free][mbuf]fail to alloc mbuf result = %d", rtRet);
            return rtRet;
        }
        buf->mbuf = nullptr;
        ACL_DELETE_AND_SET_NULL(buf);
        return ACL_SUCCESS;
    }

    aclError QueueProcessorCcpu::acltdtGetBufData(const acltdtBuf *buf, void **dataPtr, size_t *size)
    {
        ACL_REQUIRES_NOT_NULL(buf);
        ACL_REQUIRES_NOT_NULL(dataPtr);
        ACL_REQUIRES_NOT_NULL(size);
        ACL_REQUIRES_CALL_RTS_OK(rtMbufGetBuffAddr(buf->mbuf, dataPtr), rtMbufGetBuffAddr);
        ACL_REQUIRES_CALL_RTS_OK(rtMbufGetBuffSize(buf->mbuf, size), rtMbufGetBuffSize);
        return ACL_SUCCESS;
    }

    aclError QueueProcessorCcpu::acltdtGetBufPrivData(const acltdtBuf *buf, void **privBuf, size_t *size)
    {
        ACL_REQUIRES_NOT_NULL(buf);
        ACL_REQUIRES_NOT_NULL(privBuf);
        ACL_REQUIRES_NOT_NULL(size);
        ACL_REQUIRES_CALL_RTS_OK(rtMbufGetPrivInfo(buf->mbuf, privBuf, size), rtMbufGetPrivInfo);
        return ACL_SUCCESS;
    }

}