#include "toolchain/slog.h"
#include "toolchain/plog.h"

#include <gtest/gtest.h>

#ifndef private
#define private public
#include "acl/acl.h"
#include "acl/acl_mdl.h"
#include "log_inner.h"
#include "json_parser.h"
#include "toolchain/dump.h"
#include "common/common_inner.h"
#undef private
#endif

class UTEST_ACL_toolchain : public testing::Test
{
    public:
        UTEST_ACL_toolchain(){}
    protected:
        virtual void SetUp() {}
        virtual void TearDown() {}
};

TEST_F(UTEST_ACL_toolchain, dumpApiNotSupportTest)
{
    bool aclInitRet = GetAclInitFlag();
    EXPECT_EQ(aclInitRet, true);
    aclError ret = aclmdlInitDump();
    EXPECT_NE(ret, ACL_SUCCESS);

    aclInitRet = GetAclInitFlag();
    EXPECT_EQ(aclInitRet, true);
    ret = aclmdlSetDump("llt/acl/ut/json/dumpConfig.json");
    EXPECT_NE(ret, ACL_SUCCESS);

    aclInitRet = GetAclInitFlag();
    EXPECT_EQ(aclInitRet, true);
    ret = aclmdlFinalizeDump();
    EXPECT_NE(ret, ACL_SUCCESS);
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
}

TEST_F(UTEST_ACL_toolchain, dumpNotInitTest)
{
    acl::AclDump::GetInstance().SetAclDumpFlag(false);
    aclmdlFinalizeDump();
    aclError ret = aclmdlSetDump("llt/acl/ut/json/dumpConfig.json");
    EXPECT_NE(ret, ACL_SUCCESS);

    acl::AclDump::GetInstance().aclDumpFlag_ = false;
    ret = aclmdlInitDump();
    EXPECT_EQ(ret, ACL_SUCCESS);
}

