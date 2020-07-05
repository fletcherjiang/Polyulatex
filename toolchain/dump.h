/**
* @file dump.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_DUMP_H_
#define ACL_DUMP_H_

#include "executor/ge_executor.h"

#include "nlohmann/json.hpp"
#include "acl/acl_base.h"
#include "common_inner.h"

namespace acl {
    struct DumpInfo {
        std::string modelName;
        std::vector <std::string> layer;
        bool isLayer = false; // Whether the label of "layer" exists
    };

    struct DumpConfig {
        std::string dumpPath;
        std::string dumpMode;
        std::vector <DumpInfo> dumpList;
        std::string dumpStatus;
        std::string dumpOpSwitch;
    };

    class AclDump {
    public:
        static aclError HandleDumpConfig(const char *configPath);
        static AclDump &GetInstance();
        void SetAclDumpFlag(bool flag) { aclDumpFlag_ = flag; }
        bool GetAclDumpFlag() const { return aclDumpFlag_; }

    private:
        static aclError HandleDumpCommand(ge::DumpConfig &dumpConfig);
        bool aclDumpFlag_ = false;
        ~AclDump() = default;
        AclDump() = default;
    };
}

#endif // ACL_DUMP_H_