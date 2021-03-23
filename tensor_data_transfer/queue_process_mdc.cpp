/**
* @file queue_process_host.cpp
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

    aclError QueueProcessorMdc::acltdtDestroyQueue(uint32_t qid)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtEnqueueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout)
    {
        ACL_REQUIRES_NOT_NULL(buf);
        int32_t deviceId = 0;
        std::lock_guard<std::mutex> lock(muForQueueEnqueue);
        rtError_t rtRet = rtMemQueueEnQueue(deviceId, qid, buf->mbuf);
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
        std::lock_guard<std::mutex> lock(muForQueueEnqueue);
        rtError_t rtRet = rtMemQueueDeQueue(deviceId, qid, &buf->mbuf);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Dequeue][Queue]fail to enqueue result = %d", rtRet);
            return rtRet;
        }
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t permission, int32_t timeout)
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
        } while ((endTime - startTime >= (timeout * 10000)));
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtAttachQueue(uint32_t qid, int32_t timeout, uint32_t *permission)
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


    aclError QueueScheduleProcessorMdc::SendBindUnbindMsg(acltdtQueueRouteList *qRouteList, bool isBind)
    {
        int32_t deviceId = 0;
        GET_CURRENT_DEVICE_ID(deviceId);
        // get cp id
        pid_t dstPid;
        rtBindHostpidInfo_t info = {0};
        info.hostPid = mmGetPid();
        info.cpType = RT_DEV_PROCESS_QS;
        info.chipId = deviceId;
        ACL_REQUIRES_CALL_RTS_OK(rtQueryDevpid(&info, &dstPid), rtQueryDevpid);

        rtEschedEventSummary_t eventSum = {0};
        rtEschedEventReply_t ack = {0};
        bqs::QsProcMsgRsp qsRsp = {0};
        if (isBind && !isQsInit_) {
            // send init msg
            bqs::QsBindInit qsInitMsg = {0};
            qsInitMsg.pid = info.hostPid;
            qsInitMsg.grpId = 0;

            eventSum.pid = dstPid;
            eventSum.grpId = bqs::BINDQUEUEGRPID;
            eventSum.eventId = 33333; //EVENT_ID
            eventSum.subeventId = bqs::ACL_BIND_QUEUE_INIT;
            eventSum.msgLen = sizeof(qsInitMsg);
            eventSum.msg = reinterpret_cast<char *>(&qsInitMsg);
            eventSum.dstEngine = RT_MQ_DST_ENGINE_CCPU_DEVICE;

            ack.buf = reinterpret_cast<char *>(&qsRsp);
            ack.bufLen = sizeof(qsRsp);
            ACL_REQUIRES_CALL_RTS_OK(rtEschedSubmitEventSync(deviceId, &eventSum, &ack), rtEschedSubmitEventSync);
            isQsInit_ = true;
            qsContactId_ = reinterpret_cast<bqs::QsProcMsgRsp *>(ack.buf)->retValue;
        }
        // send msg
        size_t routeSize = qRouteList->routeList.size() * sizeof(bqs::QueueRoute);
        rtMbufPtr_t devPtr = nullptr;
        ACL_REQUIRES_CALL_RTS_OK(rtMBuffAlloc(&devPtr, routeSize), rtMBuffAlloc);
        size_t offset = 0;
        for (size_t i = 0; i < qRouteList->routeList.size(); ++i) {
            bqs::QueueRoute *tmp = reinterpret_cast<bqs::QueueRoute *>(static_cast<uint8_t *>(devPtr) + offset);
            tmp->srcId = qRouteList->routeList[i].srcId;
            tmp->dstId = qRouteList->routeList[i].dstId;
            offset += sizeof(bqs::QueueRoute);
        }
        // device need to use mbuff
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueEnQueue(deviceId, qsContactId_, devPtr), rtMemQueueEnQueue);

        bqs::QueueRouteList bqsBindUnbindMsg = {0};
        bqsBindUnbindMsg.routeNum = qRouteList->routeList.size();
        bqsBindUnbindMsg.routeListAddr = 0;
        eventSum.subeventId = isBind ? : bqs::ACL_BIND_QUEUE, bqs::ACL_UNBIND_QUEUE;
        eventSum.msgLen = sizeof(bqsBindUnbindMsg);
        eventSum.msg = reinterpret_cast<char *>(&bqsBindUnbindMsg);
        auto ret = rtEschedSubmitEventSync(deviceId, &eventSum, &ack);

        offset = 0;
        for (size_t i = 0; i < qRouteList->routeList.size(); ++i) {
            bqs::QueueRoute *tmp = reinterpret_cast<bqs::QueueRoute *>(static_cast<uint8_t *>(devPtr) + offset);
            acltdtQueueRoute tmpQueueRoute = {tmp->srcId, tmp->dstId, tmp->status};
            qRouteList->routeList[i].status = tmp->status;
            offset += sizeof(bqs::QueueRoute);
        }

        // (void)rtMBuffFree(devPtr);
        if (ret != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Call][Rts]call rts api rtEschedSubmitEventSync failed.");
            return ret;
        }
        return ACL_SUCCESS;
    }

    aclError QueueScheduleProcessorMdc::acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList)
    {
        ACL_REQUIRES_NOT_NULL(qRouteList);
        ACL_LOG_INFO("Start to acltdtBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        ACL_REQUIRES_OK(SendBindUnbindMsg(const_cast<acltdtQueueRouteList *>(qRouteList), true));
        ACL_LOG_INFO("Successfully to execute acltdtBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        return ACL_SUCCESS;
    }

    aclError QueueScheduleProcessorMdc::acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList)
    {
        ACL_REQUIRES_NOT_NULL(qRouteList);
        ACL_LOG_INFO("Start to acltdtUnBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        ACL_REQUIRES_OK(SendBindUnbindMsg(const_cast<acltdtQueueRouteList *>(qRouteList), false));
        ACL_LOG_INFO("Successfully to execute acltdtUnBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        return ACL_SUCCESS;
    }

    aclError QueueScheduleProcessorMdc::GetQueueRouteNum(const acltdtQueueRouteQueryInfo *queryInfo,
                                                          int32_t &deviceId,
                                                          rtEschedEventSummary_t &eventSum,
                                                          rtEschedEventReply_t &ack)
    {
        GET_CURRENT_DEVICE_ID(deviceId);
        // get cp id
        pid_t dstPid;
        rtBindHostpidInfo_t info = {0};
        info.hostPid = mmGetPid();
        info.cpType = RT_DEV_PROCESS_QS;
        info.chipId = deviceId;
        ACL_REQUIRES_CALL_RTS_OK(rtQueryDevpid(&info, &dstPid), rtQueryDevpid);
        bqs::QueueRouteQuery routeQuery= {0};
        routeQuery.queryType = queryInfo->mode;
        routeQuery.srcId = queryInfo->srcId;
        routeQuery.dstId = queryInfo->dstId;
        routeQuery.routeNum = 0;

        eventSum.pid = dstPid;
        eventSum.grpId = bqs::BINDQUEUEGRPID;
        eventSum.eventId = 333333; //EVENT_ID
        eventSum.subeventId = bqs::ACL_QUERY_QUEUE_NUM;
        eventSum.msgLen = sizeof(routeQuery);
        eventSum.msg = reinterpret_cast<char *>(&routeQuery);
        eventSum.dstEngine = RT_MQ_DST_ENGINE_CCPU_DEVICE;

        ACL_REQUIRES_CALL_RTS_OK(rtEschedSubmitEventSync(deviceId, &eventSum, &ack), rtEschedSubmitEventSync);
        return ACL_SUCCESS;
    }

    aclError QueueScheduleProcessorMdc::acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo, acltdtQueueRouteList *qRouteList)
    {
        ACL_REQUIRES_NOT_NULL(queryInfo);
        ACL_REQUIRES_NOT_NULL(qRouteList);
        ACL_LOG_INFO("Start to acltdtQueryQueueRoutes");
        int32_t deviceId = 0;
        rtEschedEventSummary_t eventSum = {0};
        rtEschedEventReply_t ack = {0};
        bqs::QsProcMsgRsp qsRsp = {0};
        ack.buf = reinterpret_cast<char *>(&qsRsp);
        ack.bufLen = sizeof(qsRsp);

        ACL_REQUIRES_OK(GetQueueRouteNum(queryInfo, deviceId, eventSum, ack));
        size_t routeNum = reinterpret_cast<bqs::QsProcMsgRsp *>(ack.buf)->retValue;
        size_t routeSize = routeNum * sizeof(bqs::QueueRoute);
        void *devPtr = nullptr;
        ACL_REQUIRES_CALL_RTS_OK(rtMBuffAlloc(&devPtr, routeSize), rtMBuffAlloc);
        // device need to use mbuff
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueEnQueue(deviceId, qsContactId_, devPtr), rtMemQueueEnQueue);

        bqs::QueueRouteQuery routeQuery= {0};
        routeQuery.queryType = queryInfo->mode;
        routeQuery.srcId = queryInfo->srcId;
        routeQuery.dstId = queryInfo->dstId;
        routeQuery.routeNum = routeNum;
        routeQuery.routeListAddr = 0;

        eventSum.subeventId = bqs::ACL_QUERY_QUEUE;
        eventSum.msgLen = sizeof(routeQuery);
        eventSum.msg = reinterpret_cast<char *>(&routeQuery);

        auto ret = rtEschedSubmitEventSync(deviceId, &eventSum, &ack);
        if (ret != RT_ERROR_NONE) {
            //(void)rtMBuffFree(devPtr);
            ACL_LOG_CALL_ERROR("[Call][Rts]call rts api rtEschedSubmitEventSync failed.");
            return ret;
        }
        size_t offset = 0;
        for (size_t i = 0; i < routeNum; ++i) {
            bqs::QueueRoute *tmp = reinterpret_cast<bqs::QueueRoute *>(static_cast<uint8_t *>(devPtr) + offset);
            acltdtQueueRoute tmpQueueRoute = {tmp->srcId, tmp->dstId, tmp->status};
            qRouteList->routeList.push_back(tmpQueueRoute);
            offset += sizeof(bqs::QueueRoute);
        }
        //(void)rtMBuffFree(devPtr);
        ACL_LOG_INFO("Successfully to execute acltdtQueryQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        return ACL_SUCCESS;
    }

}