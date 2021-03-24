/**
* @file queue_process.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "queue_process.h"
#include "log_inner.h"
#include "runtime/mem.h"
#include "runtime/rt_mem_queue.h"
#include "runtime/dev.h"
#include "aicpu/queue_schedule/qs_client.h"

namespace acl {
    aclError QueueProcessor::acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *qid)
    {
        ACL_LOG_INFO("Start to acltdtCreateQueue");
        ACL_REQUIRES_NOT_NULL(qid);
        ACL_REQUIRES_NOT_NULL(attr);
        int32_t deviceId = 0;
        rtError_t rtRet = RT_ERROR_NONE;
        if (1) {
            rtRet = rtGetDevice(&deviceId);
            if (rtRet != ACL_SUCCESS) {
                ACL_LOG_CALL_ERROR("[Get][DeviceId]fail to get deviceId result = %d", rtRet);
                return rtRet;
            }
        }
        static bool isQueueIint = false;
        if (!isQueueIint) {
            ACL_LOG_INFO("need to init queue once");
            ACL_REQUIRES_CALL_RTS_OK(rtMemQueueInit(deviceId), rtMemQueueInit);
        }
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueCreate(deviceId, attr, qid), rtMemQueueCreate);
        //TODO ccpu上需要给cp加权限
        ACL_LOG_INFO("Successfully to execute acltdtCreateQueue, qid is %u", *qid);
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtDestroyQueue(uint32_t qid)
    {
        std::lock_guard<std::recursive_mutex> lock(muForQueueCtrl);
        int32_t deviceId = 0;
        GET_CURRENT_DEVICE_ID(deviceId);
        // TODO 有绑定结果需要报错
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueDestroy(deviceId, qid), rtMemQueueDestroy);
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtEnqueueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]acltdtEnqueueBuf is not supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"acltdtEnqueueBuf", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError QueueProcessor::acltdtDequeueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]acltdtEnqueueBuf is not supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"acltdtEnqueueBuf", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError QueueProcessor::acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t flag, int32_t)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtAttachQueue(uint32_t qid, int32_t timeout, uint32_t *flag)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::GetDstInfo(int32_t deviceId, bool isDevice, pid_t &dstPid)
    {
        rtBindHostpidInfo_t info = {0};
        info.hostPid = mmGetPid();
        info.cpType = isDevice ? : RT_DEV_PROCESS_QS, RT_DEV_PROCESS_CP1;
        info.chipId = deviceId;
        ACL_REQUIRES_CALL_RTS_OK(rtQueryDevpid(&info, &dstPid), rtQueryDevpid);
    }

    aclError AllocBuf(void *devPtr, size_t size, bool isMbuf)
    {
        if (isMbuf) {
            ACL_REQUIRES_CALL_RTS_OK(rtMBuffAlloc(&devPtr, size), rtMBuffAlloc);
        } else {
            uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
            ACL_REQUIRES_CALL_RTS_OK(rtMalloc(&devPtr, size, flags), rtMalloc);
        }
        return ACL_SUCCESS;
    }

    void FreeBuf(void *devPtr, bool isMbuf)
    {
        if (isMbuf) {
            ;//(void)rtMBuffFree(devPtr);
        } else {
            (void)rtFree(devPtr);
        }
    }

    aclError QueueProcessor::SendConnectQsMsg(int32_t deviceId, rtEschedEventSummary_t &eventSum, rtEschedEventReply_t &ack)
    {
        // send contact msg
        ACL_LOG_INFO("start to send contact msg");
        bqs::QsBindInit qsInitMsg = {0};
        qsInitMsg.pid = mmGetPid();
        qsInitMsg.grpId = 0;
        eventSum.subeventId = bqs::ACL_BIND_QUEUE_INIT;
        eventSum.msgLen = sizeof(qsInitMsg);
        eventSum.msg = reinterpret_cast<char *>(&qsInitMsg);
        ACL_REQUIRES_CALL_RTS_OK(rtEschedSubmitEventSync(deviceId, &eventSum, &ack), rtEschedSubmitEventSync);
        qsContactId_ = reinterpret_cast<bqs::QsProcMsgRsp *>(ack.buf)->retValue;
        eventSum.msgLen = 0;
        eventSum.msg = nullptr;
        ACL_LOG_INFO("successfully execute to SendConnectQsMsg");
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::SendBindUnbindMsg(acltdtQueueRouteList *qRouteList,
                                               int32_t deviceId,
                                               bool isBind,
                                               bool isMbuffAlloc,
                                               rtEschedEventSummary_t &eventSum,
                                               rtEschedEventReply_t &ack)
    {
        ACL_LOG_INFO("start to send bind or unbind msg");
        // send bind or unbind msg
        size_t routeSize = qRouteList->routeList.size() * sizeof(bqs::QueueRoute);
        void *devPtr = nullptr;
        ACL_REQUIRES_OK(AllocBuf(devPtr, routeSize, isMbuffAlloc));
        size_t offset = 0;
        for (size_t i = 0; i < qRouteList->routeList.size(); ++i) {
            bqs::QueueRoute *tmp = reinterpret_cast<bqs::QueueRoute *>(static_cast<uint8_t *>(devPtr) + offset);
            tmp->srcId = qRouteList->routeList[i].srcId;
            tmp->dstId = qRouteList->routeList[i].dstId;
            offset += sizeof(bqs::QueueRoute);
        }
        if (isMbuffAlloc) {
            // device need to use mbuff
            if (rtMemQueueEnQueue(deviceId, qsContactId_, devPtr) != RT_ERROR_NONE) {
                FreeBuf(devPtr, isMbuffAlloc);
            }
        }
        bqs::QueueRouteList bqsBindUnbindMsg = {0};
        bqsBindUnbindMsg.routeNum = qRouteList->routeList.size();
        bqsBindUnbindMsg.routeListAddr = isMbuffAlloc ? : 0, reinterpret_cast<uint64_t>(devPtr);
        eventSum.subeventId = isBind ? : bqs::ACL_BIND_QUEUE, bqs::ACL_UNBIND_QUEUE;
        eventSum.msgLen = sizeof(bqsBindUnbindMsg);
        eventSum.msg = reinterpret_cast<char *>(&bqsBindUnbindMsg);
        auto ret = rtEschedSubmitEventSync(deviceId, &eventSum, &ack);
        eventSum.msgLen = 0;
        eventSum.msg = nullptr;
        if (ret == RT_ERROR_NONE) {
            offset = 0;
            for (size_t i = 0; i < qRouteList->routeList.size(); ++i) {
                bqs::QueueRoute *tmp = reinterpret_cast<bqs::QueueRoute *>(static_cast<uint8_t *>(devPtr) + offset);
                acltdtQueueRoute tmpQueueRoute = {tmp->srcId, tmp->dstId, tmp->status};
                qRouteList->routeList[i].status = tmp->status;
                offset += sizeof(bqs::QueueRoute);
            }
        } else {
            ACL_LOG_CALL_ERROR("[Call][Rts]call rts api rtEschedSubmitEventSync failed.");
        }
        FreeBuf(devPtr, isMbuffAlloc);
        return ret;
    }

    aclError QueueProcessor::GetQueueRouteNum(const acltdtQueueRouteQueryInfo *queryInfo,
                                                          int32_t deviceId,
                                                          rtEschedEventSummary_t &eventSum,
                                                          rtEschedEventReply_t &ack)
    {
        ACL_LOG_INFO("start to get queue route num");
        bqs::QueueRouteQuery routeQuery= {0};
        routeQuery.queryType = queryInfo->mode;
        routeQuery.srcId = queryInfo->srcId;
        routeQuery.dstId = queryInfo->dstId;
        routeQuery.routeNum = 0;

        eventSum.subeventId = bqs::ACL_QUERY_QUEUE_NUM;
        eventSum.msgLen = sizeof(routeQuery);
        eventSum.msg = reinterpret_cast<char *>(&routeQuery);
        ACL_REQUIRES_CALL_RTS_OK(rtEschedSubmitEventSync(deviceId, &eventSum, &ack), rtEschedSubmitEventSync);
        eventSum.msgLen = 0;
        eventSum.msg = nullptr;
        ACL_LOG_INFO("sucessfully to get queue route num.");
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::QueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo,
                                              int32_t deviceId,
                                              bool isMbufAlloc,
                                              size_t routeNum,
                                              rtEschedEventSummary_t &eventSum,
                                              rtEschedEventReply_t &ack,
                                              acltdtQueueRouteList *qRouteList)
    {
        ACL_LOG_INFO("start to query queue route %zu", routeNum);
        size_t routeSize = routeNum * sizeof(bqs::QueueRoute);
        void *devPtr = nullptr;
        ACL_REQUIRES_OK(AllocBuf(devPtr, routeSize, isMbufAlloc));
        if (isMbufAlloc) {
            // device need to use mbuff
            if (rtMemQueueEnQueue(deviceId, qsContactId_, devPtr) != RT_ERROR_NONE) {
                FreeBuf(devPtr, isMbufAlloc);
            }
        }

        bqs::QueueRouteQuery routeQuery= {0};
        routeQuery.queryType = queryInfo->mode;
        routeQuery.srcId = queryInfo->srcId;
        routeQuery.dstId = queryInfo->dstId;
        routeQuery.routeNum = routeNum;
        routeQuery.routeListAddr = isMbufAlloc ? : 0, reinterpret_cast<uint64_t>(devPtr);

        eventSum.subeventId = bqs::ACL_QUERY_QUEUE;
        eventSum.msgLen = sizeof(routeQuery);
        eventSum.msg = reinterpret_cast<char *>(&routeQuery);

        auto ret = rtEschedSubmitEventSync(deviceId, &eventSum, &ack);
        eventSum.msgLen = 0;
        eventSum.msg = nullptr;
        if (ret != RT_ERROR_NONE) {
            FreeBuf(devPtr, isMbufAlloc);
            return ret;
        }
        size_t offset = 0;
        for (size_t i = 0; i < routeNum; ++i) {
            bqs::QueueRoute *tmp = reinterpret_cast<bqs::QueueRoute *>(static_cast<uint8_t *>(devPtr) + offset);
            acltdtQueueRoute tmpQueueRoute = {tmp->srcId, tmp->dstId, tmp->status};
            qRouteList->routeList.push_back(tmpQueueRoute);
            offset += sizeof(bqs::QueueRoute);
        }
        FreeBuf(devPtr, isMbufAlloc);
        ACL_LOG_INFO("Successfully to execute acltdtQueryQueueRoutes, queue route is %zu", qRouteList->routeList.size());
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo, acltdtQueueRouteList *qRouteList)
    {
        return ACL_SUCCESS;
    }

    acltdtBuf* acltdtCreateBuf(size_t size)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]acltdtCreateBuf is not supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"acltdtCreateBuf", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return nullptr;
    }

    aclError acltdtDestroyBuf(acltdtBuf *buf)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]acltdtDestroyBuf is not supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"acltdtDestroyBuf", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError acltdtGetBufData(const acltdtBuf *buf, void **dataPtr, size_t *size)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]acltdtGetBufData is not supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"acltdtGetBufData", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError acltdtGetBufPrivData(const acltdtBuf *buf, void **privBuf, size_t *size)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]acltdtGetBufPrivData is not supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"acltdtGetBufPrivData", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
}