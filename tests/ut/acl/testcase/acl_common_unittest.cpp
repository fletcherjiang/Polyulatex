/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "toolchain/slog.h"
#include "toolchain/plog.h"
#include "framework/executor/ge_executor.h"

#include <gtest/gtest.h>

#include "acl/acl.h"
#include "log_inner.h"
#include "common/common_inner.h"
#include "toolchain/profiling.h"
#include "toolchain/profiling_manager.h"

namespace acl 
{
    class UTEST_ACL_Common : public testing::Test {
    protected:
      void SetUp(){}
      void TearDown(){}
    };

    static int32_t ctrl_callback(uint32_t type, void *data, uint32_t len)
    {
        return 0;
    }

    static int32_t reporter_callback(uint32_t moduleId, uint32_t type, void *data, uint32_t len)
    {
        return 0;
    }

    static int32_t reporter_callback1(uint32_t moduleId, uint32_t type, void *data, uint32_t len)
    {
        return 1;
    }

    TEST_F(UTEST_ACL_Common, aclInitDumpFail)
    {
        aclError ret = aclInit("llt/acl/ut/json/dumpConfig.json");
        EXPECT_NE(ret, ACL_SUCCESS);
    }

    TEST_F(UTEST_ACL_Common, aclInitDumpSuccess)
    {
        const char *configPath = "../tests/ut/acl/json/dumpConfig.json";
        aclError ret = aclInit(configPath);
        EXPECT_NE(ret, ACL_SUCCESS);
    }

    TEST_F(UTEST_ACL_Common, aclRepeatInitFailed)
    {
        MsprofCtrlCallback callback = ctrl_callback;
        AclProfilingManager::GetInstance().SetProfCtrlCallback(callback);
        MsprofReporterCallback callback2 = reporter_callback;
        AclProfilingManager::GetInstance().SetProfReporterCallback(callback2);
        aclError ret = aclInit("../tests/ut/acl/json/profilingConfig.json");
        ret = aclInit("../tests/ut/acl/json/profilingConfig.json");
        EXPECT_NE(ret, ACL_SUCCESS);
    }

    TEST_F(UTEST_ACL_Common, aclFinalizeSuccess)
    {
        SetGeFinalizeCallback(nullptr);
        aclError ret = aclFinalize();
        EXPECT_EQ(ret, ACL_SUCCESS);
    }

    TEST_F(UTEST_ACL_Common, aclRepeatFinalize)
    {
        MsprofCtrlCallback callback = ctrl_callback;
        AclProfilingManager::GetInstance().SetProfCtrlCallback(callback);
        MsprofReporterCallback callback2 = reporter_callback;
        AclProfilingManager::GetInstance().SetProfReporterCallback(callback2);
        aclError ret = aclFinalize();
        EXPECT_NE(ret, ACL_SUCCESS);
    }

    TEST_F(UTEST_ACL_Common, aclrtGetSocName)
    {
        const char *socName = aclrtGetSocName();
        EXPECT_NE(socName, nullptr);
    }

    TEST_F(UTEST_ACL_Common, getVersionSuccess)
    {
        int majorVersion = 0;
        int minorVersion = 0;
        int patchVersion = 0;
        aclError ret = aclrtGetVersion(&majorVersion, &minorVersion, &patchVersion);
        EXPECT_EQ(ret, ACL_SUCCESS);
    }
}
