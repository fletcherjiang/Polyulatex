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
    const uint32_t MAX_ENV_VALVE_LENGTH = 4096;
}

namespace acl {
    aclError AclProfiling::HandleProfilingCommand(const std::string &config, bool configFileFlag)
    {
        ACL_LOG_INFO("start to execute HandleProfilingCommand");
        MsprofCtrlCallback callback = AclProfilingManager::GetInstance().GetProfCtrlCallback();
        ACL_REQUIRES_NOT_NULL(callback);
        if (configFileFlag) {
            int32_t ret = callback(MSPROF_CTRL_INIT_ACL_JSON, const_cast<char *>(config.c_str()), config.size());
            if (ret != 0) {
                ACL_LOG_ERROR("[Handle][Json]handle json config of profiling failed, profiling result = %d", ret);
                return ACL_ERROR_INVALID_PARAM;
            }
        } else {
            int32_t ret = callback(MSPROF_CTRL_INIT_ACL_ENV, const_cast<char *>(config.c_str()), config.size());
            if (ret != 0) {
                ACL_LOG_ERROR("[Handle][Env]handle env config of profiling failed, profiling result = %d", ret);
                return ACL_ERROR_INVALID_PARAM;
            }
        }
        ACL_LOG_INFO("set profiling config successfully");
        return ACL_SUCCESS;
    }

    bool AclProfiling::GetProfilingConfigFile(std::string &envValue)
    {
        char environment[MAX_ENV_VALVE_LENGTH] = {0};
        int32_t ret = mmGetEnv("PROFILER_SAMPLECONFIG", environment, sizeof(environment));
        if (ret == EN_OK) {
            ACL_LOG_INFO("get profiling config envValue[%s]", environment);
            envValue = environment;
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
        aclError ret = ACL_SUCCESS;
        std::string envValue;
        // use profiling config from env variable if exist
        if ((GetProfilingConfigFile(envValue)) && (!envValue.empty())) {
            configFileFlag = false;
            ACL_LOG_INFO("start to use profiling config by env mode");
        }

        // use profiling config from acl.json if exist
        if (configFileFlag == true) {
            if (configPath == nullptr || strlen(configPath) == 0) {
                ACL_LOG_INFO("configPath is null, no need to do profiling.");
                return ACL_SUCCESS;
            }
            ret = jsonParser.ParseJsonFromFile(configPath, js, &strConfig, ACL_PROF_CONFIG_NAME.c_str());
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INFO("parse profiling config from file[%s] failed, result = %d", configPath, ret);
                return ACL_SUCCESS;
            }

            if (strConfig.empty()) {
                ACL_LOG_INFO("profiling config file[%s] is empty", configPath);
                return ACL_SUCCESS;
            }

            ACL_LOG_INFO("start to use profiling config by config file mode");
        }

        ACL_LOG_INFO("ParseJsonFromFile ok in HandleProfilingConfig");
        ret = HandleProfilingCommand(strConfig, configFileFlag);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("[Handle][Command]handle profiling command failed, result = %d", ret);
            return ret;
        }

        ACL_LOG_INFO("set HandleProfilingConfig success");
        return ret;
    }
}
