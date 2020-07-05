/**
* @file local_compiler.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_OP_EXEC_COMPILER_LOCAL_COMPILER_H
#define ACL_OP_EXEC_COMPILER_LOCAL_COMPILER_H

#include "op_compiler.h"

#include <mutex>
#include <atomic>

#include "framework/generator/ge_generator.h"

namespace acl {
class LocalCompiler : public OpCompiler {
public:
    LocalCompiler() = default;

    ~LocalCompiler() override;

    static OpCompiler *CreateCompiler();

    aclError Init(const std::map<std::string, std::string> &options) override;

protected:
    aclError DoCompile(CompileParam &param, std::shared_ptr<void> &modelData, size_t &modelSize) override;

private:
    aclError OnlineCompile(CompileParam &param, std::shared_ptr<void> &modelData, size_t &modelSize);

    std::map<std::string, std::string> options_;
    std::mutex mutex_;
    bool isInitialized_ = false;
    static std::atomic_int counter_;
};
} // namespace acl

#endif // ACL_OP_EXEC_COMPILER_LOCAL_COMPILER_H
