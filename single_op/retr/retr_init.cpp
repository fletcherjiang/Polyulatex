/**
* @file retr_init.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "retr_init.h"

#include <securec.h>
#include "aicpu/common/aicpu_task_struct.h"

#include "acl/acl_rt.h"
#include "common/log_inner.h"
#include "retr_internal.h"

namespace acl {
namespace retr {
namespace {
constexpr const char *RETR_KERNELNAME_INITIALIZE = "RetrInitializeKernel";
}  // namespace
AclFvInit::AclFvInit() : stream_(nullptr), runMode_(ACL_HOST)
{}

AclFvInit::~AclFvInit()
{}

/**
 * Check parameters and construct the input parameters of the rtCpuKernelLaunch
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvInit::PrepareInput(aclfvInitPara *initPara, char *args, uint32_t argsSize)
{
    aclError ret = aclrtGetRunMode(&runMode_);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Get][RunMode]aclrtGetRunMode failed, ret = %d", ret);
        return ret;
    }

    auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args);
    paramHead->length = argsSize;
    // RetrInitialize has 1 input/output aclfvInitPara
    paramHead->ioAddrNum = 1;
    auto ioAddr = reinterpret_cast<uint64_t *>(args + sizeof(aicpu::AicpuParamHead));

    ACL_REQUIRES_NOT_NULL(initPara->dataBuffer.data);
    rtError_t rtRet = rtStreamCreate(&stream_, RT_STREAM_PRIORITY_DEFAULT);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[Create][Stream]AclFvInit create stream failed, runtime result = %d", rtRet);
        return ACL_ERROR_RT_FAILURE;
    }

    ioAddr[0] = reinterpret_cast<uintptr_t>(initPara->dataBuffer.data);

    aclrtMemcpyKind memcpyKind = ACL_MEMCPY_DEVICE_TO_DEVICE;
    if (runMode_ == ACL_HOST) {
        memcpyKind = ACL_MEMCPY_HOST_TO_DEVICE;
    }
    ret = aclrtMemcpyAsync(initPara->dataBuffer.data, initPara->dataBuffer.length,
        &(initPara->initPara), sizeof(initPara->initPara), memcpyKind, stream_);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Copy][Mem]memcpy between host and device failed, kind = %d, result = %d",
            static_cast<int32_t>(memcpyKind), ret);
        (void)rtStreamDestroy(stream_);
        return ret;
    }
    return ACL_SUCCESS;
}

/**
 * execute retr init task, including check parameters launch task and check result
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvInit::Init(aclfvInitPara *initPara)
{
    ACL_LOG_INFO("start to execute aclfvInit.");
    ACL_REQUIRES_NOT_NULL(initPara);
    // aclfvInit has 1 input/output aclfvInitPara
    constexpr const int32_t ioAddrNum = 1;
    constexpr const uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
    char args[argsSize] = {0};
    aclError ret = PrepareInput(initPara, args, argsSize);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    ACL_LOG_INFO("rtCpuKernelLaunch %s begin.", RETR_KERNELNAME_INITIALIZE);
    rtError_t rtRet = rtCpuKernelLaunch(RETR_KERNELS_SONAME,
        RETR_KERNELNAME_INITIALIZE,
        1,  // blockDim default 1
        args,
        argsSize,
        nullptr,  // no need smDesc
        stream_);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[Initialize][Repr]retr initialize call rtCpuKernelLaunch failed, "
            "runtime result = %d", rtRet);
        (void)rtStreamDestroy(stream_);
        return ACL_ERROR_RT_FAILURE;
    }
    ACL_LOG_INFO("rtCpuKernelLaunch %s success.", RETR_KERNELNAME_INITIALIZE);

    aclrtMemcpyKind memcpyKind = ACL_MEMCPY_DEVICE_TO_DEVICE;
    if (runMode_ == ACL_HOST) {
        memcpyKind = ACL_MEMCPY_DEVICE_TO_HOST;
    }
    ret = aclrtMemcpyAsync(&(initPara->initPara), sizeof(initPara->initPara),
        initPara->dataBuffer.data, initPara->dataBuffer.length, memcpyKind, stream_);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Copy][Mem]memcpy between host and device failed, kind = %d, result = %d",
            static_cast<int32_t>(memcpyKind), ret);
        (void)rtStreamDestroy(stream_);
        return ret;
    }

    rtRet = rtStreamSynchronize(stream_);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[Sync][Mem]synchronize stream failed, runtime result = %d", rtRet);
        (void)rtStreamDestroy(stream_);
        return ACL_ERROR_RT_FAILURE;
    }
    (void)rtStreamDestroy(stream_);

    if (initPara->initPara.retCode != 0) {
        ACL_LOG_INNER_ERROR("[Init][Fv]aclfvInit failed, ret = %d", initPara->initPara.retCode);
        return ACL_ERROR_FAILURE;
    }
    ACL_LOG_INFO("execute aclfvInit success.");
    return ACL_SUCCESS;
}
}  // namespace retr
}  // namespace acl