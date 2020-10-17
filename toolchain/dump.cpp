/**
* @file dump.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "dump.h"

#include <mutex>
#include <sstream>

#include "mmpa/mmpa_api.h"
#include "executor/ge_executor.h"
#include "adx_datadump_server.h"

#include "json_parser.h"
#include "error_codes_inner.h"

#include "acl/acl_mdl.h"
#include "common/log_inner.h"
#include "common/common_inner.h"

namespace {
    bool aclmdlInitDumpFlag = false;
    bool isCutDumpPathFlag = false;
    std::mutex aclDumpMutex;
    const std::string ACL_DUMP_MODE_INPUT = "input";
    const std::string ACL_DUMP_MODE_OUTPUT = "output";
    const std::string ACL_DUMP_MODE_ALL = "all";
    const std::string ACL_DUMP_STATUS_ON = "on";
    const std::string ACL_DUMP_OP_SWITCH_ON = "on";
    const std::string ACL_DUMP_OP_SWITCH_OFF = "off";
    const std::string ACL_DUMP_MODEL_NAME = "model_name";
    const std::string ACL_DUMP_LAYER = "layer";
    const std::string ACL_DUMP_PATH = "dump_path";
    const std::string ACL_DUMP_LIST = "dump_list";
    const std::string ACL_DUMP_MODE = "dump_mode";
    const std::string ACL_DUMP_OP_SWITCH = "dump_op_switch";
    const std::string ACL_DUMP = "dump";
    const int ADX_ERROR_NONE = 0;
    const int MAX_IPV4_ADDRESS_LENGTH = 4;
    const int MAX_DUMP_PATH_LENGTH = 512;
    const int MAX_IPV4_ADDRESS_VALUE = 255;
}

namespace acl {
    void from_json(const nlohmann::json &js, DumpInfo &info)
    {
        info.isLayer = false;
        if (js.find(ACL_DUMP_MODEL_NAME) != js.end()) {
            info.modelName = js.at(ACL_DUMP_MODEL_NAME).get<std::string>();
        }
        if (js.find(ACL_DUMP_LAYER) != js.end()) {
            info.layer = js.at(ACL_DUMP_LAYER).get<std::vector<std::string>>();
            info.isLayer = true;
        }
    }

    void from_json(const nlohmann::json &js, DumpConfig &config)
    {
        // dump_path and dump_list are required fields
        config.dumpPath = js.at(ACL_DUMP_PATH).get<std::string>();
        config.dumpList = js.at(ACL_DUMP_LIST).get<std::vector<DumpInfo>>();
        if (js.find(ACL_DUMP_MODE) != js.end()) {
            config.dumpMode = js.at(ACL_DUMP_MODE).get<std::string>();
            ACL_LOG_DEBUG("dump_mode field parse successfully, value = %s.", config.dumpMode.c_str());
        } else {
            // dump_mode is an optional field, valid values include input/output/all
            // default value is output
            config.dumpMode = ACL_DUMP_MODE_OUTPUT;
        }
        if (js.find(ACL_DUMP_OP_SWITCH) != js.end()) {
            config.dumpOpSwitch = js.at(ACL_DUMP_OP_SWITCH).get<std::string>();
            ACL_LOG_DEBUG("dump_op_switch field parse successfully, value = %s.", config.dumpOpSwitch.c_str());
        } else {
            // dump_op_switch is an optional field, valid values include on/off
            // default value is off
            config.dumpOpSwitch = ACL_DUMP_OP_SWITCH_OFF;
        }
    }

    static std::vector<std::string> Split(const std::string &str, const char *delimiter)
    {
        std::vector<std::string> resVec;
        if (str.empty()) {
            ACL_LOG_INNER_ERROR("[Check][Str]input str is null");
            return resVec;
        }

        if (delimiter == nullptr) {
            ACL_LOG_INNER_ERROR("[Check][delimiter]delimiter is nullptr");
            return resVec;
        }

        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, *delimiter)) {
            resVec.emplace_back(token);
        }
        return resVec;
    }

    AclDump &AclDump::GetInstance()
    {
        static AclDump aclDump;
        return aclDump;
    }

    static bool CheckDumplist(const nlohmann::json &js)
    {
        ACL_LOG_INFO("start to execute CheckDumpListValidity.");
        const nlohmann::json &jsDumpConfig = js.at(ACL_DUMP);
        std::vector<DumpInfo> dumpList = jsDumpConfig.at(ACL_DUMP_LIST).get<std::vector<DumpInfo>>();
        std::string dumpOpSwitch;
        if (jsDumpConfig.find(ACL_DUMP_OP_SWITCH) != jsDumpConfig.end()) {
            dumpOpSwitch = jsDumpConfig.at(ACL_DUMP_OP_SWITCH).get<std::string>();
        } else {
            dumpOpSwitch = ACL_DUMP_OP_SWITCH_OFF;
        }
        if ((dumpOpSwitch != ACL_DUMP_OP_SWITCH_ON) &&
            (dumpOpSwitch != ACL_DUMP_OP_SWITCH_OFF)) {
            ACL_LOG_ERROR("[Check][DumpOpSwitch]dump_op_switch value[%s] is invalid in config, "
                "only supports on/off", dumpOpSwitch.c_str());
            acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
                std::vector<std::string>({"param", "value", "reason"}),
                std::vector<std::string>({"dump_op_switch value", dumpOpSwitch, "only supports on/off"}));
            return false;
        }

        // if dump_list field is null and dump_op_switch is off, can't send dump config
        if ((dumpList.empty()) && (dumpOpSwitch == ACL_DUMP_OP_SWITCH_OFF)) {
            ACL_LOG_ERROR("[Check][DumpConfig]dump_list field is null and dump_op_switch is off in config, "
                "dump config is invalid");
            acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
                std::vector<std::string>({"param", "value", "reason"}),
                std::vector<std::string>({"dump config", "", "dump_list field is null and "
                "dump_op_switch is off in config"}));            
            return false;
        }

        bool isValidDumpList = false;
        // if dump_op_switch is off and dump_list is not null but all field illegal, can't send dump config
        if (dumpOpSwitch == ACL_DUMP_OP_SWITCH_OFF) {
            for (size_t i = 0; i < dumpList.size(); ++i) {
                if (dumpList[i].modelName.empty()) {
                    continue;
                }
                if (!(dumpList[i].isLayer && dumpList[i].layer.empty())) {
                    ACL_LOG_INFO("layer field is valid");
                    isValidDumpList = true;
                    break;
                }
            }

            if (!isValidDumpList) {
                ACL_LOG_INNER_ERROR("[Check][ValidDumpList]dump_list is not null, but dump_list "
                    "field invalid, dump config is invalid");
                return false;
            }

            ACL_LOG_INFO("dump_list is valid, dump_op_switch is %s, only dump model.", dumpOpSwitch.c_str());
        }
        ACL_LOG_INFO("end to check the validity of dump_list and dump_op_switch field.");
        return true;
    }

    static bool IsValidDirStr(const std::string &dumpPath)
    {
        ACL_LOG_INFO("start to execute IsValidDirStr");
        const std::string pathWhiteList = "-=[];\\,./!@#$%^&*()_+{}:?";
        size_t len = dumpPath.length();
        for (size_t i = 0; i < len; ++i) {
            if (!std::islower(dumpPath[i]) && !std::isupper(dumpPath[i]) && !std::isdigit(dumpPath[i]) &&
                pathWhiteList.find(dumpPath[i]) == std::string::npos) {
                ACL_LOG_ERROR("[Check][PathWhiteList]invalid dump_path [%s] in dump config at "
                    "location %zu", dumpPath.c_str(), i);
                std::string errMsg = acl::AclErrorLogManager::FormatStr("dump config at location %zu", i);
                acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
                    std::vector<std::string>({"param", "value", "reason"}),
                    std::vector<std::string>({"dump_path", dumpPath.c_str(), errMsg}));
                return false;
            }
        }

        ACL_LOG_INFO("successfully execute IsValidDirChar, the dump result path[%s] is valid.", dumpPath.c_str());
        return true;
    }

    static bool CheckIpAddress(DumpConfig &config)
    {
        // check the valid of ipAddress in dump_path
        ACL_LOG_INFO("start to execute IsValidIpAddress.");
        size_t colonPos = config.dumpPath.find_first_of(":");
        if (colonPos != std::string::npos) {
            ACL_LOG_INFO("dump_path field contains ip address.");
            if (colonPos + 1 == config.dumpPath.size()) {
                ACL_LOG_ERROR("[Check][colonPos]dump_path field is invalid");
                acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
                    std::vector<std::string>({"param", "value", "reason"}),
                    std::vector<std::string>({"dump_path", config.dumpPath, "format is illegal"}));
                return false;
            }

            std::string ipAddress = config.dumpPath.substr(0, colonPos);
            std::vector<std::string> ipRet = Split(ipAddress, ".");
            if (ipRet.size() == MAX_IPV4_ADDRESS_LENGTH) {
                for (auto ret : ipRet) {
                    if (atoi(ret.c_str()) < 0 || atoi(ret.c_str()) > MAX_IPV4_ADDRESS_VALUE) {
                        ACL_LOG_WARN("ip address[%s] is invalid in dump_path field", ipAddress.c_str());
                        return false;
                    }
                }

                ACL_LOG_INFO("ip address[%s] is valid in dump_path field.", ipAddress.c_str());
                return true;
            }
        }

        ACL_LOG_WARN("the dump_path field does not contains ip address or it does't comply with the ipv4 rule.");
        return false;
    }

    static bool CheckDumpPath(const nlohmann::json &js)
    {
        ACL_LOG_INFO("start to execute CheckDumpPath.");
        const nlohmann::json &jsDumpConfig = js.at(ACL_DUMP);
        DumpConfig config = jsDumpConfig;
        if (config.dumpPath.length() > MAX_DUMP_PATH_LENGTH) {
            ACL_LOG_ERROR("[Check][dumpPath]the length[%d] of dump_path is larger than "
                "MAX_DUMP_PATH_LENGTH[%d]", config.dumpPath.length(), MAX_DUMP_PATH_LENGTH);
            std::string errMsg = acl::AclErrorLogManager::FormatStr(
                "dump_path is larger than MAX_DUMP_PATH_LENGTH[%d]", MAX_DUMP_PATH_LENGTH);
            acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
                std::vector<std::string>({"param", "value", "reason"}),
                std::vector<std::string>({"dump_path",
                std::to_string(config.dumpPath.length()), errMsg}));
            return false;
        }

        size_t colonPos = config.dumpPath.find_first_of(":");
        isCutDumpPathFlag = CheckIpAddress(config);
        if (isCutDumpPathFlag) {
            config.dumpPath = config.dumpPath.substr(colonPos + 1);
            if (!IsValidDirStr(config.dumpPath)) {
                ACL_LOG_ERROR("[Check][ValidDirStr]dump_path[%s] is invalid in dump config",
                    config.dumpPath.c_str());
                acl::AclErrorLogManager::ReportInputError(acl::INVALID_PATH_MSG,
                    std::vector<std::string>({"path", "reason"}),
                    std::vector<std::string>({"dump_path",
                    "is invalid directory"}));
                return false;
            }
        } else {
            // check dump result path in dump_path field of existence and readability
            char trustedPath[MMPA_MAX_PATH] = {'\0'};
            int32_t ret = mmRealPath(config.dumpPath.c_str(), trustedPath, sizeof(trustedPath));
            if (ret != EN_OK) {
                ACL_LOG_ERROR("[Get][RealPath]the dump_path %s is not like a real path, "
                    "mmRealPath return %d", config.dumpPath.c_str(), ret);
                acl::AclErrorLogManager::ReportInputError(acl::INVALID_PATH_MSG,
                    std::vector<std::string>({"path", "reason"}),
                    std::vector<std::string>({config.dumpPath,
                    "cannot convert to realpath"}));
                return false;
            }

            if (mmAccess2(trustedPath, M_R_OK | M_W_OK) != EN_OK) {
                ACL_LOG_ERROR("[Check][Permisssion]the dump result path[%s] does't have read and "
                    "write permisssion", trustedPath);
                acl::AclErrorLogManager::ReportInputError(acl::INVALID_PATH_MSG,
                    std::vector<std::string>({"path", "reason"}),
                    std::vector<std::string>({trustedPath, "does't have read and write permisssion"}));
                return false;
            }

            ACL_LOG_INFO("the dump result path is %s", trustedPath);
        }

        ACL_LOG_INFO("successfully execute CheckDumpPath to verify dump_path field, it's valid.");
        return true;
    }

    static bool IsValidDumpConfig(const nlohmann::json &js)
    {
        ACL_LOG_INFO("start to execute IsValidDumpConfig.");
        // if dump fields not exist, don't send dump config
        if (js.find(ACL_DUMP) == js.end()) {
            ACL_LOG_WARN("no dump item, no need to do dump!");
            return false;
        }

        const nlohmann::json &jsDumpConfig = js.at(ACL_DUMP);
        // if dump_path and dump_list is not exist, can't send dump config
        if (jsDumpConfig.find(ACL_DUMP_PATH) == jsDumpConfig.end() ||
            jsDumpConfig.find(ACL_DUMP_LIST) == jsDumpConfig.end()) {
            ACL_LOG_ERROR("[Check][jsDumpConfig]dump_path or dump_list field in dump config file "
                "is not exist");
            acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
                std::vector<std::string>({"param", "value", "reason"}),
                std::vector<std::string>({"dump_path or dump_list", "",
                "field is not exist"}));
            return false;
        }

        std::string dumpPath = jsDumpConfig.at(ACL_DUMP_PATH).get<std::string>();
        if (dumpPath.empty()) {
            ACL_LOG_WARN("dump_path field is null in config");
            return false;
        }

        if (!CheckDumpPath(js)) {
            ACL_LOG_INNER_ERROR("[Check][DumpPath]dump_path field in dump config is invalid");
            return false;
        }

        std::string dumpMode;
        if (jsDumpConfig.find(ACL_DUMP_MODE) != jsDumpConfig.end()) {
            dumpMode = jsDumpConfig.at(ACL_DUMP_MODE).get<std::string>();
        } else {
            dumpMode = ACL_DUMP_MODE_OUTPUT;
        }

        if ((dumpMode != ACL_DUMP_MODE_INPUT) &&
            (dumpMode != ACL_DUMP_MODE_OUTPUT) &&
            (dumpMode != ACL_DUMP_MODE_ALL)) {
            ACL_LOG_ERROR("[Check][dumpMode]dump_mode value[%s] error in config, only supports "
                "input/output/all", dumpMode.c_str());
            acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
                std::vector<std::string>({"param", "value", "reason"}),
                std::vector<std::string>({"dump_mode", dumpMode,
                "only supports input/output/all"}));
            return false;
        }

        if (!CheckDumplist(js)) {
            ACL_LOG_INNER_ERROR("[Check][Dumplist]dump_list or dump_op_switch field is invalid");
            return false;
        }

        ACL_LOG_INFO("successfully execute IsValidDumpConfig to verify dump config, it's valid.");
        return true;
    }

    aclError ConvertDumpConfig(const nlohmann::json &js, ge::DumpConfig &dumpConfig)
    {
        ACL_LOG_INFO("start to execute ConvertDumpConfig.");
        if (!IsValidDumpConfig(js)) {
            ACL_LOG_INNER_ERROR("[Check][DumpConfig]dump config is invalid");
            return ACL_ERROR_INVALID_DUMP_CONFIG;
        }

        const nlohmann::json &jsDumpConfig = js.at(ACL_DUMP);
        DumpConfig config = jsDumpConfig;
        dumpConfig.dump_path = config.dumpPath;
        dumpConfig.dump_mode = config.dumpMode;
        dumpConfig.dump_status = ACL_DUMP_STATUS_ON;
        dumpConfig.dump_op_switch = config.dumpOpSwitch;
        ge::ModelDumpConfig modelDumpConfig;
        for (size_t i = 0; i < config.dumpList.size(); ++i) {
            if (config.dumpList[i].modelName.empty()) {
                ACL_LOG_INNER_ERROR("[Check][modelName]the %zu modelName field is null", i);
                continue;
            }
            if (config.dumpList[i].isLayer && (config.dumpList[i].layer.empty())) {
                ACL_LOG_INNER_ERROR("[Check][Layer]layer field is null in model %s",
                    config.dumpList[i].modelName.c_str());
                continue;
            }
            modelDumpConfig.model_name = config.dumpList[i].modelName;
            for (size_t index = 0; index < config.dumpList[i].layer.size(); ++index) {
                modelDumpConfig.layers.emplace_back(config.dumpList[i].layer[index]);
            }
            dumpConfig.dump_list.emplace_back(modelDumpConfig);
        }

        ACL_LOG_INFO("convert to ge dump config successfully, dump_mode = %s, dump_path = %s, dump_op_switch = %s.",
                     dumpConfig.dump_mode.c_str(),
                     dumpConfig.dump_path.c_str(),
                     dumpConfig.dump_op_switch.c_str());
        return ACL_SUCCESS;
    }

    aclError AclDump::HandleDumpCommand(ge::DumpConfig &dumpConfig)
    {
        ACL_LOG_INFO("start to execute HandleDumpCommand.");
        ge::GeExecutor geExecutor;
        nlohmann::json js;
        int adxRet = AdxDataDumpServerInit();
        if (adxRet != ADX_ERROR_NONE) {
            ACL_LOG_INNER_ERROR("[AdxDataDumpServer][Init]dump server run failed, adx result = %d", adxRet);
            return ACL_ERROR_INTERNAL_ERROR;
        }

        acl::AclDump::GetInstance().SetAclDumpFlag(true);
        ge::Status geRet = geExecutor.SetDump(dumpConfig);
        if (geRet != ge::SUCCESS) {
            ACL_LOG_CALL_ERROR("[Set][Dump]set dump config for model failed, ge result = %d", geRet);
            return ACL_GET_ERRCODE_GE(geRet);
        }
        ACL_LOG_INFO("set dump config for model successfully, dump_path = %s, dump_mode = %s, dump_op_switch = %s.",
            dumpConfig.dump_path.c_str(), dumpConfig.dump_mode.c_str(), dumpConfig.dump_op_switch.c_str());
        return ACL_SUCCESS;
    }

    aclError AclDump::HandleDumpConfig(const char *configPath)
    {
        ACL_LOG_INFO("start to execute HandleDumpConfig.");
        nlohmann::json js;
        acl::JsonParser jsonParser;
        aclError ret = jsonParser.ParseJsonFromFile(configPath, js, nullptr, nullptr);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Parse][JsonFromFile]parse dump config from file[%s] failed, result = %d",
                configPath, ret);
            return ret;
        }
        try {
            if (js.find(ACL_DUMP) != js.end()) {
                ge::DumpConfig dumpConfig;
                ret = acl::ConvertDumpConfig(js, dumpConfig);
                if (ret != ACL_SUCCESS) {
                    ACL_LOG_INNER_ERROR("[Convert][DumpConfig]convert to ge dump config file failed, "
                        "result = %d", ret);
                    return ACL_SUCCESS;
                }
                return HandleDumpCommand(dumpConfig);
            } else {
                ACL_LOG_INFO("no dump item, no need to do dump!");
            }
        } catch (const nlohmann::json::exception &e) {
            ACL_LOG_INNER_ERROR("[Convert][DumpConfig]parse json for config failed, exception:%s.",
                e.what());
            return ACL_ERROR_INVALID_DUMP_CONFIG;
        }
        ACL_LOG_INFO("HandleDumpConfig end in HandleDumpConfig.");
        return ACL_SUCCESS;
    }
} // namespace acl

aclError aclmdlInitDump()
{
    ACL_LOG_INFO("start to execute aclmdlInitDump.");
    ACL_STAGES_REG(acl::ACL_STAGE_DUMP, acl::ACL_STAGE_DEFAULT);
    if (!GetAclInitFlag()) {
        ACL_LOG_INNER_ERROR("[Check][AclInitFlag]aclmdlInitDump is not support because it does not execute aclInit");
        return ACL_ERROR_UNINITIALIZE;
    }

    if (acl::AclDump::GetInstance().GetAclDumpFlag()) {
        ACL_LOG_INNER_ERROR("[Check][AclDumpFlag]aclmdlInitDump is not support because already execute aclInit init dump");
        return ACL_ERROR_DUMP_ALREADY_RUN;
    }

    std::unique_lock<std::mutex> lk(aclDumpMutex);
    if (aclmdlInitDumpFlag) {
        ACL_LOG_INNER_ERROR("[Check][InitDumpFlag]repeatedly initialized dump in aclmdlInitDump, "
            "only initialized once");
        return ACL_ERROR_REPEAT_INITIALIZE;
    }

    int adxRet = AdxDataDumpServerInit();
    if (adxRet != ADX_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[AdxDataDumpServer][Init]dump server run failed, adx result = %d", adxRet);
        return ACL_ERROR_INTERNAL_ERROR;
    }

    aclmdlInitDumpFlag = true;
    ACL_LOG_INFO("successfully initialized dump in aclmdlInitDump.");
    return ACL_SUCCESS;
}

aclError aclmdlSetDump(const char *dumpCfgPath)
{
    ACL_LOG_INFO("start to execute aclmdlSetDump.");
    ACL_STAGES_REG(acl::ACL_STAGE_DUMP, acl::ACL_STAGE_DEFAULT);
    if (!GetAclInitFlag()) {
        ACL_LOG_INNER_ERROR("[Check][AclInitFlag]aclmdlSetDump is not support because it does not execute aclInit");
        return ACL_ERROR_UNINITIALIZE;
    }

    if (acl::AclDump::GetInstance().GetAclDumpFlag()) {
        ACL_LOG_INNER_ERROR("[Check][AclDumpFlag]aclmdlSetDump is not support because already execute aclInit");
        return ACL_ERROR_DUMP_ALREADY_RUN;
    }

    std::unique_lock<std::mutex> lk(aclDumpMutex);
    if (!aclmdlInitDumpFlag) {
        ACL_LOG_INNER_ERROR("[Check][aclmdlInitDumpFlag]dump is not initialized in aclmdlInitDump");
        return ACL_ERROR_DUMP_NOT_RUN;
    }

    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dumpCfgPath);
    ge::GeExecutor geExecutor;
    nlohmann::json js;
    ge::DumpConfig dumpConfig;
    acl::JsonParser jsonParser;
    aclError ret = jsonParser.ParseJsonFromFile(dumpCfgPath, js, nullptr, nullptr);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Parse][JsonFromFile]parse dump config from file[%s] failed, result = %d",
            dumpCfgPath, ret);
        return ret;
    }

    try {
        ret = acl::ConvertDumpConfig(js, dumpConfig);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Convert][DumpConfig]convert to ge dump config file failed, result = %d", ret);
            return ACL_ERROR_INVALID_DUMP_CONFIG;
        }
    } catch (const nlohmann::json::exception &e) {
        ACL_LOG_INNER_ERROR("[Convert][DumpConfig]invalid dump config, exception:%s", e.what());
        return ACL_ERROR_INVALID_DUMP_CONFIG;
    }

    ge::Status geRet = geExecutor.SetDump(dumpConfig);
    if (geRet != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Set][DumpConfig]set dump config for model failed, ge result = %d", geRet);
        return ACL_GET_ERRCODE_GE(geRet);
    }

    ACL_LOG_INFO("set dump config for model successfully.");
    return ACL_SUCCESS;
}

aclError aclmdlFinalizeDump()
{
    ACL_LOG_INFO("start to execute aclmdlFinalizeDump.");
    ACL_STAGES_REG(acl::ACL_STAGE_DUMP, acl::ACL_STAGE_DEFAULT);
    if (!GetAclInitFlag()) {
        ACL_LOG_INNER_ERROR("[Check][AclInitFlag]aclmdlFinalizeDump is not support because it does not execute aclInit");
        return ACL_ERROR_UNINITIALIZE;
    }

    if (acl::AclDump::GetInstance().GetAclDumpFlag()) {
        ACL_LOG_INNER_ERROR("[Check][AclDumpFlag]aclmdlFinalizeDump is not support because already execute aclInit");
        return ACL_ERROR_DUMP_ALREADY_RUN;
    }

    std::unique_lock<std::mutex> lk(aclDumpMutex);
    if (aclmdlInitDumpFlag) {
        int adxRet = AdxDataDumpServerUnInit();
        if (adxRet != ADX_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[AdxDataDumpServer][UnInit]generate dump file failed in disk, adx result = %d", adxRet);
            return ACL_ERROR_INTERNAL_ERROR;
        }
    } else {
        ACL_LOG_INNER_ERROR("[Check][aclmdlInitDumpFlag]dump is not initialized in aclmdlInitDump");
        return ACL_ERROR_DUMP_NOT_RUN;
    }

    aclmdlInitDumpFlag = false;
    ACL_LOG_INFO("successfully execute aclmdlFinalizeDump, the dump task completed!");
    return ACL_SUCCESS;
}