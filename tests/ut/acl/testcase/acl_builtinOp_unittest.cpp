#include <gtest/gtest.h>

#include "acl/acl.h"
#include "acl/ops/acl_cblas.h"
#include "common/common_inner.h"

using namespace testing;
using namespace std;

class UTEST_ACL_BuiltinOpApi : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}

};

TEST_F(UTEST_ACL_BuiltinOpApi, CastOpTest)
{
    aclopHandle *handle = nullptr;
    int64_t shape[2] = {16, 32};
    aclTensorDesc *inputDesc = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    aclTensorDesc *outputDesc = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    aclDataBuffer *inputBuffer = aclCreateDataBuffer(nullptr, 0);
    aclDataBuffer *outputBuffer = aclCreateDataBuffer(nullptr, 0);

    SetCastHasTruncateAttr(true);
    ASSERT_NE(aclopCreateHandleForCast(inputDesc, outputDesc, false, &handle), ACL_SUCCESS);
    ASSERT_EQ(aclopCreateHandleForCast(inputDesc, outputDesc, false,&handle), ACL_ERROR_OP_NOT_FOUND);

    ASSERT_NE(aclopCast(inputDesc, inputBuffer, outputDesc, outputBuffer, false, nullptr), ACL_SUCCESS);
    ASSERT_EQ(aclopCast(inputDesc, inputBuffer, outputDesc, outputBuffer, false, nullptr), ACL_ERROR_OP_NOT_FOUND);

    aclDestroyTensorDesc(inputDesc);
    aclDestroyTensorDesc(outputDesc);
    aclDestroyDataBuffer(inputBuffer);
    aclDestroyDataBuffer(outputBuffer);
    aclopDestroyHandle(handle);
}

extern aclError aclopTransData(aclTensorDesc *srcDesc,
                        aclDataBuffer *srcBuffer,
                        aclTensorDesc *dstDesc,
                        aclDataBuffer *dstBuffer,
                        aclrtStream stream);

extern aclError aclopCreateHandleForTransData(aclTensorDesc *srcDesc,
                                       aclTensorDesc *dstDesc,
                                       aclopHandle **handle);

TEST_F(UTEST_ACL_BuiltinOpApi, AclopTransDataTest)
{
    int64_t shape[2] = {16, 32};
    aclTensorDesc *srcDesc = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    aclDataBuffer *srcBuffer = aclCreateDataBuffer(nullptr, 0);
    aclTensorDesc *dstDesc = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    aclDataBuffer *dstBuffer = aclCreateDataBuffer(nullptr, 0);
    aclrtStream stream;
    aclError ret = aclopTransData(srcDesc, srcBuffer, dstDesc, dstBuffer, stream);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclDestroyTensorDesc(srcDesc);
    aclDestroyTensorDesc(dstDesc);
    aclDestroyDataBuffer(srcBuffer);
    aclDestroyDataBuffer(dstBuffer);
}

TEST_F(UTEST_ACL_BuiltinOpApi, AclopCreateHandleForTransDataTest)
{
    aclopHandle *handle = nullptr;
    int64_t shape[2] = {16, 32};
    aclTensorDesc *srcDesc = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    aclTensorDesc *dstDesc = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, ACL_FORMAT_ND);
    aclError ret = aclopCreateHandleForTransData(srcDesc, dstDesc, &handle);
    EXPECT_NE(ret, ACL_SUCCESS);

    aclFormat format = static_cast<aclFormat>(10);
    aclTensorDesc *desc = aclCreateTensorDesc(ACL_FLOAT16, 2, shape, format);
    ret = aclopCreateHandleForTransData(desc, dstDesc, &handle);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclDestroyTensorDesc(srcDesc);
    aclDestroyTensorDesc(desc);
    aclDestroyTensorDesc(dstDesc);
}