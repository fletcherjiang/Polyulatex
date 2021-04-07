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
#include "toolchain/profiling_manager.h"
#include "toolchain/resource_statistics.h"
#include "queue_process.h"
#include "queue_manager.h"
#include "runtime/rt_mem_queue.h"
#include "runtime/dev.h"
#include "aicpu/queue_schedule/qs_client.h"

namespace {
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
        ACL_LOG_CALL_ERROR("[Get][RunMode]get run mode failed, result = %d.", getRunModeRet);
        return getRunModeRet;
    }
    if (aclRunMode == ACL_HOST) {
        runEnv = ENV_HOST;
    } else if (aclRunMode == ACL_DEVICE) {
        // get env config
        const char *sharePoolPreConfig = std::getenv("SHAREGROUP_PRECONFIG");
        if (sharePoolPreConfig == nullptr) {
            ACL_LOG_INFO("This is not share group preconfig");
            runEnv = ENV_DEVICE_CCPU;
        } else {
            ACL_LOG_INFO("This is share group preconfig");
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
    ACL_REQUIRES_OK(processor->acltdtAttachQueue(qid, timeout * 1000, permission));
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

aclError CheckQueueRouteQueryInfo(const acltdtQueueRouteQueryInfo *queryInfo)
{
    if (!queryInfo->isConfigMode) {
        ACL_LOG_ERROR("mode must be set in acltdtQueueRouteQueryInfo, please ues acltdtSetQueueRouteQueryInfo");
        return ACL_ERROR_INVALID_PARAM;
    }
    switch (queryInfo->mode) {
        case ACL_TDT_QUEUE_ROUTE_QUERY_SRC :
        {
            if (!queryInfo->isConfigSrc) {
                ACL_LOG_ERROR("src qid must be set in acltdtQueueRouteQueryInfo, please ues acltdtSetQueueRouteQueryInfo");
                return ACL_ERROR_INVALID_PARAM;
            }
            break;
        }
        case ACL_TDT_QUEUE_ROUTE_QUERY_DST :
        {
            if (!queryInfo->isConfigDst) {
                ACL_LOG_ERROR("dst qid must be set in acltdtQueueRouteQueryInfo, please ues acltdtSetQueueRouteQueryInfo");
                return ACL_ERROR_INVALID_PARAM;
            }
            break;
        }
        case ACL_TDT_QUEUE_ROUTE_QUERY_SRC_AND_DST :
        {
            if ((!queryInfo->isConfigSrc) || (!queryInfo->isConfigDst)) {
                ACL_LOG_ERROR("src and dst qid must be set in acltdtQueueRouteQueryInfo, please ues acltdtSetQueueRouteQueryInfo");
                return ACL_ERROR_INVALID_PARAM;
            }
            break;
        }
    }
    return ACL_SUCCESS;
}

aclError acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo, acltdtQueueRouteList *qRouteList)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(qRouteList);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(queryInfo);
    ACL_REQUIRES_OK(CheckQueueRouteQueryInfo(queryInfo));
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
    (void)memset_s(attr->name, RT_MQ_MAX_NAME_LEN, 0, RT_MQ_MAX_NAME_LEN);
    attr->depth = 2;
    attr->workMode = RT_MQ_MODE_DEFAULT;
    attr->flowCtrlFlag = false;
    attr->flowCtrlDropTime = 0;
    attr->overWriteFlag = false;
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ATTR);
    return attr;
}

aclError acltdtDestroyQueueAttr(const acltdtQueueAttr *attr)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ATTR);
    ACL_DELETE_AND_SET_NULL(attr);
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ATTR);
    return ACL_SUCCESS;
}

aclError acltdtSetQueueAttr(acltdtQueueAttr *attr,
                            acltdtQueueAttrType type,
                            size_t len,
                            const void *value)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attr);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(value);
    switch (type) {
        case ACL_QUEUE_NAME_PTR:
            {
                char *tmp = nullptr;
                ACL_REQUIRES_OK(CopyParam(value, len, static_cast<void *>(&tmp), sizeof(size_t)));
                if (strlen(tmp) + 1 > RT_MQ_MAX_NAME_LEN) {
                    ACL_LOG_ERROR("queue name len [%zu] can not be larger than %u",
                                  strlen(tmp) + 1, RT_MQ_MAX_NAME_LEN);
                    return ACL_ERROR_INVALID_PARAM;
                }
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
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attr);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(value);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(paramRetSize);
    ACL_LOG_INFO("start to get queue attr, type is %d, len is %zu", type, len);
    switch (type) {
        case ACL_QUEUE_NAME_PTR:
            {
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
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE);
    acltdtQueueRoute *route = new(std::nothrow) acltdtQueueRoute();
    ACL_REQUIRES_NOT_NULL_RET_NULL_INPUT_REPORT(route);
    route->srcId = srcQid;
    route->dstId = dstQid;
    route->status = 0;
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE);
    return route;
}

aclError acltdtDestroyQueueRoute(const acltdtQueueRoute *route)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
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
    }
    return ACL_SUCCESS;
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
    routeList->routeList.push_back(*route);
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
        ACL_LOG_ERROR("[Check][index] index [%zu] can not be larger than [%zu]", index, routeList->routeList.size());
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

aclError acltdtDestroyQueueRouteQueryInfo(const acltdtQueueRouteQueryInfo *param)
{
    ACL_STAGES_REG(acl::ACL_STAGE_QUEUE, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE_QUERY);
    ACL_DELETE_AND_SET_NULL(param);
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
        case ACL_QUEUE_ROUTE_QUERY_MODE_ENUM:
            {
                auto ret = CopyParam(value, len, static_cast<void *>(&param->mode), sizeof(acltdtQueueRouteQueryMode));
                if (ret == ACL_SUCCESS) {
                    param->isConfigMode = true;
                }
                return ret;
            }
        case ACL_QUEUE_ROUTE_QUERY_SRC_ID_UINT32:
            {
                auto ret = CopyParam(value, len, static_cast<void *>(&param->srcId), sizeof(uint32_t));
                if (ret == ACL_SUCCESS) {
                    param->isConfigSrc = true;
                }
                return ret;
            }
        case ACL_QUEUE_ROUTE_QUERY_DST_ID_UINT32:
        {
            auto ret = CopyParam(value, len, static_cast<void *>(&param->dstId), sizeof(uint32_t));
            if (ret == ACL_SUCCESS) {
                param->isConfigDst = true;
            }
            return ret;
        }
    }
    return ACL_SUCCESS;
}