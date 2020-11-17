#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include "utils/acl_jpeglib.h"
#include "securec.h"
#include <memory>

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

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtNotifyWait(_, _))
        .WillOnce(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(acldvppJpegEncodeAsync(&channelDesc, &inputDesc, outputDesc.dataBuffer.data, &outputDesc.dvppPicDesc.size,&config, stream),ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtCpuKernelLaunch(_, _, _, _, _, _, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_FEATURE_NOT_SUPPORT));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtGetRunMode(_))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_NE(acldvppJpegEncodeAsync(&channelDesc, &inputDesc, outputDesc.dataBuffer.data, &outputDesc.dvppPicDesc.size,
                                     &config, stream), ACL_SUCCESS);
}

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
    cinfo->marker_list = nullptr;
    jpeg_component_info comp_info[3];
    comp_info[0].h_samp_factor = comp_info[0].v_samp_factor;
    cinfo->comp_info = comp_info;
}

void jpeg_CreateDecompress_invok_ACL_JPEG_CSS_420(j_decompress_ptr cinfo, int version, size_t structsize)
{
    size_t Y = 0;
    size_t U = 1;
    size_t V = 2;
    cinfo->marker_list = nullptr;
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
    cinfo->marker_list = nullptr;
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
    cinfo->marker_list = nullptr;
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
    cinfo->marker_list = nullptr;
    cinfo->jpeg_color_space = JCS_YCbCr;
    cinfo->num_components = 3;
    jpeg_component_info comp_info[3];
    comp_info[U].h_samp_factor = comp_info[V].h_samp_factor;
    comp_info[U].v_samp_factor = comp_info[V].v_samp_factor;
    comp_info[Y].h_samp_factor = comp_info[U].h_samp_factor;
    comp_info[Y].v_samp_factor = comp_info[U].v_samp_factor * 2;
    cinfo->comp_info = comp_info;
}

TEST_F(JpegTest, TestGetJpegWidthHeightAndComponents)
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t size = 10;
    int32_t components = 0;
    char buff[100];
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_CreateDecompress(_, _, _))
        .WillRepeatedly(Invoke(jpeg_CreateDecompress_invok_ACL_JPEG_CSS_GRAY));
    aclError ret = acldvppJpegGetImageInfo(buff, size, &width, &height, &components);
    EXPECT_EQ(ret, ACL_SUCCESS);

    acldvppJpegFormat format;
    ret = acldvppJpegGetImageInfoV2(buff, size, &width, &height, &components, &format);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(format, ACL_JPEG_CSS_UNKNOWN);
    ret = acldvppJpegGetImageInfoV2(buff, size, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(JpegTest, acldvppJpegGetImageInfoV2_ACL_JPEG_CSS_GRAY)
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t size = 10;
    int32_t components = 0;
    char buff[10];

    acldvppJpegFormat format;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_read_header(_, _))
        .WillRepeatedly(Return(JPEG_HEADER_OK));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_CreateDecompress(_, _, _))
        .WillRepeatedly(Invoke(jpeg_CreateDecompress_invok_ACL_JPEG_CSS_GRAY));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_save_markers(_, _, _))
        .WillRepeatedly(Return());
    aclError ret = acldvppJpegGetImageInfoV2(buff, size, &width, &height, &components, &format);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(format, 1000);
}

TEST_F(JpegTest, acldvppJpegGetImageInfoV2_ACL_JPEG_CSS_420)
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t size = 10;
    int32_t components = 0;
    char buff[10];
    acldvppJpegFormat format;
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_CreateDecompress(_, _, _))
        .WillRepeatedly(Invoke(jpeg_CreateDecompress_invok_ACL_JPEG_CSS_420));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_save_markers(_, _, _))
        .WillRepeatedly(Return());

    aclError ret = acldvppJpegGetImageInfoV2(buff, size, &width, &height, &components, &format);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(format, 1000);
}

TEST_F(JpegTest, acldvppJpegGetImageInfoV2_ACL_JPEG_CSS_422)
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t size = 10;
    int32_t components = 0;
    char buff[10];
    acldvppJpegFormat format;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_CreateDecompress(_, _, _))
        .WillRepeatedly(Invoke(jpeg_CreateDecompress_invok_ACL_JPEG_CSS_422));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_save_markers(_, _, _))
        .WillRepeatedly(Return());

    aclError ret = acldvppJpegGetImageInfoV2(buff, size, &width, &height, &components, &format);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(format, 1000);
}

TEST_F(JpegTest, acldvppJpegGetImageInfoV2_ACL_JPEG_CSS_444)
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t size = 10;
    int32_t components = 0;
    char buff[10];
    acldvppJpegFormat format;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_CreateDecompress(_, _, _))
        .WillRepeatedly(Invoke(jpeg_CreateDecompress_invok_ACL_JPEG_CSS_444));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_save_markers(_, _, _))
        .WillRepeatedly(Return());
    aclError ret = acldvppJpegGetImageInfoV2(buff, size, &width, &height, &components, &format);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(format, 1000);
}

TEST_F(JpegTest, acldvppJpegGetImageInfoV2_ACL_JPEG_CSS_440)
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t size = 10;
    int32_t components = 0;
    char buff[10];
    acldvppJpegFormat format;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_CreateDecompress(_, _, _))
        .WillRepeatedly(Invoke(jpeg_CreateDecompress_invok_ACL_JPEG_CSS_440));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_save_markers(_, _, _))
        .WillRepeatedly(Return());

    aclError ret = acldvppJpegGetImageInfoV2(buff, size, &width, &height, &components, &format);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_EQ(format, 1000);
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

int jpeg_read_header_libjpegHandlerGrayInvoke(j_decompress_ptr cinfo, boolean require_image)
{
    const uint32_t jpegdHeight = 32;
    const uint32_t jpegdWidth = 32;
    cinfo->marker_list = nullptr;
    cinfo->image_width = jpegdWidth;
    cinfo->image_height = jpegdHeight;
    cinfo->jpeg_color_space = JCS_GRAYSCALE;
    require_image = true;
    return sizeof(cinfo);
}

TEST_F(JpegTest, TestPredictJpegDecSizeGray)
{
    const uint32_t jpegdHeight = 32;
    const uint32_t jpegdWidth = 32;
    struct jpeg_decompress_struct libjpegHandlerGray;
    libjpegHandlerGray.image_width = jpegdWidth;
    libjpegHandlerGray.image_height = jpegdHeight;
    libjpegHandlerGray.jpeg_color_space = JCS_GRAYSCALE;
    char data[10] = {0};
    uint32_t size = 0;

    // output pixel format is PIXEL_FORMAT_YUV_SEMIPLANAR_420
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_CreateDecompress(_, _, _))
        .WillRepeatedly(Invoke(jpeg_CreateDecompress_invok_ACL_JPEG_CSS_GRAY));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_read_header(_, _))
        .WillOnce(Invoke(jpeg_read_header_libjpegHandlerGrayInvoke))
        .WillRepeatedly(Return(JPEG_HEADER_OK));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_save_markers(_, _, _))
        .WillRepeatedly(Return());

    EXPECT_EQ(acldvppJpegPredictDecSize(data,
                                        sizeof(data),
                                        PIXEL_FORMAT_YUV_SEMIPLANAR_420,
                                        &size),
              ACL_SUCCESS);
    uint32_t alignWidth = (jpegdWidth + 127) & (~127);
    uint32_t alignHeight = (jpegdHeight + 15) & (~15);
    EXPECT_EQ(size, alignWidth*alignHeight*3/2);

    // output pixel format is unknown and image is gray
    EXPECT_EQ(acldvppJpegPredictDecSize(data,
                                        sizeof(data),
                                        PIXEL_FORMAT_UNKNOWN,
                                        &size),
              ACL_ERROR_INVALID_PARAM);
    EXPECT_EQ(size, alignWidth*alignHeight*3/2);
}

TEST_F(JpegTest, TestOrientationImage)
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t size = 10;
    int32_t components = 0;
    char buff[100] = {0};

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_CreateDecompress(_, _, _))
        .WillRepeatedly(Invoke(jpeg_CreateDecompress_invok_ACL_JPEG_CSS_GRAY));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_save_markers(_, _, _))
        .WillRepeatedly(Return());
    aclError ret = acldvppJpegGetImageInfo(buff, size, &width, &height, &components);
    EXPECT_EQ(ret, ACL_SUCCESS);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_CreateDecompress(_, _, _))
        .WillRepeatedly(Invoke(jpeg_CreateDecompress_invok_ACL_JPEG_CSS_GRAY));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_save_markers(_, _, _))
        .WillRepeatedly(Return());

    ret = acldvppJpegGetImageInfo(buff, size, &width, &height, &components);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(JpegTest, ImageProcessorV100_001)
{
    acl::dvpp::ImageProcessorV100 imgProcessor(ACL_HOST);
    acldvppChannelDesc channelDesc;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    acldvppJpegeConfig config;
    acldvppRoiConfig roiConfig;
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
    acldvppResizeConfig resizeConfig;
    uint32_t resizeConfigSize;

    imgProcessor.ValidateParamForDvppVpcCropResizePaste(&channelDesc, &inputDesc, &outputDesc, &roiConfig,
        nullptr, false, &resizeConfig, true, resizeConfigSize);
    void *data = (void*)0x0001;
    uint32_t size = 1;
    acldvppJpegeConfig jpegConfig;
    inputDesc.dvppPicDesc.format = 1000;
    EXPECT_EQ(imgProcessor.acldvppJpegEncodeAsync(&channelDesc, &inputDesc, data, &size, &jpegConfig, stream), ACL_ERROR_FORMAT_NOT_MATCH);
}

TEST_F(JpegTest, ImageProcessorV200_001)
{
    acl::dvpp::ImageProcessorV200 imgProcessor(ACL_HOST);
    acldvppChannelDesc channelDesc;
    channelDesc.dataBuffer.data = (void*)0x1;
    channelDesc.dataBuffer.length = sizeof(uint64_t);
    channelDesc.shareBuffer.data = (void*)0x2;
    channelDesc.shareBuffer.length = sizeof(uint32_t);
    channelDesc.notify = (void*)0x1;
    acldvppPicDesc inputDesc;
    acldvppPicDesc outputDesc;
    inputDesc.dataBuffer.data = (void*)0x1;
    inputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    outputDesc.dataBuffer.data = (void*)0x1;
    outputDesc.dataBuffer.length = sizeof(aicpu::dvpp::DvppPicDesc);
    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_YUV_400;
    outputDesc.dvppPicDesc.format = 1000;
    outputDesc.dvppPicDesc.size = 2 * 1024;
    aclrtStream stream;
    EXPECT_EQ(imgProcessor.acldvppVpcPyrDownAsync(&channelDesc, &inputDesc, &outputDesc, nullptr, stream), ACL_ERROR_FORMAT_NOT_MATCH);

    inputDesc.dvppPicDesc.format = PIXEL_FORMAT_BGR_888_PLANAR;
    acldvppHist hist;
    hist.dvppHistDesc.hist= (uint32_t*)(0x0001);
    hist.dataBuffer.data = (void*)0x1;
    hist.shareBuffer.data = (void*)0x2;
    hist.shareBuffer.length = 4;
    hist.dvppHistDesc.dims = 1;
    hist.dvppHistDesc.lens[0] = 1;
    EXPECT_EQ(imgProcessor.acldvppVpcCalcHistAsync(&channelDesc, &inputDesc, &hist, nullptr, stream), ACL_ERROR_FORMAT_NOT_MATCH);
    Mock::VerifyAndClear((void *)(&MockFunctionTest::aclStubInstance()));

    acl::dvpp::ImageProcessorV200 imgProcessorV200(ACL_DEVICE);
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemset(_, _, _, _))
        .WillRepeatedly(Return(ACL_ERROR_RT_PARAM_INVALID));
    EXPECT_EQ(imgProcessorV200.acldvppClearHist(&hist), ACL_ERROR_RT_PARAM_INVALID);

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), rtMemset(_, _, _, _))
        .WillRepeatedly(Return(ACL_SUCCESS));
    EXPECT_EQ(imgProcessorV200.acldvppClearHist(&hist), ACL_SUCCESS);
}

TEST_F(JpegTest, acldvppJpegGetImageInfoV2)
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t size = 10;
    int32_t components = 0;
    const void *data = (void *)0x1;
    acldvppJpegFormat *format = nullptr;

    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_CreateDecompress(_, _, _))
        .WillRepeatedly(Invoke(jpeg_CreateDecompress_invok_ACL_JPEG_CSS_GRAY));
    EXPECT_CALL(MockFunctionTest::aclStubInstance(), jpeg_save_markers(_, _, _))
        .WillRepeatedly(Return());
    aclError ret = acldvppJpegGetImageInfoV2(data, size, &width, &height, &components, format);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(JpegTest, acldvppJpegPredictEncSize)
{
    acldvppPicDesc *inputDesc = nullptr;
    acldvppJpegeConfig *config = nullptr;
    uint32_t *size;
    aclError ret = acldvppJpegPredictEncSize(inputDesc, config, size);
    EXPECT_NE(ret, ACL_SUCCESS);
}

TEST_F(JpegTest, acldvppJpegPredictDecSize)
{
    const void *data;
    uint32_t dataSize;
    acldvppPixelFormat outputPixelFormat;
    uint32_t *size;

    aclError ret = acldvppJpegPredictDecSize(data, dataSize, outputPixelFormat, size);
    EXPECT_NE(ret, ACL_SUCCESS);
}