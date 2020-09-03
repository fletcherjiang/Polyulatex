/**
* @file local_compiler.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "local_compiler.h"
#include "framework/common/util.h"
#include "ge/ge_api.h"
#include "op_compile_service.h"
#include "error_codes_inner.h"
#include "framework/generator/ge_generator.h"
#include "common/ge_types.h"
#include "common/common_inner.h"
#include "op_compile_processor.h"

using namespace ge;
static aclError GeFinalizeFunc()
{
    auto geRet = GEFinalize();
    if (geRet != SUCCESS) {
        ACL_LOG_CALL_ERROR("[Finalize][Ge]ge finalize failed. ge result = %u", geRet);
        return ACL_GET_ERRCODE_GE(geRet);
    }
    return ACL_SUCCESS;
}

namespace acl {
std::atomic_int LocalCompiler::counter_;

aclError LocalCompiler::Init(const std::map<std::string, std::string> &options)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (isInitialized_) {
        ACL_LOG_INNER_ERROR("[Check][Compiler]Compiler already initialized");
        return ACL_ERROR_REPEAT_INITIALIZE;
    }

    options_ = options;
    auto geRet = ge::GEInitialize(options_);
    if (geRet != SUCCESS) {
        ACL_LOG_CALL_ERROR("[Initialize][Ge]GEInitialize failed. ge result = %u", geRet);
        return ACL_GET_ERRCODE_GE(geRet);
    }
    SetGeFinalizeCallback(GeFinalizeFunc);
    ACL_LOG_INFO("LocalCompiler Init success.");
    isInitialized_ = true;
    return ACL_SUCCESS;
}

LocalCompiler::~LocalCompiler()
{
}

aclError LocalCompiler::DoCompile(CompileParam &param, std::shared_ptr<void> &modelData, size_t &modelSize)
{
    return OnlineCompile(param, modelData, modelSize);
}

aclError LocalCompiler::OnlineCompile(CompileParam &param, std::shared_ptr<void> &modelData, size_t &modelSize)
{
    aclrtContext savedContext = nullptr;
    bool needReset = aclrtGetCurrentContext(&savedContext) == SUCCESS;
    ge::ModelBufferData bufferData;
    std::map<std::string, std::string> options;
    OpCompileProcessor::GetInstance().GetGlobalCompileOpts(options);
    // need add options_ from GEInitialize
    options.insert(options_.begin(), options_.end());
    GeGenerator generator;
    OmgContext omgContext;
    SetThreadCompileOpts(options);
    auto geRet = generator.Initialize(options, omgContext);
    if (geRet != SUCCESS) {
        ACL_LOG_CALL_ERROR("[Initialize][Ge]call ge interface generator.Initialize failed. ge result = %u", geRet);
        return ACL_GET_ERRCODE_GE(geRet);
    }
    ACL_LOG_INFO("call ge interface generator.BuildSingleOpModel");
    geRet = generator.BuildSingleOpModel(param.opDesc, param.inputs, param.outputs,
                                         param.engineType, param.compileFlag, bufferData);
    if (needReset) {
        ACL_REQUIRES_OK(aclrtSetCurrentContext(savedContext));
    }

    if (geRet != SUCCESS) {
        ACL_LOG_CALL_ERROR("[BuildSingleOpModel][Ge]call ge interface generator.BuildSingleOpModel failed. "
            "ge result = %u", geRet);
        return ACL_GET_ERRCODE_GE(geRet);
    }

    geRet = generator.Finalize();
    if (geRet != SUCCESS) {
        ACL_LOG_CALL_ERROR("[Finalize][Ge]call ge interface generator.Finalize failed. ge result = %u", geRet);
        return ACL_GET_ERRCODE_GE(geRet);
    }

    modelData = bufferData.data;
    modelSize = static_cast<size_t>(bufferData.length);
    ACL_LOG_INFO("BuildSingleOpModel success");
    return SUCCESS;
}

OpCompiler *LocalCompiler::CreateCompiler()
{
    return new(std::nothrow) LocalCompiler();
}

static OpCompilerRegister g_registerNativeCompiler(NATIVE_COMPILER, &LocalCompiler::CreateCompiler);
} // namespace acl
