/**
* @file op_task.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "op_task.h"

#include "runtime/rt.h"
#include "types/tensor_desc_internal.h"
#include "error_codes_inner.h"

namespace acl {
TbeOpTask::TbeOpTask(const void *stubFunc, uint32_t blockDim) : stubFunc_(stubFunc), blockDim_(blockDim)
{
}

aclError TbeOpTask::ExecuteAsync(int numInputs,
                                 const aclDataBuffer *const inputs[],
                                 int numOutputs,
                                 aclDataBuffer *const outputs[],
                                 aclrtStream stream)
{
    // update args
    auto *ptrArgs = reinterpret_cast<uintptr_t *>(args_.get());
    int argIndex = 0;
    for (int i = 0; i < numInputs; ++i) {
        ptrArgs[argIndex++] = reinterpret_cast<uintptr_t>(inputs[i]->data);
    }
    for (int i = 0; i < numOutputs; ++i) {
        ptrArgs[argIndex++] = reinterpret_cast<uintptr_t>(outputs[i]->data);
    }

    // launch kernel
    ACL_LOG_DEBUG("To launch kernel, stubFunc = %s, block dim = %u, arg size = %zu",
                  reinterpret_cast<const char *>(stubFunc_), blockDim_, argSize_);
    auto ret = rtKernelLaunch(stubFunc_, blockDim_, const_cast<uint8_t *>(args_.get()), argSize_, nullptr, stream);
    return ACL_GET_ERRCODE_RTS(ret);
}

void OpTask::SetArgs(std::unique_ptr<uint8_t[]> &&args, uint32_t argSize)
{
    args_ = std::move(args);
    argSize_ = argSize;
}
} // namespace acl