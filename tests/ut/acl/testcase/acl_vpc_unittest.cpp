#include <vector>
#include <iostream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define protected public
#define private public
#include "acl/ops/acl_dvpp.h"
#include "single_op/op_executor.h"
#include "single_op/dvpp/common/dvpp_util.h"
#include "single_op/dvpp/mgr/dvpp_manager.h"
#include "single_op/dvpp/base/image_processor.h"
#include "single_op/dvpp/v200/image_processor_v200.h"
#include "single_op/dvpp/common/dvpp_def_internal.h"
#undef private
#undef protected

#include "acl/acl.h"
#include "runtime/rt.h"
#include "aicpu/dvpp/dvpp_def.h"
#include "acl/ops/acl_dvpp.h"
#include "acl_stub.h"

using namespace std;
using namespace testing;
using namespace acl;
using namespace acl::dvpp;

class VpcTest : public testing::Test {
protected:

    void SetUp() { }

    void TearDown()
    {
        Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
    }
};

TEST_F(VpcTest, TestDvppVpcResize_paramcheck)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    acldvppResizeConfig resizeConfig;
    aclrtStream stream;
    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    acldvppSetPicDescWidth(&inputDesc, 5000);
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    EXPECT_NE(acldvppVpcResizeAsync(nullptr, &inputDesc, &outputDesc,
                                    &resizeConfig, stream), ACL_SUCCESS);

    channelDesc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppVpcResizeAsync(&channelDesc, &inputDesc, &outputDesc,
                                    &resizeConfig, stream), ACL_SUCCESS);
    channelDesc.dataBuffer.data = (void*)0x1;

    inputDesc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppVpcResizeAsync(&channelDesc, &inputDesc, &outputDesc,
                                    &resizeConfig, stream), ACL_SUCCESS);
    inputDesc.dataBuffer.data = (void*)0x1;

    outputDesc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppVpcResizeAsync(&channelDesc, &inputDesc, &outputDesc,
                                    &resizeConfig, stream), ACL_SUCCESS);
    outputDesc.dataBuffer.data = (void*)0x1;

    // input format error
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcResizeAsync(&channelDesc, &inputDesc, &outputDesc,
                                    &resizeConfig, stream), ACL_SUCCESS);
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    // output format error
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcResizeAsync(&channelDesc, &inputDesc, &outputDesc,
                                    &resizeConfig, stream), ACL_SUCCESS);
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    channelDesc.dvppWaitTaskType = EVENT_TASK;
    EXPECT_NE(acldvppVpcResizeAsync(&channelDesc, &inputDesc, &outputDesc,
                                    &resizeConfig, stream), ACL_SUCCESS);
}

TEST_F(VpcTest, TestDvppVpcResize_cpy_input_failed)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    acldvppResizeConfig resizeConfig;
    aclrtStream stream;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    channelDesc.notify = (void*)0x1;
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    inputDesc.dvppPicDesc.format = 100;
    outputDesc.dvppPicDesc.format = 100;
    EXPECT_NE(acldvppVpcResizeAsync(&channelDesc, &inputDesc, &outputDesc,
                                    &resizeConfig, stream), ACL_SUCCESS);

    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    EXPECT_EQ(acldvppVpcResizeAsync(&channelDesc, &inputDesc, &outputDesc,
                                    &resizeConfig, stream), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppVpcResizeAsync(&channelDesc, &inputDesc, &outputDesc,
                                    &resizeConfig, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppVpcResizeAsync(&channelDesc, &inputDesc, &outputDesc, &resizeConfig, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    acldvppSetPicDescWidth(&inputDesc, 5000);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppVpcResizeAsync(&channelDesc, &inputDesc, &outputDesc, &resizeConfig, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppVpcResizeAsync(&channelDesc, &inputDesc, &outputDesc, &resizeConfig, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillOnce(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    EXPECT_NE(acldvppVpcResizeAsync(&channelDesc, &inputDesc, &outputDesc,
                                    &resizeConfig, stream), ACL_SUCCESS);
}

TEST_F(VpcTest, TestDvppVpcCrop_param_check)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    acldvppRoiConfig  roiConfig;
    aclrtStream stream;
    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    EXPECT_NE(acldvppVpcCropAsync(nullptr, &inputDesc, &outputDesc,
                                  &roiConfig, stream), ACL_SUCCESS);

    channelDesc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppVpcCropAsync(&channelDesc, &inputDesc, &outputDesc,
                                  &roiConfig, stream), ACL_SUCCESS);
    channelDesc.dataBuffer.data = (void*)0x1;

    inputDesc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppVpcCropAsync(&channelDesc, &inputDesc, &outputDesc,
                                  &roiConfig, stream), ACL_SUCCESS);
    inputDesc.dataBuffer.data = (void*)0x1;

    outputDesc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppVpcCropAsync(&channelDesc, &inputDesc, &outputDesc,
                                  &roiConfig, stream), ACL_SUCCESS);
    outputDesc.dataBuffer.data = (void*)0x1;

    // input format error
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcCropAsync(&channelDesc, &inputDesc, &outputDesc,
                                  &roiConfig, stream), ACL_SUCCESS);
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    // output format error
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcCropAsync(&channelDesc, &inputDesc, &outputDesc,
                                  &roiConfig, stream), ACL_SUCCESS);
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
}

TEST_F(VpcTest, TestDvppVpcCrop)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    acldvppRoiConfig  roiConfig;
    aclrtStream stream;
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);

    channelDesc.notify = (void*)0x1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    EXPECT_EQ(acldvppVpcCropAsync(&channelDesc, &inputDesc, &outputDesc,
                                  &roiConfig, stream), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppVpcCropAsync(&channelDesc, &inputDesc, &outputDesc,
                                  &roiConfig, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppVpcCropAsync(&channelDesc, &inputDesc, &outputDesc, &roiConfig, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppVpcCropAsync(&channelDesc, &inputDesc, &outputDesc, &roiConfig, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppVpcCropAsync(&channelDesc, &inputDesc, &outputDesc, &roiConfig, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillOnce(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    EXPECT_NE(acldvppVpcCropAsync(&channelDesc, &inputDesc, &outputDesc,
                                 &roiConfig, stream), ACL_SUCCESS);
}

TEST_F(VpcTest, TestDvppVpcCropAndPaste_param_check)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    acldvppRoiConfig cropArea;
    acldvppRoiConfig pasteArea;
    aclrtStream stream;
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);

    EXPECT_NE(acldvppVpcCropAndPasteAsync(nullptr, &inputDesc, &outputDesc,
                                          &cropArea, &pasteArea, stream), ACL_SUCCESS);

    channelDesc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppVpcCropAndPasteAsync(&channelDesc, &inputDesc, &outputDesc,
                                          &cropArea, &pasteArea, stream), ACL_SUCCESS);
    channelDesc.dataBuffer.data = (void*)0x1;

    inputDesc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppVpcCropAndPasteAsync(&channelDesc, &inputDesc, &outputDesc,
                                          &cropArea, &pasteArea, stream), ACL_SUCCESS);
    inputDesc.dataBuffer.data = (void*)0x1;

    outputDesc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppVpcCropAndPasteAsync(&channelDesc, &inputDesc, &outputDesc,
                                          &cropArea, &pasteArea, stream), ACL_SUCCESS);
    outputDesc.dataBuffer.data = (void*)0x1;

    // input format error
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcCropAndPasteAsync(&channelDesc, &inputDesc, &outputDesc,
                                          &cropArea, &pasteArea, stream), ACL_SUCCESS);
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    // output format error
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcCropAndPasteAsync(&channelDesc, &inputDesc, &outputDesc,
                                          &cropArea, &pasteArea, stream), ACL_SUCCESS);
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
}

TEST_F(VpcTest, TestDvppVpcCropAndPaste)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    acldvppRoiConfig cropArea;
    acldvppRoiConfig pasteArea;
    aclrtStream stream;
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    channelDesc.notify = (void*)0x1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    EXPECT_EQ(acldvppVpcCropAndPasteAsync(&channelDesc, &inputDesc, &outputDesc,
                                          &cropArea, &pasteArea, stream), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppVpcCropAndPasteAsync(&channelDesc, &inputDesc, &outputDesc,
                                          &cropArea, &pasteArea, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppVpcCropAndPasteAsync(&channelDesc, &inputDesc, &outputDesc, &cropArea, &pasteArea, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppVpcCropAndPasteAsync(&channelDesc, &inputDesc, &outputDesc, &cropArea, &pasteArea, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppVpcCropAndPasteAsync(&channelDesc, &inputDesc, &outputDesc, &cropArea, &pasteArea, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillOnce(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    EXPECT_NE(acldvppVpcCropAndPasteAsync(&channelDesc, &inputDesc, &outputDesc,
            &cropArea, &pasteArea, stream), ACL_SUCCESS);
}

TEST_F(VpcTest, TestDvppVpcConvertColor)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    aclrtStream stream;
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);

    inputDesc.dvppPicDesc.width = 4096;
    inputDesc.dvppPicDesc.height = 4096;
    inputDesc.dvppPicDesc.widthStride = 4096;
    inputDesc.dvppPicDesc.heightStride = 4096;

    outputDesc.dvppPicDesc.width = 0;
    outputDesc.dvppPicDesc.height = 0;
    outputDesc.dvppPicDesc.widthStride = 0;
    outputDesc.dvppPicDesc.heightStride = 0;

    channelDesc.notify = (void*)0x1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();
    EXPECT_EQ(acldvppVpcConvertColorAsync(&channelDesc, &inputDesc,
        &outputDesc, stream), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppVpcConvertColorAsync(&channelDesc, &inputDesc,
        &outputDesc, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppVpcConvertColorAsync(&channelDesc, &inputDesc,
            &outputDesc, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppVpcConvertColorAsync(&channelDesc, &inputDesc,
            &outputDesc, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    dvppManager.InitDvppProcessor();
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppVpcConvertColorAsync(&channelDesc, &inputDesc,
            &outputDesc, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillOnce(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    EXPECT_NE(acldvppVpcConvertColorAsync(&channelDesc, &inputDesc,
        &outputDesc, stream), ACL_SUCCESS);
}

TEST_F(VpcTest, TestDvppVpcConvertColor_param_check)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    aclrtStream stream;
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);

    inputDesc.dvppPicDesc.width = 4096;
    inputDesc.dvppPicDesc.height = 4096;
    inputDesc.dvppPicDesc.widthStride = 4096;
    inputDesc.dvppPicDesc.heightStride = 4096;

    outputDesc.dvppPicDesc.width = 0;
    outputDesc.dvppPicDesc.height = 0;
    outputDesc.dvppPicDesc.widthStride = 0;
    outputDesc.dvppPicDesc.heightStride = 0;

    channelDesc.notify = (void*)0x1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();
    EXPECT_NE(acldvppVpcConvertColorAsync(&channelDesc, &inputDesc,
        &outputDesc, stream), ACL_SUCCESS);

    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcConvertColorAsync(&channelDesc, &inputDesc,
        &outputDesc, stream), ACL_SUCCESS);

    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcConvertColorAsync(&channelDesc, &inputDesc,
        &outputDesc, stream), ACL_SUCCESS);

    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.width = 100;
    outputDesc.dvppPicDesc.height = 200;
    EXPECT_NE(acldvppVpcConvertColorAsync(&channelDesc, &inputDesc,
        &outputDesc, stream), ACL_SUCCESS);

    inputDesc.dvppPicDesc.width = 0;
    EXPECT_NE(acldvppVpcConvertColorAsync(&channelDesc, &inputDesc,
        &outputDesc, stream), ACL_SUCCESS);

    inputDesc.dvppPicDesc.width = 4096;
    outputDesc.dvppPicDesc.width = 4096;
    outputDesc.dvppPicDesc.height = 4096;
    EXPECT_EQ(acldvppVpcConvertColorAsync(&channelDesc, &inputDesc,
        &outputDesc, stream), ACL_SUCCESS);
}

TEST_F(VpcTest, TestDvppPyrDown)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    aclrtStream stream;
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_400;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_400;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);

    inputDesc.dvppPicDesc.width = 2048;
    inputDesc.dvppPicDesc.height = 2048;
    inputDesc.dvppPicDesc.widthStride = 2048;
    inputDesc.dvppPicDesc.heightStride = 2048;

    outputDesc.dvppPicDesc.width = 1024;
    outputDesc.dvppPicDesc.height = 1024;
    outputDesc.dvppPicDesc.widthStride = 0;
    outputDesc.dvppPicDesc.heightStride = 0;

    channelDesc.notify = (void*)0x1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    EXPECT_EQ(acldvppVpcPyrDownAsync(&channelDesc, &inputDesc,
        &outputDesc, nullptr, stream), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppVpcPyrDownAsync(&channelDesc, &inputDesc,
        &outputDesc, nullptr, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(
        acldvppVpcPyrDownAsync(&channelDesc, &inputDesc,
            &outputDesc, nullptr, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppVpcPyrDownAsync(&channelDesc, &inputDesc,
            &outputDesc, nullptr, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppVpcPyrDownAsync(&channelDesc, &inputDesc,
            &outputDesc, nullptr, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillOnce(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    EXPECT_NE(acldvppVpcPyrDownAsync(&channelDesc, &inputDesc,
        &outputDesc, nullptr, stream), ACL_SUCCESS);
}

TEST_F(VpcTest, TestDvppVpcPyrDown_param_check)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    aclrtStream stream;
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_400;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_400;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);

    inputDesc.dvppPicDesc.width = 2048;
    inputDesc.dvppPicDesc.height = 2048;
    inputDesc.dvppPicDesc.widthStride = 2048;
    inputDesc.dvppPicDesc.heightStride = 2048;

    outputDesc.dvppPicDesc.width = 1024;
    outputDesc.dvppPicDesc.height = 1024;
    outputDesc.dvppPicDesc.widthStride = 0;
    outputDesc.dvppPicDesc.heightStride = 0;

    channelDesc.notify = (void*)0x1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.InitDvppProcessor();
    EXPECT_NE(acldvppVpcPyrDownAsync(&channelDesc, &inputDesc,
        &outputDesc, nullptr, stream), ACL_SUCCESS);

    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.InitDvppProcessor();
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcPyrDownAsync(&channelDesc, &inputDesc,
        &outputDesc, nullptr, stream), ACL_SUCCESS);

    EXPECT_NE(acldvppVpcPyrDownAsync(&channelDesc, &inputDesc,
        &outputDesc, (void*)0x1, stream), ACL_SUCCESS);
}

void* mmAlignMallocStub2(mmSize mallocSize, mmSize alignSize)
{
    mallocSize = 80;
    return malloc(mallocSize);
}

TEST_F(VpcTest, TestDvppVpcBatchCrop_param_check)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub2));
    acldvppChannelDesc channelDesc;
    acldvppBatchPicDesc *inputBatch = acldvppCreateBatchPicDesc(1);
    acldvppBatchPicDesc *outputBatch = acldvppCreateBatchPicDesc(1);

    acldvppPicDesc *inputDesc = acldvppGetPicDesc(inputBatch, 0);
    acldvppPicDesc *outputDesc = acldvppGetPicDesc(outputBatch, 0);
    acldvppRoiConfig  *roiConfig =  acldvppCreateRoiConfig(1, 1, 1, 1);

    aclrtStream stream;
    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc->dataBuffer.data = (void*)0x1;
    inputDesc->dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc->dataBuffer.data = (void*)0x1;
    outputDesc->dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    inputDesc->dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc->dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    uint32_t *roiNums = new uint32_t[1];
    roiNums[0] = 1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    EXPECT_NE(acldvppVpcBatchCropAsync(nullptr, inputBatch, roiNums, 1, outputBatch,
                                       &roiConfig, stream), ACL_SUCCESS);

    channelDesc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, stream), ACL_SUCCESS);
    channelDesc.dataBuffer.data = (void*)0x1;

    inputDesc->dataBuffer.data = nullptr;
    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, stream), ACL_SUCCESS);
    inputDesc->dataBuffer.data = (void*)0x1;

    outputDesc->dataBuffer.data = nullptr;
    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                       &roiConfig, stream), ACL_SUCCESS);
    outputDesc->dataBuffer.data = (void*)0x1;

    // input format error
    inputDesc->dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, stream), ACL_SUCCESS);
    inputDesc->dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    // output format error
    outputDesc->dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, stream), ACL_SUCCESS);

    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 0, outputBatch,
                                        &roiConfig, stream), ACL_SUCCESS);

    outputDesc->dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 2, outputBatch,
                                        &roiConfig, stream), ACL_SUCCESS);

    roiNums[0] = 0;
    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, stream), ACL_SUCCESS);

    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.InitDvppProcessor();
    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 2, outputBatch,
                                        &roiConfig, stream), ACL_SUCCESS);

    acldvppDestroyRoiConfig(roiConfig);
    acldvppDestroyBatchPicDesc(inputBatch);
    acldvppDestroyBatchPicDesc(outputBatch);
    delete[] roiNums;
}


TEST_F(VpcTest, TestDvppVpcBatchCrop)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub2));

    acldvppChannelDesc channelDesc;

    acldvppBatchPicDesc *inputBatch = acldvppCreateBatchPicDesc(1);
    acldvppBatchPicDesc *outputBatch = acldvppCreateBatchPicDesc(1);

    acldvppPicDesc *inputDesc = acldvppGetPicDesc(inputBatch, 0);
    acldvppPicDesc *outputDesc = acldvppGetPicDesc(outputBatch, 0);
    acldvppRoiConfig  *roiConfig =  acldvppCreateRoiConfig(1, 1, 1, 1);
    aclrtStream stream;
    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc->dataBuffer.data = (void*)0x1;
    inputDesc->dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc->dataBuffer.data = (void*)0x1;
    outputDesc->dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    inputDesc->dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc->dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    uint32_t *roiNums = new uint32_t[1];
    roiNums[0] = 1;

    channelDesc.notify = (void*)0x1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));


    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillOnce(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, stream), ACL_SUCCESS);

    dvppManager.aclRunMode_ = ACL_DEVICE;
    dvppManager.InitDvppProcessor();
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillOnce(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, stream), ACL_SUCCESS);

    roiNums[0] = 2;
    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 2, outputBatch,
                                        &roiConfig, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_NE(acldvppVpcBatchCropAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    acldvppDestroyRoiConfig(roiConfig);
    acldvppDestroyBatchPicDesc(inputBatch);
    acldvppDestroyBatchPicDesc(outputBatch);
    delete[] roiNums;
}

TEST_F(VpcTest, TestDvppVpcBatchCropAndPaste_param_check)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub2));
    acldvppChannelDesc channelDesc;

    acldvppBatchPicDesc *inputBatch = acldvppCreateBatchPicDesc(1);
    acldvppBatchPicDesc *outputBatch = acldvppCreateBatchPicDesc(1);

    acldvppPicDesc *inputDesc = acldvppGetPicDesc(inputBatch, 0);
    acldvppPicDesc *outputDesc = acldvppGetPicDesc(outputBatch, 0);
    acldvppRoiConfig  *roiConfig =  acldvppCreateRoiConfig(1, 1, 1, 1);
    acldvppRoiConfig  *pasteRoiConfig =  acldvppCreateRoiConfig(1, 1, 1, 1);

    aclrtStream stream;
    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);

    inputDesc->dataBuffer.data = (void*)0x1;
    inputDesc->dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);

    outputDesc->dataBuffer.data = (void*)0x1;
    outputDesc->dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);

    inputDesc->dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc->dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    uint32_t *roiNums = new uint32_t[1];
    roiNums[0] = 1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    EXPECT_NE(acldvppVpcBatchCropAndPasteAsync(nullptr, inputBatch, roiNums, 1, outputBatch,
                                       &roiConfig, &pasteRoiConfig, stream), ACL_SUCCESS);

    EXPECT_NE(acldvppVpcBatchCropAndPasteAsync(&channelDesc, inputBatch, roiNums, 0, outputBatch,
                                        &roiConfig, &pasteRoiConfig, stream), ACL_SUCCESS);

    EXPECT_NE(acldvppVpcBatchCropAndPasteAsync(&channelDesc, inputBatch, roiNums, 2, outputBatch,
                                        &roiConfig, &pasteRoiConfig, stream), ACL_SUCCESS);

    channelDesc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppVpcBatchCropAndPasteAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, &pasteRoiConfig, stream), ACL_SUCCESS);
    channelDesc.dataBuffer.data = (void*)0x1;

    inputDesc->dataBuffer.data = nullptr;
    EXPECT_NE(acldvppVpcBatchCropAndPasteAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, &pasteRoiConfig, stream), ACL_SUCCESS);
    inputDesc->dataBuffer.data = (void*)0x1;

    outputDesc->dataBuffer.data = nullptr;
    EXPECT_NE(acldvppVpcBatchCropAndPasteAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                       &roiConfig, &pasteRoiConfig, stream), ACL_SUCCESS);
    outputDesc->dataBuffer.data = (void*)0x1;

    // input format error
    inputDesc->dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcBatchCropAndPasteAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, &pasteRoiConfig, stream), ACL_SUCCESS);

    inputDesc->dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    // output format error
    outputDesc->dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcBatchCropAndPasteAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, &pasteRoiConfig, stream), ACL_SUCCESS);

    roiNums[0] = 0;
    EXPECT_NE(acldvppVpcBatchCropAndPasteAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, &pasteRoiConfig, stream), ACL_SUCCESS);


    acldvppDestroyRoiConfig(roiConfig);
    acldvppDestroyRoiConfig(pasteRoiConfig);
    acldvppDestroyBatchPicDesc(inputBatch);
    acldvppDestroyBatchPicDesc(outputBatch);
    delete[] roiNums;
}

TEST_F(VpcTest, TestDvppVpcBatchCropAndPaste)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub2));
    acldvppChannelDesc channelDesc;

    acldvppBatchPicDesc *inputBatch = acldvppCreateBatchPicDesc(1);
    acldvppBatchPicDesc *outputBatch = acldvppCreateBatchPicDesc(1);

    acldvppPicDesc *inputDesc = acldvppGetPicDesc(inputBatch, 0);
    acldvppPicDesc *outputDesc = acldvppGetPicDesc(outputBatch, 0);
    acldvppRoiConfig  *roiConfig =  acldvppCreateRoiConfig(1, 1, 1, 1);
    acldvppRoiConfig  *pasteRoiConfig =  acldvppCreateRoiConfig(1, 1, 1, 1);

    aclrtStream stream;
    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);

    inputDesc->dataBuffer.data = (void*)0x1;
    inputDesc->dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);

    outputDesc->dataBuffer.data = (void*)0x1;
    outputDesc->dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);

    inputDesc->dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc->dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    uint32_t *roiNums = new uint32_t[1];
    roiNums[0] = 1;
    channelDesc.notify = (void*)0x1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillRepeatedly(Return(RT_ERROR_NONE));
    EXPECT_EQ(acldvppVpcBatchCropAndPasteAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, &pasteRoiConfig, stream), ACL_ERROR_RT_PARAM_INVALID);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_EQ(acldvppVpcBatchCropAndPasteAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, &pasteRoiConfig, stream), ACL_ERROR_RT_PARAM_INVALID);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_EQ(acldvppVpcBatchCropAndPasteAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, &pasteRoiConfig, stream), ACL_ERROR_RT_PARAM_INVALID);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_EQ(acldvppVpcBatchCropAndPasteAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, &pasteRoiConfig, stream), ACL_ERROR_RT_PARAM_INVALID);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillOnce(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT))
        .WillOnce(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    EXPECT_EQ(acldvppVpcBatchCropAndPasteAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, &pasteRoiConfig, stream), ACL_ERROR_RT_FEATURE_NOT_SUPPORT);

    // run on device
    dvppManager.aclRunMode_ = ACL_DEVICE;
    dvppManager.InitDvppProcessor();
    EXPECT_EQ(acldvppVpcBatchCropAndPasteAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                        &roiConfig, &pasteRoiConfig, stream), ACL_ERROR_RT_FEATURE_NOT_SUPPORT);

    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.InitDvppProcessor();
    EXPECT_NE(acldvppVpcBatchCropAndPasteAsync(&channelDesc, inputBatch, roiNums, 2, outputBatch,
                                        &roiConfig, &pasteRoiConfig, stream), ACL_ERROR_RT_FEATURE_NOT_SUPPORT);

    acldvppDestroyRoiConfig(pasteRoiConfig);
    acldvppDestroyRoiConfig(roiConfig);
    acldvppDestroyBatchPicDesc(inputBatch);
    acldvppDestroyBatchPicDesc(outputBatch);

    delete[] roiNums;
}

TEST_F(VpcTest, TestDvppEqualizeHist)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    acldvppLutMap lutMap;
    aclrtStream stream;
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_400;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_400;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);

    uint8_t *tempPtr = new uint8_t[3];
    lutMap.dvppLutMap.map = tempPtr;
    lutMap.dvppLutMap.lens[0] = 1;
    lutMap.dvppLutMap.lens[1] = 1;
    lutMap.dvppLutMap.lens[2] = 1;

    inputDesc.dvppPicDesc.width = 2048;
    inputDesc.dvppPicDesc.height = 2048;
    inputDesc.dvppPicDesc.widthStride = 2048;
    inputDesc.dvppPicDesc.heightStride = 2048;

    outputDesc.dvppPicDesc.width = 2048;
    outputDesc.dvppPicDesc.height = 2048;
    outputDesc.dvppPicDesc.widthStride = 2048;
    outputDesc.dvppPicDesc.heightStride = 2048;

    channelDesc.notify = (void*)0x1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    EXPECT_EQ(acldvppVpcEqualizeHistAsync(&channelDesc, &inputDesc,
        &outputDesc, &lutMap, stream), ACL_SUCCESS);

     EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppVpcEqualizeHistAsync(&channelDesc, &inputDesc,
        &outputDesc, &lutMap, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(
        acldvppVpcEqualizeHistAsync(&channelDesc, &inputDesc,
            &outputDesc, &lutMap, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(
        acldvppVpcEqualizeHistAsync(&channelDesc, &inputDesc,
            &outputDesc, &lutMap, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(
        acldvppVpcEqualizeHistAsync(&channelDesc, &inputDesc,
            &outputDesc, &lutMap, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillOnce(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    EXPECT_NE(acldvppVpcEqualizeHistAsync(&channelDesc, &inputDesc,
        &outputDesc, &lutMap, stream), ACL_SUCCESS);
    delete[] tempPtr;
}

TEST_F(VpcTest, TestDvppEqualizeHist_param_check)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    acldvppLutMap lutMap;
    aclrtStream stream;
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_400;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_400;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);

    uint8_t *tempPtr = new uint8_t[3];
    lutMap.dvppLutMap.map = tempPtr;
    lutMap.dvppLutMap.lens[0] = 1;
    lutMap.dvppLutMap.lens[1] = 1;
    lutMap.dvppLutMap.lens[2] = 1;

    inputDesc.dvppPicDesc.width = 2048;
    inputDesc.dvppPicDesc.height = 2048;
    inputDesc.dvppPicDesc.widthStride = 2048;
    inputDesc.dvppPicDesc.heightStride = 2048;

    outputDesc.dvppPicDesc.width = 2048;
    outputDesc.dvppPicDesc.height = 2048;
    outputDesc.dvppPicDesc.widthStride = 2048;
    outputDesc.dvppPicDesc.heightStride = 2048;

    channelDesc.notify = (void*)0x1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.InitDvppProcessor();
    EXPECT_NE(acldvppVpcEqualizeHistAsync(&channelDesc, &inputDesc,
        &outputDesc, &lutMap, stream), ACL_SUCCESS);

    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.InitDvppProcessor();
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcEqualizeHistAsync(&channelDesc, &inputDesc,
        &outputDesc, &lutMap, stream), ACL_SUCCESS);

    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.width = 1024;
    EXPECT_NE(acldvppVpcEqualizeHistAsync(&channelDesc, &inputDesc,
        &outputDesc, &lutMap, stream), ACL_SUCCESS);

    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.width = 1024;
    EXPECT_NE(acldvppVpcEqualizeHistAsync(&channelDesc, &inputDesc,
        &outputDesc, &lutMap, stream), ACL_SUCCESS);

    delete[] tempPtr;
}


TEST_F(VpcTest, TestDvppMakeBorder)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    acldvppBorderConfig borderConfig;
    aclrtStream stream;
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);

    inputDesc.dvppPicDesc.width = 2048;
    inputDesc.dvppPicDesc.height = 2048;
    inputDesc.dvppPicDesc.widthStride = 2048;
    inputDesc.dvppPicDesc.heightStride = 2048;

    outputDesc.dvppPicDesc.width = 2048;
    outputDesc.dvppPicDesc.height = 2048;
    outputDesc.dvppPicDesc.widthStride = 2048;
    outputDesc.dvppPicDesc.heightStride = 2048;

    channelDesc.notify = (void*)0x1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    EXPECT_EQ(acldvppVpcMakeBorderAsync(&channelDesc, &inputDesc,
        &outputDesc, &borderConfig, stream), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppVpcMakeBorderAsync(&channelDesc, &inputDesc,
        &outputDesc, &borderConfig, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(
        acldvppVpcMakeBorderAsync(&channelDesc, &inputDesc,
            &outputDesc, &borderConfig, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppVpcMakeBorderAsync(&channelDesc, &inputDesc,
            &outputDesc, &borderConfig, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
        acldvppVpcMakeBorderAsync(&channelDesc, &inputDesc,
            &outputDesc, &borderConfig, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillOnce(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    EXPECT_NE(acldvppVpcMakeBorderAsync(&channelDesc, &inputDesc,
        &outputDesc, &borderConfig, stream), ACL_SUCCESS);
}

TEST_F(VpcTest, TestDvppMakeBorder_param_check)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    acldvppBorderConfig borderConfig;
    aclrtStream stream;
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);

    inputDesc.dvppPicDesc.width = 2048;
    inputDesc.dvppPicDesc.height = 2048;
    inputDesc.dvppPicDesc.widthStride = 2048;
    inputDesc.dvppPicDesc.heightStride = 2048;

    outputDesc.dvppPicDesc.width = 2048;
    outputDesc.dvppPicDesc.height = 2048;
    outputDesc.dvppPicDesc.widthStride = 2048;
    outputDesc.dvppPicDesc.heightStride = 2048;

    channelDesc.notify = (void*)0x1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.InitDvppProcessor();
    EXPECT_NE(acldvppVpcMakeBorderAsync(&channelDesc, &inputDesc,
        &outputDesc, &borderConfig, stream), ACL_SUCCESS);

    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.InitDvppProcessor();
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcMakeBorderAsync(&channelDesc, &inputDesc,
        &outputDesc, &borderConfig, stream), ACL_SUCCESS);

    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcMakeBorderAsync(&channelDesc, &inputDesc,
        &outputDesc, &borderConfig, stream), ACL_SUCCESS);
}

TEST_F(VpcTest, TestDvppGetVersion_param_check)
{
   acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
   dvppManager.dvppVersion_ = DVPP_KERNELS_UNKOWN;
   dvppManager.InitDvppProcessor();

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    acl::dvpp::DvppManager::GetInstance();
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    acl::dvpp::DvppManager::GetInstance();
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    acl::dvpp::DvppManager::GetInstance();
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));


    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    acl::dvpp::DvppManager::GetInstance();
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();
}

TEST_F(VpcTest, TestDvppCalcHist)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppHist histDesc;
    aclrtStream stream;
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_400;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    histDesc.dvppHistDesc.lens[0] = 256;
    histDesc.dvppHistDesc.lens[1] = 256;
    histDesc.dvppHistDesc.lens[2] = 256;
    histDesc.dataBuffer.data = (void*)0x1;
    histDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppHistDesc);
    histDesc.shareBuffer.data = (void*)0x1;
    histDesc.shareBuffer.length = 3072;

    inputDesc.dvppPicDesc.width = 2048;
    inputDesc.dvppPicDesc.height = 2048;
    inputDesc.dvppPicDesc.widthStride = 2048;
    inputDesc.dvppPicDesc.heightStride = 2048;

    histDesc.dvppHistDesc.hist = (uint32_t*)0x1;

    channelDesc.notify = (void*)0x1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    EXPECT_EQ(acldvppVpcCalcHistAsync(&channelDesc, &inputDesc,
        &histDesc, nullptr, stream), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppVpcCalcHistAsync(&channelDesc, &inputDesc,
        &histDesc, nullptr, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppVpcCalcHistAsync(&channelDesc, &inputDesc,
        &histDesc, nullptr, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(acldvppVpcCalcHistAsync(&channelDesc, &inputDesc,
        &histDesc, nullptr, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(acldvppVpcCalcHistAsync(&channelDesc, &inputDesc,
        &histDesc, nullptr, stream), ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _,_))
        .WillOnce(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    EXPECT_NE(acldvppVpcCalcHistAsync(&channelDesc, &inputDesc,
        &histDesc, nullptr, stream), ACL_SUCCESS);
}

TEST_F(VpcTest, TestDvppCalcHist_param_check)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppHist histDesc;
    aclrtStream stream;
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_400;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    histDesc.dataBuffer.data = (void*)0x1;
    histDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    histDesc.shareBuffer.data = (void*)0x1;
    histDesc.dataBuffer.length = 3072;

    inputDesc.dvppPicDesc.width = 2048;
    inputDesc.dvppPicDesc.height = 2048;
    inputDesc.dvppPicDesc.widthStride = 2048;
    inputDesc.dvppPicDesc.heightStride = 2048;

    histDesc.dvppHistDesc.hist = (uint32_t*)0x1;

    channelDesc.notify = (void*)0x1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.InitDvppProcessor();
    EXPECT_NE(acldvppVpcCalcHistAsync(&channelDesc, &inputDesc,
        &histDesc, nullptr, stream), ACL_SUCCESS);

    dvppManager.imageProcessor_ = nullptr;
    EXPECT_NE(acldvppVpcCalcHistAsync(&channelDesc, &inputDesc,
        &histDesc, nullptr, stream), ACL_SUCCESS);

    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.InitDvppProcessor();
    EXPECT_NE(acldvppVpcCalcHistAsync(&channelDesc, &inputDesc,
        &histDesc, (void*)0x1, stream), ACL_SUCCESS);

    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_BUTT;
    EXPECT_NE(acldvppVpcCalcHistAsync(&channelDesc, &inputDesc,
        &histDesc, nullptr, stream), ACL_SUCCESS);
}

TEST_F(VpcTest, TestDvppVpcBatchCropResizePaste)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub2));
    acldvppChannelDesc channelDesc;
    acldvppResizeConfig resizeConfig;

    acldvppBatchPicDesc *inputBatch = acldvppCreateBatchPicDesc(1);
    acldvppBatchPicDesc *outputBatch = acldvppCreateBatchPicDesc(1);

    acldvppPicDesc *inputDesc = acldvppGetPicDesc(inputBatch, 0);
    acldvppPicDesc *outputDesc = acldvppGetPicDesc(outputBatch, 0);
    acldvppRoiConfig  *roiConfig =  acldvppCreateRoiConfig(1, 1, 1, 1);
    acldvppRoiConfig  *pasteRoiConfig =  acldvppCreateRoiConfig(1, 1, 1, 1);

    aclrtStream stream;
    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    inputDesc->dataBuffer.data = (void*)0x1;
    inputDesc->dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc->dataBuffer.data = (void*)0x1;
    outputDesc->dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    inputDesc->dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc->dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    uint32_t *roiNums = new uint32_t[1];
    roiNums[0] = 1;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    channelDesc.notify = (void*)0x1;

    EXPECT_EQ(acldvppVpcBatchCropResizePasteAsync(&channelDesc, inputBatch, roiNums, 1, outputBatch,
                                       &roiConfig, &pasteRoiConfig, &resizeConfig, stream), ACL_SUCCESS);
    acldvppDestroyRoiConfig(roiConfig);
    acldvppDestroyRoiConfig(pasteRoiConfig);
    acldvppDestroyBatchPicDesc(inputBatch);
    acldvppDestroyBatchPicDesc(outputBatch);
    delete[] roiNums;
}