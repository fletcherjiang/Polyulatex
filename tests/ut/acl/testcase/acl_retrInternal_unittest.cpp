#include "retr/retr_internal.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "acl/acl.h"
#include "securec.h"
#include "acl_stub.h"

using namespace std;
using namespace testing;

class UTEST_ACL_RetrInternal : public testing::Test {
protected:
    void SetUp() {}

    void TearDown() {
        Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
    }
};

TEST_F(UTEST_ACL_RetrInternal, aclCreateNotifySuccessTest)
{
    rtNotify_t notify;
    uint32_t notifyId;
    aclrtStream stream = (aclrtStream)0x1;
    auto ret = acl::retr::aclCreateNotify(notify, notifyId, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventCreateWithFlag(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = acl::retr::aclCreateNotify(notify, notifyId, stream);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventCreateWithFlag(_, _))
        .WillOnce(Return(RT_ERROR_NONE));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetEventID(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = acl::retr::aclCreateNotify(notify, notifyId, stream);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventCreateWithFlag(_, _))
        .WillOnce(Return(RT_ERROR_NONE));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetEventID(_, _))
        .WillOnce(Return(RT_ERROR_NONE));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventReset(_, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = acl::retr::aclCreateNotify(notify, notifyId, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_RetrInternal, aclCheckResult_001)
{
    rtStream_t stream;
    int32_t result = 0;
    void *retCode = (void *)&result;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetRunMode(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    auto ret = acl::retr::aclCheckResult(retCode);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpy(_, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = acl::retr::aclCheckResult(retCode);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    ret = acl::retr::aclCheckResult(retCode);
    EXPECT_EQ(ret, ACL_SUCCESS);
}