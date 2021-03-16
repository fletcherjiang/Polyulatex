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

#include <map>
#include <string>
#include "queue.h"

namespace acl {
class QueueProcessor
{
public:

    aclError acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *queueId);

    aclError acltdtDestroyQueue(uint32_t queueId);

    aclError acltdtEnqueueBuf(uint32_t queueId, acltdtBuf *buf, int32_t timeout);

    aclError acltdtDequeueBuf(uint32_t queueId, acltdtBuf **buf, int32_t timeout);

    aclError acltdtGrantQueue(uint32_t queueId, int32_t pid, uint32_t flag);

    aclError acltdtAttachQueue(uint32_t queueId, int32_t timeout, uint32_t *flag);

    aclError acltdtBindQueueRoutes(const acltdtQueueRouteList *qRouteList);

    aclError acltdtUnbindQueueRoutes(const acltdtQueueRouteList *qRouteList);

    aclError acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo,
                                                        acltdtQueueRouteList *qRouteList);

    acltdtBuf* acltdtCreateBuf(size_t size);

    aclError acltdtDestroyBuf(acltdtBuf *buf);

    aclError acltdtGetBufData(const acltdtBuf *buf, void **dataPtr, size_t *size);

    aclError acltdtGetBufPrivData(const acltdtBuf *buf, void **privBuf, size_t *size);

    acltdtQueueAttr *acltdtCreateQueueAttr();

    aclError acltdtDestroyQueueAttr(const acltdtQueueAttr *attr);

    aclError acltdtSetQueueAttr(acltdtQueueAttr *attr,
                                                    acltdtQueueAttrType type,
                                                    size_t len,
                                                    const void *param);

    aclError acltdtGetQueueAttr(const acltdtQueueAttr *attr,
                                                    acltdtQueueAttrType type,
                                                    size_t len,
                                                    size_t *paramRetSize,
                                                    void *param);

    acltdtQueueRoute* acltdtCreateQueueRoute(uint32_t srcQueueId, uint32_t dstQueueId);

    aclError acltdtDestroyQueueRoute(const acltdtQueueRoute *route);

    aclError acltdtGetQueueIdFromQueueRoute(const acltdtQueueRoute *route,
                                                    acltdtQueueRouteKind srcDst,
                                                    uint32_t *queueId);

    aclError acltdtGetQueueRouteStatus(const acltdtQueueRoute *route, int32_t *routeStatus);

    acltdtQueueRouteList* acltdtCreateQueueRouteList();

    aclError acltdtDestroyQueueRouteList(const acltdtQueueRouteList *routeList);

    aclError acltdtAddQueueRoute(acltdtQueueRouteList *routeList, const acltdtQueueRoute *route);

    aclError acltdtGetQueueRoute(const acltdtQueueRouteList *routeList,
                                                    size_t index,
                                                    acltdtQueueRoute *route);

     acltdtQueueRouteQueryInfo* acltdtCreateQueueRouteQueryInfo();

    aclError acltdtDestroyQueueRouteQueryInfo(const acltdtQueueRouteQueryInfo *param);

    aclError acltdtSetQueueRouteQueryInfo(acltdtQueueRouteQueryInfo *param,
                                                            acltdtQueueRouteQueryInfoParamType type,
                                                            size_t len,
                                                            const void *value);

    QueueProcessor() = default;
    ~QueueProcessor() = default;

    // not allow copy constructor and assignment operators
    QueueProcessor(const QueueProcessor &) = delete;

    QueueProcessor &operator=(const QueueProcessor &) = delete;

    QueueProcessor(QueueProcessor &&) = delete;

    QueueProcessor &&operator=(QueueProcessor &&) = delete;

private:
    void Init();
};
}



#endif // QUEUE_PROCESS_H