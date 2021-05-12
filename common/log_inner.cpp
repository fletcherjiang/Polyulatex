/**
* @file log_inner.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "log_inner.h"

#include "common/util/error_manager/error_manager.h"

namespace acl {
bool AclLog::isEnableEvent_ = false;

aclLogLevel AclLog::GetCurLogLevel()
{
    int32_t eventEnable = 0;
    int32_t dlogLevel = dlog_getlevel(ACL_MODE_ID, &eventEnable);
    aclLogLevel curLevel = ACL_INFO;
    switch (dlogLevel) {
        case DLOG_ERROR:
            curLevel = ACL_ERROR;
            break;
        case DLOG_WARN:
            curLevel = ACL_WARNING;
            break;
        case DLOG_INFO:
            curLevel = ACL_INFO;
            break;
        case DLOG_DEBUG:
            curLevel = ACL_DEBUG;
            break;
        default:
            break;
    }

    isEnableEvent_ = (eventEnable == 1); // Out put event enable
    return curLevel;
}

bool AclLog::IsEventLogOutputEnable()
{
    return isEnableEvent_;
}

bool AclLog::IsLogOutputEnable(aclLogLevel logLevel)
{
    aclLogLevel curLevel = AclLog::GetCurLogLevel();
    return (curLevel <= logLevel);
}

mmPid_t AclLog::GetTid()
{
    thread_local static mmPid_t tid = static_cast<mmPid_t>(mmGetTid());
    return tid;
}

void AclLog::ACLSaveLog(aclLogLevel logLevel, const char* strLog)
{
    if (strLog == nullptr) {
        return;
    }
    switch (logLevel) {
        case ACL_ERROR:
            dlog_error(APP_MODE_ID, "%s", strLog);
            break;
        case ACL_WARNING:
            dlog_warn(APP_MODE_ID, "%s", strLog);
            break;
        case ACL_INFO:
            dlog_info(APP_MODE_ID, "%s", strLog);
            break;
        case ACL_DEBUG:
            dlog_debug(APP_MODE_ID, "%s", strLog);
            break;
        default:
            break;
    }
}

AclErrorLogManager::AclErrorLogManager(const char *const firstStage, const char *const secondStage)
{
    ErrorManager::GetInstance().SetStage(firstStage, secondStage);
};

AclErrorLogManager::~AclErrorLogManager()
{
    ErrorManager::GetInstance().SetStage("", "");
};

const std::string AclErrorLogManager::GetStagesHeader()
{
    return ErrorManager::GetInstance().GetLogHeader();
}

std::string AclErrorLogManager::FormatStr(const char *fmt, ...)
{
    if (fmt == nullptr) {
        return "";
    }

    va_list ap;
    va_start(ap, fmt);
    char str[acl::MAX_LOG_STRING] = { '\0' };
    int32_t printRet = vsnprintf_s(str, static_cast<size_t>(acl::MAX_LOG_STRING),
        static_cast<size_t>(acl::MAX_LOG_STRING - 1), fmt, ap);
    if (printRet == -1) {
        va_end(ap);
        return "";
    }
    va_end(ap);
    return str;
}

void AclErrorLogManager::ReportInputError(std::string errorCode, const std::vector<std::string> &key,
    const std::vector<std::string> &value)
{
    REPORT_INPUT_ERROR(errorCode, key, value);
}

void AclErrorLogManager::ReportInnerError(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char errorMsgStr[LIMIT_PER_MESSAGE] = { '\0' };
    int32_t ret = vsnprintf_s(errorMsgStr, static_cast<size_t>(LIMIT_PER_MESSAGE), static_cast<size_t>(LIMIT_PER_MESSAGE - 1), fmt, ap);
    if (ret == -1) {
        va_end(ap);
        ACL_LOG_ERROR("[Call][Vsnprintf]call vsnprintf failed, ret = %d", ret);
        return;
    }
    va_end(ap);
    REPORT_INNER_ERROR("EH9999", "%s", errorMsgStr);
}

void AclErrorLogManager::ReportCallError(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char errorMsgStr[LIMIT_PER_MESSAGE] = { '\0' };
    int32_t ret = vsnprintf_s(errorMsgStr, static_cast<size_t>(LIMIT_PER_MESSAGE), static_cast<size_t>(LIMIT_PER_MESSAGE - 1), fmt, ap);
    if (ret == -1) {
        va_end(ap);
        ACL_LOG_ERROR("[Call][Vsnprintf]call vsnprintf failed, ret = %d", ret);
        return;
    }
    va_end(ap);
    REPORT_CALL_ERROR("EH9999", "%s", errorMsgStr);
}

void AclErrorLogManager::ReportInputErrorWithChar(const char *const errorCode, const char *argNames[],
    const char *argVals[], size_t size)
{
    std::vector<std::string> argNameArr;
    std::vector<std::string> argValArr;
    for (size_t i = 0; i < size; ++i) {
        argNameArr.push_back(argNames[i]);
        argValArr.push_back(argVals[i]);
    }
    REPORT_INPUT_ERROR(errorCode, argNameArr, argValArr);
}
} // namespace acl
