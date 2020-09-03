/**
* @file event.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/acl_rt.h"

#include "runtime/event.h"
#include "runtime/config.h"

#include "log_inner.h"

#include "error_codes_inner.h"

#include "toolchain/profiling_manager.h"

#include "toolchain/resource_statistics.h"

typedef void *aclrtNotify;

aclError aclrtCreateEvent(aclrtEvent *event)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_EVENT);
    ACL_LOG_INFO("start to execute aclrtCreateEvent");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(event);

    rtEvent_t rtEvent = nullptr;
    rtError_t rtErr = rtEventCreate(&rtEvent);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("create event failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    *event = static_cast<aclrtEvent>(rtEvent);
    ACL_LOG_INFO("successfully execute aclrtCreateEvent");
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_EVENT);
    return ACL_SUCCESS;
}

aclError aclrtCreateEventWithFlag(aclrtEvent *event, uint32_t flag)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_EVENT);
    ACL_LOG_INFO("start to execute aclrtCreateEventWithFlag.");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(event);
    rtEvent_t rtEvent = nullptr;
    rtError_t rtErr = rtEventCreateWithFlag(&rtEvent, flag);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("create event flag failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    *event = static_cast<aclrtEvent>(rtEvent);
    ACL_LOG_INFO("successfully execute aclrtCreateEventWithFlag.");
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_EVENT);
    return ACL_SUCCESS;
}

aclError aclrtDestroyEvent(aclrtEvent event)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_EVENT);
    ACL_LOG_INFO("start to execute aclrtDestroyEvent");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(event);

    rtError_t rtErr = rtEventDestroy(static_cast<rtEvent_t>(event));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("destroy event failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtDestroyEvent");
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_EVENT);
    return ACL_SUCCESS;
}

aclError aclrtRecordEvent(aclrtEvent event, aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_RECORD_RESET_EVENT);
    ACL_LOG_INFO("start to execute aclrtRecordEvent");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(event);

    rtError_t rtErr = rtEventRecord(static_cast<rtEvent_t>(event), static_cast<rtStream_t>(stream));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("record event failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtRecordEvent");
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_RECORD_RESET_EVENT);
    return ACL_SUCCESS;
}

aclError aclrtResetEvent(aclrtEvent event, aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_RECORD_RESET_EVENT);
    ACL_LOG_INFO("start to execute aclrtResetEvent");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(event);

    rtError_t rtErr = rtEventReset(static_cast<rtEvent_t>(event), static_cast<rtStream_t>(stream));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("reset event failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("reset event successfully.");
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_RECORD_RESET_EVENT);
    return ACL_SUCCESS;
}

aclError aclrtQueryEvent(aclrtEvent event, aclrtEventStatus *status)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtQueryEvent");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(event);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(status);

    rtError_t rtErr = rtEventQuery(static_cast<rtEvent_t>(event));
    if (rtErr == RT_ERROR_NONE) {
        *status = ACL_EVENT_STATUS_COMPLETE;
    } else if (rtErr == ACL_ERROR_RT_EVENT_NOT_COMPLETE) {
        *status = ACL_EVENT_STATUS_NOT_READY;
    } else {
        ACL_LOG_INNER_ERROR("query event status failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtQueryEvent");
    return ACL_SUCCESS;
}

aclError aclrtSynchronizeEvent(aclrtEvent event)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtSynchronizeEvent");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(event);

    rtError_t rtErr = rtEventSynchronize(static_cast<rtEvent_t>(event));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("wait event to be complete failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtSynchronizeEvent");
    return ACL_SUCCESS;
}

aclError aclrtEventElapsedTime(float *ms, aclrtEvent start, aclrtEvent end)
{
    ACL_LOG_INFO("start to execute aclrtEventElapsedTime");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(ms);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(start);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(end);

    rtError_t rtErr = rtEventElapsedTime(ms, start, end);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("computes events elapsed time failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtEventElapsedTime");
    return ACL_SUCCESS;
}

aclError aclrtCreateNotify(int32_t deviceId, aclrtNotify *notify)
{
    ACL_LOG_INFO("start to execute aclrtCreateNotify");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(notify);

    rtNotify_t rtNotify = nullptr;
    rtError_t rtErr = rtNotifyCreate(deviceId, &rtNotify);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("create notify for device %d failed, runtime result = %d", deviceId, static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    *notify = static_cast<aclrtNotify>(rtNotify);
    ACL_LOG_INFO("successfully execute aclrtCreateNotify");
    return ACL_SUCCESS;
}

aclError aclrtDestroyNotify(aclrtNotify notify)
{
    ACL_LOG_INFO("start to execute aclrtDestroyNotify");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(notify);

    rtError_t rtErr = rtNotifyDestroy(static_cast<rtNotify_t>(notify));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("destory notify failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtDestroyNotify");
    return ACL_SUCCESS;
}

aclError aclrtRecordNotify(aclrtNotify notify, aclrtStream stream)
{
    ACL_LOG_INFO("start to execute aclrtRecordNotify");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(notify);

    rtError_t rtErr = rtNotifyRecord(static_cast<rtNotify_t>(notify), static_cast<rtStream_t>(stream));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("record notify failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtRecordNotify");
    return ACL_SUCCESS;
}

aclError aclrtWaitNotify(aclrtNotify notify, aclrtStream stream)
{
    ACL_LOG_INFO("start to execute aclrtWaitNotify");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(notify);

    rtError_t rtErr = rtNotifyWait(static_cast<rtNotify_t>(notify), static_cast<rtStream_t>(stream));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("wait for a notify failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtWaitNotify");
    return ACL_SUCCESS;
}

aclError aclrtSetOpWaitTimeout(uint32_t timeout)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtSetOpWaitTimeout, timeout = %u", timeout);

    rtError_t rtErr = rtSetOpWaitTimeOut(timeout);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("set wait timeout failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }

    ACL_LOG_INFO("successfully execute aclrtSetOpWaitTimeout, timeout = %u", timeout);
    return ACL_SUCCESS;
}
