#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include "utils/acl_jpeglib.h"
#include "securec.h"

#define protected public
#define private public

#include "acl/ops/acl_dvpp.h"
#include "single_op/op_executor.h"
#include "single_op/dvpp/common/dvpp_def_internal.h"
#include "single_op/dvpp/mgr/dvpp_manager.h"
#include "single_op/dvpp/base/image_processor.h"
#include "single_op/dvpp/v100/image_processor_v100.h"
#include "single_op/dvpp/v200/image_processor_v200.h"
#undef private
#undef protected

#include "acl/acl.h"
#include "runtime/rt.h"
#include "acl_stub.h"

using namespace std;
using namespace testing;
using namespace acl;
using namespace acl::dvpp;

class JpegTest : public testing::Test {
protected:

    void SetUp()
    {
    }

    void TearDown()
    {
        Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));
    }
};

namespace acl {
    namespace dvpp {
        namespace {
            const uint32_t NUM_2 = 2;
            const uint32_t NUM_3 = 3;
            const uint32_t APP1_MAKER = 0xFFE1;
            const uint32_t EXIF0_MAKER = 0x4578;
            const uint32_t EXIF1_MAKER = 0x6966;
            const uint32_t EXIF2_MAKER = 0;
            const uint32_t BIG_ENDIAN_MAKER = 0x4D4D;
            const uint32_t LITTLE_ENDIAN_MAKER = 0x4949;
            const uint32_t START_OF_SCAN_MAKER = 0xFFDA;
            const uint32_t EIGHT_BITS_OFFSET = 8;
            const uint32_t JPEG_MARK = 0xff;
            const uint16_t BIG_ORIENTATION_MARKER = 0x0112;
            const uint32_t LITTLE_ORIENTATION_MARKER = 0x1201;
            const uint32_t JUMP_BYTE_NUM = 3;
            const uint32_t TWO_BYTE_OFFSET = 2;
            const uint32_t IFD_OFFSET = 0xA;
            const uint32_t IFD_ALL_OFFSET = 0xC;
            enum OrientationValue {
                ORIENTATION_1 = 1,
                ORIENTATION_2,
                ORIENTATION_3,
                ORIENTATION_4,
                ORIENTATION_5,
                ORIENTATION_6,
                ORIENTATION_7,
                ORIENTATION_8
            };
        }
    }
}

TEST_F(JpegTest, TestJpegDecode)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    acldvppRoiConfig decodeArea;
    aclrtStream stream;
    inputDesc.dvppPicDesc.size = 2 *1024;

    EXPECT_NE(acldvppJpegDecodeAsync(nullptr, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream), ACL_SUCCESS);

    channelDesc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppJpegDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream), ACL_SUCCESS);

    channelDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.data = nullptr;
    channelDesc.notify = (void*)0x1;
    EXPECT_NE(acldvppJpegDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream), ACL_SUCCESS);

    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppJpegDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream), ACL_SUCCESS);

    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dvppPicDesc.format = 26;
    EXPECT_NE(acldvppJpegDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream), ACL_SUCCESS);

    inputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    EXPECT_EQ(acldvppJpegDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _,_,_,_))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(
        acldvppJpegDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillRepeatedly(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(
        acldvppJpegDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream),
        ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillOnce(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT))
        .WillOnce(Return(RT_ERROR_NONE));
    EXPECT_NE(acldvppJpegDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream), ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppJpegDecodeAsync(&channelDesc, inputDesc.dataBuffer.data, inputDesc.dvppPicDesc.size, &outputDesc, stream), ACL_SUCCESS);
}


TEST_F(JpegTest, TestJpegEncode_param_check)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    acldvppJpegeConfig config;
    aclrtStream stream;
    channelDesc.dataBuffer.data = (void *) 0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    channelDesc.shareBuffer.data = (void *) 0x2;
    channelDesc.shareBuffer.length = sizeof(uint32_t);
    inputDesc.dataBuffer.data = (void *) 0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void *) 0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    EXPECT_NE(acldvppJpegEncodeAsync(nullptr, &inputDesc, outputDesc.dataBuffer.data, &outputDesc.dvppPicDesc.size,
                                     &config, stream), ACL_SUCCESS);

    channelDesc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppJpegEncodeAsync(&channelDesc, &inputDesc, outputDesc.dataBuffer.data, &outputDesc.dvppPicDesc.size,
                                     &config, stream), ACL_SUCCESS);
    channelDesc.dataBuffer.data = (void *) 0x1;

    inputDesc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppJpegEncodeAsync(&channelDesc, &inputDesc, outputDesc.dataBuffer.data, &outputDesc.dvppPicDesc.size,
                                     &config, stream), ACL_SUCCESS);
    inputDesc.dataBuffer.data = (void *) 0x1;

    outputDesc.dataBuffer.data = nullptr;
    EXPECT_NE(acldvppJpegEncodeAsync(&channelDesc, &inputDesc, outputDesc.dataBuffer.data, &outputDesc.dvppPicDesc.size,
                                     &config, stream), ACL_SUCCESS);
}

TEST_F(JpegTest, TestJpegEncode)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    acldvppJpegeConfig config;
    aclrtStream stream;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    channelDesc.shareBuffer.data = (void*)0x2;
    channelDesc.shareBuffer.length = sizeof(uint32_t);
    channelDesc.notify = (void*)0x1;
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.size = 2 * 1024;

    EXPECT_EQ(
            acldvppJpegEncodeAsync(&channelDesc, &inputDesc, outputDesc.dataBuffer.data, &outputDesc.dvppPicDesc.size,
                                   &config, stream),
            ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(
            acldvppJpegEncodeAsync(&channelDesc, &inputDesc, outputDesc.dataBuffer.data, &outputDesc.dvppPicDesc.size,
                                   &config, stream),
            ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return(RT_ERROR_NONE))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetRunMode(_))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(
            acldvppJpegEncodeAsync(&channelDesc, &inputDesc, outputDesc.dataBuffer.data, &outputDesc.dvppPicDesc.size,
                                   &config, stream),
            ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppJpegEncodeAsync(&channelDesc, &inputDesc, outputDesc.dataBuffer.data, &outputDesc.dvppPicDesc.size,&config, stream),ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetRunMode(_)) // NEVER CALLED
        .WillRepeatedly(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(acldvppJpegEncodeAsync(&channelDesc, &inputDesc, outputDesc.dataBuffer.data, &outputDesc.dvppPicDesc.size,
                                     &config, stream), ACL_SUCCESS);
}
/*
TEST_F(JpegTest, TestJpegEncode2)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    acldvppJpegeConfig config;
    aclrtStream stream;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    channelDesc.shareBuffer.data = (void*)0x2;
    channelDesc.shareBuffer.length = sizeof(uint32_t);
    channelDesc.notify = (void*)0x1;
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.size = 2 * 1024;

    acl::dvpp::DvppManager &dvppManager = acl::dvpp::DvppManager::GetInstance();
    dvppManager.dvppVersion_ = DVPP_KERNELS_V200;
    dvppManager.aclRunMode_ = ACL_DEVICE;
    dvppManager.InitDvppProcessor();

    EXPECT_EQ(
            acldvppJpegEncodeAsync(&channelDesc, &inputDesc, outputDesc.dataBuffer.data, &outputDesc.dvppPicDesc.size,
                                   &config, stream),
            ACL_SUCCESS);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemcpyAsync(_, _, _, _, _, _))
        .WillOnce(Return((ACL_ERROR_RT_PARAM_INVALID)));
    EXPECT_NE(
            acldvppJpegEncodeAsync(&channelDesc, &inputDesc, outputDesc.dataBuffer.data, &outputDesc.dvppPicDesc.size,
                                   &config, stream),
            ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    MOCKER(memcpy_s).stubs()
            .will(returnValue(EOK))
            .then(returnValue(EINVAL));
    EXPECT_NE(
            acldvppJpegEncodeAsync(&channelDesc, &inputDesc, outputDesc.dataBuffer.data, &outputDesc.dvppPicDesc.size,
                                   &config, stream),
            ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    MOCKER(rtNotifyWait).stubs()
        .will(returnValue(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(acldvppJpegEncodeAsync(&channelDesc, &inputDesc, outputDesc.dataBuffer.data, &outputDesc.dvppPicDesc.size,&config, stream),ACL_SUCCESS);
    GlobalMockObject::verify();

    MOCKER(rtCpuKernelLaunch).stubs()
            .will(returnValue(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));
    MOCKER(rtGetRunMode)
        .stubs()
        .will(returnValue(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(acldvppJpegEncodeAsync(&channelDesc, &inputDesc, outputDesc.dataBuffer.data, &outputDesc.dvppPicDesc.size,
                                     &config, stream), ACL_SUCCESS);
}*/

TEST_F(JpegTest, TestJpegEncode3)
{
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    acldvppJpegeConfig config;
    aclrtStream stream;

    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    channelDesc.shareBuffer.data = (void*)0x2;
    channelDesc.shareBuffer.length = sizeof(uint32_t);
    channelDesc.notify = (void*)0x1;
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dvppPicDesc.size = 1;
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    outputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    channelDesc.isNeedNotify = false;
    EXPECT_EQ(acldvppJpegEncodeAsync(&channelDesc, &inputDesc, outputDesc.dataBuffer.data,
        &outputDesc.dvppPicDesc.size, &config, stream), ACL_SUCCESS);
}

void jpeg_CreateDecompress_invok_ACL_JPEG_CSS_GRAY(j_decompress_ptr cinfo, int version, size_t structsize)
{
    cinfo->jpeg_color_space = JCS_YCbCr;
    cinfo->num_components = 1;
    jpeg_component_info comp_info[3];
    comp_info[0].h_samp_factor = comp_info[0].v_samp_factor;
    cinfo->comp_info = comp_info;
}

void jpeg_CreateDecompress_invok_ACL_JPEG_CSS_420(j_decompress_ptr cinfo, int version, size_t structsize)
{
    size_t Y = 0;
    size_t U = 1;
    size_t V = 2;
    cinfo->jpeg_color_space = JCS_YCbCr;
    cinfo->num_components = 3;
    jpeg_component_info comp_info[3];

    comp_info[U].h_samp_factor = comp_info[V].h_samp_factor;
    comp_info[U].v_samp_factor = comp_info[V].v_samp_factor;
    comp_info[Y].h_samp_factor = (comp_info[U].h_samp_factor) * 2;
    comp_info[Y].v_samp_factor = (comp_info[U].v_samp_factor) * 2;
    cinfo->comp_info = comp_info;
}

void jpeg_CreateDecompress_invok_ACL_JPEG_CSS_422(j_decompress_ptr cinfo, int version, size_t structsize)
{
    size_t Y = 0;
    size_t U = 1;
    size_t V = 2;
    cinfo->jpeg_color_space = JCS_YCbCr;
    cinfo->num_components = 3;
    jpeg_component_info comp_info[3];

    comp_info[U].h_samp_factor = comp_info[V].h_samp_factor;
    comp_info[U].v_samp_factor = comp_info[V].v_samp_factor;
    comp_info[Y].h_samp_factor = (comp_info[U].h_samp_factor) * 2;
    comp_info[Y].v_samp_factor = (comp_info[U].v_samp_factor);
    cinfo->comp_info = comp_info;
}

void jpeg_CreateDecompress_invok_ACL_JPEG_CSS_444(j_decompress_ptr cinfo, int version, size_t structsize)
{
    size_t Y = 0;
    size_t U = 1;
    size_t V = 2;
    cinfo->jpeg_color_space = JCS_YCbCr;
    cinfo->num_components = 3;
    jpeg_component_info comp_info[3];

    comp_info[U].h_samp_factor = comp_info[V].h_samp_factor;
    comp_info[U].v_samp_factor = comp_info[V].v_samp_factor;
    comp_info[Y].h_samp_factor = comp_info[U].h_samp_factor;
    comp_info[Y].v_samp_factor = comp_info[U].v_samp_factor;
    cinfo->comp_info = comp_info;
}

void jpeg_CreateDecompress_invok_ACL_JPEG_CSS_440(j_decompress_ptr cinfo, int version, size_t structsize)
{
    size_t Y = 0;
    size_t U = 1;
    size_t V = 2;
    cinfo->jpeg_color_space = JCS_YCbCr;
    cinfo->num_components = 3;
    jpeg_component_info comp_info[3];

    comp_info[U].h_samp_factor = comp_info[V].h_samp_factor;
    comp_info[U].v_samp_factor = comp_info[V].v_samp_factor;
    comp_info[Y].h_samp_factor = comp_info[U].h_samp_factor;
    comp_info[Y].v_samp_factor = comp_info[U].v_samp_factor * 2;
    cinfo->comp_info = comp_info;
}

static uint32_t GetEncodeAlignSize(uint32_t size)
{
    const uint32_t alignSize = 2097152;  // 2M
    size = (size + alignSize - 1) & (~(alignSize-1));
    return size;
}

TEST_F(JpegTest, TestPredictJpegEncSize)
{
    // param according dvpp capability
    const uint32_t jpegeMaxHeight     = 8192;
    const uint32_t jpegeMaxWidth      = 8192;
    const uint32_t jpegeMinHeight     = 32;
    const uint32_t jpegeMinWidth      = 32;
    const uint32_t jpegeHeaderSize    = 640;
    const uint32_t startAlignBytes    = 128;

    acldvppJpegeConfig config;
    acldvppPicDesc inputDesc;
    uint32_t size = 0;
    inputDesc.dvppPicDesc.width = jpegeMinWidth;
    inputDesc.dvppPicDesc.height = jpegeMinHeight;
    inputDesc.dvppPicDesc.widthStride = jpegeMinWidth - 1;
    inputDesc.dvppPicDesc.heightStride = jpegeMinHeight;
    EXPECT_NE(acldvppJpegPredictEncSize(&inputDesc, &config, &size), ACL_SUCCESS);

    inputDesc.dvppPicDesc.widthStride = jpegeMinWidth;
    inputDesc.dvppPicDesc.heightStride = jpegeMinHeight - 1;
    EXPECT_NE(acldvppJpegPredictEncSize(&inputDesc, &config, &size), ACL_SUCCESS);

    inputDesc.dvppPicDesc.widthStride = jpegeMinWidth;
    inputDesc.dvppPicDesc.heightStride = jpegeMinHeight;
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    EXPECT_EQ(acldvppJpegPredictEncSize(&inputDesc, &config, &size), ACL_SUCCESS);
    uint32_t aligndSize = jpegeMinWidth  * jpegeMinHeight * 3 / 2 + jpegeHeaderSize + startAlignBytes;
    aligndSize = GetEncodeAlignSize(aligndSize);
    EXPECT_EQ(size, aligndSize);

    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    EXPECT_EQ(acldvppJpegPredictEncSize(&inputDesc, &config, &size), ACL_SUCCESS);
    aligndSize = jpegeMinWidth  * jpegeMinHeight * 3 / 2 + jpegeHeaderSize + startAlignBytes;
    aligndSize = GetEncodeAlignSize(aligndSize);
    EXPECT_EQ(size, aligndSize);

    inputDesc.dvppPicDesc.widthStride = jpegeMinWidth*2;
    inputDesc.dvppPicDesc.heightStride = jpegeMinHeight*2;
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUYV_PACKED_422;
    EXPECT_EQ(acldvppJpegPredictEncSize(&inputDesc, &config, &size), ACL_SUCCESS);
    aligndSize =  jpegeMinWidth * 2 * jpegeMinHeight * 2 + jpegeHeaderSize + startAlignBytes;
    aligndSize = GetEncodeAlignSize(aligndSize);
    EXPECT_EQ(size, aligndSize);

    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_UYVY_PACKED_422;
    EXPECT_EQ(acldvppJpegPredictEncSize(&inputDesc, &config, &size), ACL_SUCCESS);
    aligndSize =  jpegeMinWidth * 2 * jpegeMinHeight * 2 + jpegeHeaderSize + startAlignBytes;
    aligndSize = GetEncodeAlignSize(aligndSize);
    EXPECT_EQ(size, aligndSize);


    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YVYU_PACKED_422;
    EXPECT_EQ(acldvppJpegPredictEncSize(&inputDesc, &config, &size), ACL_SUCCESS);
    aligndSize =  jpegeMinWidth * 2 * jpegeMinHeight * 2 + jpegeHeaderSize + startAlignBytes;
    aligndSize = GetEncodeAlignSize(aligndSize);
    EXPECT_EQ(size, aligndSize);

    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_VYUY_PACKED_422;
    EXPECT_EQ(acldvppJpegPredictEncSize(&inputDesc, &config, &size), ACL_SUCCESS);
    aligndSize =  jpegeMinWidth * 2 * jpegeMinHeight * 2 + jpegeHeaderSize + startAlignBytes;
    aligndSize = GetEncodeAlignSize(aligndSize);
    EXPECT_EQ(size, aligndSize);

    inputDesc.dvppPicDesc.heightStride = jpegeMinHeight + 1;
    EXPECT_EQ(acldvppJpegPredictEncSize(&inputDesc, &config, &size), ACL_ERROR_INVALID_PARAM);

    inputDesc.dvppPicDesc.heightStride = jpegeMinHeight;
    inputDesc.dvppPicDesc.widthStride = jpegeMinWidth + 1;
    EXPECT_EQ(acldvppJpegPredictEncSize(&inputDesc, &config, &size), ACL_ERROR_INVALID_PARAM);

    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    inputDesc.dvppPicDesc.heightStride = jpegeMinHeight;
    inputDesc.dvppPicDesc.widthStride = jpegeMinWidth + 1;
    EXPECT_EQ(acldvppJpegPredictEncSize(&inputDesc, &config, &size), ACL_ERROR_INVALID_PARAM);
}