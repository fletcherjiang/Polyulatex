#include <string>
#include <iostream>

#include "toolchain/slog.h"
#include "toolchain/plog.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifndef private
#define private public
#include "acl/acl.h"
#include "acl/acl_mdl.h"
#include "log_inner.h"
#include "json_parser.h"
#include "toolchain/dump.h"
#include "toolchain/profiling.h"
#include "toolchain/profiling_manager.h"
#include "toolchain/prof_acl_api.h"
#include "acl/acl_prof.h"
#include "common/common_inner.h"
#include "executor/ge_executor.h"
#include "acl_stub.h"
#undef private
#endif

using namespace testing;
using namespace std;
using namespace acl;
using namespace ge;
using testing::Return;

class UTEST_ACL_toolchain : public testing::Test
{
    public:
        UTEST_ACL_toolchain(){}
    protected:
        virtual void SetUp() {}
        virtual void TearDown() {
            Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
        }
};

INT32 mmGetEnvStub(const CHAR *name, CHAR *value, UINT32 len)
{
    char environment[MMPA_MAX_PATH] = "";
    (void)memcpy_s(value, MMPA_MAX_PATH, environment, MMPA_MAX_PATH);
    return 0;
}

TEST_F(UTEST_ACL_toolchain, dumpApiNotSupportTest)
{
    acl::AclDump::GetInstance().aclDumpFlag_ = true;
    aclError ret = aclmdlInitDump();
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlSetDump("llt/acl/ut/json/dumpConfig.json");
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlFinalizeDump();
    EXPECT_NE(ret, ACL_SUCCESS);
}

static int AdxDataDumpServerInitInvoke()
{
    int initRet = 1;
    return initRet;
}

TEST_F(UTEST_ACL_toolchain, dumpInitFailed)
{
    acl::AclDump::GetInstance().aclDumpFlag_ = true;
    acl::AclDump::GetInstance().SetAclDumpFlag(false);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), AdxDataDumpServerInit())
        .WillRepeatedly(Invoke(AdxDataDumpServerInitInvoke));
    aclError ret = aclmdlInitDump();
    EXPECT_EQ(ret, ACL_ERROR_INTERNAL_ERROR);
}

TEST_F(UTEST_ACL_toolchain, dumpParamTest)
{
    acl::AclDump aclDump;
    acl::AclDump::GetInstance().aclDumpFlag_ = false;
    aclError ret = aclmdlInitDump();
    EXPECT_EQ(ret, ACL_SUCCESS);

    acl::AclDump::GetInstance().aclDumpFlag_ = false;
    ret = aclmdlSetDump("../tests/ut/acl/json/testDump1.json");
    EXPECT_EQ(ret, ACL_SUCCESS);

    acl::AclDump::GetInstance().aclDumpFlag_ = false;
    ret = aclmdlSetDump("../tests/ut/acl/json/testDump2.json");
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlSetDump("../tests/ut/acl/json/testDump3.json");
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclmdlSetDump("../tests/ut/acl/json/testDump4.json");
    EXPECT_NE(ret, ACL_SUCCESS);

    // dump_list field illegal
    ret = aclDump.HandleDumpConfig("../tests/ut/acl/json/testDump5.json");
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclDump.HandleDumpConfig("../tests/ut/acl/json/testDump6.json");
    EXPECT_EQ(ret, ACL_SUCCESS);

    // invalid dump_op_switch
    ret = aclmdlSetDump("../tests/ut/acl/json/testDump7.json");
    EXPECT_NE(ret, ACL_SUCCESS);

    // invalid dumppathIp
    ret = aclmdlSetDump("../tests/ut/acl/json/testDump8.json");
    EXPECT_NE(ret, ACL_SUCCESS);

    // model name is empty
    ret = aclmdlSetDump("../tests/ut/acl/json/testDump9.json");
    EXPECT_EQ(ret, ACL_SUCCESS);

    // dump_op_switch field is not exist
    ret = aclmdlSetDump("../tests/ut/acl/json/testDump10.json");
    EXPECT_EQ(ret, ACL_SUCCESS);

    // dump_op_switch field is off,dump_list is empty
    ret = aclmdlSetDump("../tests/ut/acl/json/testDump11.json");
    EXPECT_NE(ret, ACL_SUCCESS);

    // dump_op_switch field is illegal
    ret = aclmdlSetDump("../tests/ut/acl/json/testDump12.json");
    EXPECT_NE(ret, ACL_SUCCESS);

    // dump_mode field is illegal
    ret = aclmdlSetDump("../tests/ut/acl/json/testDump13.json");
    EXPECT_NE(ret, ACL_SUCCESS);

    // dump_path is empty
    ret = aclmdlSetDump("../tests/ut/acl/json/testDump14.json");
    EXPECT_NE(ret, ACL_SUCCESS);

    // no dump item
    ret = aclmdlSetDump("../tests/ut/acl/json/testDump15.json");
    EXPECT_NE(ret, ACL_SUCCESS);

    // dump_list field is illegal
    ret = aclmdlSetDump("../tests/ut/acl/json/testDump16.json");
    EXPECT_NE(ret, ACL_SUCCESS);

    // aclInit dump
    ret = aclDump.HandleDumpConfig("../tests/ut/acl/json/dumpConfig.json");
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_toolchain, dumpNotInitTest)
{
    acl::AclDump::GetInstance().SetAclDumpFlag(false);
    aclmdlFinalizeDump();
    aclError ret = aclmdlSetDump("llt/acl/ut/json/dumpConfig.json");
    EXPECT_NE(ret, ACL_SUCCESS);
    EXPECT_NE(aclmdlFinalizeDump(), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_toolchain, repeatExecuteAclmdlInitDumpTest)
{
    acl::AclDump::GetInstance().aclDumpFlag_ = false;
    aclError ret = aclmdlInitDump();
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_NE(aclmdlInitDump(), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_toolchain, SetDumpConfigFailedTest)
{
    aclInit(nullptr);
    acl::AclDump::GetInstance().aclDumpFlag_ = false; 
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), SetDump(_))
        .WillOnce(Return((FAILED)));

    aclError ret = aclmdlSetDump("../tests/ut/acl/json/dumpConfig.json");
    EXPECT_NE(ret, ACL_SUCCESS);
}


TEST_F(UTEST_ACL_toolchain, dumpFinalizeFailedTest)
{
    acl::AclDump::GetInstance().SetAclDumpFlag(false);
    acl::AclDump::GetInstance().aclDumpFlag_ = false;
    // Clear dump config failed in aclmdlFinalizeDump 
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), SetDump(_))
        .WillOnce(Return((FAILED)));
    aclError ret = aclmdlFinalizeDump();
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    // kill dump server failed
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), AdxDataDumpServerUnInit())
        .WillOnce(Return(1));
    ret = aclmdlFinalizeDump();
    EXPECT_EQ(ret, ACL_ERROR_INTERNAL_ERROR);
}

// ========================== profiling testcase =============================

TEST_F(UTEST_ACL_toolchain, setDeviceSuccess)
{
    acl::AclProfilingManager aclProfManager;
    uint32_t deviceIdList[] = {1, 2, 3};
    aclProfManager.AddDeviceList(nullptr, 0);
    aclProfManager.AddDeviceList(deviceIdList, 3);
    aclProfManager.RemoveDeviceList(nullptr, 0);
    aclProfManager.RemoveDeviceList(deviceIdList, 3);
}

TEST_F(UTEST_ACL_toolchain, AclProfilingManagerInitFailed)
{
    acl::AclProfilingManager aclProfManager;
    aclError ret = aclProfManager.Init();
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_toolchain, AclProfilingManagerUnInitFailed)
{
    acl::AclProfilingManager aclProfManager;
    aclError ret = aclProfManager.UnInit();
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_toolchain, HandleProfilingConfig)
{
    acl::AclProfiling aclProf;
    aclError ret = aclProf.HandleProfilingConfig(nullptr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    char environment[MMPA_MAX_PATH] = "../tests/ut/acl/json/profConfig.json";
    ret = aclProf.HandleProfilingConfig("../tests/ut/acl/json/profConfig.json");
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclProf.HandleProfilingConfig("../tests/ut/acl/json/profilingConfig.json");
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_toolchain, HandleProfilingCommand)
{
    acl::AclProfiling aclprof;
    const string config = "test";
    bool configFileFlag = false;
    bool noValidConfig = false;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), MsprofInit(_,_,_))
        .WillRepeatedly(Return(1));
    aclError ret = aclprof.HandleProfilingCommand(config, configFileFlag, noValidConfig);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);

    configFileFlag = true;
    noValidConfig = false;
    ret = aclprof.HandleProfilingCommand(config, configFileFlag, noValidConfig);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
}

extern aclError aclMsprofCtrlHandle(uint32_t dataType, void* data, uint32_t dataLen);
int32_t MsprofReporterCallbackImpl(uint32_t moduleId, uint32_t type, void *data, uint32_t len)
{
    return 0;
}

TEST_F(UTEST_ACL_toolchain, MsprofCtrlHandle)
{

    aclError ret = aclMsprofCtrlHandle(RT_PROF_CTRL_REPORTER, (void*)MsprofReporterCallbackImpl, sizeof(int));
    EXPECT_EQ(ret, ACL_SUCCESS);

    rtProfCommandHandle_t command;
    command.profSwitch = 1;
    command.devNums = 1;
    command.devIdList[0] = 0;
    ret = aclMsprofCtrlHandle(RT_PROF_CTRL_SWITCH, static_cast<void *>(&command), sizeof(rtProfCommandHandle_t));
    EXPECT_EQ(ret, ACL_SUCCESS);

    command.profSwitch = 0;
    ret = aclMsprofCtrlHandle(RT_PROF_CTRL_SWITCH, static_cast<void *>(&command), sizeof(rtProfCommandHandle_t));
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclMsprofCtrlHandle(RT_PROF_CTRL_SWITCH, static_cast<void *>(&command), sizeof(rtProfCommandHandle_t) - 1);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
}

TEST_F(UTEST_ACL_toolchain, AclProfilingReporter)
{
    AclProfilingManager::GetInstance().isProfiling_ = true;
    AclProfilingManager::GetInstance().deviceList_.insert(-1);
    AclProfilingManager::GetInstance().deviceList_.insert(0);
    AclProfilingReporter reporter("testProf", ACL_PROF_FUNC_MODEL);
    reporter.~AclProfilingReporter();
    AclProfilingManager::GetInstance().isProfiling_ = false;
    AclProfilingManager::GetInstance().deviceList_.erase(-1);
    AclProfilingManager::GetInstance().deviceList_.erase(0);
}
TEST_F(UTEST_ACL_toolchain, AclProfilingReporter_2)
{
    AclProfilingManager::GetInstance().isProfiling_ = true;
    AclProfilingManager::GetInstance().deviceList_.insert(-1);
    AclProfilingManager::GetInstance().deviceList_.insert(0);
    AclProfilingReporter reporter("testProf", ACL_PROF_FUNC_MODEL);
    reporter.~AclProfilingReporter();
    AclProfilingManager::GetInstance().isProfiling_ = false;
    AclProfilingManager::GetInstance().deviceList_.erase(-1);
    AclProfilingManager::GetInstance().deviceList_.erase(0);
}