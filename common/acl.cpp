/**
* @file acl.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/acl.h"

#include <memory>
#include <mutex>

#include "framework/executor/ge_executor.h"
#include "runtime/dev.h"
#include "adx_datadump_server.h"
#include "common/util/error_manager/error_manager.h"

#include "log_inner.h"
#include "toolchain/plog.h"
#include "common_inner.h"
#include "single_op/op_model_manager.h"
#include "dump.h"
#include "profiling.h"
#include "error_codes_inner.h"
#include "toolchain/profiling_manager.h"
#include "toolchain/resource_statistics.h"
#include "graph/ge_local_context.h"

namespace {
    bool aclInitFlag = false;
    bool aclFinalizeFlag = false;
    std::mutex aclInitMutex;
    std::mutex aclSocVersionMutex;
    std::string aclSocVersion;
    const int ADX_ERROR_NONE = 0;
    GeFinalizeCallback aclGeFinalizeCallback = nullptr;
    bool isCastHasTruncateAttr = false;
}

aclError aclInit(const char *configPath)
{
    ACL_STAGES_REG(acl::ACL_STAGE_INIT, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclInit");
    std::unique_lock<std::mutex> lk(aclInitMutex);
    if (aclInitFlag) {
        ACL_LOG_WARN("repeatedly initialized");
        return ACL_ERROR_REPEAT_INITIALIZE;
    }
    ACL_LOG_INFO("call ErrorManager.Initialize");
    ge::Status geRet = static_cast<ge::Status>(ErrorManager::GetInstance().Init());
    if (geRet != ge::SUCCESS) {
        ACL_LOG_WARN("init ge error manager failed, ge result = %u", geRet);
    }
    if (DlogReportInitialize() != 0) {
        ACL_LOG_WARN("init device's log failed");
    }

    aclError ret = ACL_SUCCESS;
    if ((configPath != nullptr) && (strlen(configPath) != 0)) {
        // config dump
        ACL_LOG_INFO("set DumpConfig in aclInit");
        ret = acl::AclDump::GetInstance().HandleDumpConfig(configPath);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Process][DumpConfig]process HandleDumpConfig failed");
            return ret;
        }
        ACL_LOG_INFO("set HandleDumpConfig success in aclInit");
        // config max_opqueue_num
        ACL_LOG_INFO("set max_opqueue_num in aclInit");
        ret = acl::OpModelManager::GetInstance().HandleMaxOpQueueConfig(configPath);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Process][QueueConfig]process HandleMaxOpQueueConfig failed");
            return ret;
        }
        ACL_LOG_INFO("set HandleMaxOpQueueConfig success in aclInit");
    }
    // config profiling
    ACL_LOG_INFO("set ProfilingConfig in aclInit");
    acl::AclProfiling aclProf;
    ret = aclProf.HandleProfilingConfig(configPath);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Process][ProfConfig]process HandleProfilingConfig failed");
        return ret;
    }

    // init GeExecutor
    ge::GeExecutor executor;
    ACL_LOG_INFO("call ge interface executor.Initialize");
    geRet = executor.Initialize();
    if (geRet != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Init][Geexecutor]init ge executor failed, ge result = %u", geRet);
        return ACL_GET_ERRCODE_GE(static_cast<int32_t>(geRet));
    }
    // get socVersion
    ret = InitSocVersion();
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Init][Version]init soc version failed, ret = %d", ret);
        return ret;
    }
    aclInitFlag = true;
    ACL_LOG_INFO("successfully execute aclInit");
    return ACL_SUCCESS;
}

aclError aclFinalize()
{
    ACL_STAGES_REG(acl::ACL_STAGE_FINAL, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclFinalize");
    std::unique_lock<std::mutex> lk(aclInitMutex);
    if (aclFinalizeFlag) {
        ACL_LOG_INNER_ERROR("[Finalize][Acl]repeatedly finalized");
        return ACL_ERROR_REPEAT_FINALIZE;
    }
    if (DlogReportFinalize() != 0) {
        ACL_LOG_WARN("finalize device's log failed");
    }
    acl::ResourceStatistics::GetInstance().TraverseStatistics();
    MsprofCtrlCallback callback = acl::AclProfilingManager::GetInstance().GetProfCtrlCallback();
    if (callback != nullptr) {
        int32_t profRet = callback(static_cast<uint32_t>(MSPROF_CTRL_FINALIZE), nullptr, 0);
        if (profRet != MSPROF_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Handle][Json]handle json config of profiling failed, prof result = %d",
                static_cast<int32_t>(profRet));
        }
    }

    if (aclGeFinalizeCallback != nullptr) {
        auto ret = aclGeFinalizeCallback();
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Finalize][Ge]ge finalize failed, result = %d", static_cast<int32_t>(ret));
        }
    }

    // Finalize GeExecutor
    ge::GeExecutor executor;
    ge::Status geRet = executor.Finalize();
    if (geRet != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Finalize][Ge]finalize ge executor failed, ge result = %u", geRet);
        return ACL_GET_ERRCODE_GE(static_cast<int32_t>(geRet));
    }

    if (acl::AclDump::GetInstance().GetAclDumpFlag()) {
        int adxRet = AdxDataDumpServerUnInit();
        if (adxRet != ADX_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Generate][DumpFile]generate dump file failed in disk, adx result = %d", adxRet);
            return ACL_ERROR_INTERNAL_ERROR;
        }
    }

    aclFinalizeFlag = true;
    ACL_LOG_INFO("successfully execute aclFinalize");
    return ACL_SUCCESS;
}

aclError InitSocVersion()
{
    std::unique_lock<std::mutex> lk(aclSocVersionMutex);
    if (aclSocVersion.empty()) {
        // get socVersion
        char socVersion[SOC_VERSION_LEN] = { '\0' };
        auto rtErr = rtGetSocVersion(socVersion, sizeof(socVersion));
        if (rtErr != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Get][SocVersion]get soc version failed, runtime result is %d",
                static_cast<int32_t>(rtErr));
            return ACL_GET_ERRCODE_RTS(rtErr);
        }
        aclSocVersion = std::string(socVersion);
    }
    ACL_LOG_INFO("get SocVersion success, SocVersion = %s", aclSocVersion.c_str());
    return ACL_SUCCESS;
}

std::string GetSocVersion()
{
    ACL_LOG_INFO("socVersion is %s", aclSocVersion.c_str());
    return aclSocVersion;
}

bool GetAclInitFlag()
{
    return aclInitFlag;
}

aclError aclrtGetVersion(int32_t *majorVersion, int32_t *minorVersion, int32_t *patchVersion)
{
    ACL_LOG_INFO("start to execute aclrtGetVersion.");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(majorVersion);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(minorVersion);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(patchVersion);

    // Acl version is (*majorVersion).(*minorVersion).(*patchVersion)
    *majorVersion = ACL_MAJOR_VERSION;
    *minorVersion = ACL_MINOR_VERSION;
    *patchVersion = ACL_PATCH_VERSION;
    ACL_LOG_INFO("acl version is %d.%d.%d", *majorVersion, *minorVersion, *patchVersion);

    return ACL_SUCCESS;
}

void SetGeFinalizeCallback(GeFinalizeCallback callback)
{
    aclGeFinalizeCallback = callback;
}

void SetThreadCompileOpts(const std::map<std::string, std::string> &options)
{
    ge::GetThreadLocalContext().SetGlobalOption(options);
    return;
}

void SetCastHasTruncateAttr(bool hasTruncate)
{
    isCastHasTruncateAttr = hasTruncate;
}

bool GetIfCastHasTruncateAttr()
{
    return isCastHasTruncateAttr;
}

const char *aclrtGetSocName()
{
    ACL_LOG_INFO("start to execute aclrtGetSocName.");
    // get socVersion
    auto ret = InitSocVersion();
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("init soc version failed, ret = %d", ret);
        return nullptr;
    }
    ACL_LOG_INFO("execute aclrtGetSocName successfully");
    return aclSocVersion.c_str();
}

const char *aclGetRecentErrMsg()
{
    ACL_LOG_INFO("start to execute aclGetRecentErrMsg.");
    thread_local static std::string aclRecentErrMsg;
    aclRecentErrMsg = ErrorManager::GetInstance().GetErrorMessage();
    if (aclRecentErrMsg.empty()) {
        ACL_LOG_INNER_ERROR("get error message failed");
        return nullptr;
    }
    ACL_LOG_INFO("execute aclGetRecentErrMsg successfully.");
    return aclRecentErrMsg.c_str();
}