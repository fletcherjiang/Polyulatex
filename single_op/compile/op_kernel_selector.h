/**
* @file op_kernel_selector.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef TRUNK_PROJ_OP_KERNEL_SELECTOR_H
#define TRUNK_PROJ_OP_KERNEL_SELECTOR_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <climits>

#include "acl/acl.h"
#include "types/acl_op.h"
#include "utils/acl_op_map.h"
#include "types/tensor_desc_internal.h"
#include "types/op_attr.h"

struct aclopKernelDesc {
    std::string kernelId;
    const void *stubFunc = nullptr; // no need for deallocating
    uint32_t blockDim = 0;
    std::vector<size_t> workspaceSizes;
    std::string extendArgs;
    uint64_t timestamp = ULLONG_MAX;

    std::string opType;
    std::vector<aclTensorDesc> inputDescArr;
    std::vector<aclTensorDesc> outputDescArr;
    aclopAttr opAttr;
};

namespace acl {
using OpKernelDesc = aclopKernelDesc;

class OpKernelSelector {
public:
    ~OpKernelSelector() = default;
    static OpKernelSelector &GetInstance()
    {
        static OpKernelSelector instance;
        return instance;
    }

    bool HasSelectFunc(const std::string &opType);

    bool Register(const std::string &opType, aclopCompileFunc func);

    void Unregister(const std::string &opType);

    aclError SelectOpKernel(const AclOp &aclOp);

    aclError GetOpKernelDesc(const AclOp &aclOp, std::shared_ptr<OpKernelDesc> &desc);

private:
    OpKernelSelector() = default;
    aclopCompileFunc GetSelectFunc(const std::string &opType);

    aclError InsertAclop2KernelDesc(const AclOp &aclOp, std::shared_ptr<OpKernelDesc> desc);

    std::mutex mu_;
    std::map<std::string, aclopCompileFunc> selectors_;

    AclOpMap<std::shared_ptr<OpKernelDesc>> kernelDescMap_;
};
} // namespace acl

#endif //TRUNK_PROJ_OP_KERNEL_SELECTOR_H
