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

using namespace std;

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
    std::lock_guard<std::mutex> lock(mutex_);
    if (reporterCallback_ == nullptr) {
        ACL_LOG_ERROR("[Check][ReporterCallback]prof reporter hasn't registered yet");
        return ACL_ERROR_PROFILING_FAILURE;
    }

    int32_t result = reporterCallback_(MSPROF_MODULE_ACL, MSPROF_REPORTER_INIT, nullptr, 0);
    if (result != 0) {
        ACL_LOG_ERROR("[Init][ProfEngine]init acl profiling engine failed, result = %d", result);
        return ACL_ERROR_PROFILING_FAILURE;
    }
    isProfiling_ = true;
    return ACL_SUCCESS;
}

aclError AclProfilingManager::UnInit()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (reporterCallback_ == nullptr) {
        ACL_LOG_ERROR("[Check][ReporterCallback]prof reporter hasn't registered yet");
        return ACL_ERROR_PROFILING_FAILURE;
    }

    int32_t result = reporterCallback_(MSPROF_MODULE_ACL, MSPROF_REPORTER_UNINIT, nullptr, 0);
    if (result != MSPROF_ERROR_NONE) {
        ACL_LOG_ERROR("[Uninit][ProfEngine]Uninit profiling engine failed, result = %d", result);
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
    std::lock_guard<std::mutex> lock(mutex_);
    if (reporterCallback_ == nullptr) {
        return ACL_ERROR_PROFILING_FAILURE;
    } else {
        if (reporterCallback_(MSPROF_MODULE_ACL, MSPROF_REPORTER_REPORT, &data, sizeof(ReporterData)) != 0) {
            return ACL_ERROR_PROFILING_FAILURE;
        }
    }
    return ACL_SUCCESS;
}

aclError AclProfilingManager::AddDeviceList(const uint32_t *deviceIdList, uint32_t deviceNums)
{
    if (deviceNums == 0) {
        return ACL_SUCCESS;
    }
    ACL_REQUIRES_NOT_NULL(deviceIdList);
    for (size_t devId = 0; devId < deviceNums; devId++) {
        if (!deviceList_.count(deviceIdList[devId])) {
            deviceList_.insert(deviceIdList[devId]);
            ACL_LOG_INFO("device id %u is successfully added in acl profiling", deviceIdList[devId]);
        }
    }
    return ACL_SUCCESS;
}

aclError AclProfilingManager::RemoveDeviceList(const uint32_t *deviceIdList, uint32_t deviceNums)
{
    if (deviceNums == 0) {
        return ACL_SUCCESS;
    }
    ACL_REQUIRES_NOT_NULL(deviceIdList);
    for (size_t devId = 0; devId < deviceNums; devId++) {
        auto iter = deviceList_.find(deviceIdList[devId]);
        if (iter != deviceList_.end()) {
            deviceList_.erase(iter);
        }
    }
    return ACL_SUCCESS;
}

void AclProfilingManager::RemoveAllDeviceList()
{
    deviceList_.clear();
}

bool AclProfilingManager::IsDeviceEnable(const uint32_t &deviceId)
{
    if (deviceList_.count(deviceId)) {
        return true;
    }
    return false;
}

AclProfilingReporter::AclProfilingReporter(const char *funcName, ProfFuncType funcType) : funcName_(funcName)
{
    if (AclProfilingManager::GetInstance().AclProfilingIsRun()) {
        if (aclrtGetDevice(&deviceId_) != ACL_SUCCESS) {
            ACL_LOG_WARN("Getting device id fail in AclProfiling!");
            deviceId_ = -1;
        }
        switch (funcType) {
            case ACL_PROF_FUNC_OP:
                funcTag_ = "acl_op";
                funcType_ = "op";
                break;

            case ACL_PROF_FUNC_MODEL:
                funcTag_ = "acl_model";
                funcType_ = "model";
                break;

            case ACL_PROF_FUNC_RUNTIME:
                funcTag_ = "acl_rts";
                funcType_ = "runtime";
                break;

            default:
                funcTag_ = "acl_others";
                funcType_ = "others";
                break;
        }
        mmTimespec timespec = mmGetTickCount();
        // 1000 ^ 3 converts second to nanosecond
        startTime_ = timespec.tv_sec * 1000 * 1000 * 1000 + timespec.tv_nsec;
    }
}

AclProfilingReporter::~AclProfilingReporter()
{
    if (AclProfilingManager::GetInstance().AclProfilingIsRun()) {
        mmTimespec timespec = mmGetTickCount();
        // 1000 ^ 3 converts second to nanosecond
        int64_t endTime = timespec.tv_sec * 1000 * 1000 * 1000 + timespec.tv_nsec;
        if ((funcTag_ == nullptr) || (funcType_ == nullptr) || (funcName_ == nullptr)) {
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
        reporter_data.deviceId = static_cast<int>(deviceId_);
        string tag_name = funcTag_;
        errno_t err = EOK;
        err = memcpy_s(reporter_data.tag, MSPROF_ENGINE_MAX_TAG_LEN,
            tag_name.c_str(), tag_name.size());
        if (err != EOK) {
            ACL_LOG_ERROR("[Copy][Mem]memcpy_s failed, err = %d", err);
            return;
        }

        mmPid_t pid = static_cast<mmPid_t>(mmGetPid());
        string data = funcName_;
        int32_t tid = mmGetTid();
        data = data.append(" ").append(string(funcType_))
            .append(" ").append(std::to_string(startTime_))
            .append(" ").append(std::to_string(endTime))
            .append(" ").append(std::to_string(pid))
            .append(" ").append(std::to_string(tid))
            .append("\n");

        reporter_data.data = (unsigned char *)data.c_str();
        reporter_data.dataLen = data.size();
        ACL_LOG_DEBUG("AclProfiling reporter reports in %s, device id = %d, data = %s",
            funcName_, deviceId_, data.c_str());
        aclError ret = AclProfilingManager::GetInstance().ProfilingData(reporter_data);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_DEBUG("AclProfiling reporter reports failed, result = %d", ret);
        }
    }
}
}  // namespace acl