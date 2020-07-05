/**
* @file op_compile_processor.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "op_compile_processor.h"
#include "runtime/dev.h"
#include "single_op/op_model_manager.h"
#include "op_compile_service.h"
#include "ge/ge_api.h"
#include "error_codes_inner.h"

namespace acl {
OpCompileProcessor::OpCompileProcessor()
{
    (void)Init();
}

OpCompileProcessor::~OpCompileProcessor()
{
}

aclError OpCompileProcessor::Init()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (isInit_) {
        return ACL_SUCCESS;
    }
    aclError ret = SetOption();
    if (ret != ACL_SUCCESS) {
        ACL_LOG_ERROR("OpCompileProcessor init failed!");
        return ret;
    }

    isInit_ = true;
    return ACL_SUCCESS;
}

aclError OpCompileProcessor::SetOption()
{
    char socVersion[SOC_VERSION_LEN] = {0};
    auto ret = rtGetSocVersion(socVersion, sizeof(socVersion));
    if (ret != RT_ERROR_NONE) {
        ACL_LOG_ERROR("get soc version failed, runtime result = %d", static_cast<int32_t>(ret));
        return ACL_GET_ERRCODE_RTS(ret);
    }

    map<std::string, std::string> options = {
        { ge::SOC_VERSION, std::string(socVersion) },
        { ge::OP_SELECT_IMPL_MODE, std::string("high_precision")},
        { ge::PRECISION_MODE, std::string("allow_fp32_to_fp16")}
    };
    return OpCompileService::GetInstance().SetCompileStrategy(NATIVE_COMPILER, options);
}

aclError OpCompileProcessor::OpCompile(AclOp &aclOp)
{
    if (!isInit_) {
        ACL_LOG_ERROR("init env failed!");
        return ACL_ERROR_FAILURE;
    }

    return OpModelManager::GetInstance().GetOpModel(aclOp);
}

void OpCompileProcessor::SetCompileOpt(std::string &opt, std::string &value)
{
    std::lock_guard<std::mutex> lock(globalCompileOptsMutex);
    globalCompileOpts[opt] = value;
}

void OpCompileProcessor::GetGlobalCompileOpts(std::map<std::string, std::string> &currentOptions)
{
    currentOptions.clear();
    std::lock_guard<std::mutex> lock(globalCompileOptsMutex);
    currentOptions.insert(globalCompileOpts.begin(), globalCompileOpts.end());
}

void OpCompileProcessor::SetCompileFlag(int32_t flag)
{
    OpModelManager::GetInstance().SetCompileFlag(flag);
}
} // namespace acl

