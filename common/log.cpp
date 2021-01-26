/**
* @file log.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <cstdarg>
#include <securec.h>

#include "common/log_inner.h"
#include "toolchain/slog.h"

void aclAppLog(aclLogLevel logLevel, const char *func, const char *file, uint32_t line, const char *fmt, ...)
{
    if ((fmt == nullptr) || (func == nullptr) || (file == nullptr)) {
        return;
    }
    if (!acl::AclLog::IsLogOutputEnable(logLevel)) {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    char str[MAX_LOG_STRING] = { '\0' };
    char strLog[MAX_LOG_STRING] = { '\0' };
    int32_t printRet = vsnprintf_s(str, static_cast<size_t>(MAX_LOG_STRING), static_cast<size_t>(MAX_LOG_STRING - 1), fmt, ap);
    if (printRet != -1) {
        printRet = sprintf_s(strLog, static_cast<size_t>(MAX_LOG_STRING), "%d %s:%s:%d: \"%s\"",
            acl::AclLog::GetTid(), func, file, line, str);
        if (printRet != -1) {
            acl::AclLog::ACLSaveLog(logLevel, strLog);
        } else {
            acl::AclLog::ACLSaveLog(ACL_ERROR, "aclAppLog call sprintf_s failed");
        }
    } else {
        acl::AclLog::ACLSaveLog(ACL_ERROR, "aclAppLog call vsnprintf_s failed");
    }
    va_end(ap);
}
