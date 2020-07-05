/**
* @file retr_release.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "retr_release.h"

#include <securec.h>
#include "aicpu/common/aicpu_task_struct.h"

#include "acl/acl_rt.h"
#include "common/log_inner.h"
#include "retr_internal.h"

namespace acl {
namespace retr {
namespace {
constexpr const char *RETR_KERNELNAME_RELEASE = "RetrReleaseKernel";
}
AclFvRelease::AclFvRelease() : stream_(nullptr), retCode_(nullptr)
{}

AclFvRelease::~AclFvRelease()
{}

/**
 * Check parameters and construct the input parameters of the rtCpuKernelLaunch
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvRelease::PrepareInput(char *args, uint32_t argsSize)
{
    rtError_t rtRet = rtStreamCreate(&stream_, RT_STREAM_PRIORITY_DEFAULT);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_ERROR("AclFvRelease create stream failed, runtime result = %d.", rtRet);
        return ACL_ERROR_RT_FAILURE;
    }

    aclError ret = aclrtMalloc(&retCode_, sizeof(int32_t), ACL_MEM_MALLOC_NORMAL_ONLY);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_ERROR("AclFvRelease alloc device buffer failed, size = %zu, result = %d.", sizeof(int32_t), ret);
        (void)rtStreamDestroy(stream_);
        return ret;
    }

    auto head = reinterpret_cast<aicpu::AicpuParamHead *>(args);
    head->length = argsSize;
    // aclfvRelease has 1 output retCode
    head->ioAddrNum = 1;
    auto ioAddr = reinterpret_cast<uint64_t *>(args + sizeof(aicpu::AicpuParamHead));
    ioAddr[0] = reinterpret_cast<uintptr_t>(retCode_);

    return ACL_SUCCESS;
}

/**
 * execute retr release task, including check parameters launch task and check result
 * @return ACL_SUCCESS:success other:failed
 */
aclError AclFvRelease::Release()
{
    ACL_LOG_INFO("start to execute aclfvRelease.");
    constexpr const uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + sizeof(uint64_t);
    char args[argsSize] = {0};
    aclError ret = PrepareInput(args, argsSize);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    ACL_LOG_INFO("rtCpuKernelLaunch %s begin.", RETR_KERNELNAME_RELEASE);
    rtError_t rtRet = rtCpuKernelLaunch(RETR_KERNELS_SONAME,
        RETR_KERNELNAME_RELEASE,
        1,  // blockDim default 1
        args,
        argsSize,
        nullptr,  // no need smDesc
        stream_);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_ERROR("retr release call rtCpuKernelLaunch failed, runtime result = %d.", rtRet);
        (void)aclrtFree(retCode_);
        (void)rtStreamDestroy(stream_);
        return ACL_ERROR_RT_FAILURE;
    }
    ACL_LOG_INFO("rtCpuKernelLaunch %s success.", RETR_KERNELNAME_RELEASE);

    rtRet = rtStreamSynchronize(stream_);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_ERROR("synchronize stream failed, runtime result = %d.", rtRet);
        (void)aclrtFree(retCode_);
        (void)rtStreamDestroy(stream_);
        return ACL_ERROR_RT_FAILURE;
    }
    (void)rtStreamDestroy(stream_);
    ret = acl::retr::aclCheckResult(retCode_);
    if (ret != ACL_SUCCESS) {
        (void)aclrtFree(retCode_);
        return ret;
    }
    (void)aclrtFree(retCode_);
    ACL_LOG_INFO("execute aclfvRelease success.");
    return ACL_SUCCESS;
}
}  // namespace retr
}  // namespace acl