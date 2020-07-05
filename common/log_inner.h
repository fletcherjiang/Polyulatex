/**
* @file log_inner.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ACL_COMMON_LOG_INNER_H_
#define ACL_COMMON_LOG_INNER_H_

#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include "toolchain/slog.h"
#include "acl/acl_base.h"
#include "mmpa/mmpa_api.h"

#define ACL_MODE_ID ASCENDCL
#define APP_MODE_ID APP

constexpr int MAX_LOG_STRING = 1024;

namespace acl {
class ACL_FUNC_VISIBILITY AclLog {
public:
    static bool IsLogOutputEnable(aclLogLevel logLevel);
    static mmPid_t GetTid();
    static void ACLSaveLog(aclLogLevel logLevel, const char* strLog);
    static bool IsEventLogOutputEnable();
private:
    static aclLogLevel GetCurLogLevel();
    static bool isEnableEvent_;
};
}
#ifdef RUN_TEST
#define ACL_LOG_INFO(fmt, ...)                                                                      \
    do {                                                                                            \
            if (acl::AclLog::IsLogOutputEnable(ACL_INFO)) {                                         \
                printf("INFO %d %s:%s:%d: "#fmt "\n",acl:: AclLog::GetTid(), __FUNCTION__,       \
                    __FILE__, __LINE__, ##__VA_ARGS__);                                             \
            }                                                                                       \
    } while (0)
#define ACL_LOG_DEBUG(fmt, ...)                                                                     \
    do {                                                                                            \
            if (acl::AclLog::IsLogOutputEnable(ACL_DEBUG)) {                                        \
                printf("DEBUG %d %s:%s:%d: "#fmt "\n", acl::AclLog::GetTid(), __FUNCTION__,   \
                    __FILE__, __LINE__, ##__VA_ARGS__);                                             \
            }                                                                                       \
    } while (0)
#define ACL_LOG_WARN(fmt, ...)                                                                      \
    do {                                                                                            \
            if (acl::AclLog::IsLogOutputEnable(ACL_WARNING)) {                                      \
                printf("WARN %d %s:%s:%d: "#fmt "\n", acl::AclLog::GetTid(), __FUNCTION__,    \
                    __FILE__, __LINE__, ##__VA_ARGS__);                                             \
            }                                                                                       \
    } while (0)
#define ACL_LOG_ERROR(fmt, ...)                                                                     \
    do {                                                                                            \
            printf("ERROR %d %s:%s:%d: "#fmt "\n", acl::AclLog::GetTid(), __FUNCTION__,       \
                __FILE__, __LINE__, ##__VA_ARGS__);                                                 \
    } while (0)
#define ACL_LOG_EVENT(fmt, ...)                                                                     \
    do {                                                                                            \
            if (acl::AclLog::IsEventLogOutputEnable()) {                                            \
                printf("EVENT %d %s:%s:%d: "#fmt "\n", acl::AclLog::GetTid(), __FUNCTION__,   \
                    __FILE__, __LINE__, ##__VA_ARGS__);                                             \
            }                                                                                       \
    } while (0)
#else
#define ACL_LOG_INFO(fmt, ...)                                                                      \
    do {                                                                                            \
            if (acl::AclLog::IsLogOutputEnable(ACL_INFO)) {                                         \
                dlog_info(ACL_MODE_ID, "%d %s: " fmt,acl:: AclLog::GetTid(), __FUNCTION__,          \
                    ##__VA_ARGS__);                                                                 \
            }                                                                                       \
    } while (0)
#define ACL_LOG_DEBUG(fmt, ...)                                                                     \
    do {                                                                                            \
            if (acl::AclLog::IsLogOutputEnable(ACL_DEBUG)) {                                        \
                dlog_debug(ACL_MODE_ID, "%d %s: " fmt, acl::AclLog::GetTid(), __FUNCTION__,         \
                    ##__VA_ARGS__);                                                                 \
            }                                                                                       \
    } while (0)
#define ACL_LOG_WARN(fmt, ...)                                                                      \
    do {                                                                                            \
            if (acl::AclLog::IsLogOutputEnable(ACL_WARNING)) {                                      \
                dlog_warn(ACL_MODE_ID, "%d %s: " fmt, acl::AclLog::GetTid(), __FUNCTION__,          \
                    ##__VA_ARGS__);                                                                 \
            }                                                                                       \
    } while (0)
#define ACL_LOG_ERROR(fmt, ...)                                                                     \
    do {                                                                                            \
            dlog_error(ACL_MODE_ID, "%d %s: " fmt, acl::AclLog::GetTid(), __FUNCTION__,             \
                ##__VA_ARGS__);                                                                     \
    } while (0)
#define ACL_LOG_EVENT(fmt, ...)                                                                     \
    do {                                                                                            \
            if (acl::AclLog::IsEventLogOutputEnable()) {                                            \
                dlog_event(ACL_MODE_ID, "%d %s: " fmt, acl::AclLog::GetTid(), __FUNCTION__,         \
                    ##__VA_ARGS__);                                                                 \
            }                                                                                       \
    } while (0)
#endif

inline bool IsDebugLogEnabled()
{
    int eventEnable = 0;
    int dlogLevel = dlog_getlevel(ACL_MODE_ID, &eventEnable);
    return dlogLevel <= DLOG_DEBUG;
}

inline bool IsInfoLogEnabled()
{
    int eventEnable = 0;
    int dlogLevel = dlog_getlevel(ACL_MODE_ID, &eventEnable);
    return dlogLevel <= DLOG_INFO;
}

#define ACL_REQUIRES_OK(expr) \
    do { \
        auto __ret = (expr); \
        if (__ret != ACL_SUCCESS) { \
            return __ret; \
        } \
    } \
    while (0)

// Validate whether the expr value is true
#define ACL_REQUIRES_TRUE(expr, errCode, errDesc) \
    do { \
        auto __ret = (expr); \
        if (__ret != true) { \
            ACL_LOG_ERROR(errDesc); \
            return (errCode); \
        } \
    } \
    while (0)

#define ACL_REQUIRES_NOT_NULL(val) \
    do { \
        if ((val) == nullptr) { \
            ACL_LOG_ERROR("param [%s] must not be null.", #val); \
            return ACL_ERROR_INVALID_PARAM; } \
        } \
    while (0)

#define ACL_REQUIRES_NOT_NULL_RET_NULL(val) \
    do { \
        if ((val) == nullptr) { \
            ACL_LOG_ERROR("param [%s] must not be null.", #val); \
            return nullptr; } \
        } \
    while (0)

#define ACL_REQUIRES_NOT_NULL_RET_STR(val) \
    do { \
        if ((val) == nullptr) { \
            ACL_LOG_ERROR("param [%s] must not be null.", #val); \
            return ""; } \
        } \
    while (0)

#define ACL_REQUIRES_NOT_NULL_RET_VOID(val) \
    do { \
        if ((val) == nullptr) { \
            ACL_LOG_ERROR("param [%s] must not be null.", #val); \
            return; } \
        } \
    while (0)

#define ACL_CHECK_RANGE_INT(val, min, max) \
    do { \
        if (((val) < (min)) || ((val) > (max))) { \
            ACL_LOG_ERROR("param [%s]:[%d] must be in range of [%d] and [%d]", #val, val, min, max); \
            return ACL_ERROR_INVALID_PARAM; } \
        } \
    while (0)

#define ACL_CHECK_RANGE_UINT(val, min, max) \
    do { \
        if (((val) < (min)) || ((val) > (max))) { \
            ACL_LOG_ERROR("param [%s]:[%u] must be in range of [%u] and [%u]", #val, val, min, max); \
            return ACL_ERROR_INVALID_PARAM; } \
        } \
    while (0)

#define ACL_CHECK_RANGE_FLOAT(val, min, max) \
    do { \
        if (((val) < (min)) || ((val) > (max))) { \
            ACL_LOG_ERROR("param [%s]:[%.2f] must be in range of [%.2f] and [%.2f]", #val, val, min, max); \
            return ACL_ERROR_INVALID_PARAM; } \
        } \
    while (0)

#define ACL_CHECK_MALLOC_RESULT(val) \
    do { \
        if ((val) == nullptr) { \
            ACL_LOG_ERROR("Allocate memory for [%s] failed.", #val); \
            return ACL_ERROR_BAD_ALLOC; } \
        } \
    while (0)

#define ACL_REQUIRES_NON_NEGATIVE(val) \
    do { \
        if ((val) < 0) { \
            ACL_LOG_ERROR("param [%s] must not be non-negative.", #val); \
            return ACL_ERROR_INVALID_PARAM; } \
        } \
    while (0)

#define  ACL_REQUIRES_POSITIVE(val) \
    do { \
        if ((val) <= 0) { \
            ACL_LOG_ERROR("param [%s] must be positive.", #val); \
            return ACL_ERROR_INVALID_PARAM; } \
        } \
    while (0)

#define ACL_REQUIRES_POSITIVE_RET_NULL(val) \
    do { \
        if ((val) <= 0) { \
            ACL_LOG_ERROR("param [%s] must be positive.", #val); \
            return nullptr; } \
        } \
    while (0)

#define ACL_DELETE(memory) \
    do { \
        delete (memory); \
        (memory) = nullptr; \
    } \
    while (0)

#define ACL_DELETE_ARRAY(memory) \
    do { \
        if (memory != nullptr) { \
            delete[] static_cast<char *>(memory); \
            (memory) = nullptr; \
        } \
    } \
    while (0)

#define ACL_CHECK_WITH_MESSAGE_AND_RETURN(exp, ret, ...) \
    do { \
        if (!(exp)) { \
            ACL_LOG_ERROR(__VA_ARGS__); \
            return (ret); \
        } \
    } \
    while (0)

#define ACL_CHECK_WITH_MESSAGE_AND_NO_RETURN(exp, ...) \
    do { \
        if (exp) { \
            ACL_LOG_WARN(__VA_ARGS__); \
        } else { \
            ACL_LOG_ERROR(__VA_ARGS__); \
        } \
    } \
    while (0)

#define ACL_DELETE_AND_SET_NULL(var) \
    do { \
        if (var != nullptr) { \
            delete var; \
            var = nullptr; \
        } \
    } \
    while (0)

#define ACL_DELETE_ARRAY_AND_SET_NULL(var) \
    do { \
        if (var != nullptr) { \
            delete[] var; \
            var = nullptr; \
        } \
    } \
    while (0)

#define ACL_FREE(var) \
    do { \
        if (var != nullptr) { \
            free(var); \
            var = nullptr; \
        } \
    } \
    while (0)

#define ACL_ALIGN_FREE(var) \
    do { \
        if (var != nullptr) { \
            mmAlignFree(var); \
            var = nullptr; \
        } \
    } \
    while (0)

// If make_shared is abnormal, print the log and execute the statement
#define ACL_MAKE_SHARED(expr0, expr1) \
    try { \
        expr0; \
    } catch (const std::bad_alloc &) { \
        ACL_LOG_ERROR("Make shared failed"); \
        expr1; \
    }

#define RETURN_NULL_WITH_ALIGN_FREE(newAddr, mallocAddr) \
    do { \
        if (newAddr == nullptr) { \
            ACL_LOG_ERROR("new memory failed"); \
            mmAlignFree(mallocAddr); \
            return nullptr;; \
        } \
    } \
    while (0)

#endif // ACL_COMMON_LOG_H_
