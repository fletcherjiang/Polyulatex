#include <gtest/gtest.h>

#include "acl/acl_base.h"
#include "types/fp16_impl.h"

using namespace acl;

class UTEST_ACL_Fp16 : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}

};

TEST(UTEST_ACL_Fp16, TestHalfToFloat) {
    aclFloat16 halfVal1 = aclFloatToFloat16(1.0f);
    aclFloat16 halfVal2 = aclFloatToFloat16(1.0f);
    ASSERT_TRUE(Fp16Eq(halfVal1, halfVal2));
    halfVal2 = aclFloatToFloat16(-1.0f);
    ASSERT_FALSE(Fp16Eq(halfVal1, halfVal2));


    aclFloatToFloat16(999999999.0f);
    aclFloatToFloat16(-999999999.0f);
    aclFloatToFloat16(-0.00000001f);
}