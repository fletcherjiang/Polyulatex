/**
* @file queue_processor.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef QUEUE_PROCESSOR_H
#define QUEUE_PROCESSOR_H

#include <mutex>
#include "queue.h"
#include "mmpa/mmpa_api.h"

namespace acl {
class QueueProcessor
{
public:
    virtual aclError acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *queueId);

    virtual aclError acltdtDestroyQueue(uint32_t queueId);

    virtual aclError acltdtEnqueueBuf(uint32_t queueId, acltdtBuf *buf, int32_t timeout);

    virtual aclError acltdtDequeueBuf(uint32_t queueId, acltdtBuf *buf, int32_t timeout);

    virtual aclError acltdtGrantQueue(uint32_t queueId, int32_t pid, uint32_t flag, int32_t timeout);

    virtual aclError acltdtAttachQueue(uint32_t queueId, int32_t timeout, uint32_t *flag);

    // virtual acltdtBuf* acltdtCreateBuf(size_t size);

    // virtual aclError acltdtDestroyBuf(acltdtBuf *buf);

    // virtual aclError acltdtGetBufData(const acltdtBuf *buf, void **dataPtr, size_t *size);

    // virtual aclError acltdtGetBufPrivData(const acltdtBuf *buf, void **privBuf, size_t *size);

    QueueProcessor() = default;
    ~QueueProcessor() = default;

    // not allow copy constructor and assignment operators
    QueueProcessor(const QueueProcessor &) = delete;

    QueueProcessor &operator=(const QueueProcessor &) = delete;

    QueueProcessor(QueueProcessor &&) = delete;

    QueueProcessor &&operator=(QueueProcessor &&) = delete;

protected:
    std::recursive_mutex muForQueueCtrl;
    std::mutex muForQueueEnqueue;
    std::mutex muForQueueDequeue;

private:
};

class QueueScheduleProcessor
{
public:
    virtual aclError acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList);

    virtual aclError acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList);

    virtual aclError acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo,
                                                        acltdtQueueRouteList *qRouteList);

    QueueScheduleProcessor() = default;
    ~QueueScheduleProcessor() = default;

    // not allow copy constructor and assignment operators
    QueueScheduleProcessor(const QueueScheduleProcessor &) = delete;

    QueueScheduleProcessor &operator=(const QueueScheduleProcessor &) = delete;

    QueueScheduleProcessor(QueueScheduleProcessor &&) = delete;

    QueueScheduleProcessor &&operator=(QueueScheduleProcessor &&) = delete;

protected:
    std::mutex muForQs_;
    bool isQsInit_ = false;
    uint32_t qsContactId_ = 0;

private:
};

}



#endif // QUEUE_PROCESS_H