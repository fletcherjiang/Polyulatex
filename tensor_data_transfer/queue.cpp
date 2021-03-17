/**
* @file queue.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "queue.h"
#include <mutex>
#include "log_inner.h"
#include "queue_process.h"
#include "runtime/rt_mbuff_queue.h"
#include "runtime/dev.h"


aclError acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *queueId)
{
    ACL_REQUIRES_NOT_NULL(queueId);
    ACL_REQUIRES_NOT_NULL(attr);
    // TODO 加锁，控制面一把锁，数据面两把锁
    int32_t deviceId = 0;
    rtError_t rtRet = rtGetDevice(&deviceId);
    if (rtRet != ACL_SUCCESS) {
        ACL_LOG_CALL_ERROR("[Get][DeviceId]fail to get deviceId result = %d", rtRet);
        return rtRet;
    }
    rtRet = rtMqueueCreate(deviceId, attr, queueId);
    return rtRet;
}

aclError acltdtDestroyQueue(uint32_t queueId)
{
    // TODO 加锁和是否绑定
    int32_t deviceId = 0;
    rtError_t rtRet = rtGetDevice(&deviceId);
    if (rtRet != ACL_SUCCESS) {
        ACL_LOG_CALL_ERROR("[Get][DeviceId]fail to get deviceId result = %d", rtRet);
        return rtRet;
    }
    rtRet = rtMqueueDestroy(queueId);
    return rtRet;
}

aclError acltdtEnqueueBuf(uint32_t queueId, acltdtBuf *buf, int32_t timeout)
{
    return ACL_SUCCESS;
}

aclError acltdtDequeueBuf(uint32_t queueId, acltdtBuf **buf, int32_t timeout)
{
    return ACL_SUCCESS;
}

aclError acltdtGrantQueue(uint32_t queueId, int32_t pid, uint32_t flag)
{
    return ACL_SUCCESS;
}

aclError acltdtAttachQueue(uint32_t queueId, int32_t timeout, uint32_t *flag)
{
    return ACL_SUCCESS;
}

aclError acltdtBindQueueRoutes(const acltdtQueueRouteList *qRouteList)
{
    return ACL_SUCCESS;
}

aclError acltdtUnbindQueueRoutes(const acltdtQueueRouteList *qRouteList)
{
    return ACL_SUCCESS;
}

aclError acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo, acltdtQueueRouteList *qRouteList)
{
    return ACL_SUCCESS;
}

acltdtBuf* acltdtCreateBuf(size_t size)
{
    return nullptr;
}

aclError acltdtDestroyBuf(acltdtBuf *buf)
{
    return ACL_SUCCESS;
}

aclError acltdtGetBufData(const acltdtBuf *buf, void **dataPtr, size_t *size)
{
    return ACL_SUCCESS;
}

aclError acltdtGetBufPrivData(const acltdtBuf *buf, void **privBuf, size_t *size)
{
    return ACL_SUCCESS;
}

acltdtQueueAttr* acltdtCreateQueueAttr()
{
    return nullptr;
}

aclError acltdtDestroyQueueAttr(const acltdtQueueAttr *attr)
{
    return ACL_SUCCESS;
}

aclError acltdtSetQueueAttr(acltdtQueueAttr *attr,
                                                acltdtQueueAttrType type,
                                                size_t len,
                                                const void *param)
{
    return ACL_SUCCESS;
}

aclError acltdtGetQueueAttr(const acltdtQueueAttr *attr,
                                                acltdtQueueAttrType type,
                                                size_t len,
                                                size_t *paramRetSize,
                                                void *param)
{
    return ACL_SUCCESS;
}

acltdtQueueRoute* acltdtCreateQueueRoute(uint32_t srcQueueId, uint32_t dstQueueId)
{
    return nullptr;
}

aclError acltdtDestroyQueueRoute(const acltdtQueueRoute *route)
{
    return ACL_SUCCESS;
}

aclError acltdtGetQueueIdFromQueueRoute(const acltdtQueueRoute *route,
                                                   acltdtQueueRouteKind srcDst,
                                                   uint32_t *queueId)
{
    return ACL_SUCCESS;
}

aclError acltdtGetQueueRouteStatus(const acltdtQueueRoute *route, int32_t *routeStatus)
{
    return ACL_SUCCESS;
}

acltdtQueueRouteList* acltdtCreateQueueRouteList()
{
    return nullptr;
}

aclError acltdtDestroyQueueRouteList(const acltdtQueueRouteList *routeList)
{
    return ACL_SUCCESS;
}

aclError acltdtAddQueueRoute(acltdtQueueRouteList *routeList, const acltdtQueueRoute *route)
{
    return ACL_SUCCESS;
}

aclError acltdtGetQueueRoute(const acltdtQueueRouteList *routeList,
                                                 size_t index,
                                                 acltdtQueueRoute *route)
{
    return ACL_SUCCESS;
}

 acltdtQueueRouteQueryInfo* acltdtCreateQueueRouteQueryInfo()
 {
     return nullptr;
 }

aclError acltdtDestroyQueueRouteQueryInfo(const acltdtQueueRouteQueryInfo *param)
{
    return ACL_SUCCESS;
}

aclError acltdtSetQueueRouteQueryInfo(acltdtQueueRouteQueryInfo *param,
                                                          acltdtQueueRouteQueryInfoParamType type,
                                                          size_t len,
                                                          const void *value)
{
    return ACL_SUCCESS;
}