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
    char str[MAX_LOG_STRING] = { '\0' };
    int32_t printRet = vsnprintf_s(str, MAX_LOG_STRING, MAX_LOG_STRING - 1, fmt, ap);
    if (printRet == -1) {
        va_end(ap);
        return "";
    }
    va_end(ap);
    return str;
}
}