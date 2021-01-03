/**
* @file profiling_manager.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_PROFILING_MANAGER_H_
#define ACL_PROFILING_MANAGER_H_

#include "profiling.h"
#include <mutex>
#include <unordered_set>
#include <map>

#define PROF_RESERVE_BYTES 24

enum ProfFuncType {
    ACL_PROF_FUNC_OP = 1,
    ACL_PROF_FUNC_MODEL,
    ACL_PROF_FUNC_RUNTIME,
    ACL_PROF_FUNC_OTHERS,
};

namespace acl {
    class AclProfilingManager final {
    public:
        static AclProfilingManager &GetInstance();
        // init acl prof module
        aclError Init();
        // uninit acl prof module
        aclError UnInit();
        // acl profiling module is running or not
        bool AclProfilingIsRun() const { return isProfiling_; }

        // return flag of device list that needs report prof data is empty or not
        bool IsDeviceListEmpty() const;
        aclError ProfilingData(ReporterData &data);
        aclError AddDeviceList(const uint32_t *deviceIdList, uint32_t deviceNums);
        aclError RemoveDeviceList(const uint32_t *deviceIdList, uint32_t deviceNums);
        void RemoveAllDeviceList();
        bool IsDeviceEnable(const uint32_t &deviceId);
        void SetProfCtrlCallback(MsprofCtrlCallback callback) { ctrlCallback_ = callback; };
        MsprofCtrlCallback GetProfCtrlCallback() { return ctrlCallback_; };
        void SetProfReporterCallback(MsprofReporterCallback callback) { reporterCallback_ = callback; };
        MsprofReporterCallback GetProfReporterCallback() { return reporterCallback_; };
        aclError QueryHashValue(const char *funcName, int &deviceId, uint64_t &hashId);

    private:
        ~AclProfilingManager();
        AclProfilingManager();
        bool isProfiling_ = false;
        std::mutex mutex_;
        std::unordered_set<uint32_t> deviceList_;
        MsprofCtrlCallback ctrlCallback_ = nullptr;
        MsprofReporterCallback reporterCallback_ = nullptr;
        std::map<std::string, uint64_t> HashMap = {};
    };

    class ACL_FUNC_VISIBILITY AclProfilingReporter {
    public:
        AclProfilingReporter(const char *funcName, ProfFuncType funcType);
        virtual ~AclProfilingReporter();
    private:
        int64_t startTime_ = 0;
        int32_t deviceId_ = -1;
        ProfFuncType funcType_;
        const char* funcTag_ = nullptr;
        const char* funcName_;
    };

    struct ProfData {
        char magicNumber[2];
        uint16_t dataTag;
        uint32_t apiType;
        uint64_t apiHashValue;
        uint64_t beginTime;
        uint64_t endTime;
        uint32_t processId;
        uint32_t threadId;
        char reserve[PROF_RESERVE_BYTES];
    };
} // namespace acl

#define ACL_PROFILING_REG(funcType) \
    const acl::AclProfilingReporter g_##funcType##_profiling_reporter(__FUNCTION__, funcType)

#endif // ACL_PROFILING_MANAGER_H_