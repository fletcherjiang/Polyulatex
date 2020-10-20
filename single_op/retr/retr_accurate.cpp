/* *
 * @file retr_accurate.cpp
 *
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "retr_accurate.h"

#include <securec.h>
#include "aicpu/common/aicpu_task_struct.h"

#include "acl/acl_rt.h"
#include "common/log_inner.h"
#include "common/error_codes_inner.h"
#include "retr_internal.h"

namespace acl {
namespace retr {
namespace {
constexpr const char *RETR_KERNELNAME_REPO_ACCURATE_DEL_OR_MODIFY = "RetrRepoAccurateDelOrModifyKernel";
}
AclFvAccurate::AclFvAccurate() : notify_(nullptr), notifyId_(0), runMode_(ACL_HOST) {}

AclFvAccurate::~AclFvAccurate() {}

/* *
 * Check parameters and construct the input parameters of the rtCpuKernelLaunch
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvAccurate::PrepareInput(aclAccurateType typeAccurate, aclfvFeatureInfo *featureInfo, aclrtStream stream,
    char *args, uint32_t argsSize)
{
    ACL_REQUIRES_NOT_NULL(featureInfo);
    ACL_REQUIRES_NOT_NULL(featureInfo->dataBuffer.data);
    ACL_REQUIRES_NOT_NULL(stream);

    size_t hostSize = sizeof(featureInfo->retrFeatureInfo);
    size_t devSize = featureInfo->dataBuffer.length;
    if (hostSize != devSize) {
        ACL_LOG_INNER_ERROR("memory size between host [%u] and device [%u] not equal", hostSize, devSize);
        return ACL_ERROR_INVALID_PARAM;
    }

    aclError ret = aclrtGetRunMode(&runMode_);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    ret = acl::retr::aclCreateNotify(notify_, notifyId_, stream);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("execute aclCreateNotify failed, result = %d.", ret);
        return ret;
    }

    // aclfvAccurateDelOrModify has 1 input/output aclfvFeatureInfo
    constexpr const int32_t ioAddrNum = 1;
    auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args);
    paramHead->length = argsSize;
    paramHead->ioAddrNum = ioAddrNum;
    auto ioAddr = reinterpret_cast<uint64_t *>(args + sizeof(aicpu::AicpuParamHead));
    ioAddr[0] = reinterpret_cast<uintptr_t>(featureInfo->dataBuffer.data);
    uint32_t configOffset = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
    auto memcpyRet = memcpy_s(args + configOffset, argsSize - configOffset, &(typeAccurate), sizeof(aclAccurateType));
    if (memcpyRet != EOK) {
        ACL_LOG_INNER_ERROR("copy aclAccurateType to args failed, result = %d.", memcpyRet);
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        return ACL_ERROR_RT_FAILURE;
    }
    configOffset += sizeof(aclAccurateType);
    memcpyRet = memcpy_s(args + configOffset, argsSize - configOffset, &(notifyId_), sizeof(uint32_t));
    if (memcpyRet != EOK) {
        ACL_LOG_INNER_ERROR("copy notifyId to args failed, result = %d.", memcpyRet);
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        return ACL_ERROR_RT_FAILURE;
    }
    return ACL_SUCCESS;
}

/* *
 * execute retr accurate del or modify, including check parameters launch task and check result
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvAccurate::ExecAccurateDelOrModify(aclfvFeatureInfo *featureInfo, aclrtStream stream, const char *args,
    uint32_t argsSize)
{
    aclrtMemcpyKind copyKind = ACL_MEMCPY_DEVICE_TO_DEVICE;
    if (runMode_ == ACL_HOST) {
        copyKind = ACL_MEMCPY_HOST_TO_DEVICE;
    }
    aclError ret = aclrtMemcpyAsync(featureInfo->dataBuffer.data, featureInfo->dataBuffer.length,
        &(featureInfo->retrFeatureInfo), sizeof(featureInfo->retrFeatureInfo), copyKind, stream);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("memcpy from host to device failed, kind=%d, result = %d", copyKind, ret);
        return ret;
    }

    ACL_LOG_INFO("rtCpuKernelLaunch %s begin.", RETR_KERNELNAME_REPO_ACCURATE_DEL_OR_MODIFY);
    rtError_t rtRet = rtCpuKernelLaunch(RETR_KERNELS_SONAME, RETR_KERNELNAME_REPO_ACCURATE_DEL_OR_MODIFY,
        1, // blockDim default 1
        args, argsSize,
        nullptr, // no need smDesc
        stream);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("rtCpuKernelLaunch failed, runtime result = %d.", rtRet);
        return ACL_ERROR_RT_FAILURE;
    }
    ACL_LOG_INFO("rtCpuKernelLaunch %s success.", RETR_KERNELNAME_REPO_ACCURATE_DEL_OR_MODIFY);

    ret = aclFvNotifyWait(notify_, stream);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("wait for a notify to stream failed, runtime result = %d.", ret);
        return ret;
    }

    if (runMode_ == ACL_HOST) {
        copyKind = ACL_MEMCPY_DEVICE_TO_HOST;
    }
    ret = aclrtMemcpyAsync(&(featureInfo->retrFeatureInfo), sizeof(featureInfo->retrFeatureInfo),
        featureInfo->dataBuffer.data, featureInfo->dataBuffer.length, copyKind, stream);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("copy featureInfo from device to host failed, result = %d.", ret);
        return ret;
    }
    return ACL_SUCCESS;
}

/* *
 * execute retr accurate del or modify, including check parameters launch task and check result
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvAccurate::AccurateDelOrModify(aclAccurateType typeAccurate, aclfvFeatureInfo *featureInfo,
    aclrtStream stream)
{
    ACL_LOG_INFO("start to execute aclfvAccurateDelOrModify.");
    // aclfvAccurateDelOrModify has 1 input/output aclfvFeatureInfo
    constexpr const int32_t ioAddrNum = 1;
    constexpr const uint32_t argsSize =
        sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t) + sizeof(aclAccurateType) + sizeof(uint32_t);
    char args[argsSize] = {0};
    aclError ret = PrepareInput(typeAccurate, featureInfo, stream, args, argsSize);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    ret = ExecAccurateDelOrModify(featureInfo, stream, args, argsSize);
    if (ret != ACL_SUCCESS) {
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        return ret;
    }

    rtError_t rtRet = rtStreamSynchronize(stream);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("synchronize stream failed, runtime result = %d.", rtRet);
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        return ACL_ERROR_RT_FAILURE;
    }
    if (featureInfo->retrFeatureInfo.retCode != 0) {
        ACL_LOG_INNER_ERROR("execute aclfvAccurateDelOrModify failed, result = %d.",
            featureInfo->retrFeatureInfo.retCode);
        (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
        return ACL_ERROR_FAILURE;
    }

    (void)rtEventDestroy(static_cast<rtEvent_t>(notify_));
    ACL_LOG_INFO("execute aclfvAccurateDelOrModify success.");
    return ACL_SUCCESS;
}

/* *
 * execute retr accurate Del, including check parameters launch task and check result
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvAccurate::Delete(aclfvFeatureInfo *featureInfo)
{
    ACL_LOG_INFO("start to execute aclfvDel.");
    rtStream_t stream = nullptr;
    rtError_t rtErr = rtStreamCreate(&stream, RT_STREAM_PRIORITY_DEFAULT);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_ERROR("create stream failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    aclError ret = AccurateDelOrModify(acl::retr::ACCURATE_DELETE, featureInfo, stream);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("execute aclfvDel failed, result = %d.", ret);
        return ACL_ERROR_FAILURE;
    }
    (void)rtStreamDestroy(stream);
    ACL_LOG_INFO("execute aclfvDel success.");
    return ACL_SUCCESS;
}

/* *
 * execute retr accurate modify, including check parameters launch task and check result
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvAccurate::Modify(aclfvFeatureInfo *featureInfo)
{
    ACL_LOG_INFO("start to execute aclfvModify.");
    rtStream_t stream = nullptr;
    rtError_t rtErr = rtStreamCreate(&stream, RT_STREAM_PRIORITY_DEFAULT);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_ERROR("create stream failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    aclError ret = AccurateDelOrModify(acl::retr::ACCURATE_MODFILY, featureInfo, stream);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("execute aclfvModify failed, result = %d.", ret);
        return ACL_ERROR_FAILURE;
    }
    (void)rtStreamDestroy(stream);
    ACL_LOG_INFO("execute aclfvModify success.");
    return ACL_SUCCESS;
}
} // namespace retr
} // namespace acl