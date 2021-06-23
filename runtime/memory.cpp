/**
* @file memory.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/acl_rt.h"

#include "runtime/mem.h"

#include "runtime/dev.h"

#include "log_inner.h"

#include "error_codes_inner.h"

#include "utils/math_utils.h"

#include "toolchain/profiling_manager.h"

#include "toolchain/resource_statistics.h"


aclError aclrtMalloc(void **devPtr, size_t size, aclrtMemMallocPolicy policy)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_MALLOC_FREE);
    ACL_LOG_INFO("start to execute aclrtMalloc, size  = %zu", size);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(devPtr);
    // size must be greater than zero
    if (size == 0) {
        ACL_LOG_ERROR("malloc size must be greater than zero");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"size", std::to_string(size),
            "size must be greater than zero"}));
        return ACL_ERROR_INVALID_PARAM;
    }
    size_t alignedSize;
    ACL_REQUIRES_OK(acl::GetAlignedSize(size, alignedSize));
    uint32_t flags = RT_MEMORY_DEFAULT;
    if (policy == ACL_MEM_MALLOC_HUGE_FIRST) {
        flags |= RT_MEMORY_POLICY_HUGE_PAGE_FIRST;
    } else if (policy == ACL_MEM_MALLOC_HUGE_ONLY) {
        flags |= RT_MEMORY_POLICY_HUGE_PAGE_ONLY;
    } else if (policy == ACL_MEM_MALLOC_NORMAL_ONLY) {
        flags |= RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
    } else if (policy == ACL_MEM_MALLOC_HUGE_FIRST_P2P) {
        flags |= RT_MEMORY_POLICY_HUGE_PAGE_FIRST_P2P;
    } else if (policy == ACL_MEM_MALLOC_HUGE_ONLY_P2P) {
        flags |= RT_MEMORY_POLICY_HUGE_PAGE_ONLY_P2P;
    } else if (policy == ACL_MEM_MALLOC_NORMAL_ONLY_P2P) {
        flags |= RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY_P2P;
    } else {
        flags = RT_MEMORY_DEFAULT;
    }
    rtError_t rtErr = rtMalloc(devPtr, alignedSize, flags);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("alloc device memory failed, runtime result = %d", rtErr);
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_MALLOC_FREE);
    return ACL_SUCCESS;
}

aclError aclrtMallocCached(void **devPtr, size_t size, aclrtMemMallocPolicy policy)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_MALLOC_FREE);
    ACL_LOG_INFO("start to execute aclrtMallocCached, size = %zu", size);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(devPtr);

    if (size == 0) {
        ACL_LOG_INNER_ERROR("malloc size must be greater than zero");
        return ACL_ERROR_INVALID_PARAM;
    }
    size_t alignedSize;
    ACL_REQUIRES_OK(acl::GetAlignedSize(size, alignedSize));
    uint32_t cacheFlags = RT_MEMORY_DEFAULT;
    if (policy == ACL_MEM_MALLOC_HUGE_FIRST) {
        cacheFlags |= RT_MEMORY_POLICY_HUGE_PAGE_FIRST;
    } else if (policy == ACL_MEM_MALLOC_HUGE_ONLY) {
        cacheFlags |= RT_MEMORY_POLICY_HUGE_PAGE_ONLY;
    } else if (policy == ACL_MEM_MALLOC_NORMAL_ONLY) {
        cacheFlags |= RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
    } else {
        cacheFlags = RT_MEMORY_DEFAULT;
    }
    rtError_t rtErr = rtMallocCached(devPtr, alignedSize, cacheFlags);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("alloc device memory with cache failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_MALLOC_FREE);
    return ACL_SUCCESS;
}

aclError aclrtMemFlush(void *devPtr, size_t size)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtMemFlush, size = %zu", size);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(devPtr);

    if (size == 0) {
        ACL_LOG_INNER_ERROR("flush cache size must be greater than zero");
        return ACL_ERROR_INVALID_PARAM;
    }
    rtError_t rtErr = rtFlushCache(devPtr, size);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("flush cache data to ddr failed, runtime result = %d, size = %zu",
            static_cast<int32_t>(rtErr), size);
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    return ACL_SUCCESS;
}

aclError aclrtMemInvalidate(void *devPtr, size_t size)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtMemInvalidate, size = %zu", size);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(devPtr);

    if (size == 0) {
        ACL_LOG_INNER_ERROR("invalidate cache size must be greater than zero");
        return ACL_ERROR_INVALID_PARAM;
    }
    rtError_t rtErr = rtInvalidCache(devPtr, size);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("invalidate cache data failed, runtime result = %d, size = %zu",
            static_cast<int32_t>(rtErr), size);
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    return ACL_SUCCESS;
}

aclError aclrtFree(void *devPtr)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_MALLOC_FREE);
    ACL_LOG_INFO("start to execute aclrtFree");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(devPtr);

    rtError_t rtErr = rtFree(devPtr);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("free device memory failed, runtime result = %d", rtErr);
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_MALLOC_FREE);
    return ACL_SUCCESS;
}

aclError aclrtMallocHost(void **hostPtr, size_t size)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_MALLOC_FREE_HOST);
    ACL_LOG_INFO("start to execute aclrtMallocHost, size = %zu", size);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(hostPtr);
    // size must be greater than zero
    if (size == 0) {
        ACL_LOG_INNER_ERROR("malloc size must be greater than zero");
        return ACL_ERROR_INVALID_PARAM;
    }
    rtError_t rtErr = rtMallocHost(hostPtr, size);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("alloc host memory failed, runtime result = %d", rtErr);
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_MALLOC_FREE_HOST);
    return ACL_SUCCESS;
}

aclError aclrtFreeHost(void *hostPtr)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_MALLOC_FREE_HOST);
    ACL_LOG_INFO("start to execute aclrtFreeHost");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(hostPtr);
    rtError_t rtErr = rtFreeHost(hostPtr);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("free host memory failed, runtime result = %d", rtErr);
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_MALLOC_FREE_HOST);
    return ACL_SUCCESS;
}

static aclError MemcpyKindTranslate(aclrtMemcpyKind kind, rtMemcpyKind_t &rtKind)
{
    switch (kind) {
        case ACL_MEMCPY_HOST_TO_HOST: {
            rtKind = RT_MEMCPY_HOST_TO_HOST;
            break;
        }
        case ACL_MEMCPY_HOST_TO_DEVICE: {
            rtKind = RT_MEMCPY_HOST_TO_DEVICE;
            break;
        }
        case ACL_MEMCPY_DEVICE_TO_HOST: {
            rtKind = RT_MEMCPY_DEVICE_TO_HOST;
            break;
        }
        case ACL_MEMCPY_DEVICE_TO_DEVICE: {
            rtKind = RT_MEMCPY_DEVICE_TO_DEVICE;
            break;
        }
        default: {
            ACL_LOG_INNER_ERROR("invalid kind of memcpy, kind = %d", static_cast<int32_t>(kind));
            return ACL_ERROR_INVALID_PARAM;
        }
    }
    return ACL_SUCCESS;
}

aclError aclrtMemcpy(void *dst,
                     size_t destMax,
                     const void *src,
                     size_t count,
                     aclrtMemcpyKind kind)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtMemcpy, destMaxSize = %zu, srcSize = %zu, kind = %d",
        destMax, count, static_cast<int32_t>(kind));
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dst);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(src);

    rtMemcpyKind_t rtKind = RT_MEMCPY_RESERVED;
    aclError ret = MemcpyKindTranslate(kind, rtKind);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("invalid kind of memcpy, kind = %d", static_cast<int32_t>(kind));
        return ret;
    }

    rtError_t rtErr = rtMemcpy(dst, destMax, src, count, rtKind);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("synchronized memcpy failed, kind = %d, runtime result = %d",
            static_cast<int32_t>(kind), static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    return ACL_SUCCESS;
}

aclError aclrtMemset(void *devPtr, size_t maxCount, int32_t value, size_t count)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtMemset, maxSize = %zu, size = %zu, value = %d",
        maxCount, count, value);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(devPtr);

    rtError_t rtErr = rtMemset(devPtr, maxCount, static_cast<uint32_t>(value), count);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("set memory failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    return ACL_SUCCESS;
}

aclError aclrtMemcpyAsync(void *dst,
                          size_t destMax,
                          const void *src,
                          size_t count,
                          aclrtMemcpyKind kind,
                          aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtMemcpyAsync, destMaxSize = %zu, srcSize = %zu, kind = %d",
        destMax, count, static_cast<int32_t>(kind));
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dst);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(src);
    rtMemcpyKind_t rtKindVal = RT_MEMCPY_RESERVED;
    aclError ret = MemcpyKindTranslate(kind, rtKindVal);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("invalid kind of memcpy, kind = %d", static_cast<int32_t>(kind));
        return ret;
    }

    rtError_t rtErr = rtMemcpyAsync(dst, destMax, src, count, rtKindVal, static_cast<rtStream_t>(stream));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("asynchronized memcpy failed, kind = %d, runtime result = %d",
            static_cast<int32_t>(kind), static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    return ACL_SUCCESS;
}

aclError aclrtMemsetAsync(void *devPtr, size_t maxCount, int32_t value, size_t count, aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtMemsetAsync, maxCount = %zu, value = %d, count = %zu",
        maxCount, value, count);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(devPtr);

    rtError_t rtErr = rtMemsetAsync(devPtr, maxCount, static_cast<uint32_t>(value), count, stream);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("asynchronized memset failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    return ACL_SUCCESS;
}

aclError aclrtDeviceCanAccessPeer(int32_t *canAccessPeer, int32_t deviceId, int32_t peerDeviceId)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtDeviceCanAccessPeer");

    if (deviceId == peerDeviceId) {
        ACL_LOG_INNER_ERROR("deviceId shouldn't be equal to peeerDeviceId");
        return ACL_ERROR_INVALID_PARAM;
    }

    uint32_t peerPhyId = 0;
    rtError_t rtErr = rtGetDevicePhyIdByIndex(static_cast<uint32_t>(peerDeviceId), &peerPhyId);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("call rtGetDevicePhyIdByIndex failed, deviceId = %d, peerDeviceId = %d, "
                       "runtime result = %d", deviceId, peerDeviceId, static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    rtErr = rtDeviceCanAccessPeer(canAccessPeer, static_cast<uint32_t>(deviceId), peerPhyId);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("call rtDeviceCanAccessPeer failed, deviceId = %d, peerPhyId = %u, "
                      "runtime result = %d", deviceId, peerPhyId, static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    return ACL_SUCCESS;
}

aclError aclrtDeviceEnablePeerAccess(int32_t peerDeviceId, uint32_t flags)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtDeviceEnablePeerAccess");

    if (flags != 0) {
        ACL_LOG_INNER_ERROR("the flags must be 0, but current is %u", flags);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    int32_t deviceId = 0;
    rtError_t rtErr = rtGetDevice(&deviceId);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("call rtGetDevice failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    if (deviceId == peerDeviceId) {
        ACL_LOG_INNER_ERROR("deviceId shouldn't be equal to peeerDeviceId, deviceId = %d, peerDeviceId = %d",
                      deviceId, peerDeviceId);
        return ACL_ERROR_INVALID_PARAM;
    }

    uint32_t peerPhyId = 0;
    rtErr = rtGetDevicePhyIdByIndex(static_cast<uint32_t>(peerDeviceId), &peerPhyId);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("call rtGetDevicePhyIdByIndex failed, peerDeviceId = %d, runtime result = %d",
                      peerDeviceId, static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    rtErr = rtEnableP2P(static_cast<uint32_t>(deviceId), peerPhyId, flags);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("call rtEnableP2P failed, deviceId = %d, peerPhyId = %u, runtime result = %d",
                      deviceId, peerPhyId, static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    return ACL_SUCCESS;
}

aclError aclrtDeviceDisablePeerAccess(int32_t peerDeviceId)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtDeviceDisablePeerAccess");

    int32_t deviceId = 0;
    rtError_t rtErr = rtGetDevice(&deviceId);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("call rtGetDevice failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    if (deviceId == peerDeviceId) {
        ACL_LOG_INNER_ERROR("deviceId shouldn't be equal to peeerDeviceId, deviceId = %d, peerDeviceId = %d",
                      deviceId, peerDeviceId);
        return ACL_ERROR_INVALID_PARAM;
    }

    uint32_t peerPhyId = 0;
    rtErr = rtGetDevicePhyIdByIndex(static_cast<uint32_t>(peerDeviceId), &peerPhyId);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("call rtGetDevicePhyIdByIndex failed, peerDeviceId = %u, runtime result = %d",
                      peerDeviceId, static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    rtErr = rtDisableP2P(static_cast<uint32_t>(deviceId), peerPhyId);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("call rtDisableP2P failed, deviceId = %d, peerPhyId = %u, runtime result = %d",
                      deviceId, peerPhyId, static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    return ACL_SUCCESS;
}

aclError aclrtGetMemInfo(aclrtMemAttr attr, size_t *freeMem, size_t *totalMem)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(freeMem);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(totalMem);
    ACL_LOG_INFO("start to execute aclrtGetMemInfo, memory attribute = %d", static_cast<int32_t>(attr));

    rtError_t rtErr = rtMemGetInfoEx(static_cast<rtMemInfoType_t>(attr), freeMem, totalMem);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("get memory information failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    ACL_LOG_INFO("successfully execute aclrtGetMemInfo, memory attribute = %d, free memory = %zu, total memory = %zu",
        static_cast<int32_t>(attr), *freeMem, *totalMem);
    return ACL_SUCCESS;
}

static aclError CheckMemcpy2dParam(void *dst,
                                   size_t dpitch,
                                   const void *src,
                                   size_t spitch,
                                   size_t width,
                                   size_t height,
                                   aclrtMemcpyKind kind,
                                   rtMemcpyKind_t &rtKind)
{
    ACL_LOG_INFO("start to execute CheckMemcpy2dParam");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dst);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(src);
    ACL_REQUIRES_POSITIVE(height);
    ACL_REQUIRES_POSITIVE(width);

    if ((width > spitch) || (width > dpitch)) {
        ACL_LOG_ERROR("[Check][Width]input param width[%zu] must be smaller than spitch[%zu] and dpitch[%zu]",
            width, spitch, dpitch);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"width", std::to_string(width), "must be smaller than spitch and dpitch"}));
        return ACL_ERROR_INVALID_PARAM;
    }

    switch (kind) {
        case ACL_MEMCPY_HOST_TO_DEVICE: {
            rtKind = RT_MEMCPY_HOST_TO_DEVICE;
            break;
        }
        case ACL_MEMCPY_DEVICE_TO_HOST: {
            rtKind = RT_MEMCPY_DEVICE_TO_HOST;
            break;
        }
        default: {
            ACL_LOG_INNER_ERROR("[Check][Kind]invalid kind of memcpy, kind = %d", static_cast<int32_t>(kind));
            return ACL_ERROR_INVALID_PARAM;
        }
    }
    return ACL_SUCCESS;
}

aclError aclrtMemcpy2d(void *dst,
                       size_t dpitch,
                       const void *src,
                       size_t spitch,
                       size_t width,
                       size_t height,
                       aclrtMemcpyKind kind)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtMemcpy2d, dpitch = %zu, spitch = %zu, width = %zu, height = %zu, kind = %d",
        dpitch, spitch, width, height, static_cast<int32_t>(kind));

    rtMemcpyKind_t rtKind = RT_MEMCPY_RESERVED;
    aclError ret = CheckMemcpy2dParam(dst, dpitch, src, spitch, width, height, kind, rtKind);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    rtError_t rtErr = rtMemcpy2d(dst, dpitch, src, spitch, width, height, rtKind);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[Synchronized][Memcpy]synchronized memcpy failed, kind = %d, runtime result = %d",
            static_cast<int32_t>(kind), static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    ACL_LOG_INFO("Successfuly execute aclrtMemcpy2d, dpitch = %zu, spitch = %zu, width = %zu, height = %zu, "
        "kind = %d", dpitch, spitch, width, height, static_cast<int32_t>(kind));
    return ACL_SUCCESS;
}

aclError aclrtMemcpy2dAsync(void *dst,
                            size_t dpitch,
                            const void *src,
                            size_t spitch,
                            size_t width,
                            size_t height,
                            aclrtMemcpyKind kind,
                            aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtMemcpy2dAsync, dpitch = %zu, spitch = %zu, width = %zu, height = %zu,"
        " kind = %d", dpitch, spitch, width, height, static_cast<int32_t>(kind));

    rtMemcpyKind_t rtKindVal = RT_MEMCPY_RESERVED;
    aclError ret = CheckMemcpy2dParam(dst, dpitch, src, spitch, width, height, kind, rtKindVal);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    rtError_t rtErr = rtMemcpy2dAsync(dst, dpitch, src, spitch, width, height, rtKindVal, stream);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[Asynchronized][Memcpy]asynchronized memcpy failed, kind = %d, runtime result = %d",
            static_cast<int32_t>(kind), static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    ACL_LOG_INFO("Successfuly execute aclrtMemcpy2dAsync, dpitch = %zu, spitch = %zu, width = %zu, height = %zu, "
        "kind = %d", dpitch, spitch, width, height, static_cast<int32_t>(kind));
    return ACL_SUCCESS;
}