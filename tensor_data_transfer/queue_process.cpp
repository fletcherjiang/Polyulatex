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

namespace acl {
    aclError QueueProcessor::acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *queueId)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtDestroyQueue(uint32_t queueId)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtEnqueueBuf(uint32_t queueId, acltdtBuf *buf, int32_t timeout)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtDequeueBuf(uint32_t queueId, acltdtBuf **buf, int32_t timeout)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtGrantQueue(uint32_t queueId, int32_t pid, uint32_t flag)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtAttachQueue(uint32_t queueId, int32_t timeout, uint32_t *flag)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtBindQueueRoutes(const acltdtQueueRouteList *qRouteList)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtUnbindQueueRoutes(const acltdtQueueRouteList *qRouteList)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo, acltdtQueueRouteList *qRouteList)
    {
        return ACL_SUCCESS;
    }

    acltdtBuf* QueueProcessor::acltdtCreateBuf(size_t size)
    {
        return nullptr;
    }

    aclError QueueProcessor::acltdtDestroyBuf(acltdtBuf *buf)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtGetBufData(const acltdtBuf *buf, void **dataPtr, size_t *size)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtGetBufPrivData(const acltdtBuf *buf, void **privBuf, size_t *size)
    {
        return ACL_SUCCESS;
    }

    acltdtQueueAttr* QueueProcessor::acltdtCreateQueueAttr()
    {
        return nullptr;
    }

    aclError QueueProcessor::acltdtDestroyQueueAttr(const acltdtQueueAttr *attr)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtSetQueueAttr(acltdtQueueAttr *attr,
                                                    acltdtQueueAttrType type,
                                                    size_t len,
                                                    const void *param)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtGetQueueAttr(const acltdtQueueAttr *attr,
                                                    acltdtQueueAttrType type,
                                                    size_t len,
                                                    size_t *paramRetSize,
                                                    void *param)
    {
        return ACL_SUCCESS;
    }

    acltdtQueueRoute* QueueProcessor::acltdtCreateQueueRoute(uint32_t srcQueueId, uint32_t dstQueueId)
    {
        return nullptr;
    }

    aclError QueueProcessor::acltdtDestroyQueueRoute(const acltdtQueueRoute *route)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtGetQueueIdFromQueueRoute(const acltdtQueueRoute *route,
                                                    acltdtQueueRouteKind srcDst,
                                                    uint32_t *queueId)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtGetQueueRouteStatus(const acltdtQueueRoute *route, int32_t *routeStatus)
    {
        return ACL_SUCCESS;
    }

    acltdtQueueRouteList* QueueProcessor::acltdtCreateQueueRouteList()
    {
        return nullptr;
    }

    aclError QueueProcessor::acltdtDestroyQueueRouteList(const acltdtQueueRouteList *routeList)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtAddQueueRoute(acltdtQueueRouteList *routeList, const acltdtQueueRoute *route)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtGetQueueRoute(const acltdtQueueRouteList *routeList,
                                                    size_t index,
                                                    acltdtQueueRoute *route)
    {
        return ACL_SUCCESS;
    }

    acltdtQueueRouteQueryInfo* acltdtCreateQueueRouteQueryInfo()
    {
        return nullptr;
    }

    aclError QueueProcessor::acltdtDestroyQueueRouteQueryInfo(const acltdtQueueRouteQueryInfo *param)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtSetQueueRouteQueryInfo(acltdtQueueRouteQueryInfo *param,
                                                            acltdtQueueRouteQueryInfoParamType type,
                                                            size_t len,
                                                            const void *value)
    {
        return ACL_SUCCESS;
    }

}