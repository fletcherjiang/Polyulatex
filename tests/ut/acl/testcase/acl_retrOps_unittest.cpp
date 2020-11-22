#include "acl/ops/acl_fv.h"
#include "acl/acl.h"
#include "securec.h"
#include "acl_stub.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define protected public
#define private public

#include "retr/retr_search.h"
#include "retr/retr_internal.h"
#include "retr/retr_accurate.h"
#include "retr/retr_init.h"
#include "executor/stream_executor.h"

#undef private
#undef protected

#include "aicpu/common/aicpu_task_struct.h"

using namespace std;
using namespace testing;

class UTEST_ACL_RetrOps : public testing::Test {
protected:

    void SetUp() {}

    void TearDown() {
        Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
    }
};

TEST_F(UTEST_ACL_RetrOps, aclCheckResultSuccessTest)
{
    rtStream_t stream;
    int32_t result = 0;
    void *retCode = (void *)&result;
    auto ret = acl::retr::aclCheckResult(retCode);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_RetrOps, aclfvInit_001)
{
    aclfvInitPara *initPara = aclfvCreateInitPara(100);
    initPara->initPara.retCode = 0;
    aclError ret = aclfvInit(initPara);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(aclfvDestroyInitPara(initPara), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_RetrOps, aclfvInit_002)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclfvInitPara *initPara = aclfvCreateInitPara(100);
    aclError ret = aclfvInit(initPara);

    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    EXPECT_EQ(aclfvDestroyInitPara(initPara), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_RetrOps, aclfvInit_003)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclfvInitPara *initPara = aclfvCreateInitPara(100);
    aclError ret = aclfvInit(initPara);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    EXPECT_EQ(aclfvDestroyInitPara(initPara), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_RetrOps, aclfvInit_004)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclfvInitPara *initPara = aclfvCreateInitPara(100);
    aclError ret = aclfvInit(initPara);

    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    EXPECT_EQ(aclfvDestroyInitPara(initPara), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_RetrOps, aclfvInit_005)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclfvInitPara *initPara = aclfvCreateInitPara(100);
    aclError ret = aclfvInit(initPara);
    
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    EXPECT_EQ(aclfvDestroyInitPara(initPara), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_RetrOps, aclfvInit_006)
{
    aclfvInitPara *initPara = aclfvCreateInitPara(100);
    aclError ret = aclfvInit(initPara);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    EXPECT_EQ(aclfvDestroyInitPara(initPara), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_RetrOps, aclfvRelease_002)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvRelease();
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
}

TEST_F(UTEST_ACL_RetrOps, aclfvRelease_003)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvRelease();
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_RetrOps, aclfvRelease_004)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvRelease();
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
}

TEST_F(UTEST_ACL_RetrOps, aclfvRelease_005)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclfvRelease();
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
}

TEST_F(UTEST_ACL_RetrOps, aclfvRelease_006)
{
    aclError ret = aclfvRelease();
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoAdd_001)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;
    aclrtStream stream = (aclrtStream)0x1;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvRepoAdd(SEARCH_1_N, featureInfo);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoAdd_002)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;
    aclError ret = aclfvRepoAdd(SEARCH_1_N, featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

int streamValue = 0;
rtError_t rtStreamCreateStub(rtStream_t *stream, int32_t priority)
{
    *stream = (rtStream_t *)(&streamValue);
    return RT_ERROR_NONE;
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoAdd_003)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventCreateWithFlag(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvRepoAdd(SEARCH_1_N, featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoAdd_004)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetEventID(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvRepoAdd(SEARCH_1_N, featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoAdd_005)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvRepoAdd(SEARCH_1_N, featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoAdd_006)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    aclError ret = aclfvRepoAdd(SEARCH_1_N, featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoAdd_007)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvRepoAdd(SEARCH_1_N, featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoAdd_008)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;
    featureInfo->dataBuffer.length = 0;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    aclError ret = aclfvRepoAdd(SEARCH_1_N, featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoAdd_009)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 1;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    aclError ret = aclfvRepoAdd(SEARCH_1_N, featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoAdd_010)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    aclError ret = aclfvRepoAdd(SEARCH_N_M, featureInfo);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoAdd_011)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamWaitEvent(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvRepoAdd(SEARCH_N_M, featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoAdd_012)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;
    aclfvSearchType type = (aclfvSearchType)(3);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    aclError ret = aclfvRepoAdd(type, featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoAdd_013)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvRepoAdd(SEARCH_N_M, featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoDel_001)
{
    auto repoRange = aclfvCreateRepoRange(0, 1, 0, 1023);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    aclError ret = aclfvRepoDel(SEARCH_N_M, repoRange);
    aclfvDestroyRepoRange(repoRange);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoDel_002)
{
    auto repoRange = aclfvCreateRepoRange(0, 1, 0, 1023);
    repoRange->retrRepoRange.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvRepoDel(SEARCH_N_M, repoRange);
    aclfvDestroyRepoRange(repoRange);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoDel_003)
{
    auto repoRange = aclfvCreateRepoRange(0, 1, 0, 1023);
    repoRange->retrRepoRange.retCode = 0;
    repoRange->dataBuffer.length = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    aclError ret = aclfvRepoDel(SEARCH_N_M, repoRange);
    aclfvDestroyRepoRange(repoRange);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoDel_004)
{
    auto repoRange = aclfvCreateRepoRange(0, 1, 0, 1023);
    repoRange->retrRepoRange.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetRunMode(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvRepoDel(SEARCH_1_N, repoRange);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    aclfvDestroyRepoRange(repoRange);
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoDel_005)
{
    auto repoRange = aclfvCreateRepoRange(0, 1, 0, 1023);
    repoRange->retrRepoRange.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvRepoDel(SEARCH_1_N, repoRange);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    aclfvDestroyRepoRange(repoRange);
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoDel_006)
{
    auto repoRange = aclfvCreateRepoRange(0, 1, 0, 1023);
    repoRange->retrRepoRange.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvRepoDel(SEARCH_1_N, repoRange);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclfvRepoDel(SEARCH_1_N, repoRange);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    aclfvDestroyRepoRange(repoRange);
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoDel_007)
{
    auto repoRange = aclfvCreateRepoRange(0, 1, 0, 1023);
    repoRange->retrRepoRange.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvRepoDel(SEARCH_1_N, repoRange);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    aclfvDestroyRepoRange(repoRange);
}

TEST_F(UTEST_ACL_RetrOps, aclfvRepoDel_008)
{
    auto repoRange = aclfvCreateRepoRange(0, 1, 0, 1023);
    repoRange->retrRepoRange.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    aclError ret = aclfvRepoDel(SEARCH_1_N, repoRange);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclfvDestroyRepoRange(repoRange);
}

TEST_F(UTEST_ACL_RetrOps, aclfvDel_001)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));

    aclError ret = aclfvDel(featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvDel_002)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetRunMode(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvDel(featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvDel_003)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;
    featureInfo->dataBuffer.length = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    aclError ret = aclfvDel(featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, PrepareInputTest)
{
    acl::retr::AclFvAccurate aclFvAccurate;
    acl::retr::aclAccurateType typeAccurate;
    aclrtStream stream = (aclrtStream)0x1;
    int argsLen = sizeof(aicpu::AicpuParamHead) + 64;
    char *args = new char[argsLen];
    uint32_t argsSize = 20;
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;
    aclError ret = aclFvAccurate.PrepareInput(typeAccurate, featureInfo, stream, args, argsSize);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    argsSize = 35;
    ret = aclFvAccurate.PrepareInput(typeAccurate, featureInfo, stream, args, argsSize);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);

    delete[] args;
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvDel_005)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvDel(featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclfvDel(featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvDel_006)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    aclError ret = aclfvDel(featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvDel_007)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamWaitEvent(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclfvDel(featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvDel_008)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclfvDel(featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvDel_009)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 1;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    aclError ret = aclfvDel(featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvModify_001)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    aclError ret = aclfvModify(featureInfo);

    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvModify_002)
{
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    aclrtStream stream = (aclrtStream)0x1;
    featureInfo->retrFeatureInfo.retCode = 0;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));

    aclError ret = aclfvModify(featureInfo);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvModifyTest)
{
    acl::retr::AclFvAccurate aclFvAccurate;
    uint8_t *featureData = (uint8_t *)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,36,1,featureData,36);
    featureInfo->retrFeatureInfo.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    aclError ret = aclFvAccurate.Modify(featureInfo);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclfvDestroyFeatureInfo(featureInfo);
    featureInfo = nullptr;
}

TEST_F(UTEST_ACL_RetrOps, aclfvSearch_001)
{
    uint8_t *tableDataDev = (uint8_t *)0x1;
    auto searchQueryTable =
        aclfvCreateQueryTable(1, 32768, reinterpret_cast<uint8_t *>(tableDataDev), 32768);
    auto searchRange = aclfvCreateRepoRange(0, 1023, 0, 1023);
    auto searchInput = aclfvCreateSearchInput(searchQueryTable, searchRange, 5);

    auto searchResult = aclfvCreateSearchResult(1,
        reinterpret_cast<uint32_t *>(tableDataDev),
        4,
        reinterpret_cast<uint32_t *>(tableDataDev),
        reinterpret_cast<uint32_t *>(tableDataDev),
        reinterpret_cast<uint32_t *>(tableDataDev),
        reinterpret_cast<float *>(tableDataDev),
        20);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    aclError ret = aclfvSearch(SEARCH_N_M, searchInput, searchResult);
    aclfvDestroyQueryTable(searchQueryTable);
    aclfvDestroyRepoRange(searchRange);
    aclfvDestroySearchInput(searchInput);
    aclfvDestroySearchResult(searchResult);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
}

aclError aclCreateNotify_stub(rtNotify_t *notify, uint32_t &notifyId)
{
    *notify = (rtNotify_t)0x1;
    notifyId = 1;
    return ACL_SUCCESS;
}

TEST_F(UTEST_ACL_RetrOps, aclfvSearch_002)
{
    uint8_t *tableDataDev = (uint8_t *)0x1;
    auto searchQueryTable =
        aclfvCreateQueryTable(1, 32768, reinterpret_cast<uint8_t *>(tableDataDev), 32768);
    auto searchRange = aclfvCreateRepoRange(0, 1023, 0, 1023);
    auto searchInput = aclfvCreateSearchInput(searchQueryTable, searchRange, 5);

    auto searchResult = aclfvCreateSearchResult(1,
        reinterpret_cast<uint32_t *>(tableDataDev),
        4,
        reinterpret_cast<uint32_t *>(tableDataDev),
        reinterpret_cast<uint32_t *>(tableDataDev),
        reinterpret_cast<uint32_t *>(tableDataDev),
        reinterpret_cast<float *>(tableDataDev),
        20);
    aclfvSearchResult *searchRst;
    searchRst->searchResult.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));

    aclError ret = aclfvSearch(SEARCH_N_M, searchInput, searchResult);
    aclfvDestroyQueryTable(searchQueryTable);
    aclfvDestroyRepoRange(searchRange);
    aclfvDestroySearchInput(searchInput);
    aclfvDestroySearchResult(searchResult);

    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}

TEST_F(UTEST_ACL_RetrOps, aclfvSearch_003)
{
    uint8_t *tableDataDev = (uint8_t *)0x1;
    auto searchQueryTable =
        aclfvCreateQueryTable(1, 32768, reinterpret_cast<uint8_t *>(tableDataDev), 32768);
    auto searchRange = aclfvCreateRepoRange(0, 1023, 0, 1023);
    auto searchInput = aclfvCreateSearchInput(searchQueryTable, searchRange, 5);
    auto searchResult = aclfvCreateSearchResult(1,
        reinterpret_cast<uint32_t *>(tableDataDev),
        4,
        reinterpret_cast<uint32_t *>(tableDataDev),
        reinterpret_cast<uint32_t *>(tableDataDev),
        reinterpret_cast<uint32_t *>(tableDataDev),
        reinterpret_cast<float *>(tableDataDev),
        20);
    searchInput->dataBuffer.length = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    aclError ret = aclfvSearch(SEARCH_1_N, searchInput, searchResult);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    searchInput->dataBuffer.length = 44;
    searchResult->dataBuffer.length = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    ret = aclfvSearch(SEARCH_1_N, searchInput, searchResult);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);

    aclfvDestroyQueryTable(searchQueryTable);
    aclfvDestroyRepoRange(searchRange);
    aclfvDestroySearchInput(searchInput);
    aclfvDestroySearchResult(searchResult);
}

TEST_F(UTEST_ACL_RetrOps, aclfvSearch_004)
{
    uint8_t *tableDataDev = (uint8_t *)0x1;
    auto searchQueryTable =
        aclfvCreateQueryTable(1, 32768, reinterpret_cast<uint8_t *>(tableDataDev), 32768);
    auto searchRange = aclfvCreateRepoRange(0, 1023, 0, 1023);
    auto searchInput = aclfvCreateSearchInput(searchQueryTable, searchRange, 5);
    auto searchResult = aclfvCreateSearchResult(1,
        reinterpret_cast<uint32_t *>(tableDataDev),
        4,
        reinterpret_cast<uint32_t *>(tableDataDev),
        reinterpret_cast<uint32_t *>(tableDataDev),
        reinterpret_cast<uint32_t *>(tableDataDev),
        reinterpret_cast<float *>(tableDataDev),
        20);
    searchInput->dataBuffer.length = 44;
    searchResult->dataBuffer.length = 56;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetRunMode(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclfvSearch(SEARCH_1_N, searchInput, searchResult);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclfvSearch(SEARCH_1_N, searchInput, searchResult);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclfvSearch(SEARCH_1_N, searchInput, searchResult);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    // InitSearchTask
    aclfvSearchInput searchIn;
    aclfvSearchResult searchRst;
    searchIn.dataBuffer.data = (void *)0x1;
    searchRst.dataBuffer.data = (void *)0x1;
    aclrtStream stream = (aclrtStream)0x1;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillRepeatedly(Return(RT_ERROR_NONE));

    int argsLen = sizeof(aicpu::AicpuParamHead) + 64;
    char *args = new char[argsLen];
    acl::retr::AclFvSearch fvSearch;
    ret = fvSearch.InitSearchTask(&searchIn, &searchRst, 10, args, 0, stream);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    ret = fvSearch.InitSearchTask(&searchIn, &searchRst, 4, args, 53, stream);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    argsLen = sizeof(aicpu::AicpuParamHead) + 128;
    args = new char[argsLen];
    ret = fvSearch.InitSearchTask(&searchIn, &searchRst, 4, args, 56, stream);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventCreateWithFlag(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = fvSearch.InitSearchTask(&searchIn, &searchRst, 4, args, 57, stream);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    ret = fvSearch.InitSearchTask(&searchIn, &searchRst, 4, args, 58, stream);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    ret = fvSearch.InitSearchTask(&searchIn, &searchRst, 4, args, 60, stream);
    EXPECT_EQ(ret, ACL_SUCCESS);
    delete[] args;

    // Search1vN
    acl::retr::aclfvArgs retrArgs;
    char args_str[20] = {0};
    retrArgs.args = args_str;
    retrArgs.argsSize = 20;
    retrArgs.configOffset = 0;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    ret = fvSearch.Search1vN(retrArgs, &searchIn);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamWaitEvent(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = fvSearch.Search1vN(retrArgs, &searchIn);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    retrArgs.argsSize = 3;
    ret = fvSearch.Search1vN(retrArgs, &searchIn);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    retrArgs.argsSize = 4;
    ret = fvSearch.Search1vN(retrArgs, &searchIn);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    // SearchNvM
    aclfvSearchInput searchInput2;
    searchInput2.searchInput.queryTable.queryCnt = 1;
    searchInput2.searchInput.queryTable.tableLen = 1;
    searchInput2.searchInput.queryTable.tableDataLen = 1;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    ret = fvSearch.SearchNvM(retrArgs, &searchInput2);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);

    aclfvDestroyQueryTable(searchQueryTable);
    aclfvDestroyRepoRange(searchRange);
    aclfvDestroySearchInput(searchInput);
    aclfvDestroySearchResult(searchResult);
}

TEST_F(UTEST_ACL_RetrOps, aclfvSearch_005)
{
    uint8_t *tableDataDev = (uint8_t *)0x1;
    auto searchQueryTable =
        aclfvCreateQueryTable(1, 32768, reinterpret_cast<uint8_t *>(tableDataDev), 32768);
    auto searchRange = aclfvCreateRepoRange(0, 1023, 0, 1023);
    auto searchInput = aclfvCreateSearchInput(searchQueryTable, searchRange, 5);
    auto searchResult = aclfvCreateSearchResult(1,
        reinterpret_cast<uint32_t *>(tableDataDev),
        4,
        reinterpret_cast<uint32_t *>(tableDataDev),
        reinterpret_cast<uint32_t *>(tableDataDev),
        reinterpret_cast<uint32_t *>(tableDataDev),
        reinterpret_cast<float *>(tableDataDev),
        20);
    searchInput->dataBuffer.length = 44;
    searchResult->dataBuffer.length = 56;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    aclError ret = aclfvSearch(SEARCH_1_N, searchInput, searchResult);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    ret = aclfvSearch(SEARCH_N_M, searchInput, searchResult);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    aclfvSearchType type = (aclfvSearchType)(3);
    ret = aclfvSearch(type, searchInput, searchResult);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclfvSearch(SEARCH_N_M, searchInput, searchResult);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Invoke(rtStreamCreateStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpy(_, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    ret = aclfvSearch(SEARCH_N_M, searchInput, searchResult);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
}