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
#include "runtime/rt_mem_queue.h"
#include "runtime/dev.h"



// 最后需要排查rprofling统计，大小阶段，错误日志上报
namespace {
    RunEnv RUN_ENV = ENV_UNKNOWN;
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
     aclError CopyParam(const void *src, size_t srcLen, void *dst, size_t dstLen)
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

        return ACL_SUCCESS;
    }
}

aclError GetRunningEnv()
{
    // get acl run mode
    if (RUN_ENV != ENV_UNKNOWN) {
        return ACL_SUCCESS;
    }
    aclrtRunMode aclRunMode;
    aclError getRunModeRet = aclrtGetRunMode(&aclRunMode);
    if (getRunModeRet != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Get][RunMode]get run mode failed, result = %d.", getRunModeRet);
        return getRunModeRet;
    }
    if (aclRunMode == ACL_HOST) {
        RUN_ENV = ENV_HOST;
    } else if (aclRunMode == ACL_DEVICE) {
        // get env config
        const char *sharePoolPreConfig = std::getenv("SHAREGROUP_PRECONFIG");
        if (sharePoolPreConfig == nullptr) {
            RUN_ENV = ENV_DEVICE_DEFAULT;
        } else {
            RUN_ENV = ENV_DEVICE_MDC;
        }
    } else {
        ACL_LOG_INNER_ERROR("[Get][RunMode]get run mode failed, result = %d.", getRunModeRet);
        return ACL_ERROR_FAILURE;
    }
    return ACL_SUCCESS;
}

aclError acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *qid)
{
    ACL_REQUIRES_NOT_NULL(qid);
    ACL_REQUIRES_NOT_NULL(attr);
    ACL_REQUIRES_OK(GetRunningEnv());
    // TODO 加锁，控制面一把锁，数据面两把锁
    int32_t deviceId = 0;
    rtError_t rtRet = RT_ERROR_NONE;
    if (RUN_ENV != ENV_DEVICE_MDC) {
        rtRet = rtGetDevice(&deviceId);
        if (rtRet != ACL_SUCCESS) {
            ACL_LOG_CALL_ERROR("[Get][DeviceId]fail to get deviceId result = %d", rtRet);
            return rtRet;
        }
    }
    static bool isQueueIint = false;
    if (!isQueueIint) {
        rtRet = rtMemQueueInit(deviceId);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Init][Queue]fail to init queue result = %d", rtRet);
            return rtRet;
        } 
    }
    rtRet = rtMemQueueCreate(deviceId, attr, qid);
    //TODO ccpu上需要给cp加权限
    return rtRet;
}

aclError acltdtDestroyQueue(uint32_t qid)
{
    ACL_REQUIRES_OK(GetRunningEnv());
    // TODO 加锁和是否绑定
    int32_t deviceId = 0;
    GET_CURRENT_DEVICE_ID(deviceId);
    // TODO 有绑定结果需要报错
    rtError_t rtRet = rtMemQueueDestroy(deviceId, qid);
    return rtRet;
}

aclError acltdtEnqueueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout)
{
    ACL_REQUIRES_NOT_NULL(buf);
    int32_t deviceId = 0;
    GET_CURRENT_DEVICE_ID(deviceId);
    // 加锁
    rtError_t rtRet = rtMemQueueEnqueue(deviceId, qid, buf->mbuf);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[Enqueue][Queue]fail to enqueue result = %d", rtRet);
        return rtRet;
    }
    return ACL_SUCCESS;
}

aclError acltdtDequeueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout)
{
    ACL_REQUIRES_NOT_NULL(buf);
    int32_t deviceId = 0;
    GET_CURRENT_DEVICE_ID(deviceId);
    // 加锁
    rtError_t rtRet = rtMemQueueDequeue(deviceId, qid, &buf->mbuf);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[Dequeue][Queue]fail to enqueue result = %d", rtRet);
        return rtRet;
    }
    return ACL_SUCCESS;
}

aclError acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t flag, int32_t timeout)
{
    ACL_REQUIRES_OK(GetRunningEnv());
    ACL_REQUIRES_POSITIVE(timeout);
    if (RUN_ENV != ENV_DEVICE_MDC) {
        if (flag & 0x1) {
            ACL_LOG_ERROR("[CHECK][permission]permission manager is not allowed");
            return ACL_ERROR_INVALID_PARAM;
        };// 校验不能是管理权限
    }
    uint64_t startTime = GetTimestamp();
    uint64_t endTime = 0;
    do {
        if (RUN_ENV == ENV_DEVICE_MDC) {
            ; // 调用驱动函数
        }
        endTime = GetTimestamp();
    } while ((endTime - startTime >= (timeout * 10000)));
    return ACL_SUCCESS;
}

aclError acltdtAttachQueue(uint32_t qid, int32_t timeout, uint32_t *permission)
{
    ACL_REQUIRES_NOT_NULL(permission);
    ACL_REQUIRES_OK(GetRunningEnv());
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
    if (GetRunningEnv() != ACL_SUCCESS) {
        return nullptr;
    };

    if (RUN_ENV == ENV_DEVICE_DEFAULT) {
        ; // 需要创建共享组
    }
    void *buf = nullptr;
    rtError_t rtRet = rtMemQueueAllocMbuf(size, &buf);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[Alloc][mbuf]fail to alloc mbuf result = %d", rtRet);
        return nullptr;
    }
    return new(std::nothrow) acltdtBuf(buf);
}

aclError acltdtDestroyBuf(acltdtBuf *buf)
{
    if (buf == nullptr) {
        return ACL_SUCCESS;
    }
    ACL_REQUIRES_NOT_NULL(buf->mbuf);
    rtError_t rtRet = rtMemQueueFreeMbuf(buf->mbuf);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[Free][mbuf]fail to alloc mbuf result = %d", rtRet);
        return rtRet;
    }
    buf->mbuf = nullptr;
    ACL_DELETE_AND_SET_NULL(buf);
    return ACL_SUCCESS;
}

aclError acltdtGetBufData(const acltdtBuf *buf, void **dataPtr, size_t *size)
{
    ACL_REQUIRES_NOT_NULL(buf);
    ACL_REQUIRES_NOT_NULL(dataPtr);
    ACL_REQUIRES_NOT_NULL(size);
    // if 判断
    rtMemQueueGetMbufAddr(buf->mbuf, dataPtr);
    rtMemQueueGetMbufSize(buf->mbuf, size);
    return ACL_SUCCESS;
}

aclError acltdtGetBufPrivData(const acltdtBuf *buf, void **privBuf, size_t *size)
{
    ACL_REQUIRES_NOT_NULL(buf);
    ACL_REQUIRES_NOT_NULL(privBuf);
    ACL_REQUIRES_NOT_NULL(size);
    // if 判断
    rtMemQueueGetPrivInfo(buf->mbuf, privBuf, size);
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

acltdtQueueRoute* acltdtCreateQueueRoute(uint32_t srcQid, uint32_t dstQid)
{
    return new(std::nothrow) acltdtQueueRoute(srcQid, dstQid);
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