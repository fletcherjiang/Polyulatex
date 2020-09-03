/**
* @file callback.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/acl_rt.h"

#include "runtime/kernel.h"
#include "runtime/base.h"

#include "log_inner.h"
#include "error_codes_inner.h"
#include "toolchain/profiling_manager.h"

namespace {
    const uint32_t ACL_ERROR_INVALID_EXCEPTION_INFO = 0xFFFFFFFF;
}

aclError aclrtSubscribeReport(uint64_t threadId, aclrtStream stream)
{
    ACL_LOG_INFO("start to execute aclrtSubscribeReport, threadId is %lu.", threadId);
    rtError_t rtErr = rtSubscribeReport(threadId, static_cast<rtStream_t>(stream));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("subscribe report failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtSubscribeReport, threadId is %lu.", threadId);
    return ACL_SUCCESS;
}

aclError aclrtSetExceptionInfoCallback(aclrtExceptionInfoCallback callback)
{
    ACL_LOG_INFO("start to execute aclrtSetExceptionInfoCallback.");
    rtError_t rtErr = rtSetTaskFailCallback(static_cast<rtTaskFailCallback>(callback));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("set callback of fail task failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtSetExceptionInfoCallback");
    return ACL_SUCCESS;
}

uint32_t aclrtGetTaskIdFromExceptionInfo(const aclrtExceptionInfo *info)
{
    if (info == nullptr) {
        ACL_LOG_INNER_ERROR("exception information is null, get task id failed.");
        return ACL_ERROR_INVALID_EXCEPTION_INFO;
    }
    return info->taskid;
}

uint32_t aclrtGetStreamIdFromExceptionInfo(const aclrtExceptionInfo *info)
{
    if (info == nullptr) {
        ACL_LOG_INNER_ERROR("exception information is null, get stream id failed.");
        return ACL_ERROR_INVALID_EXCEPTION_INFO;
    }
    return info->streamid;
}

uint32_t aclrtGetThreadIdFromExceptionInfo(const aclrtExceptionInfo *info)
{
    if (info == nullptr) {
        ACL_LOG_INNER_ERROR("exception information is null, get thread id failed.");
        return ACL_ERROR_INVALID_EXCEPTION_INFO;
    }
    return info->tid;
}

uint32_t aclrtGetDeviceIdFromExceptionInfo(const aclrtExceptionInfo *info)
{
    if (info == nullptr) {
        ACL_LOG_INNER_ERROR("exception information is null, get device id failed.");
        return ACL_ERROR_INVALID_EXCEPTION_INFO;
    }
    return info->deviceid;
}

aclError aclrtLaunchCallback(aclrtCallback fn, void *userData, aclrtCallbackBlockType blockType,
    aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtLaunchCallback.");
    if ((blockType != ACL_CALLBACK_BLOCK) && (blockType != ACL_CALLBACK_NO_BLOCK)) {
        ACL_LOG_INNER_ERROR("invalid block type, the current blockType = %d", static_cast<int32_t>(blockType));
        return ACL_ERROR_INVALID_PARAM;
    }
    bool isBlock = (blockType == ACL_CALLBACK_BLOCK);
    rtError_t rtErr = rtCallbackLaunch(static_cast<rtCallback_t>(fn), userData,
        static_cast<rtStream_t>(stream), isBlock);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("launch callback task failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtLaunchCallback");
    return ACL_SUCCESS;
}

aclError aclrtProcessReport(int32_t timeout)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtProcessReport, timeout is %d.", timeout);
    // -1 represents infinite wait, tmeout value greater than 0 represents waiting for a fixed time.
    // other value is invalid.
    if ((timeout < -1) || (timeout == 0)) {
        ACL_LOG_CALL_ERROR("invalid timeout value, timeout[%d]", timeout);
        std::string timeoutStr = acl::AclErrorLogManager::FormatStr("%d", timeout);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG, std::vector<std::string>({"param", "value",
            "reason"}), std::vector<std::string>({"timeout", timeoutStr, "-1 represents infinite wait, "
            "tmeout value greater than 0 represents waiting for a fixed time"}));
        return ACL_ERROR_INVALID_PARAM;
    }
    rtError_t rtErr = rtProcessReport(timeout);
    if (rtErr != RT_ERROR_NONE) {
        if (rtErr == ACL_ERROR_RT_THREAD_SUBSCRIBE) {
            ACL_LOG_WARN("no subscribereport info, runtime result = %d", static_cast<int32_t>(rtErr));
        } else if (rtErr == ACL_ERROR_RT_REPORT_TIMEOUT) {
            ACL_LOG_WARN("wait subscribereport timeout, runtime result = %d", static_cast<int32_t>(rtErr));
        } else {
            ACL_LOG_CALL_ERROR("process report failed, runtime result = %d", static_cast<int32_t>(rtErr));
        }
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully aclrtProcessReport, timeout is %d.", timeout);
    return ACL_SUCCESS;
}

aclError aclrtUnSubscribeReport(uint64_t threadId, aclrtStream stream)
{
    ACL_LOG_INFO("start to execute aclrtUnSubscribeReport, threadId is %lu.", threadId);
    rtError_t rtErr = rtUnSubscribeReport(threadId, static_cast<rtStream_t>(stream));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("unsubscribe report failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtUnSubscribeReport, threadId is %lu.", threadId);
    return ACL_SUCCESS;
}
