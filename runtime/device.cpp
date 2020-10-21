/**
* @file device.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/acl_rt.h"

#include <string.h>
#include "runtime/dev.h"
#include "runtime/context.h"
#include "framework/executor/ge_executor.h"

#include "log_inner.h"

#include "error_codes_inner.h"
#include "toolchain/profiling_manager.h"
#include "toolchain/resource_statistics.h"

#include "set_device_vxx.h"
#include "common_inner.h"

namespace {
    std::map<int32_t, uint64_t> g_deviceCounterMap;
    std::mutex g_deviceCounterMutex;
}
static void IncDeviceCounter(int32_t deviceId)
{
    std::unique_lock<std::mutex> lk(g_deviceCounterMutex);
    auto iter = g_deviceCounterMap.find(deviceId);
    if (iter == g_deviceCounterMap.end()) {
        g_deviceCounterMap[deviceId] = 1;
    } else {
        ++iter->second;
    }
}

static bool DecDeviceCounter(int32_t deviceId)
{
    std::unique_lock<std::mutex> lk(g_deviceCounterMutex);
    auto iter = g_deviceCounterMap.find(deviceId);
    if (iter != g_deviceCounterMap.end()) {
        if (iter->second != 0) {
            --iter->second;
            if (iter->second == 0) {
                return true;
            }
        }
    } else {
        ACL_LOG_INFO("device %d has not been set.", deviceId);
    }
    return false;
}

aclError aclrtSetDevice(int32_t deviceId)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_SET_RESET_DEVICE);
    ACL_LOG_INFO("start to execute aclrtSetDevice, deviceId = %d.", deviceId);
    rtError_t rtErr = rtSetDevice(deviceId);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("open device %d failed, runtime result = %d.", deviceId, static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    IncDeviceCounter(deviceId);
    ACL_LOG_INFO("successfully execute aclrtSetDevice, deviceId = %d", deviceId);
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_SET_RESET_DEVICE);
    return ACL_SUCCESS;
}

aclError aclrtSetDeviceWithoutTsdVXX(int32_t deviceId)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_SET_RESET_DEVICE);
    ACL_LOG_INFO("start to execute aclrtSetDeviceWithoutTsdVXX, deviceId = %d.", deviceId);
    std::string socVersion = GetSocVersion();
    if (strncmp(socVersion.c_str(), "Ascend910", strlen("Ascend910")) != 0) {
        ACL_LOG_INFO("The soc version is not Ascend910, not support");
        return ACL_ERROR_API_NOT_SUPPORT;
    }
    rtError_t rtErr = rtSetDeviceWithoutTsd(deviceId);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("open device %d failed, runtime result = %d.", deviceId, static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    IncDeviceCounter(deviceId);
    ACL_LOG_INFO("open device %d successfully.", deviceId);
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_SET_RESET_DEVICE);
    return ACL_SUCCESS;
}

aclError aclrtResetDevice(int32_t deviceId)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_SET_RESET_DEVICE);
    ACL_LOG_INFO("start to execute aclrtResetDevice, deviceId = %d.", deviceId);
    if (DecDeviceCounter(deviceId)) {
        ACL_LOG_INFO("device %d reference count is 0.", deviceId);
        // get default context
        rtContext_t rtDefaultCtx = nullptr;
        rtError_t rtErr = rtGetPriCtxByDeviceId(deviceId, &rtDefaultCtx);
        if ((rtErr == RT_ERROR_NONE) && (rtDefaultCtx != nullptr)) {
            ACL_LOG_INFO("try release op resource for device %d.", deviceId);
            (void) ge::GeExecutor::ReleaseSingleOpResource(rtDefaultCtx);
        }
    }
    rtError_t rtErr = rtDeviceReset(deviceId);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("reset device %d failed, runtime result = %d.", deviceId, static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtResetDevice, reset device %d.", deviceId);
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_SET_RESET_DEVICE);
    return ACL_SUCCESS;
}

aclError aclrtResetDeviceWithoutTsdVXX(int32_t deviceId)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_SET_RESET_DEVICE);
    ACL_LOG_INFO("start to execute aclrtResetDeviceWithoutTsdVXX, deviceId = %d.", deviceId);
    std::string socVersion = GetSocVersion();
    if (0 != strncmp(socVersion.c_str(), "Ascend910", strlen("Ascend910"))) {
        ACL_LOG_INNER_ERROR("The soc version is not Ascend910, not support");
        return ACL_ERROR_API_NOT_SUPPORT;
    }
    if (DecDeviceCounter(deviceId)) {
        ACL_LOG_INFO("device %d reference count is 0.", deviceId);
        // get default context
        rtContext_t rtDefaultCtx = nullptr;
        rtError_t rtErr = rtGetPriCtxByDeviceId(deviceId, &rtDefaultCtx);
        if ((rtErr == RT_ERROR_NONE) && (rtDefaultCtx != nullptr)) {
            ACL_LOG_INFO("try release op resource for device %d.", deviceId);
            (void) ge::GeExecutor::ReleaseSingleOpResource(rtDefaultCtx);
        }
    }
    rtError_t rtErr = rtDeviceResetWithoutTsd(deviceId);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("reset device %d failed, runtime result = %d.", deviceId, static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtResetDeviceWithoutTsdVXX, reset device %d", deviceId);
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_SET_RESET_DEVICE);
    return ACL_SUCCESS;
}

aclError aclrtGetDevice(int32_t *deviceId)
{
    ACL_LOG_INFO("start to execute aclrtGetDevice");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(deviceId);
    rtError_t rtErr = rtGetDevice(deviceId);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_WARN("get device failed, runtime result = %d.", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_DEBUG("successfully execute aclrtGetDevice, get device id is %d.", *deviceId);
    return ACL_SUCCESS;
}

aclError aclrtGetRunMode(aclrtRunMode *runMode)
{
    ACL_LOG_INFO("start to execute aclrtGetRunMode");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(runMode);
    rtRunMode rtMode;
    rtError_t rtErr = rtGetRunMode(&rtMode);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("get runMode failed, runtime result = %d.", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    if (rtMode == RT_RUN_MODE_OFFLINE) {
        *runMode = ACL_DEVICE;
        return ACL_SUCCESS;
    }
    *runMode = ACL_HOST;
    ACL_LOG_INFO("successfully execute aclrtGetRunMode, current runMode is %d.", static_cast<int32_t>(*runMode));
    return ACL_SUCCESS;
}

aclError aclrtSynchronizeDevice()
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtSynchronizeDevice");
    rtError_t rtErr = rtDeviceSynchronize();
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("wait for compute device to finish failed, runtime result = %d.",
            static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("device synchronize successfully.");
    return ACL_SUCCESS;
}

aclError aclrtSetTsDevice(aclrtTsId tsId)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_RUNTIME);
    ACL_LOG_INFO("start to execute aclrtSetTsDevice, tsId = %d.", static_cast<int32_t>(tsId));
    if ((tsId != ACL_TS_ID_AICORE) && (tsId != ACL_TS_ID_AIVECTOR)) {
        ACL_LOG_INNER_ERROR("invalid tsId, tsID is %d.", static_cast<int32_t>(tsId));
        return ACL_ERROR_INVALID_PARAM;
    }
    rtError_t rtErr = rtSetTSDevice(static_cast<uint32_t>(tsId));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("set device ts %d failed, runtime result = %d.", static_cast<int32_t>(tsId), rtErr);
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtSetTsDevice, set device ts %d", static_cast<int32_t>(tsId));
    return ACL_SUCCESS;
}

aclError aclrtGetDeviceCount(uint32_t *count)
{
    ACL_LOG_INFO("start to execute aclrtGetDeviceCount");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(count);

    rtError_t rtErr = rtGetDeviceCount(reinterpret_cast<int32_t *>(count));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("get device count failed, runtime result = %d.", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully execute aclrtGetDeviceCount, get device count is %u.", *count);
    return ACL_SUCCESS;
}

