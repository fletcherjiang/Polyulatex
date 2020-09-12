/**
* @file acl_op.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_TYPES_ACL_OP_H
#define ACL_TYPES_ACL_OP_H

#include <string>
#include <vector>

#include "acl/acl_op.h"
#include "op_attr.h"
#include "tensor_desc_internal.h"
#include "op_model.h"

namespace acl {
constexpr uint64_t DEFAULT_MAX_OPQUEUE_NUM = 10000;

enum OpCompileType {
    OP_COMPILE_SYS,
    OP_COMPILE_UNREGISTERED,
};

enum OpExecuteType {
    ACL_OP_EXECUTE,
    ACL_OP_EXECUTE_V2,
};


// AclOp does NOT own any of the fields
struct ACL_FUNC_VISIBILITY AclOp {
    AclOp() = default;
    ~AclOp();
    AclOp(const AclOp& aclOp);
    AclOp &operator=(const AclOp &aclOp);

    std::string opType;
    int numInputs = 0;
    int numOutputs = 0;
    const aclTensorDesc * const *inputDesc = nullptr;
    const aclTensorDesc * const *outputDesc = nullptr;
    const aclDataBuffer *const *inputs = nullptr;
    aclDataBuffer *const *outputs = nullptr;
    const aclopAttr *opAttr = nullptr;
    aclopEngineType engineType = ACL_ENGINE_SYS;
    std::string opPath;
    OpCompileType compileType = OP_COMPILE_SYS;
    bool isCompile = false;
    OpExecuteType exeucteType = ACL_OP_EXECUTE;
    bool isCopyConstructor = false;
    bool isMatched = false;
    bool isDynamic = false;
    OpModel opModel;
    std::string DebugString() const;
    void Init(const AclOp& aclOp);
};
} // namespace acl

#endif // ACL_TYPES_ACL_OP_H
