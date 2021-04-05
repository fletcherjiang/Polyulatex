/**
* @file queue_processor_mdc.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef QUEUE_PROCESSOR_CCPU_H
#define QUEUE_PROCESSOR_CCPU_H

#include <map>
#include <string>
#include "queue_process.h"

namespace acl {

enum PROCESS_STATUS
{
    PROCESS_UNKNOWN,
    PROCESS_MASTER,
    PROCESS_SLAVE,
};

class QueueProcessorCcpu : public QueueProcessor
{
public:
    aclError acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *queueId);

    aclError acltdtDestroyQueue(uint32_t queueId);

    aclError acltdtEnqueueBuf(uint32_t queueId, acltdtBuf buf, int32_t timeout);

    aclError acltdtDequeueBuf(uint32_t queueId, acltdtBuf *buf, int32_t timeout);

    aclError acltdtGrantQueue(uint32_t queueId, int32_t pid, uint32_t flag, int32_t timeout);

    aclError acltdtAttachQueue(uint32_t queueId, int32_t timeout, uint32_t *flag);

    aclError acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList);

    aclError acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList);

    aclError acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo,
                                                        acltdtQueueRouteList *qRouteList);

    aclError QueryGroup(int32_t pid, size_t &grpNum, std::string &grpName);

    aclError CreateGroupIfNoGroup();

    aclError acltdtAllocBuf(size_t size, acltdtBuf *buf);

    aclError acltdtFreeBuf(acltdtBuf buf);

    aclError acltdtGetBufData(const acltdtBuf buf, void **dataPtr, size_t *size);

    QueueProcessorCcpu() = default;
    ~QueueProcessorCcpu() = default;

    // not allow copy constructor and assignment operators
    QueueProcessorCcpu(const QueueProcessorCcpu &) = delete;

    QueueProcessorCcpu &operator=(const QueueProcessorCcpu &) = delete;

    QueueProcessorCcpu(QueueProcessorCcpu &&) = delete;

    QueueProcessorCcpu &&operator=(QueueProcessorCcpu &&) = delete;

private:
    std::string grpName_;
    PROCESS_STATUS procStatus_ = PROCESS_UNKNOWN;
    bool isMbufInit_ = false;
};
}



#endif // QUEUE_PROCESS_CCPU_H