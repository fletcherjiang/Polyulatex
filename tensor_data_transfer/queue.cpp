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
#include <map>
#include "common/log_inner.h"
#include "queue_process.h"
#include "queue_manager.h"
#include "runtime/rt_mem_queue.h"
#include "runtime/dev.h"
#include "aicpu/queue_schedule/qs_client.h"

// 最后需要排查rprofling统计，大小阶段，错误日志上报
namespace {
    aclError CopyParam(const void *src, size_t srcLen, void *dst, size_t dstLen, size_t *realCopySize = nullptr)
    {
        ACL_REQUIRES_NOT_NULL(src);
        ACL_REQUIRES_NOT_NULL(dst);
        if (srcLen > dstLen) {
            ACL_LOG_INNER_ERROR("[Check][Len]src length=%zu is larger than dst length=%zu when memcpy", srcLen, dstLen);
            return ACL_ERROR_INVALID_PARAM;
        }
        auto ret = memcpy_s(dst, dstLen, src, srcLen);
        if (ret != EOK) {
            ACL_LOG_INNER_ERROR("[Call][MemCpy]call memcpy failed, result=%d, srcLen=%zu, dstLen=%zu",
                ret, srcLen, dstLen);
            return ACL_ERROR_FAILURE;
        }
        if (realCopySize != nullptr) {
            *realCopySize = srcLen;
        }
        return ACL_SUCCESS;
    }
}

uint64_t GetTimestamp()
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

aclError GetRunningEnv(RunEnv &runEnv)
{
    // get acl run mode
    if (runEnv != ENV_UNKNOWN) {
        return ACL_SUCCESS;
    }
    aclrtRunMode aclRunMode;
    aclError getRunModeRet = aclrtGetRunMode(&aclRunMode);
    if (getRunModeRet != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Get][RunMode]get run mode failed, result = %d.", getRunModeRet);
        return getRunModeRet;
    }
    if (aclRunMode == ACL_HOST) {
        runEnv = ENV_HOST;
    } else if (aclRunMode == ACL_DEVICE) {
        // get env config
        const char *sharePoolPreConfig = std::getenv("SHAREGROUP_PRECONFIG");
        if (sharePoolPreConfig == nullptr) {
            runEnv = ENV_DEVICE_DEFAULT;
        } else {
            runEnv = ENV_DEVICE_MDC;
        }
    } else {
        ACL_LOG_INNER_ERROR("[Get][RunMode]get run mode failed, result = %d.", getRunModeRet);
        return ACL_ERROR_FAILURE;
    }
    return ACL_SUCCESS;
}

aclError acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *qid)
{
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    if (processor == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][Queueprocessor] queue processor is nullptr");
        return ACL_ERROR_FAILURE;
    }
    ACL_REQUIRES_OK(processor->acltdtCreateQueue(attr, qid));
    return ACL_SUCCESS;
}

aclError acltdtDestroyQueue(uint32_t qid)
{
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    if (processor == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][Queueprocessor] queue processor is nullptr");
        return ACL_ERROR_FAILURE;
    }
    ACL_REQUIRES_OK(processor->acltdtDestroyQueue(qid));
    return ACL_SUCCESS;
}

aclError acltdtEnqueueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout)
{
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    if (processor == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][Queueprocessor] queue processor is nullptr");
        return ACL_ERROR_FAILURE;
    }
    ACL_REQUIRES_OK(processor->acltdtEnqueueBuf(qid, buf, timeout));
    return ACL_SUCCESS;
}

aclError acltdtDequeueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout)
{
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    if (processor == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][Queueprocessor] queue processor is nullptr");
        return ACL_ERROR_FAILURE;
    }
    ACL_REQUIRES_OK(processor->acltdtDequeueBuf(qid, buf, timeout));
    return ACL_SUCCESS;
}

aclError acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t permission, int32_t timeout)
{
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    if (processor == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][Queueprocessor] queue processor is nullptr");
        return ACL_ERROR_FAILURE;
    }
    ACL_REQUIRES_OK(processor->acltdtGrantQueue(qid, pid, permission, timeout));
    return ACL_SUCCESS;
}

aclError acltdtAttachQueue(uint32_t qid, int32_t timeout, uint32_t *permission)
{
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    if (processor == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][Queueprocessor] queue processor is nullptr");
        return ACL_ERROR_FAILURE;
    }
    ACL_REQUIRES_OK(processor->acltdtAttachQueue(qid, timeout, permission));
    return ACL_SUCCESS;
}

aclError acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList)
{
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    if (processor == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][QueueScheduleprocessor] queue schedule processor is nullptr");
        return ACL_ERROR_FAILURE;
    }
    ACL_REQUIRES_OK(processor->acltdtBindQueueRoutes(qRouteList));
    return ACL_SUCCESS;
}

aclError acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList)
{
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    if (processor == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][QueueScheduleprocessor] queue schedule processor is nullptr");
        return ACL_ERROR_FAILURE;
    }
    ACL_REQUIRES_OK(processor->acltdtUnbindQueueRoutes(qRouteList));
    return ACL_SUCCESS;
}

aclError acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo, acltdtQueueRouteList *qRouteList)
{
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    if (processor == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][QueueScheduleprocessor] queue schedule processor is nullptr");
        return ACL_ERROR_FAILURE;
    }
    ACL_REQUIRES_OK(processor->acltdtQueryQueueRoutes(queryInfo, qRouteList));
    return ACL_SUCCESS;
}

acltdtBuf* acltdtCreateBuf(size_t size)
{
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    if (processor == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][QueueScheduleprocessor] queue schedule processor is nullptr");
        return nullptr;
    }
    return processor->acltdtCreateBuf(size);
}

aclError acltdtDestroyBuf(acltdtBuf *buf)
{
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    if (processor == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][QueueScheduleprocessor] queue schedule processor is nullptr");
        return ACL_ERROR_FAILURE;
    }
    ACL_REQUIRES_OK(processor->acltdtDestroyBuf(buf));
    return ACL_SUCCESS;
}

aclError acltdtGetBufData(const acltdtBuf *buf, void **dataPtr, size_t *size)
{
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    if (processor == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][QueueScheduleprocessor] queue schedule processor is nullptr");
        return ACL_ERROR_FAILURE;
    }
    ACL_REQUIRES_OK(processor->acltdtGetBufData(buf, dataPtr, size));
    return ACL_SUCCESS;
}

aclError acltdtGetBufPrivData(const acltdtBuf *buf, void **privBuf, size_t *size)
{
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    if (processor == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][QueueScheduleprocessor] queue schedule processor is nullptr");
        return ACL_ERROR_FAILURE;
    }
    ACL_REQUIRES_OK(processor->acltdtGetBufPrivData(buf, privBuf, size));
    return ACL_SUCCESS;
}

acltdtQueueAttr* acltdtCreateQueueAttr()
{
    acltdtQueueAttr *attr = new(std::nothrow) acltdtQueueAttr();
    if (attr == nullptr) {
        ACL_LOG_ERROR("[Check][Malloc]Allocate memory for acltdtQueueAttr.");
        return nullptr;
    }
    attr->workMode = RT_MQ_MODE_DEFAULT;
    attr->flowCtrlFlag = false;
    attr->flowCtrlDropTime = 0;
    attr->overWriteFlag = false;
    return attr;
}

aclError acltdtDestroyQueueAttr(const acltdtQueueAttr *attr)
{
    ACL_DELETE_AND_SET_NULL(attr);
    return ACL_SUCCESS;
}

aclError acltdtSetQueueAttr(acltdtQueueAttr *attr,
                                                acltdtQueueAttrType type,
                                                size_t len,
                                                const void *value)
{
    ACL_REQUIRES_NOT_NULL(attr);
    ACL_REQUIRES_NOT_NULL(value);
    switch (type) {
        case ACL_QUEUE_NAME_PTR:
            {
                // 需要测试下
                char *tmp = nullptr;
                ACL_REQUIRES_OK(CopyParam(value, len, static_cast<void *>(&tmp), sizeof(size_t)));
                return CopyParam(tmp, strlen(tmp) + 1, static_cast<void *>(attr->name), RT_MQ_MAX_NAME_LEN);
            }
        case ACL_QUEUE_DEPTH_UINT32:
            return CopyParam(value, len, static_cast<void *>(&attr->depth), sizeof(uint32_t));
    }
    return ACL_SUCCESS;
}

aclError acltdtGetQueueAttr(const acltdtQueueAttr *attr,
                                                acltdtQueueAttrType type,
                                                size_t len,
                                                size_t *paramRetSize,
                                                void *value)
{
    ACL_REQUIRES_NOT_NULL(attr);
    ACL_REQUIRES_NOT_NULL(value);
    ACL_REQUIRES_NOT_NULL(paramRetSize);
    switch (type) {
        case ACL_QUEUE_NAME_PTR:
            {
                // 需要测试下
                const char *tmp = &attr->name[0];
                return CopyParam(static_cast<const void *>(&tmp), sizeof(size_t), value, len, paramRetSize);
            }
        case ACL_QUEUE_DEPTH_UINT32:
            return CopyParam(static_cast<const void *>(&attr->depth), sizeof(uint32_t), value, len, paramRetSize);
    }
    return ACL_SUCCESS;
}

acltdtQueueRoute* acltdtCreateQueueRoute(uint32_t srcQid, uint32_t dstQid)
{
    acltdtQueueRoute *route = new(std::nothrow) acltdtQueueRoute();
    if (route == nullptr) {
        ACL_LOG_ERROR("new acltdtQueueRoute failed");
        return nullptr;
    }
    route->srcId = srcQid;
    route->dstId = dstQid;
    route->status = 0;
    return route;
}

aclError acltdtDestroyQueueRoute(const acltdtQueueRoute *route)
{
    ACL_DELETE_AND_SET_NULL(route);
    return ACL_SUCCESS;
}

aclError acltdtGetqidFromQueueRoute(const acltdtQueueRoute *route,
                                    acltdtQueueRouteKind routeKind,
                                    uint32_t *qid)
{
    ACL_REQUIRES_NOT_NULL(route);
    ACL_REQUIRES_NOT_NULL(qid);
    switch (routeKind) {
        case ACL_QUEUE_SRC_ID: 
            *qid = route->srcId;
            break;
        case ACL_QUEUE_DST_ID: 
            *qid = route->dstId;
            break;
    }
    return ACL_SUCCESS;
}

aclError acltdtGetQueueRouteStatus(const acltdtQueueRoute *route, int32_t *routeStatus)
{
    ACL_REQUIRES_NOT_NULL(route);
    ACL_REQUIRES_NOT_NULL(routeStatus);
    *routeStatus = route->status;
    return ACL_SUCCESS;
}

acltdtQueueRouteList* acltdtCreateQueueRouteList()
{
    return new(std::nothrow) acltdtQueueRouteList();
}

aclError acltdtDestroyQueueRouteList(const acltdtQueueRouteList *routeList)
{
    ACL_DELETE_AND_SET_NULL(routeList);
    return ACL_SUCCESS;
}

aclError acltdtAddQueueRoute(acltdtQueueRouteList *routeList, const acltdtQueueRoute *route)
{
    ACL_REQUIRES_NOT_NULL(routeList);
    ACL_REQUIRES_NOT_NULL(route);
    routeList->routeList.push_back(*route);
    return ACL_SUCCESS;
}

aclError acltdtGetQueueRoute(const acltdtQueueRouteList *routeList,
                             size_t index,
                             acltdtQueueRoute *route)
{
    ACL_REQUIRES_NOT_NULL(routeList);
    ACL_REQUIRES_NOT_NULL(route);
    if (index >= routeList->routeList.size()) {
        ACL_LOG_ERROR("[Check][index] index [%zu] can not be larger than [%zu]", index, routeList->routeList.size());
        // 上报
        return ACL_ERROR_INVALID_PARAM;
    }
    *route = routeList->routeList[index];
    return ACL_SUCCESS;
}

 acltdtQueueRouteQueryInfo* acltdtCreateQueueRouteQueryInfo()
 {
    return new(std::nothrow) acltdtQueueRouteQueryInfo();
 }

aclError acltdtDestroyQueueRouteQueryInfo(const acltdtQueueRouteQueryInfo *param)
{
    ACL_DELETE_AND_SET_NULL(param);
    return ACL_SUCCESS;
}

aclError acltdtSetQueueRouteQueryInfo(acltdtQueueRouteQueryInfo *param,
                                      acltdtQueueRouteQueryInfoParamType type,
                                      size_t len,
                                      const void *value)
{
    ACL_REQUIRES_NOT_NULL(param);
    ACL_REQUIRES_NOT_NULL(value);
    switch (type) {
        case ACL_QUEUE_ROUTE_QUERY_MODE_ENUM:
            return CopyParam(value, len, static_cast<void *>(&param->mode), sizeof(acltdtQueueRouteQueryMode));
        case ACL_QUEUE_ROUTE_QUERY_SRC_ID_UINT32:
            return CopyParam(value, len, static_cast<void *>(&param->srcId), sizeof(uint32_t));
        case ACL_QUEUE_ROUTE_QUERY_DST_ID_UINT32:
            return CopyParam(value, len, static_cast<void *>(&param->dstId), sizeof(uint32_t));
    }
    return ACL_SUCCESS;
}