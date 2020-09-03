/**
* @file stream.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/acl_rt.h"

#include "runtime/stream.h"
#include "framework/executor/ge_executor.h"

#include "log_inner.h"
#include "single_op/executor/stream_executor.h"

#include "error_codes_inner.h"
#include "toolchain/profiling_manager.h"
#include "toolchain/resource_statistics.h"

aclError aclrtCreateStream(aclrtStream *stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_STREAM);
    ACL_LOG_INFO("start to execute aclrtCreateStream");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(stream);

    rtStream_t rtStream = nullptr;
    rtError_t rtErr = rtStreamCreate(&rtStream, RT_STREAM_PRIORITY_DEFAULT);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("create stream failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    *stream = static_cast<aclrtStream>(rtStream);
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_STREAM);
    return ACL_SUCCESS;
}

aclError aclrtDestroyStream(aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_STREAM);
    ACL_LOG_INFO("start to execute aclrtDestroyStream");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(stream);

    (void) ge::GeExecutor::ReleaseSingleOpResource(stream);
    (void) acl::Executors::Remove(nullptr, stream);
    ACL_LOG_INFO("release singleop success");
    rtError_t rtErr = rtStreamDestroy(static_cast<rtStream_t>(stream));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("destroy stream failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("aclrtDestroyStream success");
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_STREAM);
    return ACL_SUCCESS;
}

aclError aclrtSynchronizeStream(aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtSynchronizeStream");

    rtError_t rtErr = rtStreamSynchronize(static_cast<rtStream_t>(stream));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("synchronize stream failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    ACL_LOG_INFO("Synchronize stream success");
    return ACL_SUCCESS;
}

aclError aclrtStreamWaitEvent(aclrtStream stream, aclrtEvent event)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtStreamWaitEvent");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(event);

    rtError_t rtErr = rtStreamWaitEvent(static_cast<rtStream_t>(stream), static_cast<rtEvent_t>(event));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("stream wait event failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    ACL_LOG_INFO("stream wait event success");
    return ACL_SUCCESS;
}
