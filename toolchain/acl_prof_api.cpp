/**
* @file acl_prof_api.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "securec.h"
#include "mmpa/mmpa_api.h"
#include "executor/ge_executor.h"
#include "common/profiling/ge_profiling.h"

#include "log_inner.h"
#include "toolchain/profiling_manager.h"
#include "common/common_inner.h"
#include "error_codes_inner.h"
#include "acl/acl_rt.h"

using namespace std;
using namespace acl;

namespace {
    std::mutex g_aclProfMutex;

    const std::string ACL_PROFILING_INIT = "prof_init";
    const std::string ACL_PROFILING_FINALIZE = "prof_finalize";
    const std::string ACL_PROFILING_START = "prof_start";
    const std::string ACL_PROFILING_STOP = "prof_stop";

    const std::string ACL_PROFILING_MODEL_SUB = "prof_model_subscribe";
    const std::string ACL_PROFILING_MODEL_CANCEL_SUB = "prof_model_cancel_subscribe";

    const char *ACL_PROFILING_REG_NAME = "profiling";

    const std::string ACL_NUM_DEVICES = "devNums";
    const std::string ACL_DEVICE_ID_LIST = "devIdList";
    const std::string ACL_PROF_MODEL_ID = "modelId";

    const size_t ACL_MAX_DEVICE_NUM = 64;

    const uint32_t PROF_COMMANDHANDLE_INIT = 0;
    const uint32_t PROF_COMMANDHANDLE_START = 1;
    const uint32_t PROF_COMMANDHANDLE_STOP = 2;
    const uint32_t PROF_COMMANDHANDLE_FINALIZE = 3;
    const uint32_t PROF_COMMANDHANDLE_MODEL_SUB = 4;
    const uint32_t PROF_COMMANDHANDLE_MODEL_UNSUB = 5;

    const uint64_t ACL_PROF_ACL_API = 0x0001;
    const uint64_t ACL_PROF_MODEL_LOAD = 0x8000000000000000;

    struct AclprofCommandHandle {
        uint64_t profSwitch;
        uint32_t devNums;                          // length of device id list
        uint32_t devIdList[ACL_MAX_DEVICE_NUM];
        uint32_t modelId;
    };
}

// inner interface for registering prof ctrl callback
aclError _aclprofRegisterCtrlCallback(MsprofCtrlCallback callback)
{
    ACL_LOG_INFO("start to register ctrl callback");
    std::lock_guard<std::mutex> lock(g_aclProfMutex);
    ACL_REQUIRES_NOT_NULL(callback);
    // ctrl callback cannot be repeatly regitered
    if (AclProfilingManager::GetInstance().GetProfCtrlCallback() != nullptr) {
        ACL_LOG_WARN("ctrl callback cannot be repeatly registered");
        return ACL_SUCCESS;
    }

    // register ctrl callback
    AclProfilingManager::GetInstance().SetProfCtrlCallback(callback);
    ACL_LOG_INFO("successfully register ctrl callback");
    return ACL_SUCCESS;
}

// inner interface for registering setdevice callback
aclError _aclprofRegisterSetDeviceCallback(MsprofSetDeviceCallback callback)
{
    ACL_LOG_INFO("start to register setdevice callback");
    std::lock_guard<std::mutex> lock(g_aclProfMutex);
    ACL_REQUIRES_NOT_NULL(callback);
    // register setdevice callback to rts
    rtError_t rtErr = rtRegDeviceStateCallback(ACL_PROFILING_REG_NAME, static_cast<rtDeviceStateCallback>(callback));
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_ERROR("register device state callback failed, runtime result = %d", static_cast<int32_t>(rtErr));
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_LOG_INFO("successfully register setdevice callback");
    return ACL_SUCCESS;
}

// inner interface for registering reporter callback
aclError _aclprofRegisterReporterCallback(MsprofReporterCallback callback)
{
    ACL_LOG_INFO("start to register reporter callback");
    std::lock_guard<std::mutex> lock(g_aclProfMutex);
    ACL_REQUIRES_NOT_NULL(callback);

    // reporter callback cannot be repeatly regitered
    if (AclProfilingManager::GetInstance().GetProfReporterCallback() != nullptr) {
        ACL_LOG_WARN("prof reporter callback cannot be repeatly registered");
        return ACL_SUCCESS;
    }
    AclProfilingManager::GetInstance().SetProfReporterCallback(callback);
    // register reporter callback to ge
    ge::Status geRet = RegProfReporterCallback(callback);
    if (geRet != ge::SUCCESS) {
        ACL_LOG_WARN("register reporter callback to ge failed, ge result = %u", geRet);
        return ACL_SUCCESS;
    }
    ACL_LOG_INFO("successfully register reporter callback");
    return ACL_SUCCESS;
}

// inner interface for initializing profiling
static aclError aclprofInnerInit()
{
    ACL_LOG_INFO("start to execute aclprofInnerInit");
    std::lock_guard<std::mutex> lock(g_aclProfMutex);
    // init config to ge
    ge::GeExecutor geExecutor;
    ge::Command command;
    command.cmd_params.clear();
    command.cmd_type = ACL_PROFILING_INIT;
    command.module_index = ACL_PROF_MODEL_LOAD;
    ge::Status geRet = geExecutor.CommandHandle(command);
    if (geRet != ge::SUCCESS) {
        ACL_LOG_ERROR("handle profiling command failed, ge result = %d", geRet);
        return ACL_GET_ERRCODE_GE(geRet);
    }
    ACL_LOG_INFO("successfully execute aclprofInnerInit");
    return ACL_SUCCESS;
}

// transfer struct profilerConfig to string vector ge needs
static bool TransAclProfConfigToStrArr(const AclprofCommandHandle *profilerConfig, vector<string> &profConfigStrArr)
{
    if (profilerConfig == nullptr) {
        ACL_LOG_ERROR("profilerConfig is nullptr");
        return false;
    }

    profConfigStrArr.clear();
    profConfigStrArr.push_back(ACL_NUM_DEVICES);
    profConfigStrArr.push_back(std::to_string(profilerConfig->devNums));
    profConfigStrArr.push_back(ACL_DEVICE_ID_LIST);
    string devID;
    for (size_t i = 0; i < profilerConfig->devNums; ++i) {
        devID.append(std::to_string(profilerConfig->devIdList[i]));
        if (i != (profilerConfig->devNums - 1)) {
            devID.append(",");
        }
    }
    profConfigStrArr.push_back(devID);
    return true;
}

// inner interface for starting profiling
static aclError aclprofInnerStart(void *data, uint32_t len)
{
    ACL_LOG_INFO("start to execute aclprofInnerStart");
    std::lock_guard<std::mutex> lock(g_aclProfMutex);
    ACL_REQUIRES_NOT_NULL(data);
    size_t commandLen = sizeof(AclprofCommandHandle);
    if ((len == 0) || (len != commandLen)) {
        ACL_LOG_ERROR("len[%u] is invalid, it should be %zu", len, commandLen);
        return ACL_ERROR_INVALID_PARAM;
    }
    ge::GeExecutor geExecutor;
    ge::Command command;
    command.cmd_params.clear();
    command.cmd_type = ACL_PROFILING_START;
    AclprofCommandHandle *profilerConfig = static_cast<AclprofCommandHandle *>(data);
    vector<string> profConfigStrArr;
    if (!TransAclProfConfigToStrArr(profilerConfig, profConfigStrArr)) {
        ACL_LOG_ERROR("transfer profilerConfig to string vector failed");
        return ACL_ERROR_INVALID_PARAM;
    }
    command.cmd_params = profConfigStrArr;
    command.module_index = profilerConfig->profSwitch;
    ge::Status geRet = geExecutor.CommandHandle(command);
    if (geRet != ge::SUCCESS) {
        ACL_LOG_ERROR("handle profiling command failed");
        return ACL_GET_ERRCODE_GE(geRet);
    }
    ACL_LOG_INFO("GE has been allocated start profiling config");
    if ((profilerConfig->profSwitch & ACL_PROF_ACL_API) != 0) {
        ACL_LOG_DEBUG("acl profiling has been configed by api mode");
        if (!AclProfilingManager::GetInstance().AclProfilingIsRun()) {
            aclError ret = AclProfilingManager::GetInstance().Init();
            if (ret != ACL_SUCCESS) {
                ACL_LOG_ERROR("start acl profiling module failed, result = %d", ret);
                return ret;
            }
        }
        AclProfilingManager::GetInstance().AddDeviceList(profilerConfig->devIdList, profilerConfig->devNums);
    }
    ACL_LOG_INFO("successfully execute aclprofInnerStart");
    return ACL_SUCCESS;
}

// inner interface for stoping profiling
static aclError aclprofInnerStop(void *data, uint32_t len)
{
    ACL_LOG_INFO("start to execute aclprofInnerStop");
    std::lock_guard<std::mutex> lock(g_aclProfMutex);
    ACL_REQUIRES_NOT_NULL(data);
    size_t requiredLen = sizeof(AclprofCommandHandle);
    if ((len == 0) || (len != requiredLen)) {
        ACL_LOG_ERROR("data len[%u] is invalid, it should be %zu", len, requiredLen);
        return ACL_ERROR_INVALID_PARAM;
    }
    ge::GeExecutor geExecutor;
    ge::Command command;
    command.cmd_params.clear();
    command.cmd_type = ACL_PROFILING_STOP;
    AclprofCommandHandle *profilerConfig = static_cast<AclprofCommandHandle *>(data);
    command.module_index = profilerConfig->profSwitch;
    vector<string> profConfigStrArr;
    if (!TransAclProfConfigToStrArr(profilerConfig, profConfigStrArr)) {
        ACL_LOG_ERROR("transfer profilerConfig to string vector failed");
        return ACL_ERROR_INVALID_PARAM;
    }
    command.cmd_params = profConfigStrArr;
    ge::Status geRet = geExecutor.CommandHandle(command);
    if (geRet != ge::SUCCESS) {
        ACL_LOG_ERROR("handle profiling command failed, ge result = %u", geRet);
        return ACL_GET_ERRCODE_GE(geRet);
    }

    if (!AclProfilingManager::GetInstance().IsDeviceListEmpty()) {
        AclProfilingManager::GetInstance().RemoveDeviceList(profilerConfig->devIdList, profilerConfig->devNums);
    }

    if ((AclProfilingManager::GetInstance().IsDeviceListEmpty()) &&
        (AclProfilingManager::GetInstance().AclProfilingIsRun())) {
        aclError ret = AclProfilingManager::GetInstance().UnInit();
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("stop acl failed, result = %d", ret);
            return ret;
        }
    }
    ACL_LOG_INFO("successfully execute aclprofInnerStop");
    return ACL_SUCCESS;
}

// inner interface for finalizing profiling
static aclError aclprofInnerFinalize()
{
    ACL_LOG_INFO("start to execute aclprofInnerFinalize");
    std::lock_guard<std::mutex> lock(g_aclProfMutex);
    ge::GeExecutor geExecutor;
    ge::Command command;
    command.cmd_params.clear();
    command.cmd_type = ACL_PROFILING_FINALIZE;
    ge::Status geRet = geExecutor.CommandHandle(command);
    if (geRet != ge::SUCCESS) {
        ACL_LOG_ERROR("handle profiling command failed, ge result = %u", geRet);
        return ACL_GET_ERRCODE_GE(geRet);
    }

    if (AclProfilingManager::GetInstance().AclProfilingIsRun()) {
        aclError ret = AclProfilingManager::GetInstance().UnInit();
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("stop acl profiling module failed, result = %d", ret);
            return ret;
        }
        AclProfilingManager::GetInstance().RemoveAllDeviceList();
    }
    ACL_LOG_INFO("successfully execute aclprofInnerFinalize");
    return ACL_SUCCESS;
}

// inner interface for subscribing model
static aclError aclprofInnerModelSubscribe(void *data, uint32_t len)
{
    ACL_LOG_INFO("start to execute aclprofInnerModelSubscribe");
    std::lock_guard<std::mutex> lock(g_aclProfMutex);
    ACL_REQUIRES_NOT_NULL(data);
    size_t requiredLen = sizeof(AclprofCommandHandle);
    if ((len == 0) || (len != requiredLen)) {
        ACL_LOG_ERROR("data len[%u] is invalid, it should be %zu", len, requiredLen);
        return ACL_ERROR_INVALID_PARAM;
    }
    AclprofCommandHandle *profilerConfig = static_cast<AclprofCommandHandle *>(data);
    ge::GeExecutor geExecutor;
    ge::Command command;
    command.cmd_type = ACL_PROFILING_MODEL_SUB;
    command.module_index = profilerConfig->profSwitch;
    command.cmd_params.clear();
    command.cmd_params.push_back(ACL_PROF_MODEL_ID);
    command.cmd_params.push_back(std::to_string(profilerConfig->modelId));
    ge::Status geRet = geExecutor.CommandHandle(command);
    if (geRet != ge::SUCCESS) {
        ACL_LOG_ERROR("handle profiling command failed, ge result = %u", geRet);
        return ACL_GET_ERRCODE_GE(geRet);
    }
    ACL_LOG_INFO("successfully execute aclprofInnerModelSubscribe");
    return ACL_SUCCESS;
}

// inner interface for unsubscribing model
static aclError aclprofInnerModelUnSubscribe(void *data, uint32_t len)
{
    ACL_LOG_INFO("start to execute aclprofInnerModelUnSubscribe");
    std::lock_guard<std::mutex> lock(g_aclProfMutex);
    ACL_REQUIRES_NOT_NULL(data);
    uint32_t requiredLen = sizeof(AclprofCommandHandle);
    if ((len == 0) || (len != requiredLen)) {
        ACL_LOG_ERROR("data len[%u] is invalid, it should be %u", len, requiredLen);
        return ACL_ERROR_INVALID_PARAM;
    }
    AclprofCommandHandle *profilerConfig = static_cast<AclprofCommandHandle *>(data);
    ge::GeExecutor geExecutor;
    ge::Command command;
    command.cmd_type = ACL_PROFILING_MODEL_CANCEL_SUB;
    command.cmd_params.clear();
    command.cmd_params.push_back(ACL_PROF_MODEL_ID);
    command.cmd_params.push_back(std::to_string(profilerConfig->modelId));
    ge::Status geRet = geExecutor.CommandHandle(command);
    if (geRet != ge::SUCCESS) {
        ACL_LOG_ERROR("handle profiling command failed, ge result = %u", geRet);
        return ACL_GET_ERRCODE_GE(geRet);
    }
    ACL_LOG_INFO("successfully execute aclprofInnerModelUnSubscribe");
    return ACL_SUCCESS;
}

aclError _aclprofCommandHandle(uint32_t type, void *data, uint32_t len)
{
    ACL_LOG_INFO("start to execute _aclprofCommandHandle");
    aclError ret;
    switch (type) {
        case PROF_COMMANDHANDLE_INIT:
            ret = aclprofInnerInit();
            break;
        case PROF_COMMANDHANDLE_START:
            ret = aclprofInnerStart(data, len);
            break;
        case PROF_COMMANDHANDLE_STOP:
            ret = aclprofInnerStop(data, len);
            break;
        case PROF_COMMANDHANDLE_FINALIZE:
            ret = aclprofInnerFinalize();
            break;
        case PROF_COMMANDHANDLE_MODEL_SUB:
            ret = aclprofInnerModelSubscribe(data, len);
            break;
        case PROF_COMMANDHANDLE_MODEL_UNSUB:
            ret = aclprofInnerModelUnSubscribe(data, len);
            break;
        default:
            ACL_LOG_ERROR("command type[%u] is invalid, it should be between in [%u, %u]", type,
                PROF_COMMANDHANDLE_INIT, PROF_COMMANDHANDLE_MODEL_UNSUB);
            ret = ACL_ERROR_INVALID_PARAM;
            return ret;
    }
    ACL_LOG_INFO("successfully execute _aclprofCommandHandle");
    return ret;
}

aclError _aclprofGetDeviceByModelId(uint32_t modelId, uint32_t &deviceId)
{
    ACL_LOG_INFO("start to execute _aclprofGetDeviceByModelId");
    ge::GeExecutor geExecutor;
    ge::Status geRet = geExecutor.GetDeviceIdByModelId(modelId, deviceId);
    if (geRet != ge::SUCCESS) {
        ACL_LOG_ERROR("model id[%u] has not been loaded", modelId);
        return ACL_GET_ERRCODE_GE(geRet);
    }
    ACL_LOG_INFO("successfully execute _aclprofGetDeviceByModelId");
    return ACL_SUCCESS;
}

bool _aclprofGetInitFlag()
{
    return GetAclInitFlag();
}
