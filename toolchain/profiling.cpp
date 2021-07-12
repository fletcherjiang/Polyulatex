/**
* @file profiling.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "profiling.h"
#include "nlohmann/json.hpp"
#include "mmpa/mmpa_api.h"
#include "executor/ge_executor.h"
#include "common/log_inner.h"
#include "json_parser.h"
#include "profiling_manager.h"

namespace {
    const std::string ACL_PROF_CONFIG_NAME = "profiler";
    const uint32_t MAX_ENV_VALVE_LENGTH = 4096U;
    std::mutex g_aclProfMutex;
    const uint64_t ACL_PROF_ACL_API = 0x0001U;
    const uint32_t START_PROFILING = 1U;
    const uint32_t STOP_PROFILING = 2U;
}

namespace acl {
    aclError AclProfiling::HandleProfilingCommand(const std::string &config, bool configFileFlag, bool noValidConfig)
    {
        ACL_LOG_INFO("start to execute HandleProfilingCommand");
        int32_t ret = MSPROF_ERROR_NONE;
        if (noValidConfig) {
            ret = MsprofInit(MSPROF_CTRL_INIT_DYNA, nullptr, 0U);
            if (ret != MSPROF_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Init][Profiling]init profiling with nullptr failed, profiling result = %d", ret);
                return ACL_SUCCESS;
            }
        } else {
            if (configFileFlag) {
                ret = MsprofInit(MSPROF_CTRL_INIT_ACL_JSON,
                                 const_cast<char *>(config.c_str()),
                                 static_cast<uint32_t>(config.size()));
                if (ret != MSPROF_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Init][Profiling]handle json config of profiling failed, profiling result = %d", ret);
                    return ACL_ERROR_INVALID_PARAM;
                }
            } else {
                ret = MsprofInit(MSPROF_CTRL_INIT_ACL_ENV,
                                 const_cast<char *>(config.c_str()),
                                 static_cast<uint32_t>(config.size()));
                if (ret != MSPROF_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Init][Profiling]handle env config of profiling failed, profiling result = %d", ret);
                    return ACL_ERROR_INVALID_PARAM;
                }
            }
        }
        ACL_LOG_INFO("set profiling config successfully");
        return ACL_SUCCESS;
    }

    bool AclProfiling::GetProfilingConfigFile(std::string &fileName)
    {
        char environment[MAX_ENV_VALVE_LENGTH] = {0};
        const int32_t ret = mmGetEnv("PROFILER_SAMPLECONFIG", environment, sizeof(environment));
        if (ret == EN_OK) {
            ACL_LOG_INFO("get profiling config envValue[%s]", environment);
            fileName = environment;
            return true;
        }
        ACL_LOG_INFO("no profiling config file.");
        return false;
    }

    aclError AclProfiling::HandleProfilingConfig(const char *configPath)
    {
        ACL_LOG_INFO("start to execute HandleProfilingConfig");
        nlohmann::json js;
        std::string strConfig;
        acl::JsonParser jsonParser;
        bool configFileFlag = true; // flag of using config flie mode or not
        bool noValidConfig = false;
        aclError ret = ACL_SUCCESS;
        std::string envValue;
        // use profiling config from env variable if exist
        if ((GetProfilingConfigFile(envValue)) && (!envValue.empty())) {
            configFileFlag = false;
            ACL_LOG_INFO("start to use profiling config by env mode");
        }

        // use profiling config from acl.json if exist
        if (configFileFlag) {
            if ((configPath == nullptr) || (strlen(configPath) == 0U)) {
                ACL_LOG_INFO("configPath is null, no need to do profiling.");
                noValidConfig = true;
                ret = HandleProfilingCommand(strConfig, configFileFlag, noValidConfig);
                if (ret != ACL_SUCCESS) {
                    ACL_LOG_INNER_ERROR("[Handle][Command]handle profiling command failed, result = %d", ret);
                }
                return ret;
            }
            ret = jsonParser.ParseJsonFromFile(configPath, js, &strConfig, ACL_PROF_CONFIG_NAME.c_str());
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INFO("parse profiling config from file[%s] failed, result = %d", configPath, ret);
                noValidConfig = true;
            }

            if (strConfig.empty()) {
                ACL_LOG_INFO("profiling config file[%s] is empty", configPath);
                noValidConfig = true;
            }

            ACL_LOG_INFO("start to use profiling config by config file mode");
        }

        ACL_LOG_INFO("ParseJsonFromFile ok in HandleProfilingConfig");
        ret = HandleProfilingCommand(strConfig, configFileFlag, noValidConfig);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Handle][Command]handle profiling command failed, result = %d", ret);
            return ret;
        }

        ACL_LOG_INFO("set HandleProfilingConfig success");
        return ret;
    }
}

static aclError aclProfInnerStart(rtProfCommandHandle_t *profilerConfig)
{
    ACL_LOG_INFO("start to execute aclprofInnerStart");
    if (!acl::AclProfilingManager::GetInstance().AclProfilingIsRun()) {
        const aclError ret = acl::AclProfilingManager::GetInstance().Init();
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Init][ProfilingManager]start acl profiling module failed,"" result = %d", ret);
            return ret;
        }
    }
    (void)acl::AclProfilingManager::GetInstance().AddDeviceList(profilerConfig->devIdList, profilerConfig->devNums);
    ACL_LOG_INFO("successfully execute aclprofInnerStart");
    return ACL_SUCCESS;
}

// inner interface for stoping profiling
static aclError aclProfInnerStop(rtProfCommandHandle_t *profilerConfig)
{
    ACL_LOG_INFO("start to execute aclprofInnerStop");
    if (!acl::AclProfilingManager::GetInstance().IsDeviceListEmpty()) {
        (void)acl::AclProfilingManager::GetInstance().RemoveDeviceList(profilerConfig->devIdList,
                                                                       profilerConfig->devNums);
    }

    if ((acl::AclProfilingManager::GetInstance().IsDeviceListEmpty()) &&
        (acl::AclProfilingManager::GetInstance().AclProfilingIsRun())) {
        const aclError ret = acl::AclProfilingManager::GetInstance().UnInit();
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Uninit][ProfilingManager]stop acl failed, result = %d", ret);
            return ret;
        }
    }
    ACL_LOG_INFO("successfully execute aclprofInnerStop");
    return ACL_SUCCESS;
}

static aclError aclProcessProfData(void *data, uint32_t len)
{
    ACL_LOG_INFO("start to execute aclProcessProfData");
    std::lock_guard<std::mutex> locker(g_aclProfMutex);
    ACL_REQUIRES_NOT_NULL(data);
    size_t commandLen = sizeof(rtProfCommandHandle_t);
    if (len != commandLen) {
        ACL_LOG_INNER_ERROR("[Check][Len]len[%u] is invalid, it should be %zu", len, commandLen);
        return ACL_ERROR_INVALID_PARAM;
    }
    rtProfCommandHandle_t *profilerConfig = static_cast<rtProfCommandHandle_t *>(data);
    aclError ret = ACL_SUCCESS;
    uint64_t profSwitch = profilerConfig->profSwitch;
    uint32_t type = profilerConfig->type;
    if (((profSwitch & ACL_PROF_ACL_API) != 0U) && (type == START_PROFILING)) {
        ret = aclProfInnerStart(profilerConfig);
    }
    if (((profSwitch & ACL_PROF_ACL_API) != 0U) && (type == STOP_PROFILING)) {
        ret = aclProfInnerStop(profilerConfig);
    }

    return ret;
}

aclError aclMsprofCtrlHandle(uint32_t dataType, void* data, uint32_t dataLen)
{
    ACL_REQUIRES_NOT_NULL(data);
    if (dataType == RT_PROF_CTRL_REPORTER) {
        MsprofReporterCallback callback = reinterpret_cast<MsprofReporterCallback>(data);
        acl::AclProfilingManager::GetInstance().SetProfReporterCallback(callback);
    } else if (dataType == RT_PROF_CTRL_SWITCH) {
        aclError ret = aclProcessProfData(data, static_cast<uint32_t>(dataLen));
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Process][ProfSwitch]failed to call aclProcessProfData, result is %u", ret);
            return ret;
       }
    } else {
        ACL_LOG_WARN("get unsupported dataType %u while processing profiling data", dataType);
    }
    return ACL_SUCCESS;
}
