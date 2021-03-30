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

        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtDestroyQueue(uint32_t qid)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtEnqueueBuf(uint32_t qid, acltdtBuf buf, int32_t timeout)
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

    aclError QueueProcessor::GetQueuePermission(int32_t deviceId, uint32_t qid, rtMemQueueShareAttr_t &permission)
    {
        uint32_t outLen = sizeof(permission);
        if (rtMemQueueQuery(deviceId, RT_MQ_QUERY_QUE_ATTR_OF_CUR_PROC,
                            &qid, sizeof(qid), &permission, &outLen) != RT_ERROR_NONE) {
            ACL_LOG_INNER_ERROR("get queue permission failed");
            return ACL_ERROR_FAILURE;
        }
        return ACL_SUCCESS;
    }

    bool QueueProcessor::HasQueuePermission(rtMemQueueShareAttr_t &permission) 
    {
        return (permission.manage) || (permission.read) || (permission.write);
    }

    aclError QueueProcessor::GetDstInfo(int32_t deviceId, PID_QUERY_TYPE type, pid_t &dstPid)
    {
        rtBindHostpidInfo_t info = {0};
        info.hostPid = mmGetPid();
        if (type == CP_PID) {
            info.cpType = RT_DEV_PROCESS_CP1;
        } else {
            info.cpType = RT_DEV_PROCESS_QS;
        }
        info.chipId = deviceId;
        ACL_REQUIRES_CALL_RTS_OK(rtQueryDevpid(&info, &dstPid), rtQueryDevpid);
        ACL_LOG_INFO("get dst pid %d success, type is %d", dstPid, type);
    }

    aclError AllocBuf(void *devPtr, void *mBuf, size_t size, bool isMbuf)
    {
        if (isMbuf) {
            ACL_REQUIRES_CALL_RTS_OK(rtMbufAlloc(&mBuf, size), rtMbufAlloc);
            if (rtMbufGetBuffAddr(mBuf, &devPtr) != RT_ERROR_NONE) {
                (void)rtMbufFree(mBuf);
                ACL_LOG_INNER_ERROR("[Get][mbuf]get mbuf failed.");
                return ACL_ERROR_BAD_ALLOC;
            }
        } else {
            uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
            ACL_REQUIRES_CALL_RTS_OK(rtMalloc(&devPtr, size, flags), rtMalloc);
        }
        return ACL_SUCCESS;
    }

    void FreeBuf(void *devPtr, void *mbuf, bool isMbuf)
    {
        if (isMbuf) {
            (void)rtMbufFree(mbuf);
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
        void *mBuf = nullptr;
        ACL_REQUIRES_OK(AllocBuf(devPtr, mBuf, routeSize, isMbuffAlloc));
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
                FreeBuf(devPtr, mBuf, isMbuffAlloc);
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
        FreeBuf(devPtr, mBuf, isMbuffAlloc);
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
        void *mBuf = nullptr;
        ACL_REQUIRES_OK(AllocBuf(devPtr, mBuf, routeSize, isMbufAlloc));
        if (isMbufAlloc) {
            // device need to use mbuff
            if (rtMemQueueEnQueue(deviceId, qsContactId_, devPtr) != RT_ERROR_NONE) {
                FreeBuf(devPtr, mBuf, isMbufAlloc);
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
            FreeBuf(devPtr, mBuf, isMbufAlloc);
            return ret;
        }
        size_t offset = 0;
        for (size_t i = 0; i < routeNum; ++i) {
            bqs::QueueRoute *tmp = reinterpret_cast<bqs::QueueRoute *>(static_cast<uint8_t *>(devPtr) + offset);
            acltdtQueueRoute tmpQueueRoute = {tmp->srcId, tmp->dstId, tmp->status};
            qRouteList->routeList.push_back(tmpQueueRoute);
            offset += sizeof(bqs::QueueRoute);
        }
        FreeBuf(devPtr, mBuf, isMbufAlloc);
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

    aclError acltdtAllocBuf(size_t size, acltdtBuf *buf)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]acltdtAllocBuf is not supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"acltdtAllocBuf", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError acltdtFreeBuf(acltdtBuf buf)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]acltdtFreeBuf is not supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"acltdtFreeBuf", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError acltdtGetBufData(const acltdtBuf buf, void **dataPtr, size_t *size)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]acltdtGetBufData is not supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"acltdtGetBufData", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError acltdtGetBufPrivData(const acltdtBuf buf, void **privBuf, size_t *size)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]acltdtGetBufPrivData is not supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"acltdtGetBufPrivData", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
}