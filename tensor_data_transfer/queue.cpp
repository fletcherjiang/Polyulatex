/**
* @file queue.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "queue.h"
#include <mutex>
#include <map>
#include "common/log_inner.h"
#include "toolchain/profiling_manager.h"
#include "toolchain/resource_statistics.h"
#include "queue_process.h"
#include "queue_manager.h"
#include "runtime/rt_mem_queue.h"
#include "runtime/dev.h"
#include "aicpu/queue_schedule/qs_client.h"

namespace {
    constexpr uint32_t DEFAULT_QUEU_DEPTH = 2U;
    aclError CopyParam(const void *src, size_t srcLen, void *dst, size_t dstLen, size_t *realCopySize = nullptr)
    {
        ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(src);
        ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dst);
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

namespace acl {
    aclError CheckQueueRouteQueryInfo(const acltdtQueueRouteQueryInfo *queryInfo)
    {
        if (!queryInfo->isConfigMode) {
            ACL_LOG_ERROR("mode must be set in acltdtQueueRouteQueryInfo, please use acltdtSetQueueRouteQueryInfo");
            return ACL_ERROR_INVALID_PARAM;
        }
        switch (queryInfo->mode) {
            case ACL_TDT_QUEUE_ROUTE_QUERY_SRC: {
                if (!queryInfo->isConfigSrc) {
                    ACL_LOG_ERROR("src qid must be set in acltdtQueueRouteQueryInfo,"
                                "please use acltdtSetQueueRouteQueryInfo");
                    return ACL_ERROR_INVALID_PARAM;
                }
                break;
            }
            case ACL_TDT_QUEUE_ROUTE_QUERY_DST: {
                if (!queryInfo->isConfigDst) {
                    ACL_LOG_ERROR("dst qid must be set in acltdtQueueRouteQueryInfo,"
                                "please use acltdtSetQueueRouteQueryInfo");
                    return ACL_ERROR_INVALID_PARAM;
                }
                break;
            }
            case ACL_TDT_QUEUE_ROUTE_QUERY_SRC_AND_DST: {
                if ((!queryInfo->isConfigSrc) || (!queryInfo->isConfigDst)) {
                    ACL_LOG_ERROR("src and dst qid must be set in acltdtQueueRouteQueryInfo,"
                                "please use acltdtSetQueueRouteQueryInfo");
                    return ACL_ERROR_INVALID_PARAM;
                }
                break;
            }
            default: {
                ACL_LOG_INNER_ERROR("[Check][Type]unkown mode %d.", queryInfo->mode);
                return ACL_ERROR_INVALID_PARAM;
            }
        }
        return ACL_SUCCESS;
    }
}

aclError acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *qid)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ID);
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    ACL_REQUIRES_NOT_NULL_WITH_INNER_REPORT(processor);
    ACL_REQUIRES_OK(processor->acltdtCreateQueue(attr, qid));
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ID);
    return ACL_SUCCESS;
}

aclError acltdtDestroyQueue(uint32_t qid)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ID);
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    ACL_REQUIRES_NOT_NULL_WITH_INNER_REPORT(processor);
    ACL_REQUIRES_OK(processor->acltdtDestroyQueue(qid));
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ID);
    return ACL_SUCCESS;
}

aclError acltdtEnqueue(uint32_t qid, acltdtBuf buf, int32_t timeout)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_PROFILING_REG(MSPROF_ACL_API_TYPE_OTHERS);
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    ACL_REQUIRES_NOT_NULL_WITH_INNER_REPORT(processor);
    ACL_REQUIRES_OK(processor->acltdtEnqueue(qid, buf, timeout));
    return ACL_SUCCESS;
}

aclError acltdtDequeue(uint32_t qid, acltdtBuf *buf, int32_t timeout)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_PROFILING_REG(MSPROF_ACL_API_TYPE_OTHERS);
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    ACL_REQUIRES_NOT_NULL_WITH_INNER_REPORT(processor);
    ACL_REQUIRES_OK(processor->acltdtDequeue(qid, buf, timeout));
    return ACL_SUCCESS;
}

aclError acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t permission, int32_t timeout)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    ACL_REQUIRES_NOT_NULL_WITH_INNER_REPORT(processor);
    ACL_REQUIRES_OK(processor->acltdtGrantQueue(qid, pid, permission, timeout));
    return ACL_SUCCESS;
}

aclError acltdtAttachQueue(uint32_t qid, int32_t timeout, uint32_t *permission)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    ACL_REQUIRES_NOT_NULL_WITH_INNER_REPORT(processor);
    ACL_REQUIRES_OK(processor->acltdtAttachQueue(qid, timeout, permission));
    return ACL_SUCCESS;
}

aclError acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    ACL_REQUIRES_NOT_NULL_WITH_INNER_REPORT(processor);
    ACL_REQUIRES_OK(processor->acltdtBindQueueRoutes(qRouteList));
    return ACL_SUCCESS;
}

aclError acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    ACL_REQUIRES_NOT_NULL_WITH_INNER_REPORT(processor);
    ACL_REQUIRES_OK(processor->acltdtUnbindQueueRoutes(qRouteList));
    return ACL_SUCCESS;
}

aclError acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo, acltdtQueueRouteList *qRouteList)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(qRouteList);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(queryInfo);
    qRouteList->routeList.clear();
    ACL_REQUIRES_OK(acl::CheckQueueRouteQueryInfo(queryInfo));
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    ACL_REQUIRES_NOT_NULL_WITH_INNER_REPORT(processor);
    ACL_REQUIRES_OK(processor->acltdtQueryQueueRoutes(queryInfo, qRouteList));
    return ACL_SUCCESS;
}

aclError acltdtAllocBuf(size_t size, acltdtBuf *buf)
{
    ACL_STAGES_REG(acl::ACL_STAGE_MBUF, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_MBUF);
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    ACL_REQUIRES_NOT_NULL_WITH_INNER_REPORT(processor);
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_MBUF);
    return processor->acltdtAllocBuf(size, buf);
}

aclError acltdtFreeBuf(acltdtBuf buf)
{
    ACL_STAGES_REG(acl::ACL_STAGE_MBUF, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_MBUF);
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    ACL_REQUIRES_NOT_NULL_WITH_INNER_REPORT(processor);
    ACL_REQUIRES_OK(processor->acltdtFreeBuf(buf));
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_MBUF);
    return ACL_SUCCESS;
}

aclError acltdtGetBufData(const acltdtBuf buf, void **dataPtr, size_t *size)
{
    ACL_STAGES_REG(acl::ACL_STAGE_MBUF, acl::ACL_STAGE_DEFAULT);
    auto& qManager = acl::QueueManager::GetInstance();
    auto processor = qManager.GetQueueProcessor();
    ACL_REQUIRES_NOT_NULL_WITH_INNER_REPORT(processor);
    ACL_REQUIRES_OK(processor->acltdtGetBufData(buf, dataPtr, size));
    return ACL_SUCCESS;
}

acltdtQueueAttr* acltdtCreateQueueAttr()
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ATTR);
    acltdtQueueAttr *attr = new(std::nothrow) acltdtQueueAttr();
    ACL_REQUIRES_NOT_NULL_RET_NULL_INPUT_REPORT(attr);
    (void)memset_s(attr->name, static_cast<size_t>(RT_MQ_MAX_NAME_LEN), 0, static_cast<size_t>(RT_MQ_MAX_NAME_LEN));
    attr->depth = static_cast<uint32_t>(DEFAULT_QUEU_DEPTH);
    attr->workMode = static_cast<uint32_t>(RT_MQ_MODE_DEFAULT);
    attr->flowCtrlFlag = false;
    attr->flowCtrlDropTime = 0U;
    attr->overWriteFlag = false;
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ATTR);
    return attr;
}

aclError acltdtDestroyQueueAttr(const acltdtQueueAttr *attr)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attr);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ATTR);
    ACL_DELETE_AND_SET_NULL(attr);
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ATTR);
    return ACL_SUCCESS;
}

aclError acltdtSetQueueAttr(acltdtQueueAttr *attr,
                            acltdtQueueAttrType type,
                            size_t len,
                            const void *param)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attr);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(param);
    switch (type) {
        case ACL_TDT_QUEUE_NAME_PTR: {
                char *tmp = nullptr;
                ACL_REQUIRES_OK(CopyParam(param, len, static_cast<void *>(&tmp), sizeof(size_t)));
                if ((strlen(tmp) + 1) > static_cast<uint64_t>(RT_MQ_MAX_NAME_LEN)) {
                    ACL_LOG_ERROR("queue name len [%zu] can not be larger than %d",
                                  (strlen(tmp) + 1), RT_MQ_MAX_NAME_LEN);
                    return ACL_ERROR_INVALID_PARAM;
                }
                return CopyParam(tmp, (strlen(tmp) + 1), static_cast<void *>(attr->name), RT_MQ_MAX_NAME_LEN);
            }
        case ACL_TDT_QUEUE_DEPTH_UINT32:
            return CopyParam(param, len, static_cast<void *>(&attr->depth), sizeof(uint32_t));
        default: {
            ACL_LOG_INNER_ERROR("[Check][Type]unkown acltdtQueueAttrType %d.", type);
            return ACL_ERROR_INVALID_PARAM;
        }
    }
}

aclError acltdtGetQueueAttr(const acltdtQueueAttr *attr,
                            acltdtQueueAttrType type,
                            size_t len,
                            size_t *paramRetSize,
                            void *param)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attr);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(param);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(paramRetSize);
    ACL_LOG_INFO("start to get queue attr, type is %d, len is %zu", type, len);
    switch (type) {
        case ACL_TDT_QUEUE_NAME_PTR: {
                const char *tmp = &attr->name[0];
                return CopyParam(static_cast<const void *>(&tmp), sizeof(size_t), param, len, paramRetSize);
            }
        case ACL_TDT_QUEUE_DEPTH_UINT32:
            return CopyParam(static_cast<const void *>(&attr->depth), sizeof(uint32_t), param, len, paramRetSize);
        default: {
            ACL_LOG_INNER_ERROR("[Check][Type]unkown acltdtQueueAttrType %d.", type);
            return ACL_ERROR_INVALID_PARAM;
        }
    }
}

acltdtQueueRoute* acltdtCreateQueueRoute(uint32_t srcId, uint32_t dstId)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE);
    acltdtQueueRoute *route = new(std::nothrow) acltdtQueueRoute();
    ACL_REQUIRES_NOT_NULL_RET_NULL_INPUT_REPORT(route);
    route->srcId = srcId;
    route->dstId = dstId;
    route->status = 0;
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE);
    return route;
}

aclError acltdtDestroyQueueRoute(const acltdtQueueRoute *route)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(route);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE);
    ACL_DELETE_AND_SET_NULL(route);
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE);
    return ACL_SUCCESS;
}

aclError acltdtGetQueueRouteParam(const acltdtQueueRoute *route,
                                  acltdtQueueRouteParamType type,
                                  size_t len,
                                  size_t *paramRetSize,
                                  void *param)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(route);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(param);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(paramRetSize);
    ACL_LOG_INFO("get route type %d, len is %zu", type, len);
    switch (type) {
        case ACL_TDT_QUEUE_ROUTE_SRC_UINT32:
            return CopyParam(static_cast<const void *>(&route->srcId), sizeof(uint32_t), param, len, paramRetSize);
        case ACL_TDT_QUEUE_ROUTE_DST_UINT32:
            return CopyParam(static_cast<const void *>(&route->dstId), sizeof(uint32_t), param, len, paramRetSize);
        case ACL_TDT_QUEUE_ROUTE_STATUS_INT32:
            return CopyParam(static_cast<const void *>(&route->status), sizeof(int32_t), param, len, paramRetSize);
        default: {
            ACL_LOG_INNER_ERROR("[Check][Type]unkown acltdtQueueRouteParamType %d.", type);
            return ACL_ERROR_INVALID_PARAM;
        }
    }
}

acltdtQueueRouteList* acltdtCreateQueueRouteList()
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE_LIST);
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE_LIST);
    return new(std::nothrow) acltdtQueueRouteList();
}

aclError acltdtDestroyQueueRouteList(const acltdtQueueRouteList *routeList)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(routeList);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE_LIST);
    ACL_DELETE_AND_SET_NULL(routeList);
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE_LIST);
    return ACL_SUCCESS;
}

aclError acltdtAddQueueRoute(acltdtQueueRouteList *routeList, const acltdtQueueRoute *route)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(routeList);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(route);
    routeList->routeList.emplace_back(*route);
    return ACL_SUCCESS;
}

aclError acltdtGetQueueRoute(const acltdtQueueRouteList *routeList,
                             size_t index,
                             acltdtQueueRoute *route)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(routeList);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(route);
    if (index >= routeList->routeList.size()) {
        ACL_LOG_ERROR("[Check][index] index [%zu] must be smaller than [%zu]", index, routeList->routeList.size());
        return ACL_ERROR_INVALID_PARAM;
    }
    *route = routeList->routeList[index];
    return ACL_SUCCESS;
}

acltdtQueueRouteQueryInfo* acltdtCreateQueueRouteQueryInfo()
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE_QUERY);
    acltdtQueueRouteQueryInfo *info = new(std::nothrow) acltdtQueueRouteQueryInfo();
    ACL_REQUIRES_NOT_NULL_RET_NULL_INPUT_REPORT(info);
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE_QUERY);
    info->isConfigDst = false;
    info->isConfigSrc = false;
    info->isConfigMode = false;
    return info;
}

aclError acltdtDestroyQueueRouteQueryInfo(const acltdtQueueRouteQueryInfo *info)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(info);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE_QUERY);
    ACL_DELETE_AND_SET_NULL(info);
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE_QUERY);
    return ACL_SUCCESS;
}

aclError acltdtSetQueueRouteQueryInfo(acltdtQueueRouteQueryInfo *param,
                                      acltdtQueueRouteQueryInfoParamType type,
                                      size_t len,
                                      const void *value)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(param);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(value);
    switch (type) {
        case ACL_TDT_QUEUE_ROUTE_QUERY_MODE_ENUM: {
                auto ret = CopyParam(value, len, static_cast<void *>(&param->mode), sizeof(acltdtQueueRouteQueryMode));
                if (ret == ACL_SUCCESS) {
                    param->isConfigMode = true;
                }
                return ret;
            }
        case ACL_TDT_QUEUE_ROUTE_QUERY_SRC_ID_UINT32: {
                auto ret = CopyParam(value, len, static_cast<void *>(&param->srcId), sizeof(uint32_t));
                if (ret == ACL_SUCCESS) {
                    param->isConfigSrc = true;
                }
                return ret;
            }
        case ACL_TDT_QUEUE_ROUTE_QUERY_DST_ID_UINT32: {
            auto ret = CopyParam(value, len, static_cast<void *>(&param->dstId), sizeof(uint32_t));
            if (ret == ACL_SUCCESS) {
                param->isConfigDst = true;
            }
            return ret;
        }
        default: {
            ACL_LOG_INNER_ERROR("[Check][Type]unkown acltdtQueueRouteQueryInfoParamType %d.", type);
            return ACL_ERROR_INVALID_PARAM;
        }
    }
}