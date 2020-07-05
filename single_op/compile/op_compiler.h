/**
* @file op_compiler.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_OP_EXEC_COMPILER_OP_COMPILER_H
#define ACL_OP_EXEC_COMPILER_OP_COMPILER_H

#include "acl/acl_op.h"

#include "graph/op_desc.h"
#include "graph/ge_tensor.h"
#include "common/ge_types.h"

#include "common/common_inner.h"
#include "types/acl_op.h"
#include "types/op_model.h"

namespace acl {
struct CompileParam {
    ge::OpDescPtr opDesc;
    std::vector<ge::GeTensor> inputs;
    std::vector<ge::GeTensor> outputs;
    ge::OpEngineType engineType;
    int32_t compileFlag;
};

class OpCompiler {
public:
    OpCompiler() = default;

    virtual ~OpCompiler() = default;

    virtual aclError Init(const std::map<std::string, std::string> &options) = 0;

    aclError CompileOp(const AclOp &aclOp, std::shared_ptr<void> &modelData, size_t &modelSize);

protected:
    virtual aclError DoCompile(CompileParam &param, std::shared_ptr<void> &modelData, size_t &modelSize) = 0;

private:
    static aclError MakeCompileParam(const AclOp &aclOp, CompileParam &param, int32_t compileFlag);
};
} // namespace acl

#endif // ACL_OP_EXEC_COMPILER_OP_COMPILER_H
