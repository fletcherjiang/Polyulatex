/**
* @file error_codes_api.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "error_codes_api.h"

namespace acl {
namespace {
    const int32_t MIN_GE_ERROR_CODE = 145000;
    const int32_t MAX_GE_ERROR_CODE = 545999;
    const int32_t MIN_RTS_ERROR_CODE = 107000;
    const int32_t MAX_RTS_ERROR_CODE = 507999;
}
    ErrorCodeFactory& ErrorCodeFactory::Instance()
    {
        static ErrorCodeFactory instance;
        return instance;
    }

    void ErrorCodeFactory::RegisterErrorCode(aclError errCode,
                                             ModuleId moduleId,
                                             int32_t moduleErrCode,
                                             const std::string &errDesc)
    {
        ErrorCodeHandle handle;
        handle.errCode = errCode;
        handle.moduleId = moduleId;
        handle.moduleErrCode = moduleErrCode;
        handle.errDesc = errDesc;

        auto key = std::make_pair(moduleId, moduleErrCode);
        auto iter = errMap_.find(key);
        if (iter != errMap_.end()) {
            return;
        }
        errMap_[key] = handle;
    }

    aclError ErrorCodeFactory::GetErrorCode(ModuleId moduleId, int32_t moduleErrCode)
    {
        // if ge error code is not in [MIN_GE_ERROR_CODE, MAX_GE_ERROR_CODE]
        // or [MIN_RTS_ERROR_CODE, MAX_RTS_ERROR_CODE], return common error code
        if (moduleId == MODULE_GE) {
            if (((moduleErrCode >= MIN_GE_ERROR_CODE) && (moduleErrCode <= MAX_GE_ERROR_CODE)) ||
                    ((moduleErrCode >= MIN_RTS_ERROR_CODE) && (moduleErrCode <= MAX_RTS_ERROR_CODE))) {
                return moduleErrCode;
            }
            return ACL_ERROR_GE_FAILURE;
        }
        return moduleErrCode;
    }

    const char* ErrorCodeFactory::GetErrDesc(ModuleId moduleId, int32_t moduleErrCode)
    {
        auto key = std::make_pair(moduleId, moduleErrCode);
        auto iter = errMap_.find(key);
        if (iter == errMap_.end()) {
            return "";
        }
        return iter->second.errDesc.c_str();
    }
}
