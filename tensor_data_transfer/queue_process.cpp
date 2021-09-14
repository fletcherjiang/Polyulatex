/**
* @file queue_process.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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
    aclError QueueProcessor::acltdtEnqueue(uint32_t qid, acltdtBuf buf, int32_t timeout)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]acltdtEnqueue is not supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"acltdtEnqueue", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError QueueProcessor::acltdtDequeue(uint32_t qid, acltdtBuf *buf, int32_t timeout)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]acltdtEnqueue is not supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"acltdtEnqueue", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError QueueProcessor::acltdtDestroyQueueOndevice(uint32_t qid)
    {
        ACL_LOG_INFO("Start to destroy queue %u", qid);
        int32_t deviceId = 0;
        // get qs id
        int32_t dstPid = 0;
        size_t routeNum = 0;
        std::lock_guard<std::recursive_mutex> lk(muForQueueCtrl_);
        if (GetDstInfo(deviceId, QS_PID, dstPid) == ACL_SUCCESS) {
            ACL_LOG_INFO("find qs pid %d", dstPid);
            rtEschedEventSummary_t eventSum = {0};
            rtEschedEventReply_t ack = {0};
            QsProcMsgRsp qsRsp = {0};
            eventSum.pid = dstPid;
            eventSum.grpId = BINDQUEUEGRPID;
            eventSum.eventId = RT_MQ_SCHED_EVENT_QS_MSG; // qs EVENT_ID
            eventSum.dstEngine = RT_MQ_DST_ENGINE_CCPU_DEVICE;
            ack.buf = reinterpret_cast<char *>(&qsRsp);
            ack.bufLen = sizeof(qsRsp);
            acltdtQueueRouteQueryInfo queryInfo = {BQS_QUERY_TYPE_SRC_OR_DST, qid, qid, true, true, true};
            ACL_REQUIRES_OK(GetQueueRouteNum(&queryInfo, deviceId, eventSum, ack, routeNum));
        }
        if (routeNum > 0) {
            ACL_LOG_ERROR("qid [%u] can not be destroyed, it need to be unbinded first.", qid);
            return ACL_ERROR_FAILURE;
        }
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueDestroy(deviceId, qid), rtMemQueueDestroy);
        DeleteMutexForData(qid);
        ACL_LOG_INFO("successfully to execute destroy queue %u", qid);
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtEnqueueOnDevice(uint32_t qid, acltdtBuf buf, int32_t timeout)
    {
        ACL_REQUIRES_NOT_NULL(buf);
        QueueDataMutexPtr muPtr = GetMutexForData(qid);
        ACL_CHECK_MALLOC_RESULT(muPtr);
        uint64_t startTime = GetTimestamp();
        uint64_t endTime = 0;
        bool continueFlag = (timeout < 0) ? true : false;
        do {
            std::lock_guard<std::mutex> lk(muPtr->muForDequeue);
            int32_t deviceId = 0;
            rtError_t rtRet = rtMemQueueEnQueue(deviceId, qid, buf);
            if (rtRet == RT_ERROR_NONE) {
                return ACL_SUCCESS;
            } else if (rtRet != ACL_ERROR_RT_QUEUE_FULL) {
                ACL_LOG_CALL_ERROR("[Enqueue][Queue]fail to enqueue result = %d", rtRet);
                return rtRet;
            }
            mmSleep(1); // sleep 1ms
            endTime = GetTimestamp();
            continueFlag = !continueFlag && ((endTime - startTime) <= (static_cast<uint64_t>(timeout) * MSEC_TO_USEC));
        } while (continueFlag);
        return ACL_ERROR_FAILURE;
    }

    aclError QueueProcessor::acltdtDequeueOnDevice(uint32_t qid, acltdtBuf *buf, int32_t timeout)
    {
        ACL_REQUIRES_NOT_NULL(buf);
        QueueDataMutexPtr muPtr = GetMutexForData(qid);
        ACL_CHECK_MALLOC_RESULT(muPtr);
        uint64_t startTime = GetTimestamp();
        uint64_t endTime = 0;
        bool continueFlag = (timeout < 0) ? true : false;
        do {
            std::lock_guard<std::mutex> lk(muPtr->muForEnqueue);
            int32_t deviceId = 0;
            rtError_t rtRet = rtMemQueueDeQueue(deviceId, qid, buf);
            if (rtRet == RT_ERROR_NONE) {
                return ACL_SUCCESS;
            } else if (rtRet != ACL_ERROR_RT_QUEUE_EMPTY) {
                ACL_LOG_CALL_ERROR("[Dequeue][Queue]fail to dequeue result = %d", rtRet);
                return rtRet;
            }
            mmSleep(1); // sleep 1ms
            endTime = GetTimestamp();
            continueFlag = !continueFlag && ((endTime - startTime) <= (static_cast<uint64_t>(timeout) * MSEC_TO_USEC));
        } while (continueFlag);
        return ACL_ERROR_FAILURE;
    }

    aclError QueueProcessor::acltdtAllocBufOnDevice(size_t size, acltdtBuf *buf)
    {
        rtError_t rtRet = rtMbufAlloc(buf, size);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Alloc][mbuf]fail to alloc mbuf result = %d", rtRet);
            return rtRet;
        }
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtFreeBufOnDevice(acltdtBuf buf)
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

    aclError QueueProcessor::acltdtGetBufDataOnDevice(const acltdtBuf buf, void **dataPtr, size_t *size)
    {
        ACL_REQUIRES_NOT_NULL(buf);
        ACL_REQUIRES_NOT_NULL(dataPtr);
        ACL_REQUIRES_NOT_NULL(size);
        ACL_REQUIRES_CALL_RTS_OK(rtMbufGetBuffAddr(buf, dataPtr), rtMbufGetBuffAddr);
        ACL_REQUIRES_CALL_RTS_OK(rtMbufGetBuffSize(buf, size), rtMbufGetBuffSize);
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

    aclError QueueProcessor::InitQueueSchedule(int32_t devId)
    {
        static bool isInitQs = false;
        if (!isInitQs) {
            ACL_LOG_INFO("need to init queue schedule");
            ACL_REQUIRES_CALL_RTS_OK(rtMemQueueInitQS(devId), rtMemQueueInitQS);
            isInitQs = true;
        }
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::GetDstInfo(int32_t deviceId, PID_QUERY_TYPE type, int32_t &dstPid)
    {
        rtBindHostpidInfo_t info = {0};
        info.hostPid = mmGetPid();
        if (type == CP_PID) {
            info.cpType = RT_DEV_PROCESS_CP1;
        } else {
            info.cpType = RT_DEV_PROCESS_QS;
        }
        info.chipId = deviceId;
        ACL_LOG_INFO("start to get dst pid, deviceId is %d, type is %d", deviceId, type);
        ACL_REQUIRES_CALL_RTS_OK(rtQueryDevPid(&info, &dstPid), rtQueryDevPid);
        ACL_LOG_INFO("get dst pid %d success, type is %d", dstPid, type);
        return ACL_SUCCESS;
    }

    aclError AllocBuf(void **devPtr, void **mBuf, size_t size, bool isMbuf)
    {
        if (isMbuf) {
            ACL_REQUIRES_CALL_RTS_OK(rtMbufAlloc(mBuf, size), rtMbufAlloc);
            ACL_CHECK_MALLOC_RESULT(*mBuf);
            if (rtMbufGetBuffAddr(*mBuf, devPtr) != RT_ERROR_NONE) {
                (void)rtMbufFree(*mBuf);
                ACL_LOG_INNER_ERROR("[Get][mbuf]get mbuf failed.");
                return ACL_ERROR_BAD_ALLOC;
            }
            if (*devPtr == nullptr) {
                (void)rtMbufFree(*mBuf);
                ACL_LOG_INNER_ERROR("[Get][mbuf]get dataPtr failed.");
                return ACL_ERROR_BAD_ALLOC;
            }
        } else {
            uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
            ACL_REQUIRES_CALL_RTS_OK(rtMalloc(devPtr, size, flags), rtMalloc);
            ACL_CHECK_MALLOC_RESULT(*devPtr);
        }
        return ACL_SUCCESS;
    }

    void FreeBuf(void *devPtr, void *mbuf, bool isMbuf)
    {
        if (isMbuf) {
            if (mbuf != nullptr) {
                (void)rtMbufFree(mbuf);
            }
        } else {
            if (devPtr != nullptr) {
                (void)rtFree(devPtr);
            }
        }
    }

    aclError QueueProcessor::SendConnectQsMsg(int32_t deviceId,
                                              rtEschedEventSummary_t &eventSum,
                                              rtEschedEventReply_t &ack)
    {
        // send contact msg
        ACL_LOG_INFO("start to send contact msg");
        QsBindInit qsInitMsg = {0};
        qsInitMsg.pid = mmGetPid();
        qsInitMsg.grpId = 0;
        eventSum.subeventId = ACL_BIND_QUEUE_INIT;
        eventSum.msgLen = sizeof(qsInitMsg);
        eventSum.msg = reinterpret_cast<char *>(&qsInitMsg);
        ACL_REQUIRES_CALL_RTS_OK(rtEschedSubmitEventSync(deviceId, &eventSum, &ack), rtEschedSubmitEventSync);
        QsProcMsgRsp *rsp = reinterpret_cast<QsProcMsgRsp *>(ack.buf);
        if (rsp->retCode != 0) {
            ACL_LOG_INNER_ERROR("send connet qs failed,  ret code id %d", rsp->retCode);
            return ACL_ERROR_FAILURE;
        }
        qsContactId_ = rsp->retValue;
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
        size_t routeSize = qRouteList->routeList.size() * sizeof(QueueRoute);
        void *devPtr = nullptr;
        void *mBuf = nullptr;
        ACL_REQUIRES_OK(AllocBuf(&devPtr, &mBuf, routeSize, isMbuffAlloc));
        size_t offset = 0;
        for (size_t i = 0; i < qRouteList->routeList.size(); ++i) {
            QueueRoute *tmp = reinterpret_cast<QueueRoute *>(static_cast<uint8_t *>(devPtr) + offset);
            tmp->srcId = qRouteList->routeList[i].srcId;
            tmp->dstId = qRouteList->routeList[i].dstId;
            offset += sizeof(QueueRoute);
        }
        if (isMbuffAlloc) {
            // device need to use mbuff
            auto ret = rtMemQueueEnQueue(deviceId, qsContactId_, devPtr);
            if (ret != RT_ERROR_NONE) {
                FreeBuf(devPtr, mBuf, isMbuffAlloc);
                devPtr = nullptr;
                mBuf = nullptr;
                ACL_LOG_INNER_ERROR("[Call][Rts]call rtMemQueueEnQueue failed");
                return ret;
            }
        }
        QueueRouteList bqsBindUnbindMsg = {0};
        bqsBindUnbindMsg.routeNum = qRouteList->routeList.size();
        bqsBindUnbindMsg.routeListAddr = isMbuffAlloc ? 0 : reinterpret_cast<uint64_t>(devPtr);
        eventSum.subeventId = isBind ? ACL_BIND_QUEUE : ACL_UNBIND_QUEUE;
        eventSum.msgLen = sizeof(bqsBindUnbindMsg);
        eventSum.msg = reinterpret_cast<char *>(&bqsBindUnbindMsg);
        auto ret = rtEschedSubmitEventSync(deviceId, &eventSum, &ack);
        eventSum.msgLen = 0;
        eventSum.msg = nullptr;
        if (ret == RT_ERROR_NONE) {
            QsProcMsgRsp *rsp = reinterpret_cast<QsProcMsgRsp *>(ack.buf);
            if (rsp->retCode != 0) {
                ACL_LOG_INNER_ERROR("send connet qs failed,  ret code id %d", rsp->retCode);
                FreeBuf(devPtr, mBuf, isMbuffAlloc);
                devPtr = nullptr;
                mBuf = nullptr;
                return ACL_ERROR_FAILURE;
            }
            offset = 0;
            for (size_t i = 0; i < qRouteList->routeList.size(); ++i) {
                QueueRoute *tmp = reinterpret_cast<QueueRoute *>(static_cast<uint8_t *>(devPtr) + offset);
                qRouteList->routeList[i].status = tmp->status;
                offset += sizeof(QueueRoute);
            }
        } else {
            ACL_LOG_CALL_ERROR("[Call][Rts]call rts api rtEschedSubmitEventSync failed.");
        }
        FreeBuf(devPtr, mBuf, isMbuffAlloc);
        devPtr = nullptr;
        mBuf = nullptr;
        return ret;
    }

    aclError QueueProcessor::GetQueueRouteNum(const acltdtQueueRouteQueryInfo *queryInfo,
                                              int32_t deviceId,
                                              rtEschedEventSummary_t &eventSum,
                                              rtEschedEventReply_t &ack,
                                              size_t &routeNum)
    {
        ACL_LOG_INFO("start to get queue route num");
        QueueRouteQuery routeQuery= {0};
        routeQuery.queryType = queryInfo->mode;
        routeQuery.srcId = queryInfo->srcId;
        routeQuery.dstId = queryInfo->dstId;
        routeQuery.routeNum = 0;

        eventSum.subeventId = ACL_QUERY_QUEUE_NUM;
        eventSum.msgLen = sizeof(routeQuery);
        eventSum.msg = reinterpret_cast<char *>(&routeQuery);
        ACL_REQUIRES_CALL_RTS_OK(rtEschedSubmitEventSync(deviceId, &eventSum, &ack), rtEschedSubmitEventSync);
        QsProcMsgRsp *rsp = reinterpret_cast<QsProcMsgRsp *>(ack.buf);
        if (rsp->retCode != 0) {
            ACL_LOG_INNER_ERROR("get queue route num failed,  ret code id %d", rsp->retCode);
            return ACL_ERROR_FAILURE;
        }
        routeNum = rsp->retValue;
        eventSum.msgLen = 0;
        eventSum.msg = nullptr;
        ACL_LOG_INFO("sucessfully to get queue route num %zu.", routeNum);
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
        size_t routeSize = routeNum * sizeof(QueueRoute);
        void *devPtr = nullptr;
        void *mBuf = nullptr;
        ACL_REQUIRES_OK(AllocBuf(&devPtr, &mBuf, routeSize, isMbufAlloc));
        if (isMbufAlloc) {
            // device need to use mbuff
            auto ret = rtMemQueueEnQueue(deviceId, qsContactId_, devPtr);
            if (ret != RT_ERROR_NONE) {
                FreeBuf(devPtr, mBuf, isMbufAlloc);
                devPtr = nullptr;
                mBuf = nullptr;
                ACL_LOG_INNER_ERROR("[Call][Rts]call rtMemQueueEnQueue failed");
                return ret;
            }
        }

        QueueRouteQuery routeQuery= {0};
        routeQuery.queryType = queryInfo->mode;
        routeQuery.srcId = queryInfo->srcId;
        routeQuery.dstId = queryInfo->dstId;
        routeQuery.routeNum = routeNum;
        routeQuery.routeListAddr = isMbufAlloc ? 0 : reinterpret_cast<uint64_t>(devPtr);
        eventSum.subeventId = ACL_QUERY_QUEUE;
        eventSum.msgLen = sizeof(routeQuery);
        eventSum.msg = reinterpret_cast<char *>(&routeQuery);

        auto ret = rtEschedSubmitEventSync(deviceId, &eventSum, &ack);
        eventSum.msgLen = 0;
        eventSum.msg = nullptr;
        if (ret != RT_ERROR_NONE) {
            FreeBuf(devPtr, mBuf, isMbufAlloc);
            devPtr = nullptr;
            mBuf = nullptr;
            return ret;
        }
        QsProcMsgRsp *rsp = reinterpret_cast<QsProcMsgRsp *>(ack.buf);
        if (rsp->retCode != 0) {
            ACL_LOG_INNER_ERROR("query queue route failed,  ret code id %d", rsp->retCode);
            FreeBuf(devPtr, mBuf, isMbufAlloc);
            devPtr = nullptr;
            mBuf = nullptr;
            return ACL_ERROR_FAILURE;
        }
        size_t offset = 0;
        for (size_t i = 0; i < routeNum; ++i) {
            QueueRoute *tmp = reinterpret_cast<QueueRoute *>(static_cast<uint8_t *>(devPtr) + offset);
            acltdtQueueRoute tmpQueueRoute = {tmp->srcId, tmp->dstId, tmp->status};
            qRouteList->routeList.push_back(tmpQueueRoute);
            offset += sizeof(QueueRoute);
        }
        FreeBuf(devPtr, mBuf, isMbufAlloc);
        devPtr = nullptr;
        mBuf = nullptr;
        ACL_LOG_INFO("Successfully to execute acltdtQueryQueueRoutes, queue route is %zu",
                     qRouteList->routeList.size());
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtAllocBuf(size_t size, acltdtBuf *buf)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]acltdtAllocBuf is not supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"acltdtAllocBuf", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError QueueProcessor::acltdtFreeBuf(acltdtBuf buf)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]acltdtFreeBuf is not supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"acltdtFreeBuf", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError QueueProcessor::acltdtGetBufData(const acltdtBuf buf, void **dataPtr, size_t *size)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]acltdtGetBufData is not supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"acltdtGetBufData", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    QueueDataMutexPtr QueueProcessor::GetMutexForData(uint32_t qid)
    {
        std::lock_guard<std::mutex> lk(muForQueueMap_);
        auto it = muForQueue_.find(qid);
        if (it != muForQueue_.end()) {
            return it->second;
        } else {
            QueueDataMutexPtr p = std::shared_ptr<QueueDataMutex>(new (std::nothrow)(QueueDataMutex));
            muForQueue_[qid] = p;
            return p;
        }
    }

    void QueueProcessor::DeleteMutexForData(uint32_t qid)
    {
        std::lock_guard<std::mutex> lk(muForQueueMap_);
        auto it = muForQueue_.find(qid);
        if (it != muForQueue_.end()) {
            muForQueue_.erase(it);
        }
        return;
    }

    uint64_t QueueProcessor::GetTimestamp()
    {
        mmTimeval tv{};
        auto ret = mmGetTimeOfDay(&tv, nullptr);
        if (ret != EN_OK) {
            ACL_LOG_WARN("Func mmGetTimeOfDay failed, ret = %d", ret);
        }
        // 1000000: seconds to microseconds
        uint64_t total_use_time = tv.tv_usec + static_cast<uint64_t>(tv.tv_sec) * 1000000;
        return total_use_time;
    }
}