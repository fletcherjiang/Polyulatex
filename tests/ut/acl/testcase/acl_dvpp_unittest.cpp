#include <vector>
#include <iostream>
#include "securec.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define protected public
#define private public
#include "single_op/op_executor.h"
#include "types/tensor_desc_internal.h"
#include "acl/ops/acl_dvpp.h"
#include "acl/acl_rt.h"
#include "runtime/mem.h"
#include "single_op/dvpp/mgr/dvpp_manager.h"
#include "single_op/dvpp/common/dvpp_def_internal.h"
#include "single_op/dvpp/v200/video_processor_v200.h"
#include "single_op/dvpp/v100/video_processor_v100.h"
#include "single_op/dvpp/mgr/dvpp_manager.h"
#include "single_op/dvpp/common/dvpp_util.h"
#include "single_op/dvpp/base/image_processor.h"
#include "single_op/dvpp/v100/image_processor_v100.h"
#include "single_op/dvpp/v200/image_processor_v200.h"
#undef private
#undef protected

#include "acl/acl.h"
#include "acl_stub.h"

using namespace std;
using namespace testing;
using namespace acl;
using namespace acl::dvpp;

class DvppTest : public testing::Test {
protected:
    void SetUp()
    {
    }

    void TearDown()
    {
        Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
    }
};

void callback(acldvppStreamDesc *input, acldvppPicDesc *output, void *userdata)
{
        ;
}

rtError_t rtGetRunMode_dvpp(rtRunMode *mode)
{
    *mode = RT_RUN_MODE_OFFLINE;
    return RT_ERROR_NONE;
}

TEST_F(DvppTest, acldvppSetPicDescData)
{
    void *devPtr = nullptr;
    size_t size = 1;
    EXPECT_EQ(acldvppSetPicDescData(nullptr, devPtr), ACL_ERROR_INVALID_PARAM);

    devPtr = (void*)0x1;
    acldvppPicDesc desc;
    // acldvppSetPicDescData
    acldvppSetPicDescData(&desc, devPtr);
    EXPECT_NE(desc.dvppPicDesc.data, (uintptr_t)nullptr);
}

TEST_F(DvppTest, acldvppSetPicDescSize)
{
    acldvppPicDesc desc;
    uint32_t size = 1;
    // acldvppSetPicDescSize
    EXPECT_EQ(acldvppSetPicDescSize(nullptr, size), ACL_ERROR_INVALID_PARAM);
    acldvppSetPicDescSize(&desc, size);
    EXPECT_EQ(desc.dvppPicDesc.size, 1);
}

TEST_F(DvppTest, acldvppSetPicDescFormat)
{
    acldvppPicDesc desc;
    // acldvppSetPicDescFormat
    EXPECT_EQ(acldvppSetPicDescFormat(nullptr, PIXEL_FORMAT_YUV_400), ACL_ERROR_INVALID_PARAM);
    acldvppSetPicDescFormat(&desc, PIXEL_FORMAT_YUV_400);
    EXPECT_EQ(desc.dvppPicDesc.format, PIXEL_FORMAT_YUV_400);
}

TEST_F(DvppTest, acldvppSetPicDescWidth)
{
    acldvppPicDesc desc;
    uint32_t width = 256;
    // acldvppSetPicDescWidth
    EXPECT_EQ(acldvppSetPicDescWidth(nullptr, width), ACL_ERROR_INVALID_PARAM);
    acldvppSetPicDescWidth(&desc, width);
    EXPECT_EQ(desc.dvppPicDesc.width, 256);
}

TEST_F(DvppTest, acldvppSetPicDescHeight)
{
    acldvppPicDesc desc;
    uint32_t height = 256;
    // acldvppSetPicDescHeight
    EXPECT_EQ(acldvppSetPicDescHeight(nullptr, height), ACL_ERROR_INVALID_PARAM);
    acldvppSetPicDescHeight(&desc, height);
    EXPECT_EQ(desc.dvppPicDesc.height, 256);
}

TEST_F(DvppTest, acldvppSetPicDescWidthStride)
{
    acldvppPicDesc desc;
    uint32_t widthStride = 256;
    // acldvppSetPicDescWidthStride
    EXPECT_EQ(acldvppSetPicDescWidthStride(nullptr, widthStride), ACL_ERROR_INVALID_PARAM);
    acldvppSetPicDescWidthStride(&desc, widthStride);
    EXPECT_EQ(desc.dvppPicDesc.widthStride, 256);
}

TEST_F(DvppTest, acldvppSetPicDescHeightStride)
{
    acldvppPicDesc desc;
    uint32_t heightStride = 256;
    // acldvppSetPicDescHeightStride
    EXPECT_EQ(acldvppSetPicDescHeightStride(nullptr, heightStride), ACL_ERROR_INVALID_PARAM);
    acldvppSetPicDescHeightStride(&desc, heightStride);
    EXPECT_EQ(desc.dvppPicDesc.heightStride, 256);
}

TEST_F(DvppTest, acldvppSetPicDescRetCode)
{
    acldvppPicDesc desc;
    uint32_t retCode = 0;
    // acldvppSetPicDescRetcode
    EXPECT_EQ(acldvppSetPicDescRetCode(nullptr, retCode), ACL_ERROR_INVALID_PARAM);
    acldvppSetPicDescRetCode(&desc, retCode);
    EXPECT_EQ(desc.dvppPicDesc.retCode, 0);
}

TEST_F(DvppTest, acldvppGetPicDescFormat)
{
    acldvppPicDesc desc;
    desc.dvppPicDesc.format = PIXEL_FORMAT_YUV_400;
    acldvppPixelFormat format = acldvppGetPicDescFormat(nullptr);
    EXPECT_EQ(format, PIXEL_FORMAT_YUV_400);
    format = acldvppGetPicDescFormat(&desc);
    EXPECT_EQ(format, PIXEL_FORMAT_YUV_400);
}

TEST_F(DvppTest, acldvppGetPicDescWidth)
{
    acldvppPicDesc desc;
    desc.dvppPicDesc.width = 256;
    uint32_t width = acldvppGetPicDescWidth(nullptr);
    EXPECT_EQ(width, 0);
    width = acldvppGetPicDescWidth(&desc);
    EXPECT_EQ(width, 256);
}

TEST_F(DvppTest, acldvppGetPicDescHeight)
{
    acldvppPicDesc desc;
    desc.dvppPicDesc.height = 256;
    uint32_t height = acldvppGetPicDescHeight(nullptr);
    EXPECT_EQ(height, 0);
    height = acldvppGetPicDescHeight(&desc);
    EXPECT_EQ(height, 256);
}

TEST_F(DvppTest, acldvppGetPicDescWidthStride)
{
    acldvppPicDesc desc;
    desc.dvppPicDesc.widthStride = 256;
    uint32_t widthStride = acldvppGetPicDescWidthStride(nullptr);
    EXPECT_EQ(widthStride, 0);
    widthStride = acldvppGetPicDescWidthStride(&desc);
    EXPECT_EQ(widthStride, 256);
}

TEST_F(DvppTest, acldvppGetPicDescHeightStride)
{
    acldvppPicDesc desc;
    desc.dvppPicDesc.heightStride = 256;
    uint32_t heightStride = acldvppGetPicDescHeightStride(nullptr);
    EXPECT_EQ(heightStride, 0);
    heightStride = acldvppGetPicDescHeightStride(&desc);
    EXPECT_EQ(heightStride, 256);
}

TEST_F(DvppTest, acldvppGetPicDescRetcode)
{
    acldvppPicDesc desc;
    desc.dvppPicDesc.retCode = 2;
    uint32_t retCode = acldvppGetPicDescRetCode(nullptr);
    EXPECT_EQ(retCode, 0);
    retCode = acldvppGetPicDescRetCode(&desc);
    EXPECT_EQ(retCode, 2);
}

TEST_F(DvppTest, acldvppSetRoiConfigTest)
{
    acldvppRoiConfig *config = acldvppCreateRoiConfig(0, 0, 0, 0);
    aclError ret = acldvppSetRoiConfig(config, 65536, 1, 2, 3);
    EXPECT_NE(ret, ACL_SUCCESS);
    acldvppDestroyRoiConfig(config);
}

TEST_F(DvppTest, acldvppSetJpegeConfigLevel)
{
    uint32_t level = 10;
    acldvppJpegeConfig* config = nullptr;
    config = acldvppCreateJpegeConfig();
    acldvppSetJpegeConfigLevel(config, level);
    EXPECT_NE(config, nullptr);
    acldvppDestroyJpegeConfig(config);
    config = nullptr;
}

TEST_F(DvppTest, acldvppSetJpegeConfigLevel02)
{
    acldvppJpegeConfig *jpegeConfig = nullptr;
    EXPECT_EQ(acldvppGetJpegeConfigLevel(jpegeConfig), 0);
}

TEST_F(DvppTest, acldvppSetResizeConfigInterpolation1)
{
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.InitDvppProcessor();
    // acldvppCreateResizeConfig
    acldvppResizeConfig *resizeConfig = nullptr;
    resizeConfig = acldvppCreateResizeConfig();
    aclError ret = acldvppSetResizeConfigInterpolation(resizeConfig, 0);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(acldvppSetResizeConfigInterpolation(resizeConfig, 3), ACL_SUCCESS);
    EXPECT_NE(acldvppSetResizeConfigInterpolation(resizeConfig, 6), ACL_SUCCESS);
    acldvppDestroyResizeConfig(resizeConfig);
}

TEST_F(DvppTest, acldvppSetResizeConfigInterpolation2)
{
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.InitDvppProcessor();

    // acldvppCreateResizeConfig
    acldvppResizeConfig *resizeConfig = nullptr;
    resizeConfig = acldvppCreateResizeConfig();
    aclError ret = acldvppSetResizeConfigInterpolation(resizeConfig, 0);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_NE(acldvppSetResizeConfigInterpolation(resizeConfig, 3), ACL_SUCCESS);
    acldvppDestroyResizeConfig(resizeConfig);

    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.InitDvppProcessor();
}

TEST_F(DvppTest, acldvppGetResizeConfigInterpolation)
{
    // acldvppCreateResizeConfig
    acldvppResizeConfig *resizeConfig = nullptr;
    uint32_t ret = acldvppGetResizeConfigInterpolation(resizeConfig);
    EXPECT_EQ(ret, 0);
    resizeConfig = acldvppCreateResizeConfig();
    ret = acldvppGetResizeConfigInterpolation(resizeConfig);
    EXPECT_EQ(ret, 0);
    acldvppDestroyResizeConfig(resizeConfig);
}

void* mmAlignMallocStub(mmSize mallocSize, mmSize alignSize)
{
    acldvppChannelDesc *dvppChannelDesc = nullptr;
    mallocSize = CalAclDvppStructSize(dvppChannelDesc);
    return malloc(mallocSize);
}

rtError_t rtMallocStub(void **devPtr, uint64_t size, rtMemType_t type)
{
    *devPtr = malloc(size);
    return RT_ERROR_NONE;
}

rtError_t rtDvppMallocStub(void **devPtr, uint64_t size)
{
    *devPtr = malloc(size);
    return RT_ERROR_NONE;
}

TEST_F(DvppTest, acldvppCreateChannelDesc)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub));

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    // acldvppCreateChannelDesc
    acldvppChannelDesc *dvppChannelDesc = nullptr;
    acldvppDestroyChannelDesc(dvppChannelDesc);
    dvppChannelDesc = acldvppCreateChannelDesc();
    EXPECT_NE(dvppChannelDesc, nullptr);
    acldvppDestroyChannelDesc(dvppChannelDesc);
    dvppChannelDesc = nullptr;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    dvppChannelDesc = acldvppCreateChannelDesc();
    EXPECT_EQ(dvppChannelDesc, nullptr);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtDvppMalloc(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    dvppChannelDesc = acldvppCreateChannelDesc();
    EXPECT_EQ(dvppChannelDesc, nullptr);
}

TEST_F(DvppTest, acldvppCreateChannelDesc_2)
{
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_DEVICE;
    dvppManager.InitDvppProcessor();

    // acldvppCreateChannelDesc
    acldvppChannelDesc *dvppChannelDesc = nullptr;
    acldvppDestroyChannelDesc(dvppChannelDesc);
    dvppChannelDesc = acldvppCreateChannelDesc();
    EXPECT_NE(dvppChannelDesc, nullptr);
    acldvppDestroyChannelDesc(dvppChannelDesc);
    dvppChannelDesc = nullptr;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillRepeatedly(Return(RT_ERROR_NONE));
    dvppChannelDesc = acldvppCreateChannelDesc();
    EXPECT_EQ(dvppChannelDesc, nullptr);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtDvppMalloc(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    dvppChannelDesc = acldvppCreateChannelDesc();
    EXPECT_EQ(dvppChannelDesc, nullptr);

    dvppManager.imageProcessor_ = nullptr;
    dvppChannelDesc = acldvppCreateChannelDesc();
    EXPECT_EQ(dvppChannelDesc, nullptr);
}

TEST_F(DvppTest, acldvppGetPicDescData)
{
    // acldvppGetPicDescData
    EXPECT_EQ(acldvppGetPicDescData(nullptr), nullptr);

    acldvppPicDesc picDesc;
    picDesc.dvppPicDesc.data = 1;
    EXPECT_NE(acldvppGetPicDescData(&picDesc), nullptr);
}

TEST_F(DvppTest, acldvppGetPicDescSize)
{
    // acldvppGetPicDescSize
    EXPECT_EQ(acldvppGetPicDescSize(nullptr), 0);
    acldvppPicDesc picDesc;
    picDesc.dvppPicDesc.size = 123;
    EXPECT_EQ(acldvppGetPicDescSize(&picDesc), 123);
}

TEST_F(DvppTest, acldvppGetChannelDescIndex)
{
    // acldvppGetChannelDescIndex
    EXPECT_EQ(acldvppGetChannelDescChannelId(nullptr), 0);
    acldvppChannelDesc channelDesc;
    channelDesc.channelIndex = 12;
    EXPECT_EQ(acldvppGetChannelDescChannelId(&channelDesc), 12);
}

TEST_F(DvppTest, memory_malloc_device)
{
    void *devPtr = nullptr;
    size_t size = 1;

    aclError ret = acldvppMalloc(&devPtr, size);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_NE(devPtr, nullptr);

    ret = acldvppFree(devPtr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtDvppMalloc(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)))
        .WillRepeatedly(Return(RT_ERROR_NONE));
    ret = acldvppMalloc(&devPtr, size);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = acldvppMalloc(nullptr, size);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = acldvppMalloc(&devPtr, size);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = acldvppMalloc(&devPtr, size);
    EXPECT_EQ(ret, ACL_SUCCESS);

    size = static_cast<size_t>(-1);
    ret = acldvppMalloc(&devPtr, size);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
}

TEST_F(DvppTest, aclvdecCreateChannelDesc_onHost_1)
{
    // aclvdecCreateChannelDesc
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    EXPECT_NE(vdecChannelDesc, nullptr);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(DvppTest, aclvdecCreateChannelDesc_onHost_2)
{
    // aclvdecCreateChannelDesc
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    EXPECT_NE(vdecChannelDesc, nullptr);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

TEST_F(DvppTest, aclvdecCreateChannelDesc_onDevice)
{
    // aclvdecCreateChannelDesc
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetRunMode(_))
        .WillRepeatedly(Invoke((rtGetRunMode_dvpp)));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_,_,_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    EXPECT_EQ(vdecChannelDesc, nullptr);
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;
}

void* mmAlignMallocStub5(mmSize mallocSize, mmSize alignSize)
{
    aclvdecChannelDesc *aclChannelDesc = nullptr;
    mallocSize = CalAclDvppStructSize(aclChannelDesc);
    return malloc(mallocSize);
}

TEST_F(DvppTest, aclvdecChannelDesc_v200_host_1)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub5));
    auto instance = &(acl::dvpp::DvppManager::GetInstance());
    instance->videoProcessor_ = std::unique_ptr<VideoProcessorV200>(new (std::nothrow)VideoProcessorV200(ACL_HOST));
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();

    aclError ret = aclvdecSetChannelDescOutMode(vdecChannelDesc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    uint32_t outMode = aclvdecGetChannelDescOutMode(vdecChannelDesc);
    EXPECT_EQ(outMode, 1);

    outMode = aclvdecGetChannelDescOutMode(nullptr);
    EXPECT_EQ(outMode, 0);

    ret = aclvdecSetChannelDescChannelId(vdecChannelDesc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescEnType(vdecChannelDesc, acldvppStreamFormat(-1));
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescEnType(vdecChannelDesc, H265_MAIN_LEVEL);
    EXPECT_EQ(ret, ACL_SUCCESS);

    acldvppStreamFormat enType = aclvdecGetChannelDescEnType(vdecChannelDesc);
    EXPECT_EQ(enType, H265_MAIN_LEVEL);

    ret = aclvdecSetChannelDescOutPicFormat(vdecChannelDesc, PIXEL_FORMAT_YUV_SEMIPLANAR_420);
    EXPECT_EQ(ret, ACL_SUCCESS);

    acldvppPixelFormat outPicFormat = aclvdecGetChannelDescOutPicFormat(vdecChannelDesc);
    EXPECT_EQ(outPicFormat, PIXEL_FORMAT_YUV_SEMIPLANAR_420);

    outPicFormat = aclvdecGetChannelDescOutPicFormat(nullptr);
    EXPECT_EQ(outPicFormat, PIXEL_FORMAT_YUV_400);

    ret = aclvdecSetChannelDescOutPicWidth(vdecChannelDesc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    uint32_t outPicWidth = aclvdecGetChannelDescOutPicWidth(vdecChannelDesc);
    EXPECT_EQ(outPicWidth, 1);

    ret = aclvdecSetChannelDescOutPicHeight(vdecChannelDesc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    uint32_t outPicHeight = aclvdecGetChannelDescOutPicHeight(vdecChannelDesc);
    EXPECT_EQ(outPicHeight, 1);

    ret = aclvdecSetChannelDescRefFrameNum(vdecChannelDesc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    uint32_t refFrameNum = aclvdecGetChannelDescRefFrameNum(vdecChannelDesc);
    EXPECT_EQ(refFrameNum, 1);

    ret = aclvdecSetChannelDescChannelId(vdecChannelDesc, 256);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescChannelId(vdecChannelDesc, 5);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescBitDepth(vdecChannelDesc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    auto bitDepth = aclvdecGetChannelDescBitDepth(vdecChannelDesc);
    EXPECT_EQ(bitDepth, 1);

    bitDepth = aclvdecGetChannelDescBitDepth(nullptr);
    EXPECT_EQ(bitDepth, 0);

    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    instance->videoProcessor_ = nullptr;
    ret = aclvdecSetChannelDescBitDepth(vdecChannelDesc, 1);
    EXPECT_EQ(ret, ACL_ERROR_INTERNAL_ERROR);

    bitDepth = aclvdecGetChannelDescBitDepth(vdecChannelDesc);
    EXPECT_EQ(bitDepth, 0);

    instance = &(acl::dvpp::DvppManager::GetInstance());
    instance->videoProcessor_ = std::unique_ptr<VideoProcessorV100>(new (std::nothrow)VideoProcessorV100(ACL_HOST));
}

TEST_F(DvppTest, aclvdecChannelDesc_v200_host_2)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub5));
    auto instance = &(acl::dvpp::DvppManager::GetInstance());
    instance->videoProcessor_ = std::unique_ptr<VideoProcessorV200>(new (std::nothrow)VideoProcessorV200(ACL_HOST));

    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    instance->videoProcessor_ = nullptr;

    auto ret = aclvdecSetChannelDescEnType(vdecChannelDesc, H265_MAIN_LEVEL);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescOutPicFormat(vdecChannelDesc, PIXEL_FORMAT_YUV_SEMIPLANAR_420);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescOutPicWidth(vdecChannelDesc, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescOutPicHeight(vdecChannelDesc, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescRefFrameNum(vdecChannelDesc, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescChannelId(vdecChannelDesc, 5);
    EXPECT_NE(ret, ACL_SUCCESS);

    instance->videoProcessor_ = std::unique_ptr<VideoProcessorV200>(new (std::nothrow)VideoProcessorV200(ACL_HOST));
    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    instance = &(acl::dvpp::DvppManager::GetInstance());
    instance->videoProcessor_ = std::unique_ptr<VideoProcessorV100>(new (std::nothrow)VideoProcessorV100(ACL_HOST));
}

TEST_F(DvppTest, aclvdecChannelDesc_v100_host)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub5));
    auto instance = &(acl::dvpp::DvppManager::GetInstance());
    instance->videoProcessor_ = std::unique_ptr<VideoProcessorV100>(new (std::nothrow)VideoProcessorV100(ACL_HOST));

    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();

    auto ret = aclvdecSetChannelDescEnType(vdecChannelDesc, H265_MAIN_LEVEL);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescOutPicFormat(vdecChannelDesc, PIXEL_FORMAT_YUV_SEMIPLANAR_420);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescOutPicWidth(vdecChannelDesc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescOutPicHeight(vdecChannelDesc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescRefFrameNum(vdecChannelDesc, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescBitDepth(vdecChannelDesc, 1);
    EXPECT_EQ(ret, ACL_ERROR_FEATURE_UNSUPPORTED);

    auto bitDepth = aclvdecGetChannelDescBitDepth(vdecChannelDesc);
    EXPECT_EQ(bitDepth, 0);

    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    instance = &(acl::dvpp::DvppManager::GetInstance());
    instance->videoProcessor_ = std::unique_ptr<VideoProcessorV100>(new (std::nothrow)VideoProcessorV100(ACL_HOST));
}

TEST_F(DvppTest, aclvdecChannelDesc_v200_device)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub5));
    auto instance = &(acl::dvpp::DvppManager::GetInstance());
    instance->videoProcessor_ = std::unique_ptr<VideoProcessorV200>(new (std::nothrow)VideoProcessorV200(ACL_HOST));

    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();

    auto ret = aclvdecSetChannelDescEnType(vdecChannelDesc, acldvppStreamFormat(-1));
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescEnType(vdecChannelDesc, H265_MAIN_LEVEL);
    EXPECT_EQ(ret, ACL_SUCCESS);

    acldvppStreamFormat enType = aclvdecGetChannelDescEnType(vdecChannelDesc);
    EXPECT_EQ(enType, H265_MAIN_LEVEL);

    ret = aclvdecSetChannelDescOutPicFormat(vdecChannelDesc, PIXEL_FORMAT_YUV_SEMIPLANAR_420);
    EXPECT_EQ(ret, ACL_SUCCESS);

    acldvppPixelFormat outPicFormat = aclvdecGetChannelDescOutPicFormat(vdecChannelDesc);
    EXPECT_EQ(outPicFormat, PIXEL_FORMAT_YUV_SEMIPLANAR_420);

    ret = aclvdecSetChannelDescOutPicWidth(vdecChannelDesc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    uint32_t outPicWidth = aclvdecGetChannelDescOutPicWidth(vdecChannelDesc);
    EXPECT_EQ(outPicWidth, 1);

    ret = aclvdecSetChannelDescOutPicHeight(vdecChannelDesc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    uint32_t outPicHeight = aclvdecGetChannelDescOutPicHeight(vdecChannelDesc);
    EXPECT_EQ(outPicHeight, 1);

    ret = aclvdecSetChannelDescRefFrameNum(vdecChannelDesc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    uint32_t refFrameNum = aclvdecGetChannelDescRefFrameNum(vdecChannelDesc);
    EXPECT_EQ(refFrameNum, 1); // error

    refFrameNum = aclvdecGetChannelDescRefFrameNum(nullptr);
    EXPECT_EQ(refFrameNum, 0);

    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    instance = &(acl::dvpp::DvppManager::GetInstance());
    instance->videoProcessor_ = std::unique_ptr<VideoProcessorV100>(new (std::nothrow)VideoProcessorV100(ACL_HOST));
}

TEST_F(DvppTest, aclvdecGetChannelDescEnType)
{
    aclvdecChannelDesc *channelDesc = nullptr;
    acldvppStreamFormat ret = aclvdecGetChannelDescEnType(channelDesc);
    EXPECT_EQ(ret, H265_MAIN_LEVEL);
}

TEST_F(DvppTest, aclvdecGetChannelDescOutPicWidth)
{
    aclvdecChannelDesc *channelDesc = nullptr;
    uint32_t ret = aclvdecGetChannelDescOutPicWidth(channelDesc);
    EXPECT_EQ(ret, 0);
}

TEST_F(DvppTest, aclvdecGetChannelDescOutPicHeight)
{
    aclvdecChannelDesc *channelDesc = nullptr;
    uint32_t ret = aclvdecGetChannelDescOutPicHeight(channelDesc);
    EXPECT_EQ(ret, 0);
}

TEST_F(DvppTest, aclvencSetChannelDescCallback)
{
    aclvencChannelDesc *channelDesc = aclvencCreateChannelDesc();
    aclvencCallback callback = nullptr;
    aclError ret = aclvencSetChannelDescCallback(channelDesc, callback);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    aclvencDestroyChannelDesc(channelDesc);
}

TEST_F(DvppTest, aclvencGetFrameConfigForceIFrame)
{
    aclvencFrameConfig *config = nullptr;
    aclError ret = aclvencGetFrameConfigForceIFrame(config);
    EXPECT_EQ(ret, 0);
}

TEST_F(DvppTest, aclvencGetFrameConfigEos)
{
    aclvencFrameConfig *config = nullptr;
    uint8_t ret = aclvencGetFrameConfigEos(config);
    EXPECT_EQ(ret, 0);
}

TEST_F(DvppTest, acldvppSetBorderConfigTop)
{
    acldvppBorderConfig *borderConfig;
    uint32_t top = 65536;
    aclError ret = acldvppSetBorderConfigTop(borderConfig, top);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
}

TEST_F(DvppTest, acldvppSetBorderConfigBottom)
{
    acldvppBorderConfig *borderConfig;
    uint32_t bottom = 65536;
    aclError ret = acldvppSetBorderConfigTop(borderConfig, bottom);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
}

TEST_F(DvppTest, acldvppSetBorderConfigLeft)
{
    acldvppBorderConfig *borderConfig;
    uint32_t left = 65536;
    aclError ret = acldvppSetBorderConfigTop(borderConfig, left);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
}

TEST_F(DvppTest, acldvppSetBorderConfigRight)
{
    acldvppBorderConfig *borderConfig;
    uint32_t right = 65536;
    aclError ret = acldvppSetBorderConfigTop(borderConfig, right);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
}

TEST_F(DvppTest, acldvppGetBorderConfigValue)
{
    acldvppBorderConfig *borderConfig = (acldvppBorderConfig *)0x1;
    uint32_t index = 5;
    aclError ret = acldvppGetBorderConfigValue(borderConfig, index);
    EXPECT_EQ(ret, -1);
}

TEST_F(DvppTest, aclvdecChannelDesc)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub5));
    // aclvdecSetChannelDesc
    aclvdecChannelDesc *vdecChannelDesc = aclvdecCreateChannelDesc();
    aclvdecCallback callback;

    aclError ret = aclvdecSetChannelDescChannelId(vdecChannelDesc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    uint32_t channelId = aclvdecGetChannelDescChannelId(vdecChannelDesc);
    EXPECT_EQ(channelId, 1);

    ret = aclvdecSetChannelDescChannelId(vdecChannelDesc, 60);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescThreadId(vdecChannelDesc, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    uint32_t threadId = aclvdecGetChannelDescThreadId(vdecChannelDesc);
    EXPECT_EQ(threadId, 1);

    ret = aclvdecSetChannelDescCallback(vdecChannelDesc, callback);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclvdecCallback getvdecCallback = aclvdecGetChannelDescCallback(vdecChannelDesc);
    EXPECT_EQ(getvdecCallback, callback);

    ret = aclvdecSetChannelDescCallback(vdecChannelDesc, nullptr);
    EXPECT_NE(ret, ACL_SUCCESS);

    aclvdecDestroyChannelDesc(vdecChannelDesc);
    vdecChannelDesc = nullptr;

    ret = aclvdecSetChannelDescChannelId(vdecChannelDesc, 0);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescThreadId(vdecChannelDesc, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = aclvdecSetChannelDescCallback(vdecChannelDesc, callback);
    EXPECT_NE(ret, ACL_SUCCESS);

    channelId = aclvdecGetChannelDescChannelId(vdecChannelDesc);
    EXPECT_EQ(channelId, 0);

    threadId = aclvdecGetChannelDescThreadId(vdecChannelDesc);
    EXPECT_EQ(threadId, 0);

    uint32_t refFrameNum = aclvdecGetChannelDescRefFrameNum(vdecChannelDesc);
    EXPECT_EQ(refFrameNum, 0);

    getvdecCallback = aclvdecGetChannelDescCallback(vdecChannelDesc);
    EXPECT_EQ(getvdecCallback, nullptr);
}

void* mmAlignMallocStub4(mmSize mallocSize, mmSize alignSize)
{
    mallocSize = 64;
    return malloc(mallocSize);
}

TEST_F(DvppTest, acldvppCreateStreamDesc_1)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub4));
    acldvppStreamDesc *streamDesc = acldvppCreateStreamDesc();
    EXPECT_NE(streamDesc, nullptr); // error 
    acldvppDestroyStreamDesc(streamDesc);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub4));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _)) //error
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    streamDesc = acldvppCreateStreamDesc();
    EXPECT_EQ(streamDesc, nullptr);
}

rtError_t CreateStreamOnDevice(rtRunMode *mode)
{
    *mode = RT_RUN_MODE_OFFLINE;
    return RT_ERROR_NONE;
}

TEST_F(DvppTest, acldvppCreateStreamDesc_2)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub4));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetRunMode(_))
        .WillRepeatedly(Invoke((CreateStreamOnDevice)));
    acldvppStreamDesc *streamDesc = acldvppCreateStreamDesc();
    EXPECT_NE(streamDesc, nullptr);
    acldvppDestroyStreamDesc(streamDesc);
}

TEST_F(DvppTest, acldvppStreamDescSize)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub4));
    acldvppStreamDesc *streamDesc = acldvppCreateStreamDesc();
    aclError ret = acldvppSetStreamDescSize(streamDesc, 16);
    EXPECT_EQ(ret, ACL_SUCCESS);
    uint32_t size = 0;
    size = acldvppGetStreamDescSize(streamDesc);
    EXPECT_EQ(size, 16);
    acldvppDestroyStreamDesc(streamDesc);

    streamDesc = nullptr;
    ret = acldvppSetStreamDescSize(streamDesc, 16);
    EXPECT_NE(ret, ACL_SUCCESS);
    size = acldvppGetStreamDescSize(streamDesc);
    EXPECT_EQ(size, 0);
}

TEST_F(DvppTest, acldvppStreamDescFormat)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub4));
    acldvppStreamDesc *streamDesc = acldvppCreateStreamDesc();
    aclError ret = acldvppSetStreamDescFormat(streamDesc, H265_MAIN_LEVEL);
    EXPECT_EQ(ret, ACL_SUCCESS);
    acldvppStreamFormat format;
    format = acldvppGetStreamDescFormat(streamDesc);
    EXPECT_EQ(format, H265_MAIN_LEVEL);
    acldvppDestroyStreamDesc(streamDesc);

    streamDesc = nullptr;
    ret = acldvppSetStreamDescFormat(streamDesc, static_cast<acldvppStreamFormat>(16));
    EXPECT_NE(ret, ACL_SUCCESS);
    format = acldvppGetStreamDescFormat(streamDesc);
    EXPECT_EQ(format, H265_MAIN_LEVEL);
}

TEST_F(DvppTest, acldvppStreamDescTimestamp)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub4));
    acldvppStreamDesc *streamDesc = acldvppCreateStreamDesc();
    aclError ret = acldvppSetStreamDescTimestamp(streamDesc, 16); //error
    EXPECT_EQ(ret, ACL_SUCCESS);
    uint64_t timestamp = 0;
    timestamp = acldvppGetStreamDescTimestamp(streamDesc);
    EXPECT_EQ(timestamp, 16);
    acldvppDestroyStreamDesc(streamDesc);

    streamDesc = nullptr;
    ret = acldvppSetStreamDescTimestamp(streamDesc, 16);
    EXPECT_NE(ret, ACL_SUCCESS);
    timestamp = acldvppGetStreamDescTimestamp(streamDesc);
    EXPECT_EQ(timestamp, 0);
}

TEST_F(DvppTest, acldvppStreamDescRetCode)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub4));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub4));
    acldvppStreamDesc *streamDesc = acldvppCreateStreamDesc();
    aclError ret = acldvppSetStreamDescRetCode(streamDesc, 16);
    EXPECT_EQ(ret, ACL_SUCCESS);
    uint32_t retCode = 0;
    retCode = acldvppGetStreamDescRetCode(streamDesc);
    EXPECT_EQ(retCode, 16);
    acldvppDestroyStreamDesc(streamDesc);

    streamDesc = nullptr;
    ret = acldvppSetStreamDescRetCode(streamDesc, 16);
    EXPECT_NE(ret, ACL_SUCCESS);
    retCode = acldvppGetStreamDescRetCode(streamDesc);
    EXPECT_EQ(retCode, 0);
}

TEST_F(DvppTest, acldvppStreamDescEos)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub4));
    acldvppStreamDesc *streamDesc = acldvppCreateStreamDesc();
    aclError ret = acldvppSetStreamDescEos(streamDesc, 0);
    EXPECT_EQ(ret, ACL_SUCCESS);
    uint8_t eos = 1;
    eos = acldvppGetStreamDescEos(streamDesc);
    EXPECT_EQ(eos, 0);
    acldvppDestroyStreamDesc(streamDesc);

    ret = acldvppSetStreamDescEos(nullptr, eos);
    EXPECT_NE(ret, ACL_SUCCESS);
    eos = acldvppGetStreamDescEos(nullptr);
    EXPECT_EQ(eos, 0);
}

void* mmAlignMallocStub3(mmSize mallocSize, mmSize alignSize)
{
    mallocSize = 80;
    return malloc(mallocSize);
}

TEST_F(DvppTest, acldvppCreateBatchPicDesc)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillRepeatedly(Invoke(mmAlignMallocStub3));
    acldvppBatchPicDesc *batchDesc = acldvppCreateBatchPicDesc(0);
    EXPECT_EQ(batchDesc, nullptr);

    batchDesc = acldvppCreateBatchPicDesc(1);
    EXPECT_NE(batchDesc, nullptr);

    acldvppPicDesc *picDesc = acldvppGetPicDesc(nullptr, 0);
    EXPECT_EQ(picDesc, nullptr);

    picDesc = acldvppGetPicDesc(batchDesc, 0);
    EXPECT_NE(picDesc, nullptr);

    picDesc = acldvppGetPicDesc(batchDesc, 2);
    EXPECT_EQ(picDesc, nullptr);

    aclError ret = acldvppDestroyBatchPicDesc(batchDesc);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = acldvppDestroyBatchPicDesc(nullptr);
    EXPECT_EQ(ret, ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _)) //error
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    batchDesc = acldvppCreateBatchPicDesc(1);
    EXPECT_EQ(batchDesc, nullptr);
}

TEST_F(DvppTest, acldvppLutMap)
{
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.InitDvppProcessor();

    acldvppLutMap *lutMap = acldvppCreateLutMap();
    EXPECT_NE(lutMap, nullptr);

    uint32_t dim = acldvppGetLutMapDims(nullptr);
    EXPECT_EQ(dim, 0);

    dim = acldvppGetLutMapDims(lutMap);
    EXPECT_NE(dim, 0);

    uint32_t len = 0;
    uint8_t *data = nullptr;
    dim = 4;
    aclError ret = acldvppGetLutMapData(lutMap, dim, &data, &len);
    EXPECT_NE(ret, ACL_SUCCESS);

    dim = 1;
    ret = acldvppGetLutMapData(lutMap, dim, &data, &len);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = acldvppDestroyLutMap(lutMap);
    EXPECT_EQ(ret, ACL_SUCCESS);

    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.InitDvppProcessor();

    lutMap = acldvppCreateLutMap();
    EXPECT_EQ(lutMap, nullptr);

    dim = acldvppGetLutMapDims(nullptr);
    EXPECT_EQ(dim, 0);

    ret = acldvppGetLutMapData(lutMap, dim, &data, &len);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = acldvppDestroyLutMap(lutMap);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(DvppTest, acldvppBorderConfig)
{
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.InitDvppProcessor();

    acldvppBorderConfig *borderConfig = acldvppCreateBorderConfig();
    EXPECT_EQ(borderConfig, nullptr);

    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.InitDvppProcessor();

    borderConfig = acldvppCreateBorderConfig();
    EXPECT_NE(borderConfig, nullptr);

    aclError ret = acldvppSetBorderConfigValue(borderConfig, 1, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = acldvppSetBorderConfigValue(borderConfig, 5, 1);
    EXPECT_NE(ret, ACL_SUCCESS);

    ret = acldvppSetBorderConfigBorderType(borderConfig, BORDER_CONSTANT);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = acldvppSetBorderConfigTop(borderConfig, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = acldvppSetBorderConfigBottom(borderConfig, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = acldvppSetBorderConfigLeft(borderConfig, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = acldvppSetBorderConfigRight(borderConfig, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    double value = acldvppGetBorderConfigValue(borderConfig, 1);
    EXPECT_EQ(value, 1);

    value = acldvppGetBorderConfigValue(nullptr, 1);
    EXPECT_EQ(value, -1);

    value = acldvppGetBorderConfigValue(nullptr, 5);
    EXPECT_EQ(value, -1);

    acldvppBorderType borderType = acldvppGetBorderConfigBorderType(borderConfig);
    EXPECT_EQ(borderType, BORDER_CONSTANT);

    borderType = acldvppGetBorderConfigBorderType(nullptr);
    EXPECT_EQ(borderType, BORDER_CONSTANT);

    uint32_t retValue = acldvppGetBorderConfigTop(nullptr);
    EXPECT_EQ(retValue, 0);
    retValue = acldvppGetBorderConfigTop(borderConfig);
    EXPECT_EQ(retValue, 1);

    retValue = acldvppGetBorderConfigBottom(nullptr);
    EXPECT_EQ(retValue, 0);
    retValue = acldvppGetBorderConfigBottom(borderConfig);
    EXPECT_EQ(retValue, 1);

    retValue = acldvppGetBorderConfigLeft(nullptr);
    EXPECT_EQ(retValue, 0);
    retValue = acldvppGetBorderConfigLeft(borderConfig);
    EXPECT_EQ(retValue, 1);

    retValue = acldvppGetBorderConfigRight(nullptr);
    EXPECT_EQ(retValue, 0);
    retValue = acldvppGetBorderConfigRight(borderConfig);
    EXPECT_EQ(retValue, 1);

    ret = acldvppDestroyBorderConfig(borderConfig);
    EXPECT_EQ(ret, ACL_SUCCESS);

    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.InitDvppProcessor();
    ret = acldvppDestroyBorderConfig(borderConfig);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(DvppTest, acldvppHistDesc)
{
    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    acldvppHist *histDesc = acldvppCreateHist();
    EXPECT_NE(histDesc, nullptr);

    uint32_t dims = acldvppGetHistDims(nullptr);
    EXPECT_EQ(dims, 0);

    dims = acldvppGetHistDims(histDesc);
    EXPECT_EQ(dims, 3);

    aclError aclRet = ACL_SUCCESS;

    for (uint32_t i = 0; i < dims; ++i) {
        uint16_t len = 0;
        uint32_t *data = nullptr;
        aclRet = acldvppGetHistData(histDesc, i, &data, &len);
        EXPECT_EQ(aclRet, ACL_SUCCESS);
        EXPECT_EQ(len, 256);
        EXPECT_NE(data, nullptr);
    }

    uint16_t len = 0;
    uint32_t *data = nullptr;
    uint32_t dim = 0;
    aclRet = acldvppGetHistData(nullptr, dim, &data, &len);
    EXPECT_NE(aclRet, ACL_SUCCESS);

    aclRet = acldvppGetHistData(histDesc, dim, nullptr, &len);
    EXPECT_NE(aclRet, ACL_SUCCESS);

    aclRet = acldvppGetHistData(histDesc, dim, &data, nullptr);
    EXPECT_NE(aclRet, ACL_SUCCESS);

    dim = 4;
    aclRet = acldvppGetHistData(histDesc, dim, &data, &len);
    EXPECT_NE(aclRet, ACL_SUCCESS);

    uint32_t retCode = acldvppGetHistRetCode(histDesc);
    EXPECT_EQ(retCode, 0);

    retCode = acldvppGetHistRetCode(nullptr);
    EXPECT_NE(retCode, 0);

    aclRet = acldvppClearHist(histDesc);
    EXPECT_EQ(aclRet, ACL_SUCCESS);

    aclRet = acldvppClearHist(nullptr);
    EXPECT_NE(aclRet, ACL_SUCCESS);

    aclRet = acldvppDestroyHist(histDesc);
    EXPECT_EQ(aclRet, ACL_SUCCESS);

    aclRet = acldvppDestroyHist(nullptr);
    EXPECT_EQ(aclRet, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    histDesc = acldvppCreateHist();
    EXPECT_EQ(histDesc, nullptr);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    dvppManager.imageProcessor_ = nullptr;
    histDesc = acldvppCreateHist();
    EXPECT_EQ(histDesc, nullptr);

    aclRet = acldvppClearHist(histDesc);
    EXPECT_NE(aclRet, ACL_SUCCESS);

    aclRet = acldvppDestroyHist(histDesc);
    EXPECT_NE(aclRet, ACL_SUCCESS);

    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aclRunMode_ = ACL_DEVICE;
    dvppManager.InitDvppProcessor();

    histDesc = acldvppCreateHist();
    EXPECT_NE(histDesc, nullptr);

    aclRet = acldvppDestroyHist(histDesc);
    EXPECT_EQ(aclRet, ACL_SUCCESS);

    dvppManager.dvppVersion_ = DVPP_KERNELS_V100;
    dvppManager.aclRunMode_ = ACL_HOST;
    dvppManager.InitDvppProcessor();

    histDesc = acldvppCreateHist();
    EXPECT_EQ(histDesc, nullptr);

    dims = acldvppGetHistDims(histDesc);
    EXPECT_EQ(dims, 0);

    retCode = acldvppGetHistRetCode(histDesc);
    EXPECT_EQ(retCode, 0);

    aclRet = acldvppGetHistData(histDesc, dim, &data, &len);
    EXPECT_NE(aclRet, ACL_SUCCESS);

    aclRet = acldvppClearHist(histDesc);
    EXPECT_NE(aclRet, ACL_SUCCESS);

    aclRet = acldvppDestroyHist(histDesc);
    EXPECT_NE(aclRet, ACL_SUCCESS);
}

TEST_F(DvppTest, IsSupportSuperTask)
{
    DvppVersion dvppVersion;
    uint32_t aiCpuVersion;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDevice(_))
        .WillRepeatedly(Return((RT_ERROR_NONE)));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetDeviceCapability(_,_,_,_))
        .WillRepeatedly(Return((1)));
    bool ret = IsSupportSuperTask(dvppVersion, aiCpuVersion);
    EXPECT_EQ(ret, false);
}

TEST_F(DvppTest, CopyDvppPicDescAsync)
{
    acldvppPicDesc *aclPicDesc = acldvppCreatePicDesc();
    aclrtMemcpyKind memcpyKind = ACL_MEMCPY_DEVICE_TO_DEVICE;
    aclrtStream stream;

    aclError ret = CopyDvppPicDescAsync(aclPicDesc, memcpyKind, stream);
    acldvppDestroyPicDesc(aclPicDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(DvppTest, CopyDvppBatchPicDescAsyncTest)
{
    acldvppBatchPicDesc *aclBatchPicDesc = acldvppCreateBatchPicDesc(1);
    aclrtMemcpyKind memcpyKind;
    uint32_t batchSize = 10;
    aclrtStream stream;
    aclError ret = CopyDvppBatchPicDescAsync(aclBatchPicDesc, memcpyKind, batchSize, stream);
    EXPECT_NE(ret, ACL_SUCCESS);

    batchSize = 0;
    ret = CopyDvppBatchPicDescAsync(aclBatchPicDesc, memcpyKind, batchSize, stream);
    EXPECT_NE(ret, ACL_SUCCESS);
    acldvppDestroyBatchPicDesc(aclBatchPicDesc);
}

TEST_F(DvppTest, CopyDvppHistDescAsync)
{
    acldvppHist *aclDvppHist = acldvppCreateHist();
    aclrtMemcpyKind memcpyKind = ACL_MEMCPY_HOST_TO_HOST;
    aclrtStream stream;

    aclError ret = CopyDvppHistDescAsync(aclDvppHist, memcpyKind, stream);
    EXPECT_NE(ret, ACL_SUCCESS);
    acldvppDestroyHist(aclDvppHist);
}

TEST_F(DvppTest, CalcImageSizeKernel)
{
    uint32_t width;
    uint32_t height;
    acldvppPixelFormat format = acldvppPixelFormat::PIXEL_FORMAT_UNKNOWN;
    uint32_t *size;
    EXPECT_NE(CalcImageSizeKernel(width, height, format, size), true);
}

TEST_F(DvppTest, DvppCreateChannelWithoutNotifyTest01)
{
    acldvppChannelDesc channelDesc;
    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.notify = (void*)0x1;
    channelDesc.dvppDesc.retCode = 1;
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return((1)));
    aclError ret = imageProcessor->DvppCreateChannelWithoutNotify(&channelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillRepeatedly(Return((1)));
    ret = imageProcessor->DvppCreateChannelWithoutNotify(&channelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return((1)));
    ret = imageProcessor->DvppCreateChannelWithoutNotify(&channelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _)) //error
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillRepeatedly(Return((1)));
    ret = imageProcessor->DvppCreateChannelWithoutNotify(&channelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _)) //error
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamDestroy(_)) //error
        .WillRepeatedly(Return((1)));
    ret = imageProcessor->DvppCreateChannelWithoutNotify(&channelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _)) //error
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    ret = imageProcessor->DvppCreateChannelWithoutNotify(&channelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(DvppTest, DvppCreateChannelWithoutNotifyTest02)
{
    acldvppChannelDesc channelDesc;
    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.notify = (void*)0x1;
    channelDesc.dvppDesc.retCode = 1;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return((RT_ERROR_NONE)));
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    aclError ret = imageProcessor->DvppCreateChannelWithoutNotify(&channelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    channelDesc.dvppDesc.retCode = 0;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillRepeatedly(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamSynchronize(_))
        .WillOnce(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamDestroy(_))
        .WillOnce(Return((1)));
    ret = imageProcessor->DvppCreateChannelWithoutNotify(&channelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(DvppTest, DvppDestroyChannelWithoutNotify)
{
    acldvppChannelDesc channelDesc;
    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.notify = (void*)0x1;

    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return((1)));
    aclError ret = imageProcessor->DvppDestroyChannelWithoutNotify(&channelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillOnce(Return((1)));
    ret = imageProcessor->DvppDestroyChannelWithoutNotify(&channelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamCreate(_, _))
        .WillOnce(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillOnce(Return((RT_ERROR_NONE)));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamDestroy(_))
        .WillOnce(Return((1)));
    ret = imageProcessor->DvppDestroyChannelWithoutNotify(&channelDesc);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(DvppTest, LaunchDvppTask)
{
    acldvppChannelDesc channelDesc;
    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.notify = (void*)0x1;
    channelDesc.isNeedNotify = false;
    char *args;
    uint32_t argsSize;
    char *kernelName;
    aclrtStream stream;

    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillOnce(Return((1)));
    aclError ret = imageProcessor->LaunchDvppTask(&channelDesc, args, argsSize, kernelName, stream);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(DvppTest, CreateVencChannelDescOnDeviceTest)
{
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return((1)));
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    aclvencChannelDesc *ret = imageProcessor->CreateVencChannelDescOnDevice();
    EXPECT_EQ(ret, nullptr);
}

TEST_F(DvppTest, ValidateDvppResizeConfigTest)
{
    acldvppResizeConfig *config = acldvppCreateResizeConfig();
    config->dvppResizeConfig.interpolation = 5;
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    imageProcessor->ValidateDvppResizeConfig(config);
    acldvppDestroyResizeConfig(config);
}

TEST_F(DvppTest, DestroyStreamTest)
{
    rtStream_t stream = (rtStream_t)0x11;
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtStreamDestroy(_))
        .WillOnce(Return((1)));
    imageProcessor->DestroyStream(stream);
}

TEST_F(DvppTest, CreateChannelDescOnHostTest)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillOnce(Return((VOID *)nullptr));
    imageProcessor->CreateChannelDescOnHost();
}

TEST_F(DvppTest, CreatePicDescOnDeviceTest)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return((1)));
    imageProcessor->CreatePicDescOnDevice();
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return((0)));
    imageProcessor->CreatePicDescOnDevice();
}

TEST_F(DvppTest, CreatePicDescOnHostTest)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), mmAlignMalloc(_, _))
        .WillOnce(Return((VOID *)nullptr));
    imageProcessor->CreatePicDescOnHost();
}

TEST_F(DvppTest, aclvencV200_CreateHistOnDeviceTest)
{
    acl::dvpp::ImageProcessorV200 v200(ACL_HOST);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return((1)));
    v200.CreateHistOnDevice();
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMalloc(_, _, _))
        .WillOnce(Return((0)));
    v200.CreateHistOnDevice();
}

TEST_F(DvppTest, aclvencV200_acldvppSetChannelDescModeTest)
{
    acl::dvpp::ImageProcessorV200 v200(ACL_HOST);
    acldvppChannelDesc *channelDesc = acldvppCreateChannelDesc();
    uint32_t mode = -1;
    v200.acldvppSetChannelDescMode(channelDesc, mode);
    acldvppDestroyChannelDesc(channelDesc);
}

TEST_F(DvppTest, aclvencV200_ValidateDvppResizeConfigTest)
{
    acl::dvpp::ImageProcessorV200 v200(ACL_HOST);
    acldvppResizeConfig *config = acldvppCreateResizeConfig();
    config->dvppResizeConfig.interpolation = 3;
    v200.ValidateDvppResizeConfig(config);
    acldvppDestroyResizeConfig(config);
}
