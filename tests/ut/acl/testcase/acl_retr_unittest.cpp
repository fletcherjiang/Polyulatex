#define private public
#include "retr/retr_repo.h"
#include "retr/retr_init.h"
#include "retr/retr_search.h"
#include "acl/ops/acl_fv.h"
#undef private

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "acl/acl.h"
#include "runtime/rt.h"
#include "runtime/base.h"
#include "retr/retr_internal.h"
#include "securec.h"
#include "acl_stub.h"

using namespace std;
using namespace testing;

class UTEST_ACL_Retr : public testing::Test {
protected:

    void SetUp() {}

    void TearDown() {
        Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
    }
};

TEST_F(UTEST_ACL_Retr, aclfvCreateFeatureInfo_001)
{
    uint8_t *featureData = (uint8_t*)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,0,10000001,featureData,0);
    EXPECT_EQ(featureInfo, nullptr);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateFeatureInfo_002)
{
    uint8_t *featureData = (uint8_t*)0x1;
    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(0,0,0,0,10000000,featureData,0);
    EXPECT_EQ(featureInfo, nullptr);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateQueryTable_001)
{
    uint8_t *tableData = (uint8_t*)0x1;
    aclfvQueryTable *queryTable = aclfvCreateQueryTable(1, 0, tableData, 10);
    EXPECT_EQ(queryTable, nullptr);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateQueryTable_002)
{
    uint8_t *tableData = (uint8_t*)0x1;
    aclfvQueryTable *queryTable = aclfvCreateQueryTable(1025, 32768, tableData, 10);
    EXPECT_EQ(queryTable, nullptr);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateQueryTable_003)
{
    uint8_t *tableData = (uint8_t*)0x1;
    aclfvQueryTable *queryTable = aclfvCreateQueryTable(1024, 32768, tableData, 10);
    EXPECT_EQ(queryTable, nullptr);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateSearchResult_001)
{
    uint32_t *resultNum = (uint32_t*)0x0;
    uint32_t *id0 = (uint32_t *)0x1;
    uint32_t *id1 = (uint32_t *)0x1;
    uint32_t *resultOffset = (uint32_t *)0x1;
    float *resultDistance = (float *)0x1;
    aclfvSearchResult *sr = aclfvCreateSearchResult(1024, resultNum, 1, id0, id1, resultOffset, resultDistance, 10);
    EXPECT_EQ(sr, nullptr);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateSearchResult_002)
{
    uint32_t *resultNum = (uint32_t*)0x1;
    uint32_t *id0 = (uint32_t*)0x0;
    uint32_t *id1 = (uint32_t *)0x1;
    uint32_t *resultOffset = (uint32_t *)0x1;
    float *resultDistance = (float *)0x1;
    aclfvSearchResult *sr = aclfvCreateSearchResult(1024, resultNum, 1, id0, id1, resultOffset, resultDistance, 10);
    EXPECT_EQ(sr, nullptr);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateSearchResult_003)
{
    uint32_t *resultNum = (uint32_t*)0x1;
    uint32_t *id0 = (uint32_t *)0x1;
    uint32_t *id1 = (uint32_t*)0x0;
    uint32_t *resultOffset = (uint32_t *)0x1;
    float *resultDistance = (float *)0x1;
    aclfvSearchResult *sr = aclfvCreateSearchResult(1024, resultNum, 1, id0, id1, resultOffset, resultDistance, 10);
    EXPECT_EQ(sr, nullptr);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateSearchResult_004)
{
    uint32_t *resultNum = (uint32_t*)0x1;
    uint32_t *id0 = (uint32_t *)0x1;
    uint32_t *id1 = (uint32_t *)0x1;
    uint32_t *resultOffset = (uint32_t*)0x0;
    float *resultDistance = (float *)0x1;
    aclfvSearchResult *sr = aclfvCreateSearchResult(1024, resultNum, 1, id0, id1, resultOffset, resultDistance, 10);
    EXPECT_EQ(sr, nullptr);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateSearchResult_005)
{
    uint32_t *resultNum = (uint32_t*)0x1;
    uint32_t *id0 = (uint32_t *)0x1;
    uint32_t *id1 = (uint32_t *)0x1;
    uint32_t *resultOffset = (uint32_t *)0x1;
    float *resultDistance = (float*)0x0;
    aclfvSearchResult *sr = aclfvCreateSearchResult(1024, resultNum, 1, id0, id1, resultOffset, resultDistance, 10);
    EXPECT_EQ(sr, nullptr);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateSearchResult_006)
{
    uint32_t *resultNum = (uint32_t*)0x1;
    uint32_t *id0 = (uint32_t *)0x1;
    uint32_t *id1 = (uint32_t *)0x1;
    uint32_t *resultOffset = (uint32_t *)0x1;
    float *resultDistance = (float *)0x1;
    aclfvSearchResult *sr = aclfvCreateSearchResult(1025, resultNum, 1, id0, id1, resultOffset, resultDistance, 10);
    EXPECT_EQ(sr, nullptr);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateSearchResult_007)
{
    uint32_t *resultNum = (uint32_t*)0x1;
    uint32_t *id0 = (uint32_t *)0x1;
    uint32_t *id1 = (uint32_t *)0x1;
    uint32_t *resultOffset = (uint32_t *)0x1;
    float *resultDistance = (float *)0x1;
    aclfvSearchResult *sr = aclfvCreateSearchResult(1024, resultNum, 1, id0, id1, resultOffset, resultDistance, 10);
    EXPECT_EQ(sr, nullptr);
}

TEST_F(UTEST_ACL_Retr, aclfvSet1NTopNum_001)
{
    aclfvInitPara  *initPara = aclfvCreateInitPara(100);
    EXPECT_NE(initPara, nullptr);
    EXPECT_EQ(aclfvSet1NTopNum(initPara, 200), ACL_SUCCESS);
    EXPECT_EQ(aclfvDestroyInitPara(initPara), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Retr, aclfvSet1NTopNum_002)
{
    aclfvInitPara  *initPara = aclfvCreateInitPara(100);
    EXPECT_NE(initPara, nullptr);
    EXPECT_EQ(aclfvSet1NTopNum(initPara, 1), ACL_ERROR_INVALID_PARAM);
    EXPECT_EQ(aclfvDestroyInitPara(initPara), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Retr, aclfvSetNMTopNum_001)
{
    aclfvInitPara  *initPara = aclfvCreateInitPara(100);
    EXPECT_NE(initPara, nullptr);
    EXPECT_EQ(aclfvSetNMTopNum(initPara, 600), ACL_SUCCESS);
    EXPECT_EQ(aclfvDestroyInitPara(initPara), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Retr, aclfvSetNMTopNum_002)
{
    aclfvInitPara  *initPara = aclfvCreateInitPara(100);
    EXPECT_NE(initPara, nullptr);
    EXPECT_EQ(aclfvSetNMTopNum(initPara, 1), ACL_ERROR_INVALID_PARAM);
    EXPECT_EQ(aclfvDestroyInitPara(initPara), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateInitParaTest)
{
    uint64_t fsNum = 0;
    EXPECT_EQ(aclfvCreateInitPara(fsNum), nullptr);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateFeatureInfoTest)
{
    uint32_t id0;
    uint32_t id1;
    uint32_t offset;
    uint32_t featureLen;
    uint32_t featureCount;
    uint8_t *featureData = nullptr;
    uint32_t featureDataLen;
    EXPECT_EQ(aclfvCreateFeatureInfo(id0, id1, offset, featureLen, featureCount, featureData, featureDataLen), nullptr);

    featureData = (uint8_t *)malloc(5);
    id0 = 1024;
    EXPECT_EQ(aclfvCreateFeatureInfo(id0, id1, offset, featureLen, featureCount, featureData, featureDataLen), nullptr);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    id0 = 10;
    id1 = 10;
    featureCount = 100000001;
    EXPECT_EQ(aclfvCreateFeatureInfo(id0, id1, offset, featureLen, featureCount, featureData, featureDataLen), nullptr);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    featureLen = 36;
    featureDataLen = 20;
    featureCount = 10;
    EXPECT_EQ(aclfvCreateFeatureInfo(id0, id1, offset, featureLen, featureCount, featureData, featureDataLen), nullptr);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    featureDataLen = 360;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_EQ(aclfvCreateFeatureInfo(id0, id1, offset, featureLen, featureCount, featureData, featureDataLen), nullptr);
    free(featureData);
}

rtError_t rtFreeStub(void *devPtr)
{
    free(devPtr);
    return 0;
}

TEST_F(UTEST_ACL_Retr, aclfvDestroyFeatureInfoTest)
{
    uint32_t id0 = 10;
    uint32_t id1 = 10;
    uint32_t offset = 20;
    uint32_t featureLen = 36;
    uint32_t featureCount = 10;
    uint8_t *featureData = (uint8_t *)malloc(5);
    uint32_t featureDataLen = 360;

    aclfvFeatureInfo *featureInfo = aclfvCreateFeatureInfo(id0, id1, offset, featureLen, featureCount, featureData, featureDataLen);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtFree(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID))
        .WillRepeatedly(Invoke(rtFreeStub));
    void* data = featureInfo->dataBuffer.data;
    aclError ret = aclfvDestroyFeatureInfo(featureInfo);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateRepoRangeTest)
{
    uint32_t id0Min = 2;
    uint32_t id0Max = 1;
    uint32_t id1Min;
    uint32_t id1Max;
    EXPECT_EQ(aclfvCreateRepoRange(id0Min, id0Max, id1Min, id1Max), nullptr);

    id0Min = 1;
    id0Max = 2;
    id1Min = 2;
    id1Max = 1;
    EXPECT_EQ(aclfvCreateRepoRange(id0Min, id0Max, id1Min, id1Max), nullptr);

    id0Min = 1;
    id0Max = 2;
    id1Min = 1;
    id1Max = 2;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_EQ(aclfvCreateRepoRange(id0Min, id0Max, id1Min, id1Max), nullptr);
}

TEST_F(UTEST_ACL_Retr, aclfvDestroyRepoRangeTest)
{
    uint32_t id0Min = 1;
    uint32_t id0Max = 2;
    uint32_t id1Min = 1;
    uint32_t id1Max = 2;
    aclfvRepoRange *repoRange = aclfvCreateRepoRange(id0Min, id0Max, id1Min, id1Max);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtFree(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID))
        .WillRepeatedly(Invoke(rtFreeStub));
    void* data = repoRange->dataBuffer.data;
    aclError ret = aclfvDestroyRepoRange(repoRange);
    EXPECT_EQ(ret, ACL_SUCCESS);
    aclrtFree(data);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateQueryTableTest)
{
    uint32_t queryCnt;
    uint32_t tableLen;
    uint8_t *tableData = nullptr;
    uint32_t tableDataLen;
    EXPECT_EQ(aclfvCreateQueryTable(queryCnt, tableLen, tableData, tableDataLen), nullptr);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateSearchInputTest01)
{
    aclfvQueryTable *queryTable = nullptr;
    aclfvRepoRange *repoRange = nullptr;
    uint32_t topK;
    EXPECT_EQ(aclfvCreateSearchInput(queryTable, repoRange, topK), nullptr);

    queryTable = (aclfvQueryTable *)malloc(5);
    EXPECT_EQ(aclfvCreateSearchInput(queryTable, repoRange, topK), nullptr);

    repoRange = (aclfvRepoRange *)malloc(5);
    topK = 4801;
    EXPECT_EQ(aclfvCreateSearchInput(queryTable, repoRange, topK), nullptr);
    free(queryTable);
    free(repoRange);
}

TEST_F(UTEST_ACL_Retr, aclfvCreateSearchInputTest02)
{
    uint32_t topK = 200;
    uint32_t queryCnt = 10;
    uint32_t tableLen = 32768;
    uint8_t *tableData = (uint8_t *)malloc(5);
    uint32_t tableDataLen = 327680;
    aclfvQueryTable *queryTable = aclfvCreateQueryTable(queryCnt, tableLen, tableData, tableDataLen);

    uint32_t id0Min = 1;
    uint32_t id0Max = 2;
    uint32_t id1Min = 1;
    uint32_t id1Max = 2;
    aclfvRepoRange *repoRange = aclfvCreateRepoRange(id0Min, id0Max, id1Min, id1Max);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_EQ(aclfvCreateSearchInput(queryTable, repoRange, topK), nullptr);
    aclfvDestroyQueryTable(queryTable);
    aclfvDestroyRepoRange(repoRange);
    free(tableData);
}

TEST_F(UTEST_ACL_Retr, aclfvDestroySearchInputTest)
{
    uint32_t topK = 200;

    uint32_t queryCnt = 10;
    uint32_t tableLen = 32768;
    uint8_t *tableData = (uint8_t *)malloc(5);
    uint32_t tableDataLen = 327680;
    aclfvQueryTable *queryTable = aclfvCreateQueryTable(queryCnt, tableLen, tableData, tableDataLen);

    uint32_t id0Min = 1;
    uint32_t id0Max = 2;
    uint32_t id1Min = 1;
    uint32_t id1Max = 2;
    aclfvRepoRange *repoRange = aclfvCreateRepoRange(id0Min, id0Max, id1Min, id1Max);

    aclfvSearchInput *retrSearchInput = aclfvCreateSearchInput(queryTable, repoRange, topK);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtFree(_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID))
        .WillRepeatedly(Invoke(rtFreeStub));
    void* data = retrSearchInput->dataBuffer.data;
    aclError ret = aclfvDestroySearchInput(retrSearchInput);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclfvDestroyQueryTable(queryTable);
    aclfvDestroyRepoRange(repoRange);
    free(tableData);
}

TEST_F(UTEST_ACL_Retr, aclFvNotifyWaitTest)
{
    rtNotify_t notify;
    aclrtStream stream;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamWaitEvent(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = acl::retr::aclFvNotifyWait(notify, stream);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamWaitEvent(_, _))
        .WillOnce(Return((ACL_SUCCESS)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtEventReset(_, _))
        .WillRepeatedly(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = acl::retr::aclFvNotifyWait(notify, stream);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Retr, PrepareInputTest)
{
    aclfvInitPara *initPara = aclfvCreateInitPara(100);
    char *args;
    uint32_t argsSize = 1;
    acl::retr::AclFvInit aclFvInit;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetRunMode(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = aclFvInit.PrepareInput(initPara, args, argsSize);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_EQ(aclfvDestroyInitPara(initPara), ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Retr, DelExecuteTest)
{
    uint32_t id0Min = 1;
    uint32_t id0Max = 2;
    uint32_t id1Min = 1;
    uint32_t id1Max = 2;
    aclfvRepoRange *repoRange = aclfvCreateRepoRange(id0Min, id0Max, id1Min, id1Max);
    aclrtStream stream;
    const char *args;
    uint32_t argsSize;
    acl::retr::AclFvRepo aclFvRepo;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(ACL_SUCCESS));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclFvRepo.DelExecute(repoRange, stream, args, argsSize);
    EXPECT_EQ(ret, ACL_ERROR_RT_FAILURE);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(ACL_SUCCESS));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_SUCCESS));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamWaitEvent(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclFvRepo.DelExecute(repoRange, stream, args, argsSize);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(ACL_SUCCESS))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillOnce(Return(ACL_SUCCESS))
        .WillRepeatedly(Return(1));
    ret = aclFvRepo.DelExecute(repoRange, stream, args, argsSize);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclfvDestroyRepoRange(repoRange);
}

TEST_F(UTEST_ACL_Retr, AddTaskTest)
{
    acl::retr::aclfvArgs retrArgs;
    uint32_t addFeatureNum;
    uint32_t addFeatureCount;
    aclrtStream stream;
    rtNotify_t notify;
    acl::retr::AclFvRepo aclFvRepo;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return(1));
    aclError ret = aclFvRepo.AddTask(retrArgs, addFeatureNum, addFeatureCount, stream, notify);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    char args[500];
    retrArgs.args = args;
    retrArgs.configOffset = 10;
    retrArgs.argsSize = 25;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_SUCCESS));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamWaitEvent(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    ret = aclFvRepo.AddTask(retrArgs, addFeatureNum, addFeatureCount, stream, notify);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(UTEST_ACL_Retr, DelTest)
{
    aclfvSearchType type = SEARCH_1_N;
    uint32_t id0Min = 1;
    uint32_t id0Max = 2;
    uint32_t id1Min = 1;
    uint32_t id1Max = 2;
    aclfvRepoRange *repoRange = aclfvCreateRepoRange(id0Min, id0Max, id1Min, id1Max);
    acl::retr::AclFvRepo aclFvRepo;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    aclError ret = aclFvRepo.Del(type, repoRange);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
}

TEST_F(UTEST_ACL_Retr, CopySearchToDeviceTest)
{
    aclfvSearchType type;
    uint8_t *tableData = (uint8_t*)0x1;
    aclfvQueryTable *queryTable = aclfvCreateQueryTable(1, 32768, tableData, 32768);
    uint32_t topK = 200;
    uint32_t id0Min = 1;
    uint32_t id0Max = 2;
    uint32_t id1Min = 1;
    uint32_t id1Max = 2;
    aclfvRepoRange *repoRange = aclfvCreateRepoRange(id0Min, id0Max, id1Min, id1Max);
    aclfvSearchInput *retrSearchInput = aclfvCreateSearchInput(queryTable, repoRange, topK);
    uint32_t *resultNum = (uint32_t*)0x01;
    uint32_t *id0 = (uint32_t *)0x1;
    uint32_t *id1 = (uint32_t *)0x1;
    uint32_t *resultOffset = (uint32_t *)0x1;
    float *resultDistance = (float *)0x1;
    aclfvSearchResult *searchRst = aclfvCreateSearchResult(1024, resultNum, 4096, id0, id1, resultOffset, resultDistance, 10);

    acl::retr::AclFvSearch fvSearch;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = fvSearch.CopySearchToDevice(retrSearchInput, searchRst);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(0))
        .WillRepeatedly(Return(1));
    ret = fvSearch.CopySearchToDevice(retrSearchInput, searchRst);
    EXPECT_NE(ret, ACL_SUCCESS);
    aclfvDestroyRepoRange(repoRange);
    aclfvDestroySearchInput(retrSearchInput);
    aclfvDestroySearchResult(searchRst);
    aclfvDestroyQueryTable(queryTable);
}

TEST_F(UTEST_ACL_Retr, GetSearchResultTest)
{
    uint32_t *resultNum = (uint32_t*)0x01;
    uint32_t *id0 = (uint32_t *)0x1;
    uint32_t *id1 = (uint32_t *)0x1;
    uint32_t *resultOffset = (uint32_t *)0x1;
    float *resultDistance = (float *)0x1;
    aclfvSearchResult *searchRst = aclfvCreateSearchResult(1024, resultNum, 4096, id0, id1, resultOffset, resultDistance, 10);

    acl::retr::AclFvSearch fvSearch;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpy(_, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclError ret = fvSearch.GetSearchResult(searchRst);
    EXPECT_EQ(ret, ACL_ERROR_RT_PARAM_INVALID);
    aclfvDestroySearchResult(searchRst);
}