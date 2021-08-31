/**
* @file op_compiler_service.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "op_compile_service.h"

namespace acl {
OpCompileService::~OpCompileService()
{
    ACL_DELETE_AND_SET_NULL(compiler_);
}

void OpCompileService::RegisterCreator(CompileStrategy strategy,
    CompilerCreator creatorFn)
{
    (void)creators_.emplace(strategy, creatorFn);
}

aclError OpCompileService::CompileOp(const acl::AclOp &aclOp, std::shared_ptr<void> &modelData, size_t &modelSize)
{
    if (compiler_ == nullptr) {
        ACL_LOG_DEBUG("compiler is not set");
        return ACL_ERROR_COMPILER_NOT_REGISTERED;
    }

    return compiler_->CompileOp(aclOp, modelData, modelSize);
}

aclError OpCompileService::SetCompileStrategy(CompileStrategy strategy,
    const std::map<std::string, std::string> &options)
{
    ACL_DELETE_AND_SET_NULL(compiler_);

    ACL_LOG_INFO("Set compile strategy to [%d]", static_cast<int32_t>(strategy));
    if (strategy == NO_COMPILER) {
        return ACL_SUCCESS;
    }

    if ((strategy != NATIVE_COMPILER) && (strategy != REMOTE_COMPILER)) {
        ACL_LOG_INNER_ERROR("[Check][Strategy]The current compile strategy[%d] is invalid.",
            static_cast<int32_t>(strategy));
        return ACL_ERROR_INVALID_PARAM;
    }

    auto it = creators_.find(strategy);
    if (it == creators_.end()) {
        ACL_LOG_INNER_ERROR("[Find][Strategy]Unsupported compile strategy, compile strategy is %d.",
            static_cast<int32_t>(strategy));
        return ACL_ERROR_COMPILER_NOT_REGISTERED;
    }

    auto newCompiler = it->second();
    ACL_CHECK_MALLOC_RESULT(newCompiler);

    auto ret = newCompiler->Init(options);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Init][Compiler]Init compiler failed");
        ACL_DELETE_AND_SET_NULL(newCompiler);
        return ret;
    }

    compiler_ = newCompiler;
    return ACL_SUCCESS;
}

OpCompilerRegister::OpCompilerRegister(CompileStrategy strategy, CompilerCreator creatorFn)
{
    OpCompileService::GetInstance().RegisterCreator(strategy, creatorFn);
}
} // namespace acl
