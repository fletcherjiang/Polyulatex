
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iostream>
#include <stdio.h>
#include "securec.h"

#define protected public
#define private public
#include "acl/ops/acl_dvpp.h"
#include "single_op/op_executor.h"
#include "single_op/dvpp/common/dvpp_def_internal.h"
#include "single_op/dvpp/mgr/dvpp_manager.h"
#undef private
#undef protected

#include "acl/acl.h"
#include "runtime/rt.h"
#include "acl_stub.h"

using namespace std;
using namespace testing;
using namespace acl;
using namespace acl::dvpp;

class PngTest : public testing::Test {
protected:

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

TEST_F(PngTest, TestPngDecode100)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    aclrtStream stream;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.notify = (void*)0x1;
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    inputDesc.dvppPicDesc.size = 2 * 1024;
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_RGBA_8888;
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aicpuVersion_ = 1;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    EXPECT_NE(acldvppPngDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream), ACL_SUCCESS);

    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_RGB_888;
    EXPECT_EQ(acldvppPngDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream), ACL_SUCCESS);

    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_RGBA_8888;
    EXPECT_NE(acldvppPngDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream), ACL_SUCCESS);

    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_RGB_888;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    EXPECT_NE(acldvppPngDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillRepeatedly(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppPngDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillRepeatedly(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(
        acldvppPngDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppPngDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    EXPECT_NE(acldvppPngDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillRepeatedly(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppPngDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream), ACL_SUCCESS);
}

TEST_F(PngTest, TestGetPngWidthHeightAndComponents100)
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t size = 30;
    int32_t components = 0;
    char data[30] = {(char)0x89, (char)0x50, (char)0x4e, (char)0x47, (char)0x0d, (char)0x0a, (char)0x1a, (char)0x0a, (char)0x0,
        (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0,
        (char)0x1, (char)0x0, (char)0x0, (char)0x0, (char)0x1, (char)0x0, (char)0x0, (char)0x2};
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aicpuVersion_ = 1;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    aclError ret = acldvppPngGetImageInfo(data, size, &width, &height, &components);
    EXPECT_EQ(ret, ACL_SUCCESS);
    size = 26;
    ret = acldvppPngGetImageInfo(data, size, &width, &height, &components);
    EXPECT_NE(ret, ACL_SUCCESS);

    char data1[30];
    ret = acldvppPngGetImageInfo(data1, 30, &width, &height, &components);
    EXPECT_NE(ret, ACL_SUCCESS);

    char data2[30] = {(char)0x89, (char)0x50, (char)0x4e, (char)0x47, (char)0x0d, (char)0x0a, (char)0x1a, (char)0x0a, (char)0x0,
        (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0,
        (char)0x1, (char)0x0, (char)0x0, (char)0x0, (char)0x1, (char)0x0, (char)0x0, (char)0x6};
    ret = acldvppPngGetImageInfo(data2, 30, &width, &height, &components);
    EXPECT_EQ(ret, ACL_SUCCESS);

    char data3[30] = {(char)0x89, (char)0x50, (char)0x4e, (char)0x47, (char)0x0d, (char)0x0a, (char)0x1a, (char)0x0a, (char)0x0,
        (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0,
        (char)0x1, (char)0x0, (char)0x0, (char)0x0, (char)0x1, (char)0x0, (char)0x0, (char)0x8};
    ret = acldvppPngGetImageInfo(data3, 30, &width, &height, &components);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(PngTest, TestGetPngImgInfo100)
{
    uint32_t width = 0;
    uint32_t dataSize = 30;
    uint32_t height = 0;
    uint32_t size = 30;
    int32_t components = 1;
    char data1[30] = {(char)0x89, (char)0x50, (char)0x4e, (char)0x47, (char)0x0d, (char)0x0a, (char)0x1a, (char)0x0a, (char)0x0,
        (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0,
        (char)0x1, (char)0x0, (char)0x0, (char)0x0, (char)0x1, (char)0x0, (char)0x0, (char)0x0};

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aicpuVersion_ = 1;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();
    auto *imagePtr = dvppManager.GetImageProcessor();
    aclError ret = acldvppPngGetImageInfo(data1, dataSize, &width, &height, &components);
    EXPECT_EQ(ret, ACL_SUCCESS);

    char data2[30] = {(char)0x89, (char)0x50, (char)0x4e, (char)0x47, (char)0x0d, (char)0x0a, (char)0x1a, (char)0x0a, (char)0x0,
        (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0,
        (char)0x1, (char)0x0, (char)0x0, (char)0x0, (char)0x1, (char)0x0, (char)0x0, (char)0x4};
    ret = acldvppPngGetImageInfo(data2, dataSize, &width, &height, &components);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(PngTest, TestPredictPngDecSize100)
{
    char data[30] = {(char)0x89, (char)0x50, (char)0x4e, (char)0x47, (char)0x0d, (char)0x0a, (char)0x1a, (char)0x0a, (char)0x0,
        (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0, (char)0x0,
        (char)0x1, (char)0x0, (char)0x0, (char)0x0, (char)0x1, (char)0x0, (char)0x1, (char)0x2};
    uint32_t size = 0;
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aicpuVersion_ = 1;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    EXPECT_EQ(acldvppPngPredictDecSize(data, sizeof(data), PIXEL_FORMAT_UNKNOWN, &size), ACL_SUCCESS);
    EXPECT_NE(acldvppPngPredictDecSize(data, sizeof(data), PIXEL_FORMAT_BGR_888, &size), ACL_SUCCESS);
    EXPECT_EQ(acldvppPngPredictDecSize(data, sizeof(data), PIXEL_FORMAT_RGB_888, &size), ACL_SUCCESS);
    EXPECT_EQ(acldvppPngPredictDecSize(data, sizeof(data), PIXEL_FORMAT_RGBA_8888, &size), ACL_SUCCESS);
}

TEST_F(PngTest, TestPngDecode200)
{
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aicpuVersion_ = 1;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    EXPECT_EQ(acldvppPngDecodeAsync(nullptr, nullptr, 0, nullptr, nullptr), ACL_ERROR_FEATURE_UNSUPPORTED);
}

TEST_F(PngTest, TestGetPngWidthHeightAndComponents200)
{
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aicpuVersion_ = 1;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    EXPECT_EQ(acldvppPngGetImageInfo(nullptr, 0, nullptr, nullptr, nullptr), ACL_ERROR_FEATURE_UNSUPPORTED);
}

TEST_F(PngTest, TestPredictPngDecSize200)
{
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aicpuVersion_ = 1;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();
    EXPECT_EQ(acldvppPngPredictDecSize(nullptr, 0, PIXEL_FORMAT_RGBA_8888, nullptr), ACL_ERROR_FEATURE_UNSUPPORTED);
}

TEST_F(PngTest, TestPngDecodeAicpu0)
{
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.aicpuVersion_ = 0;
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();
    EXPECT_EQ(acldvppPngDecodeAsync(nullptr, nullptr, 0, nullptr, nullptr), ACL_ERROR_RESOURCE_NOT_MATCH);
}

TEST_F(PngTest, TestGetPngWidthHeightAndComponentsAicpu0)
{
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.aicpuVersion_ = 0;
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();
    EXPECT_EQ(acldvppPngGetImageInfo(nullptr, 0, nullptr, nullptr, nullptr), ACL_ERROR_RESOURCE_NOT_MATCH);
}

TEST_F(PngTest, TestPredictPngDecSizeAicpu0)
{
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.aicpuVersion_ = 0;
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();
    EXPECT_EQ(acldvppPngPredictDecSize(nullptr, 0, PIXEL_FORMAT_RGBA_8888, nullptr), ACL_ERROR_RESOURCE_NOT_MATCH);
}

TEST_F(PngTest, GetDvppKernelVersion)
{
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    dvppManager.GetDvppKernelVersion();
}