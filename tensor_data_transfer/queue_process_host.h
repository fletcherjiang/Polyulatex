/**
* @file queue_processor_host.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef QUEUE_PROCESSOR_HOST_H
#define QUEUE_PROCESSOR_HOST_H

#include <map>
#include <string>
#include "queue_process.h"

namespace acl {
class QueueProcessorHost : public QueueProcessor
{
public:
    aclError acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *queueId);

    aclError acltdtDestroyQueue(uint32_t queueId);

    aclError acltdtEnqueue(uint32_t queueId, acltdtBuf *buf, int32_t timeout);

    aclError acltdtDequeue(uint32_t queueId, acltdtBuf **buf, int32_t timeout);

    aclError acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t permission, int32_t timeout);

    aclError acltdtAttachQueue(uint32_t queueId, int32_t timeout, uint32_t *flag);

    aclError acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList);

    aclError acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList);

    aclError acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo,
                                                        acltdtQueueRouteList *qRouteList);


    QueueProcessorHost() = default;
    ~QueueProcessorHost() = default;

    // not allow copy constructor and assignment operators
    QueueProcessorHost(const QueueProcessorHost &) = delete;

    QueueProcessorHost &operator=(const QueueProcessorHost &) = delete;

    QueueProcessorHost(QueueProcessorHost &&) = delete;

    QueueProcessorHost &&operator=(QueueProcessorHost &&) = delete;
};

}



#endif // QUEUE_PROCESS_HOST_H