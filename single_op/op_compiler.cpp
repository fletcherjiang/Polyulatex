/**
* @file op_compiler.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/acl_op_compiler.h"
#include "framework/common/ge_format_util.h"
#include "common/log_inner.h"
#include "types/tensor_desc_internal.h"
#include "types/acl_op.h"
#include "op_executor.h"
#include "utils/array_utils.h"
#include "op_model_manager.h"
#include "toolchain/profiling_manager.h"
#include "compile/op_compile_processor.h"

using namespace acl;
namespace {
    std::map<aclCompileOpt, std::string> compileOptMap = {
        {ACL_PRECISION_MODE, ge::PRECISION_MODE},
        {ACL_AICORE_NUM, ge::AICORE_NUM},
        {ACL_AUTO_TUNE_MODE, ge::AUTO_TUNE_MODE},
        {ACL_OP_SELECT_IMPL_MODE, ge::OP_SELECT_IMPL_MODE},
        {ACL_OPTYPELIST_FOR_IMPLMODE, ge::OPTYPELIST_FOR_IMPLMODE},
        {ACL_OP_DEBUG_LEVEL, ge::OP_DEBUG_LEVEL},
        {ACL_DEBUG_DIR, ge::DEBUG_DIR},
        {ACL_OP_COMPILER_CACHE_MODE, ge::OP_COMPILER_CACHE_MODE},
        {ACL_OP_COMPILER_CACHE_DIR, ge::OP_COMPILER_CACHE_DIR},
        {ACL_OP_PERFORMANCE_MODE, ge::PERFORMANCE_MODE}
    };
}

aclError aclopCompile(const char *opType,
                      int numInputs,
                      const aclTensorDesc *const inputDesc[],
                      int numOutputs,
                      const aclTensorDesc *const outputDesc[],
                      const aclopAttr *attr,
                      aclopEngineType engineType,
                      aclopCompileType compileFlag,
                      const char *opPath)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_REQUIRES_NON_NEGATIVE(numInputs);
    ACL_REQUIRES_NON_NEGATIVE(numOutputs);
    if (compileFlag != ACL_COMPILE_SYS && compileFlag != ACL_COMPILE_UNREGISTERED) {
        ACL_LOG_INNER_ERROR("[Check][CompileFlag]aclopCompile compile type[%d] not support",
            static_cast<int32_t>(compileFlag));
        acl::AclErrorLogManager::ReportInputError(acl::UNSUPPORTED_FEATURE_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"compile type", std::to_string(compileFlag),
            "not in range"}));
        return ACL_ERROR_API_NOT_SUPPORT;
    }
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(opType);
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numInputs, inputDesc));
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numOutputs, outputDesc));
    if (array_utils::IsHostMemTensorDesc(numInputs, inputDesc) != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Check][TensorDesc]aclopCompile ACL_MEMTYPE_HOST placeMent in inputDesc not support");
        return ACL_ERROR_API_NOT_SUPPORT;
    }
    if (array_utils::IsHostMemTensorDesc(numOutputs, outputDesc) != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Check][TensorDesc]aclopCompile ACL_MEMTYPE_HOST placeMent in outputDesc not support");
        return ACL_ERROR_API_NOT_SUPPORT;
    }

    ACL_LOG_INFO("start to execute aclopCompile. opType = %s, engineType = %d, compileFlag = %d",
                 opType,
                 static_cast<int32_t>(engineType),
                 static_cast<int32_t>(compileFlag));
    AclOp aclOp;
    aclOp.opType = std::string(opType);
    aclOp.numInputs = numInputs;
    aclOp.inputDesc = inputDesc;
    aclOp.numOutputs = numOutputs;
    aclOp.outputDesc = outputDesc;
    aclOp.opAttr = attr;
    aclOp.isCompile = true;
    aclOp.engineType = engineType;
    aclOp.compileType = static_cast<OpCompileType>(compileFlag);
    if (compileFlag == ACL_COMPILE_UNREGISTERED) {
        if (opPath == nullptr) {
            ACL_LOG_INNER_ERROR("[Check][CompileFlag]opPath cannot be null while compileFlag is %d",
                static_cast<int32_t>(compileFlag));
            acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
                std::vector<std::string>({"param"}),
                std::vector<std::string>({"opPath"}));
            return ACL_ERROR_INVALID_PARAM;
        }
        aclOp.opPath = std::string(opPath);
    }
    ACL_LOG_INFO("aclopCompile::aclOp = %s", aclOp.DebugString().c_str());
    return OpCompileProcessor::GetInstance().OpCompile(aclOp);
}

aclError aclopCompileAndExecute(const char *opType,
    int numInputs, const aclTensorDesc *const inputDesc[], const aclDataBuffer *const inputs[],
    int numOutputs, const aclTensorDesc *const outputDesc[], aclDataBuffer *const outputs[],
    const aclopAttr *attr, aclopEngineType engineType, aclopCompileType compileFlag,
    const char *opPath, aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_LOG_INFO("start to execute aclopCompileAndExecute");
    ACL_REQUIRES_NON_NEGATIVE(numInputs);
    ACL_REQUIRES_NON_NEGATIVE(numOutputs);
    if (compileFlag != ACL_COMPILE_SYS && compileFlag != ACL_COMPILE_UNREGISTERED) {
        ACL_LOG_INNER_ERROR("[Check][Type]aclopCompile compile type[%d] not support",
            static_cast<int32_t>(compileFlag));
        acl::AclErrorLogManager::ReportInputError(acl::UNSUPPORTED_FEATURE_MSG,
            std::vector<std::string>({"feature", "reason"}),
            std::vector<std::string>({"compile type",
            "must be equal to ACL_COMPILE_SYS or ACL_COMPILE_UNREGISTERED"}));
        return ACL_ERROR_API_NOT_SUPPORT;
    }
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(opType);
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numInputs, inputDesc));
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numOutputs, outputDesc));
    if (array_utils::IsAllTensorEmpty(numOutputs, outputDesc)) {
        ACL_LOG_INFO("all ouput tensor are empty");
        return ACL_SUCCESS;
    }

    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numInputs, inputs));
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numOutputs, outputs));

    ACL_LOG_INFO("start to execute aclopCompileAndExecute, opType = %s, engineType = %d, compileFlag = %d",
        opType, static_cast<int32_t>(engineType), static_cast<int32_t>(compileFlag));

    AclOp aclOp;
    aclOp.opType = std::string(opType);
    aclOp.numInputs = numInputs;
    aclOp.inputDesc = inputDesc;
    aclOp.numOutputs = numOutputs;
    aclOp.outputDesc = outputDesc;
    aclOp.inputs = inputs;
    aclOp.outputs = outputs;
    aclOp.opAttr = attr;
    aclOp.isCompile = true;
    aclOp.engineType = engineType;
    aclOp.compileType = static_cast<OpCompileType>(compileFlag);
    if (compileFlag == ACL_COMPILE_UNREGISTERED) {
        if (opPath == nullptr) {
            ACL_LOG_INNER_ERROR("[Check][OpPath]opPath cannot be null while compileFlag is %d",
                static_cast<int32_t>(compileFlag));
            acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
                std::vector<std::string>({"param"}),
                std::vector<std::string>({"opPath"}));
            return ACL_ERROR_INVALID_PARAM;
        }
        aclOp.opPath = std::string(opPath);
    }
    ACL_LOG_INFO("aclopCompile::aclOp = %s", aclOp.DebugString().c_str());
    auto ret = OpCompileProcessor::GetInstance().OpCompile(aclOp);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("build op model failed, result = %d", ret);
        return ret;
    }
    ACL_LOG_INFO("ExecuteAsync::aclOp = %s", aclOp.DebugString().c_str());
    aclOp.isCompile = false;
    return OpExecutor::ExecuteAsync(aclOp, inputs, outputs, stream);
}

aclError aclSetCompileopt(aclCompileOpt opt, const char *value)
{
    ACL_LOG_INFO("start to execute aclSetCompileopt");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(value);
    std::string optStr = compileOptMap.find(opt) != compileOptMap.end() ? compileOptMap[opt] : "";
    if (optStr.empty()) {
        ACL_LOG_INNER_ERROR("[Check][Opt]Can not find any options[%d] valid in enum aclCompileOpt, "
            "please check input option.", opt);
        return ACL_ERROR_INTERNAL_ERROR;
    }
    std::string valueStr = std::string(value);
    OpCompileProcessor::GetInstance().SetCompileOpt(optStr, valueStr);
    ACL_LOG_INFO("Set compile option [%s] and value [%s]", optStr.c_str(), valueStr.c_str());
    return ACL_SUCCESS;
}

aclError aclopSetCompileFlag(aclOpCompileFlag flag)
{
    ACL_LOG_INFO("start to execute aclopSetCompileFlag, flag is %d", static_cast<int32_t>(flag));
    OpCompileProcessor::GetInstance().SetCompileFlag(static_cast<int32_t>(flag));
    return ACL_SUCCESS;
}

