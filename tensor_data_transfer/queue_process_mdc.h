/**
* @file queue_processor_mdc.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef QUEUE_PROCESSOR_MDC_H
#define QUEUE_PROCESSOR_MDC_H

#include <map>
#include <string>
#include "queue_process.h"

namespace acl {
class QueueProcessorMdc : public QueueProcessor
{
public:
    aclError acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *queueId) override;

    aclError acltdtDestroyQueue(uint32_t queueId) override;

    aclError acltdtEnqueue(uint32_t queueId, acltdtBuf buf, int32_t timeout) override;

    aclError acltdtDequeue(uint32_t queueId, acltdtBuf *buf, int32_t timeout) override;

    aclError acltdtGrantQueue(uint32_t queueId, int32_t pid, uint32_t flag, int32_t timeout) override;

    aclError acltdtAttachQueue(uint32_t queueId, int32_t timeout, uint32_t *flag) override;

    aclError acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList) override;

    aclError acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList) override;

    aclError GrantQueue2Cp(int32_t deviceId, uint32_t qid);

    aclError acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo,
                                    acltdtQueueRouteList *qRouteList) override;

    aclError acltdtAllocBuf(size_t size, acltdtBuf *buf) override;

    aclError acltdtFreeBuf(acltdtBuf buf) override;

    aclError acltdtGetBufData(const acltdtBuf buf, void **dataPtr, size_t *size) override;

    QueueProcessorMdc() = default;
    ~QueueProcessorMdc() = default;

    // not allow copy constructor and assignment operators
    QueueProcessorMdc(const QueueProcessorMdc &) = delete;

    QueueProcessorMdc &operator=(const QueueProcessorMdc &) = delete;

    QueueProcessorMdc(QueueProcessorMdc &&) = delete;

    QueueProcessorMdc &&operator=(QueueProcessorMdc &&) = delete;
};
}



#endif // QUEUE_PROCESS_MDC_H