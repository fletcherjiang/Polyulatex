/**
* @file queue_processor_mdc.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
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
    aclError acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *queueId);

    aclError acltdtDestroyQueue(uint32_t queueId);

    aclError acltdtEnqueue(uint32_t queueId, acltdtBuf buf, int32_t timeout);

    aclError acltdtDequeue(uint32_t queueId, acltdtBuf *buf, int32_t timeout);

    aclError acltdtGrantQueue(uint32_t queueId, int32_t pid, uint32_t flag, int32_t timeout);

    aclError acltdtAttachQueue(uint32_t queueId, int32_t timeout, uint32_t *flag);

    aclError acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList);

    aclError acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList);

    aclError GrantQueue2Cp(int32_t deviceId, uint32_t qid);

    aclError acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo,
                                                        acltdtQueueRouteList *qRouteList);

    aclError acltdtAllocBuf(size_t size, acltdtBuf *buf);

    aclError acltdtFreeBuf(acltdtBuf buf);

    aclError acltdtGetBufData(const acltdtBuf buf, void **dataPtr, size_t *size);

    QueueProcessorMdc() = default;
    ~QueueProcessorMdc() = default;

    // not allow copy constructor and assignment operators
    QueueProcessorMdc(const QueueProcessorMdc &) = delete;

    QueueProcessorMdc &operator=(const QueueProcessorMdc &) = delete;

    QueueProcessorMdc(QueueProcessorMdc &&) = delete;

    QueueProcessorMdc &&operator=(QueueProcessorMdc &&) = delete;

private:
    /* data */
};
}



#endif // QUEUE_PROCESS_MDC_H