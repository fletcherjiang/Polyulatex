/**
* @file op_compiler_service.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_OP_EXEC_COMPILER_OP_COMPILE_SERVICE_H
#define ACL_OP_EXEC_COMPILER_OP_COMPILE_SERVICE_H

#include "op_compiler.h"

namespace acl {
using CompilerCreator = OpCompiler *(*)();

const int ACL_ERROR_COMPILER_NOT_REGISTERED = 16;

enum CompileStrategy {
    NO_COMPILER,
    NATIVE_COMPILER,
    REMOTE_COMPILER
};

class ACL_FUNC_VISIBILITY OpCompileService {
public:
    ~OpCompileService();

    static OpCompileService &GetInstance()
    {
        static OpCompileService instance;
        return instance;
    }

    void RegisterCreator(CompileStrategy strategy, CompilerCreator creatorFn);

    aclError SetCompileStrategy(CompileStrategy strategy, std::map<std::string, std::string> &options);

    aclError CompileOp(const AclOp &aclOp, std::shared_ptr<void> &modelData, size_t &modelSize);

private:
    OpCompileService() = default;

    std::map<CompileStrategy, CompilerCreator> creators_;
    OpCompiler *compiler_ = nullptr;
};

class ACL_FUNC_VISIBILITY OpCompilerRegister {
public:
    OpCompilerRegister(CompileStrategy strategy, CompilerCreator creatorFn);
    ~OpCompilerRegister() = default;
};
} // namespace acl

#endif // ACL_OP_EXEC_COMPILER_OP_COMPILE_SERVICE_H
