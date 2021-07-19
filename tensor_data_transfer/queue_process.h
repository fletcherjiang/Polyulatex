/**
* @file queue_processor.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef QUEUE_PROCESSOR_H
#define QUEUE_PROCESSOR_H

#include <mutex>
#include <memory>
#include <map>
#include "queue.h"
#include "mmpa/mmpa_api.h"

namespace acl {

typedef struct QueueDataMutex {
    std::mutex muForEnqueue;
    std::mutex muForDequeue;
} QueueDataMutex;

enum PID_QUERY_TYPE {
    CP_PID,
    QS_PID
};

constexpr int32_t BQS_QUERY_TYPE_SRC_OR_DST = 3;
constexpr int32_t MSEC_TO_USEC = 1000;

using QueueDataMutexPtr = std::shared_ptr<QueueDataMutex>;

class QueueProcessor
{
public:
    virtual aclError acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *queueId) = 0;

    virtual aclError acltdtDestroyQueue(uint32_t queueId) = 0;

    virtual aclError acltdtEnqueue(uint32_t queueId, acltdtBuf buf, int32_t timeout);

    virtual aclError acltdtDequeue(uint32_t queueId, acltdtBuf *buf, int32_t timeout);

    virtual aclError acltdtGrantQueue(uint32_t queueId, int32_t pid, uint32_t flag, int32_t timeout) = 0;

    virtual aclError acltdtAttachQueue(uint32_t queueId, int32_t timeout, uint32_t *flag) = 0;

    virtual aclError acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList) = 0;

    virtual aclError acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList) = 0;

    virtual aclError acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo,
                                            acltdtQueueRouteList *qRouteList) = 0;

    virtual aclError acltdtAllocBuf(size_t size, acltdtBuf *buf);

    virtual aclError acltdtFreeBuf(acltdtBuf buf);

    virtual aclError acltdtGetBufData(const acltdtBuf buf, void **dataPtr, size_t *size);

    aclError InitQueueSchedule(int32_t devId);

    aclError acltdtDestroyQueueOndevice(uint32_t qid);

    aclError acltdtEnqueueOnDevice(uint32_t queueId, acltdtBuf buf, int32_t timeout);

    aclError acltdtDequeueOnDevice(uint32_t queueId, acltdtBuf *buf, int32_t timeout);

    aclError acltdtAllocBufOnDevice(size_t size, acltdtBuf *buf);

    aclError acltdtFreeBufOnDevice(acltdtBuf buf);

    aclError acltdtGetBufDataOnDevice(const acltdtBuf buf, void **dataPtr, size_t *size);

    aclError SendBindUnbindMsg(acltdtQueueRouteList *qRouteList,
                                               int32_t devieId,
                                               bool isBind,
                                               bool isMbuffAlloc,
                                               rtEschedEventSummary_t &eventSum,
                                               rtEschedEventReply_t &ack);

    aclError SendConnectQsMsg(int32_t devieId, rtEschedEventSummary_t &eventSum, rtEschedEventReply_t &ack);
    aclError GetDstInfo(int32_t deviceId, PID_QUERY_TYPE type, int32_t &dstPid);
    aclError GetQueuePermission(int32_t deviceId, uint32_t qid, rtMemQueueShareAttr_t &permission);
    aclError GetQueueRouteNum(const acltdtQueueRouteQueryInfo *queryInfo,
                              int32_t deviceId,
                              rtEschedEventSummary_t &eventSum,
                              rtEschedEventReply_t &ack,
                              size_t &routeNum);

    aclError QueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo,
                                              int32_t deviceId,
                                              bool isMbufAlloc,
                                              size_t routeNum,
                                              rtEschedEventSummary_t &eventSum,
                                              rtEschedEventReply_t &ack,
                                              acltdtQueueRouteList *qRouteList);

    QueueDataMutexPtr GetMutexForData(uint32_t qid);
    void DeleteMutexForData(uint32_t qid);

    uint64_t GetTimestamp();

    QueueProcessor() = default;
    virtual ~QueueProcessor() = default;

    // not allow copy constructor and assignment operators
    QueueProcessor(const QueueProcessor &) = delete;

    QueueProcessor &operator=(const QueueProcessor &) = delete;

    QueueProcessor(QueueProcessor &&) = delete;

    QueueProcessor &&operator=(QueueProcessor &&) = delete;

protected:
    std::recursive_mutex muForQueueCtrl_;
    std::mutex muForQueueMap_;
    std::map<uint32_t, QueueDataMutexPtr> muForQueue_;
    bool isQsInit_ = false;
    uint32_t qsContactId_ = 0;

};

}



#endif // QUEUE_PROCESS_H