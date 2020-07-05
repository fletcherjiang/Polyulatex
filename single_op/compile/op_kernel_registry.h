/**
* @file op_kernel_registry.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef TRUNK_PROJ_OP_KERNEL_REGISTRY_H
#define TRUNK_PROJ_OP_KERNEL_REGISTRY_H

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include "acl/acl.h"

namespace acl {
struct OpKernelRegistration {
    OpKernelRegistration() = default;
    ~OpKernelRegistration()
    {
        if (deallocator != nullptr) {
            deallocator(binData, binSize);
        }
    }

    std::string opType;
    std::string kernelId;
    std::string stubName;
    std::string kernelName;
    aclopEngineType enginetype = ACL_ENGINE_SYS;
    void *binData = nullptr;
    uint64_t binSize = 0;
    void (*deallocator)(void *data, size_t length) = nullptr;
    void *binHandle = nullptr;
};

class OpKernelRegistry {
public:
    ~OpKernelRegistry();

    static OpKernelRegistry &GetInstance()
    {
        static OpKernelRegistry instance;
        return instance;
    }

    const void *GetStubFunc(const std::string &opType, const std::string &kernelId);

    aclError Register(std::unique_ptr<OpKernelRegistration> &&registration);

private:
    OpKernelRegistry() = default;

    // opType, KernelId, OpKernelRegistration
    std::map<std::string, std::map<std::string, std::unique_ptr<OpKernelRegistration>>> kernels_;
    std::mutex mu_;
};
} // namespace acl

#endif //TRUNK_PROJ_OP_KERNEL_REGISTRY_H
