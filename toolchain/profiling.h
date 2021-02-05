/**
* @file profiling.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_PROFILING_H_
#define ACL_PROFILING_H_

#include "acl/acl_base.h"
#include "toolchain/prof_callback.h"
#include <string>

namespace acl {
    class AclProfiling {
    public:
        static aclError HandleProfilingConfig(const char *configPath);

    private:
        static aclError HandleProfilingCommand(const std::string &config, bool configFileFlag, bool validConfig);

        static bool GetProfilingConfigFile(std::string &fileName);
    };
}

aclError aclMsprofCtrlHandle(uint32_t dataType, void* data, uint32_t dataLen);

#endif // ACL_PROFILING_H_

