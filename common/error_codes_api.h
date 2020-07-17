/**
* @file error_codes_api.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ACL_ERROR_CODES_API_H_
#define ACL_ERROR_CODES_API_H_

#include <map>
#include <string>
#include "acl/acl_base.h"

enum ModuleId {
    MODULE_ACL = 0,
    MODULE_GE = 1,
    MODULE_RTS = 2,
    MODULE_INVALID
};

namespace acl {
struct ErrorCodeHandle {
    ModuleId moduleId = MODULE_INVALID;
    int32_t moduleErrCode = -1;
    aclError errCode = ACL_ERROR_INTERNAL_ERROR;
    std::string errDesc;
};

using InnerCodeType = std::pair<ModuleId, int32_t>;
using ErrorCodeMap = std::map<InnerCodeType, ErrorCodeHandle>;

class ACL_FUNC_VISIBILITY ErrorCodeFactory {
public:
    static ErrorCodeFactory &Instance();
    void RegisterErrorCode(aclError errCode, ModuleId moduleId, int32_t moduleErrCode, const std::string &errDesc);
    aclError GetErrorCode(ModuleId moduleId, int32_t moduleErrCode);
    const char *GetErrDesc(ModuleId moduleId, int32_t moduleErrCode);
private:
    ErrorCodeFactory() = default;
    ~ErrorCodeFactory() = default;
    ErrorCodeMap errMap_;
};

class ErrorCodeRegister {
public:
    ErrorCodeRegister(aclError errCode, ModuleId moduleId, int32_t moduleErrCode, const std::string &errDesc)
    {
        ErrorCodeFactory::Instance().RegisterErrorCode(errCode, moduleId, moduleErrCode, errDesc);
    }

    ~ErrorCodeRegister() {}
};
} // namespace acl

#define ACL_REG_ERRCODE(errCode, moduleId, moduleErrCode, errDesc) \
    const acl::ErrorCodeRegister g_##moduleId##_##moduleErrCode##_errorno(errCode, moduleId, moduleErrCode, errDesc)

#define ACL_GET_ERRCODE(moduleId, moduleErrCode) \
    acl::ErrorCodeFactory::Instance().GetErrorCode(moduleId, moduleErrCode)

#define ACL_GET_ERRDESC(moduleId, moduleErrCode) \
    acl::ErrorCodeFactory::Instance().GetErrDesc(moduleId, moduleErrCode)

#endif // ACL_ERROR_CODES_API_H_
