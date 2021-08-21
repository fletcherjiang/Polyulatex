/**
* @file profiling_manager.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "profiling_manager.h"

#include <thread>
#include <cstring>
#include <sstream>
#include <securec.h>

#include "framework/common/ge_format_util.h"
#include "mmpa/mmpa_api.h"

#include "acl/acl_rt.h"
#include "common/log_inner.h"

namespace {
    const std::string ACL_PROFILING_MODULE = "AclModule";
}

namespace acl {
AclProfilingManager::AclProfilingManager() {}

AclProfilingManager::~AclProfilingManager() {}

AclProfilingManager &AclProfilingManager::GetInstance()
{
    static AclProfilingManager profilingManager;
    return profilingManager;
}

aclError AclProfilingManager::Init()
{
    const std::lock_guard<std::mutex> lock(mutex_);
    if (reporterCallback_ == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][ReporterCallback]prof reporter hasn't registered yet");
        return ACL_ERROR_PROFILING_FAILURE;
    }

    const int32_t result = reporterCallback_(static_cast<uint32_t>(MSPROF_MODULE_ACL), 
        static_cast<uint32_t>(MSPROF_REPORTER_INIT), nullptr, 0U);
    if (result != 0) {
        ACL_LOG_INNER_ERROR("[Init][ProfEngine]init acl profiling engine failed, result = %d", result);
        return ACL_ERROR_PROFILING_FAILURE;
    }
    isProfiling_ = true;
    return ACL_SUCCESS;
}

aclError AclProfilingManager::UnInit()
{
    const std::lock_guard<std::mutex> lock(mutex_);
    if (reporterCallback_ == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][ReporterCallback]prof reporter hasn't registered yet");
        return ACL_ERROR_PROFILING_FAILURE;
    }

    const int32_t result = reporterCallback_(static_cast<uint32_t>(MSPROF_MODULE_ACL),
        static_cast<uint32_t>(MSPROF_REPORTER_UNINIT), nullptr, 0U);
    if (result != MSPROF_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[Uninit][ProfEngine]Uninit profiling engine failed, result = %d", result);
        return ACL_ERROR_PROFILING_FAILURE;
    }
    isProfiling_ = false;
    return ACL_SUCCESS;
}

bool AclProfilingManager::IsDeviceListEmpty() const
{
    return deviceList_.empty();
}

aclError AclProfilingManager::ProfilingData(ReporterData &data)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    if (reporterCallback_ == nullptr) {
        return ACL_ERROR_PROFILING_FAILURE;
    } else {
        if (reporterCallback_(static_cast<uint32_t>(MSPROF_MODULE_ACL), static_cast<uint32_t>(MSPROF_REPORTER_REPORT),
            &data, sizeof(ReporterData)) != 0) {
            return ACL_ERROR_PROFILING_FAILURE;
        }
    }
    return ACL_SUCCESS;
}

aclError AclProfilingManager::AddDeviceList(const uint32_t *const deviceIdList, uint32_t deviceNums)
{
    if (deviceNums == 0U) {
        return ACL_SUCCESS;
    }
    ACL_REQUIRES_NOT_NULL(deviceIdList);
    for (size_t devId = 0U; devId < deviceNums; devId++) {
        if (deviceList_.count(*(deviceIdList + devId)) == 0U) {
            (void)deviceList_.insert(*(deviceIdList + devId));
            ACL_LOG_INFO("device id %u is successfully added in acl profiling", *(deviceIdList + devId));
        }
    }
    return ACL_SUCCESS;
}

aclError AclProfilingManager::RemoveDeviceList(const uint32_t *const deviceIdList, uint32_t deviceNums)
{
    if (deviceNums == 0U) {
        return ACL_SUCCESS;
    }
    ACL_REQUIRES_NOT_NULL(deviceIdList);
    for (size_t devId = 0U; devId < deviceNums; devId++) {
        const auto iter = deviceList_.find(*(deviceIdList + devId));
        if (iter != deviceList_.end()) {
            (void)deviceList_.erase(iter);
        }
    }
    return ACL_SUCCESS;
}

bool AclProfilingManager::IsDeviceEnable(const uint32_t &deviceId) const
{
    return deviceList_.count(deviceId) > 0U;
}

aclError AclProfilingManager::QueryHashValue(const char *const funcName, int32_t deviceId, uint64_t &hashId)
{
    const std::string apiName(funcName);
    const std::lock_guard<std::mutex> lock(mutex_);
    const auto iter = AclProfilingManager::GetInstance().HashMap.find(apiName);
    if (iter != AclProfilingManager::GetInstance().HashMap.end()) {
        hashId = iter->second;
    } else {
        if (reporterCallback_ == nullptr) {
            ACL_LOG_INNER_ERROR("[Check][Param]reporter callback is nullptr");
            return ACL_ERROR_PROFILING_FAILURE;
        }
        MsprofHashData hashData;
        hashData.deviceId = deviceId;
        hashData.dataLen = apiName.size();
        hashData.data = reinterpret_cast<unsigned char *>(const_cast<char *>(funcName));
        const int32_t ret = reporterCallback_(MSPROF_MODULE_ACL, MSPROF_REPORTER_HASH, &hashData, sizeof(MsprofHashData));
        if ( ret != 0) {
            ACL_LOG_CALL_ERROR("[Get][HashId]call reporter failed, type is MSPROF_REPORTER_HASH, result = %d", ret);
            return ACL_ERROR_PROFILING_FAILURE;
        }
        AclProfilingManager::GetInstance().HashMap.insert({apiName, hashData.hashId});
        hashId = hashData.hashId;
    }
    return ACL_SUCCESS;
}

AclProfilingReporter::AclProfilingReporter(const char *const funName, MsprofAclApiType funType) :
    funcName_(funName), funcType_(funType) 
{
    if (AclProfilingManager::GetInstance().AclProfilingIsRun()) {
        if (aclrtGetDevice(&deviceId_) != ACL_SUCCESS) {
            ACL_LOG_WARN("Getting device id fail in AclProfiling!");
            deviceId_ = -1;
        }
        switch (funType) {
            case ACL_PROF_FUNC_OP:
                funcTag_ = "acl_op";
                break;

            case ACL_PROF_FUNC_MODEL:
                funcTag_ = "acl_model";
                break;

            case ACL_PROF_FUNC_RUNTIME:
                funcTag_ = "acl_rts";
                break;

            default:
                funcTag_ = "acl_others";
                break;
        }
        const mmTimespec timeSpec = mmGetTickCount();
        // 1000 ^ 3 converts second to nanosecond
        startTime_ = (timeSpec.tv_sec * 1000 * 1000 * 1000) + timeSpec.tv_nsec;
    }
}

AclProfilingReporter::~AclProfilingReporter()
{
    if (AclProfilingManager::GetInstance().AclProfilingIsRun()) {
        const mmTimespec timeSpec = mmGetTickCount();
        // 1000 ^ 3 converts second to nanosecond
        const int64_t endTime = (timeSpec.tv_sec * 1000 * 1000 * 1000) + timeSpec.tv_nsec;
        if ((funcTag_ == nullptr) || (funcName_ == nullptr)) {
            ACL_LOG_WARN("function context is nullptr!");
            return;
        }
        // -1 represents invalid device id
        if (deviceId_ == -1) {
            if (aclrtGetDevice(&deviceId_) != ACL_SUCCESS) {
                ACL_LOG_WARN("Getting device id fail in AclProfiling!");
                return;
            }
        }

        if (!AclProfilingManager::GetInstance().IsDeviceEnable(static_cast<uint32_t>(deviceId_))) {
            ACL_LOG_WARN("device id %d is not enable in Aclprofiling!", deviceId_);
            return;
        }

        ReporterData reporter_data{};
        reporter_data.deviceId = deviceId_;
        const std::string tag_name = funcTag_;
        errno_t err = EOK;
        err = memcpy_s(reporter_data.tag, MSPROF_ENGINE_MAX_TAG_LEN,
            tag_name.c_str(), tag_name.size());
        if (err != EOK) {
            ACL_LOG_INNER_ERROR("[Copy][Mem]memcpy_s failed, err = %d", err);
            return;
        }

        uint64_t hashId = 0U;
        aclError ret = AclProfilingManager::GetInstance().QueryHashValue(funcName_, deviceId_, hashId);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Query][HashValue]Failed to query hash value, error code is %u", ret);
            return;
        }

        const mmPid_t pid = static_cast<mmPid_t>(mmGetPid());
        const int32_t tid = mmGetTid();
        //magic number is "5A5A" and tag is 0 for acl
        MsprofAclProfData profData{MSPROF_DATA_HEAD_MAGIC_NUM, 0U};
        profData.apiType = static_cast<uint32_t>(funcType_);
        profData.apiHashValue = hashId;
        profData.beginTime = static_cast<uint64_t>(startTime_);
        profData.endTime = endTime;
        profData.processId = static_cast<uint32_t>(pid);
        profData.threadId = static_cast<uint32_t>(tid);

        reporter_data.data = reinterpret_cast<unsigned char *>(&profData);
        reporter_data.dataLen = sizeof(MsprofAclProfData);
        ACL_LOG_DEBUG("AclProfiling reporter reports in %s, device id = %d", funcName_, deviceId_);
        ret = AclProfilingManager::GetInstance().ProfilingData(reporter_data);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_DEBUG("AclProfiling reporter reports failed, result = %d", ret);
        }
    }
}
}  // namespace acl
