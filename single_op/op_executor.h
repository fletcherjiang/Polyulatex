/**
* @file op_executor.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ACL_OP_EXEC_OP_EXECUTOR_H
#define ACL_OP_EXEC_OP_EXECUTOR_H

#include "framework/executor/ge_executor.h"

#include <memory>
#include <map>
#include <unordered_map>
#include <map>

#include "acl/acl_op.h"
#include "types/acl_op.h"
#include "types/op_model.h"
#include "single_op/compile/op_kernel_selector.h"

namespace acl {
struct OpHandle {
    std::string opType;
    int numInputs = 0;
    int numOutputs = 0;
    OpModel opModel;
    std::unordered_map<aclrtStream, ge::SingleOp *> cachedOperators;
    std::map<aclrtStream, ge::DynamicSingleOp *> cachedDynamicOperators;
    std::shared_ptr<aclopKernelDesc> kernelDesc;
    AclOp aclOp;
    bool isDynamic = false;
};

class ACL_FUNC_VISIBILITY OpExecutor {
public:
    static aclError CreateOpHandle(const AclOp &aclOp, OpHandle **handle);

    static aclError ExecuteAsync(const AclOp &aclOp,
                                  const aclDataBuffer *const inputs[],
                                  aclDataBuffer *const outputs[],
                                  aclrtStream stream);

    static aclError ExecuteAsync(OpHandle &opHandle,
                                  const aclDataBuffer *const inputs[],
                                  aclDataBuffer *const outputs[],
                                  aclrtStream stream);

    static aclError ExecuteAsync(const std::string &opType,
                                 int numInputs,
                                 const aclDataBuffer *const inputs[],
                                 int numOutputs,
                                 aclDataBuffer *const outputs[],
                                 aclrtStream stream);

private:
    static ge::SingleOp *LoadSingleOp(const OpModel &modelInfo, aclrtStream stream);

    static ge::DynamicSingleOp *LoadDynamicSingleOp(const OpModel &modelInfo, aclrtStream stream);

    static aclError DoExecuteAsync(ge::DynamicSingleOp *singleOp,
                                   const AclOp &aclOp,
                                   const aclDataBuffer *const inputs[],
                                   aclDataBuffer *const outputs[],
                                   bool executeWithExactModel = true);

    static aclError DoExecuteAsync(ge::SingleOp *singleOp,
                                   const AclOp &aclOp,
                                   const aclDataBuffer *const inputs[],
                                   aclDataBuffer *const outputs[],
                                   bool executeWithExactModel = true);
};
} // namespace acl

#endif // ACL_OP_EXEC_OP_EXECUTOR_H
