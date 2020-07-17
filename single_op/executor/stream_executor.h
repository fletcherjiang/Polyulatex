/**
* @file stream_executor.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef TRUNK_PROJ_STREAM_EXECUTOR_H
#define TRUNK_PROJ_STREAM_EXECUTOR_H

#include <memory>
#include <map>

#include "acl/acl.h"
#include "types/acl_op.h"
#include "resource_manager.h"
#include "single_op/compile/op_kernel_selector.h"
#include "op_task.h"

namespace acl {
class StreamExecutor {
public:
    ~StreamExecutor();

    aclError ExecuteAsync(const AclOp &aclOp,
                          const aclDataBuffer *const *inputs,
                          aclDataBuffer *const *outputs);

    aclError ExecuteAsync(const OpKernelDesc &kernelDesc,
                          int numInputs,
                          const aclDataBuffer *const *inputs,
                          int numOutputs,
                          aclDataBuffer *const *outputs);

private:
    friend class Executors;

    StreamExecutor(ResourceManager *resMgr, aclrtStream stream);

    aclError InitTbeTask(const OpKernelDesc &desc, int numInputs, int numOutputs, TbeOpTask &task);

    aclError AllocateWorkspaces(const std::vector<size_t> &workspaceSizes, vector<uintptr_t> &workspaces);

    std::unique_ptr<ResourceManager> resMgr_;
    aclrtStream stream_;
    std::mutex mu_;
};

class Executors {
public:
    Executors() = default;

    ~Executors() = default;

    static StreamExecutor *GetOrCreate(aclrtContext context, aclrtStream stream);

    static void Remove(aclrtContext context, aclrtStream stream);

private:
    static std::mutex mu;
    static std::map<uintptr_t, std::unique_ptr<StreamExecutor>> executors; //lint !e665
};
} // namespace acl


#endif // TRUNK_PROJ_STREAM_EXECUTOR_H
