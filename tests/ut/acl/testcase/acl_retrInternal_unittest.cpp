#include "retr/retr_internal.h"
#include <gtest/gtest.h>

#include "acl/acl.h"
#include "securec.h"

using namespace std;
using namespace testing;

class UTEST_ACL_RetrInternal : public testing::Test {
protected:
    void SetUp() {}

    void TearDown() {}
};

TEST_F(UTEST_ACL_RetrInternal, aclCreateNotifySuccessTest)
{
    rtNotify_t notify;
    uint32_t notifyId;
    aclrtStream stream = (aclrtStream)0x1;
    auto ret = acl::retr::aclCreateNotify(notify, notifyId, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);
}