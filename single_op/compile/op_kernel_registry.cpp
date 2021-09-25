/**
* @file op_kernel_registry.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "op_kernel_registry.h"
#include "runtime/rt.h"
#include "framework/common/util.h"
#include "utils/file_utils.h"
#include "common/log_inner.h"
#include "error_codes_inner.h"

namespace acl {
namespace {
    const char *const STUB_NAME_PREFIX = "acl_dynamic_";
} // namespace
const void *OpKernelRegistry::GetStubFunc(const std::string &opType, const std::string &kernelId)
{
    std::lock_guard<std::mutex> lk(mu_);
    auto kernelsIter = kernels_.find(opType);
    if (kernelsIter == kernels_.end()) {
        ACL_LOG_INNER_ERROR("[Find][OpType]No kernel was compiled for op = %s", opType.c_str());
        return nullptr;
    }

    auto &kernelsOfOp = kernelsIter->second;
    auto it = kernelsOfOp.find(kernelId);
    if (it == kernelsOfOp.end()) {
        ACL_LOG_INNER_ERROR("[Find][KernelId]Kernel not compiled for opType = %s and kernelId = %s",
            opType.c_str(), kernelId.c_str());
        return nullptr;
    }

    return it->second->stubName.c_str();
}

OpKernelRegistry::~OpKernelRegistry()
{
    for (auto &kernelsOfOp : kernels_) {
        ACL_LOG_DEBUG("To unregister kernel of op: %s", kernelsOfOp.first.c_str());
        for (auto &it : kernelsOfOp.second) {
            ACL_LOG_DEBUG("To unregister bin by handle: %p, kernelId = %s", it.second->binHandle, it.first.c_str());
            (void)rtDevBinaryUnRegister(it.second->binHandle);
        }
    }
}

aclError OpKernelRegistry::Register(std::unique_ptr<OpKernelRegistration> &&registration)
{
    std::lock_guard<std::mutex> lk(mu_);
    auto deallocator = registration->deallocator;

    // do not deallocate if register failed
    registration->deallocator = nullptr;
    auto iter = kernels_.find(registration->opType);
    if ((iter != kernels_.end()) && (iter->second.count(registration->kernelId) > 0U)) {
        ACL_LOG_INNER_ERROR("[Find][Kernel]Kernel already registered. kernelId = %s",
            registration->kernelId.c_str());
        return ACL_ERROR_KERNEL_ALREADY_REGISTERED;
    }

    registration->stubName = std::string(STUB_NAME_PREFIX);
    registration->stubName += registration->kernelId;
    rtDevBinary_t binary;
    binary.version = 0U;
    binary.data = registration->binData;
    binary.length = registration->binSize;
    if (registration->enginetype == ACL_ENGINE_AICORE) {
        binary.magic = RT_DEV_BINARY_MAGIC_ELF;
    } else if (registration->enginetype == ACL_ENGINE_VECTOR) {
        binary.magic = RT_DEV_BINARY_MAGIC_ELF_AIVEC;
    } else {
        return ACL_ERROR_INVALID_PARAM;
    }
    void *binHandle = nullptr;
    auto ret = rtDevBinaryRegister(&binary, &binHandle);
    if (ret != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[Register][Dev]rtDevBinaryRegister failed, runtime result = %d", static_cast<int32_t>(ret));
        return ACL_GET_ERRCODE_RTS(ret);
    }

    auto rtRet = rtFunctionRegister(binHandle, registration->stubName.c_str(), registration->stubName.c_str(),
        registration->kernelName.c_str(), static_cast<uint32_t>(FUNC_MODE_NORMAL));
    if (rtRet != RT_ERROR_NONE) {
        (void)rtDevBinaryUnRegister(binHandle);
        ACL_LOG_CALL_ERROR("[Register][Dev]rtFunctionRegister failed. bin key = %s, kernel name = %s, "
            "runtime result = %d", registration->stubName.c_str(), registration->kernelName.c_str(),
            static_cast<int32_t>(rtRet));
        return ACL_GET_ERRCODE_RTS(rtRet);
    }

    registration->binHandle = binHandle;
    registration->deallocator = deallocator;
    (void)kernels_[registration->opType].emplace(registration->kernelId, std::move(registration));
    return ACL_SUCCESS;
}
} // namespace acl

