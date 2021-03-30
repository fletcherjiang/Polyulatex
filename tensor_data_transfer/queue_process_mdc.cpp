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
        pid_t cpPid;
        // if cp is found
        if (GetDstInfo(deviceId, CP_PID, cpPid) == ACL_SUCCESS) {
            rtMemQueueShareAttr_t permission = {0};
            ACL_REQUIRES_CALL_RTS_OK(GetQueuePermission(deviceId, qid, permission), GetQueuePermission);
            if (HasQueuePermission(permission)) {
                ACL_REQUIRES_CALL_RTS_OK(rtMemQueueGrant(deviceId, qid, cpPid, &permission), rtMemQueueGrant);
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
        if (!isQueueIint) {
            ACL_LOG_INFO("need to init queue once");
            ACL_REQUIRES_CALL_RTS_OK(rtMemQueueInit(deviceId), rtMemQueueInit);
        }
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueCreate(deviceId, attr, qid), rtMemQueueCreate);
        pid_t cpPid;
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
        std::lock_guard<std::recursive_mutex> lock(muForQueueCtrl);
        int32_t deviceId = 0;
        // get qs id
        pid_t dstPid;
        size_t routeNum = 0;
        if (GetDstInfo(deviceId, QS_PID, dstPid) == ACL_SUCCESS) {
            ACL_LOG_INFO("find qs pid %d", dstPid);
            rtEschedEventSummary_t eventSum = {0};
            rtEschedEventReply_t ack = {0};
            bqs::QsProcMsgRsp qsRsp = {0};
            eventSum.pid = dstPid;
            eventSum.grpId = bqs::BINDQUEUEGRPID;
            eventSum.eventId = 25; //qs EVENT_ID
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

    aclError QueueProcessorMdc::acltdtEnqueueBuf(uint32_t qid, acltdtBuf buf, int32_t timeout)
    {
        ACL_REQUIRES_NOT_NULL(buf);
        int32_t deviceId = 0;
        rtError_t rtRet = rtMemQueueEnQueue(deviceId, qid, buf);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Enqueue][Queue]fail to enqueue result = %d", rtRet);
            return rtRet;
        }
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtDequeueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout)
    {
        ACL_REQUIRES_NOT_NULL(buf);
        int32_t deviceId = 0;
        rtError_t rtRet = rtMemQueueDeQueue(deviceId, qid, buf);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Dequeue][Queue]fail to enqueue result = %d", rtRet);
            return rtRet;
        }
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t permission, int32_t timeout)
    {
        ACL_LOG_INFO("start to acltdtGrantQueue, qid is %u, pid is %d, permisiion is %u, timeout is %d",
                     qid, pid, permission, timeout);
        int32_t deviceId = 0;
        rtMemQueueShareAttr_t attr = {0};
        attr.manage = permission & ACL_TDTQUEUE_PERMISSION_MANAGER;
        attr.read = permission & ACL_TDTQUEUE_PERMISSION_READ;
        attr.write = permission & ACL_TDTQUEUE_PERMISSION_WRITE;
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
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueAttach(deviceId, qid, timeout), rtMemQueueAttach);
        (void)GrantQueue2Cp(deviceId, qid);
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
        pid_t dstPid;
        ACL_REQUIRES_OK(GetDstInfo(deviceId, QS_PID, dstPid));
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
        pid_t dstPid;
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
        pid_t dstPid;
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
        ACL_REQUIRES_OK(GetQueueRouteNum(queryInfo, deviceId, eventSum, ack));
        size_t routeNum = reinterpret_cast<bqs::QsProcMsgRsp *>(ack.buf)->retValue;
        ACL_REQUIRES_OK(QueryQueueRoutes(queryInfo, deviceId, true, routeNum, eventSum, ack, qRouteList));
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtAllocBuf(size_t size, acltdtBuf *buf)
    {
        rtError_t rtRet = rtMbufAlloc(buf, size);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Alloc][mbuf]fail to alloc mbuf result = %d", rtRet);
            return rtRet;
        }
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtFreeBuf(acltdtBuf buf)
    {
        if (buf == nullptr) {
            return ACL_SUCCESS;
        }
        ACL_REQUIRES_NOT_NULL(buf);
        rtError_t rtRet = rtMbufFree(buf);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Free][mbuf]fail to alloc mbuf result = %d", rtRet);
            return rtRet;
        }
        buf = nullptr;
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtGetBufData(const acltdtBuf buf, void **dataPtr, size_t *size)
    {
        ACL_REQUIRES_NOT_NULL(buf);
        ACL_REQUIRES_NOT_NULL(dataPtr);
        ACL_REQUIRES_NOT_NULL(size);
        ACL_REQUIRES_CALL_RTS_OK(rtMbufGetBuffAddr(buf, dataPtr), rtMbufGetBuffAddr);
        ACL_REQUIRES_CALL_RTS_OK(rtMbufGetBuffSize(buf, size), rtMbufGetBuffSize);
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtGetBufPrivData(const acltdtBuf buf, void **privBuf, size_t *size)
    {
        ACL_REQUIRES_NOT_NULL(buf);
        ACL_REQUIRES_NOT_NULL(privBuf);
        ACL_REQUIRES_NOT_NULL(size);
        ACL_REQUIRES_CALL_RTS_OK(rtMbufGetPrivInfo(buf, privBuf, size), rtMbufGetPrivInfo);
        return ACL_SUCCESS;
    }

}