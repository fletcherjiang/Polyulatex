/**
* @file context.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/acl_rt.h"

#include "runtime/context.h"
#include "framework/executor/ge_executor.h"

#include "log_inner.h"

#include "error_codes_inner.h"

#include "toolchain/profiling_manager.h"

#include "toolchain/resource_statistics.h"

aclError aclrtCreateContext(aclrtContext *context, int32_t deviceId)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_CONTEXT);
    ACL_LOG_INFO("start to execute aclrtCreateContext, device is %d.", deviceId);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(context);

    rtContext_t rtCtx = nullptr;
    rtError_t rtErr = rtCtxCreateEx(&rtCtx, static_cast<uint32_t>(RT_CTX_NORMAL_MODE), deviceId);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("create context failed, device is %d, runtime result is %d",
            deviceId, static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    ACL_LOG_INFO("successfully execute aclrtCreateContext, device is %d.", deviceId);
    *context = static_cast<aclrtContext>(rtCtx);
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_CONTEXT);
    return ACL_SUCCESS;
}

aclError aclrtDestroyContext(aclrtContext context)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_CONTEXT);
    ACL_LOG_INFO("start to execute aclrtDestroyContext.");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(context);

    (void) ge::GeExecutor::ReleaseSingleOpResource(context);

    rtError_t rtErr = rtCtxDestroyEx(static_cast<rtContext_t>(context));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("destory context failed, runtime result is %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtDestroyContext");
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_CONTEXT);
    return ACL_SUCCESS;
}

aclError aclrtSetCurrentContext(aclrtContext context)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtSetCurrentContext.");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(context);

    rtError_t rtErr = rtCtxSetCurrent(static_cast<rtContext_t>(context));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("set current context failed, runtime result is %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtSetCurrentContext");
    return ACL_SUCCESS;
}

aclError aclrtGetCurrentContext(aclrtContext *context)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtGetCurrentContext");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(context);

    rtContext_t rtCtx = nullptr;
    rtError_t rtErr = rtCtxGetCurrent(&rtCtx);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_WARN("get current context failed, runtime result is %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    *context = rtCtx;
    ACL_LOG_INFO("successfully execute aclrtGetCurrentContext");
    return ACL_SUCCESS;
}

