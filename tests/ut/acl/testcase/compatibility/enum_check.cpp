#include <gtest/gtest.h>

#ifndef private
#define private public
#include "acl/acl.h"
#include "acl/acl_op_compiler.h"
#include "acl/acl_prof.h"
#include "acl/acl_tdt.h"
#include "acl/ops/acl_cblas.h"
#include "acl/ops/acl_dvpp.h"
#include "acl/ops/acl_fv.h"
#include "log_inner.h"
#undef private
#endif

class UTEST_ACL_compatibility_enum_check : public testing::Test
{
    public:
        UTEST_ACL_compatibility_enum_check() {}
    protected:
        virtual void SetUp() {}
        virtual void TearDown() {}
};

TEST_F(UTEST_ACL_compatibility_enum_check, aclDataType)
{
    aclDataType dataType;
    dataType = (aclDataType)-1;
    EXPECT_EQ(dataType, ACL_DT_UNDEFINED);
 
    dataType = (aclDataType)0;
    EXPECT_EQ(dataType, ACL_FLOAT);

    dataType = (aclDataType)1;
    EXPECT_EQ(dataType, ACL_FLOAT16);

    dataType = (aclDataType)2;
    EXPECT_EQ(dataType, ACL_INT8);

    dataType = (aclDataType)3;
    EXPECT_EQ(dataType, ACL_INT32);

    dataType = (aclDataType)4;
    EXPECT_EQ(dataType, ACL_UINT8);

    dataType = (aclDataType)6;
    EXPECT_EQ(dataType, ACL_INT16);

    dataType = (aclDataType)7;
    EXPECT_EQ(dataType, ACL_UINT16);

    dataType = (aclDataType)8;
    EXPECT_EQ(dataType, ACL_UINT32);

    dataType = (aclDataType)9;
    EXPECT_EQ(dataType, ACL_INT64);
 
    dataType = (aclDataType)10;
    EXPECT_EQ(dataType, ACL_UINT64);

    dataType = (aclDataType)11;
    EXPECT_EQ(dataType, ACL_DOUBLE);

    dataType = (aclDataType)12;
    EXPECT_EQ(dataType, ACL_BOOL);

    dataType = (aclDataType)13;
    EXPECT_EQ(dataType, ACL_STRING);

    dataType = (aclDataType)16;
    EXPECT_EQ(dataType, ACL_COMPLEX64);

    dataType = (aclDataType)17;
    EXPECT_EQ(dataType, ACL_COMPLEX128);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclFormat)
{
    aclFormat format;
    format = (aclFormat)-1;
    EXPECT_EQ(format, ACL_FORMAT_UNDEFINED);

    format = (aclFormat)0;
    EXPECT_EQ(format, ACL_FORMAT_NCHW);

    format = (aclFormat)1;
    EXPECT_EQ(format, ACL_FORMAT_NHWC);

    format = (aclFormat)2;
    EXPECT_EQ(format, ACL_FORMAT_ND);

    format = (aclFormat)3;
    EXPECT_EQ(format, ACL_FORMAT_NC1HWC0);

    format = (aclFormat)4;
    EXPECT_EQ(format, ACL_FORMAT_FRACTAL_Z);

    format = (aclFormat)12;
    EXPECT_EQ(format, ACL_FORMAT_NC1HWC0_C04);

    format = (aclFormat)27;
    EXPECT_EQ(format, ACL_FORMAT_NDHWC);

    format = (aclFormat)29;
    EXPECT_EQ(format, ACL_FORMAT_FRACTAL_NZ);

    format = (aclFormat)30;
    EXPECT_EQ(format, ACL_FORMAT_NCDHW);
 
    format = (aclFormat)32;
    EXPECT_EQ(format, ACL_FORMAT_NDC1HWC0);

    format = (aclFormat)33;
    EXPECT_EQ(format, ACL_FRACTAL_Z_3D);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclMemType)
{
    aclMemType memType;
    memType = (aclMemType)0;
    EXPECT_EQ(memType, ACL_MEMTYPE_DEVICE);

    memType = (aclMemType)1;
    EXPECT_EQ(memType, ACL_MEMTYPE_HOST);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclAippInputFormat)
{
    aclAippInputFormat format;
    format = (aclAippInputFormat)1;
    EXPECT_EQ(format, ACL_YUV420SP_U8);

    format = (aclAippInputFormat)2;
    EXPECT_EQ(format, ACL_XRGB8888_U8);

    format = (aclAippInputFormat)3;
    EXPECT_EQ(format, ACL_RGB888_U8);

    format = (aclAippInputFormat)4;
    EXPECT_EQ(format, ACL_YUV400_U8);

    format = (aclAippInputFormat)5;
    EXPECT_EQ(format, ACL_NC1HWC0DI_FP16);

    format = (aclAippInputFormat)6;
    EXPECT_EQ(format, ACL_NC1HWC0DI_S8);

    format = (aclAippInputFormat)7;
    EXPECT_EQ(format, ACL_ARGB8888_U8);

    format = (aclAippInputFormat)8;
    EXPECT_EQ(format, ACL_YUYV_U8);

    format = (aclAippInputFormat)9;
    EXPECT_EQ(format, ACL_YUV422SP_U8);

    format = (aclAippInputFormat)10;
    EXPECT_EQ(format, ACL_AYUV444_U8);

    format = (aclAippInputFormat)11;
    EXPECT_EQ(format, ACL_RAW10);

    format = (aclAippInputFormat)12;
    EXPECT_EQ(format, ACL_RAW12);

    format = (aclAippInputFormat)13;
    EXPECT_EQ(format, ACL_RAW16);

    format = (aclAippInputFormat)14;
    EXPECT_EQ(format, ACL_RAW24);

    format = (aclAippInputFormat)0xffff;
    EXPECT_EQ(format, ACL_AIPP_RESERVED);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclmdlConfigAttr)
{
    aclmdlConfigAttr configAttr;
    configAttr = (aclmdlConfigAttr)0;
    EXPECT_EQ(configAttr, ACL_MDL_PRIORITY_INT32);

    configAttr = (aclmdlConfigAttr)1;
    EXPECT_EQ(configAttr, ACL_MDL_LOAD_TYPE_SIZET);

    configAttr = (aclmdlConfigAttr)2;
    EXPECT_EQ(configAttr, ACL_MDL_PATH_PTR);

    configAttr = (aclmdlConfigAttr)3;
    EXPECT_EQ(configAttr, ACL_MDL_MEM_ADDR_PTR);

    configAttr = (aclmdlConfigAttr)4;
    EXPECT_EQ(configAttr, ACL_MDL_MEM_SIZET);

    configAttr = (aclmdlConfigAttr)5;
    EXPECT_EQ(configAttr, ACL_MDL_WEIGHT_ADDR_PTR);

    configAttr = (aclmdlConfigAttr)6;
    EXPECT_EQ(configAttr, ACL_MDL_WEIGHT_SIZET);

    configAttr = (aclmdlConfigAttr)7;
    EXPECT_EQ(configAttr, ACL_MDL_WORKSPACE_ADDR_PTR);

    configAttr = (aclmdlConfigAttr)8;
    EXPECT_EQ(configAttr, ACL_MDL_WORKSPACE_SIZET);

    configAttr = (aclmdlConfigAttr)9;
    EXPECT_EQ(configAttr, ACL_MDL_INPUTQ_NUM_SIZET);

    configAttr = (aclmdlConfigAttr)10;
    EXPECT_EQ(configAttr, ACL_MDL_INPUTQ_ADDR_PTR);

    configAttr = (aclmdlConfigAttr)11;
    EXPECT_EQ(configAttr, ACL_MDL_OUTPUTQ_NUM_SIZET);

    configAttr = (aclmdlConfigAttr)12;
    EXPECT_EQ(configAttr, ACL_MDL_OUTPUTQ_ADDR_PTR);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclmdlInputAippType)
{
    aclmdlInputAippType inputAippType;
    inputAippType = (aclmdlInputAippType)0;
    EXPECT_EQ(inputAippType, ACL_DATA_WITHOUT_AIPP);

    inputAippType = (aclmdlInputAippType)1;
    EXPECT_EQ(inputAippType, ACL_DATA_WITH_STATIC_AIPP);

    inputAippType = (aclmdlInputAippType)2;
    EXPECT_EQ(inputAippType, ACL_DATA_WITH_DYNAMIC_AIPP);

    inputAippType = (aclmdlInputAippType)3;
    EXPECT_EQ(inputAippType, ACL_DYNAMIC_AIPP_NODE);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclCompileType)
{
    aclopCompileType compileType;
    compileType = (aclopCompileType)0;
    EXPECT_EQ(compileType, ACL_COMPILE_SYS);

    compileType = (aclopCompileType)1;
    EXPECT_EQ(compileType, ACL_COMPILE_UNREGISTERED);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclCompileOpt)
{
    aclCompileOpt compileOpt;
    compileOpt = (aclCompileOpt)0;
    EXPECT_EQ(compileOpt, ACL_PRECISION_MODE);

    compileOpt = (aclCompileOpt)1;
    EXPECT_EQ(compileOpt, ACL_AICORE_NUM);

    compileOpt = (aclCompileOpt)2;
    EXPECT_EQ(compileOpt, ACL_AUTO_TUNE_MODE);

    compileOpt = (aclCompileOpt)3;
    EXPECT_EQ(compileOpt, ACL_OP_SELECT_IMPL_MODE);

    compileOpt = (aclCompileOpt)4;
    EXPECT_EQ(compileOpt, ACL_OPTYPELIST_FOR_IMPLMODE);

    compileOpt = (aclCompileOpt)5;
    EXPECT_EQ(compileOpt, ACL_OP_DEBUG_LEVEL);

    compileOpt = (aclCompileOpt)6;
    EXPECT_EQ(compileOpt, ACL_DEBUG_DIR);

    compileOpt = (aclCompileOpt)7;
    EXPECT_EQ(compileOpt, ACL_OP_COMPILER_CACHE_MODE);

    compileOpt = (aclCompileOpt)8;
    EXPECT_EQ(compileOpt, ACL_OP_COMPILER_CACHE_DIR);

    compileOpt = (aclCompileOpt)9;
    EXPECT_EQ(compileOpt, ACL_OP_PERFORMANCE_MODE);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclCompileFlag)
{
    aclOpCompileFlag compileFlag;
    compileFlag = (aclCompileFlag)0;
    EXPECT_EQ(compileFlag, ACL_OP_COMPILE_DEFAULT);

    compileFlag = (aclCompileFlag)1;
    EXPECT_EQ(compileFlag, ACL_OP_COMPILE_FUZZ);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclopEngineType)
{
    aclopEngineType engineType;
    engineType = (aclopEngineType)0;
    EXPECT_EQ(engineType, ACL_ENGINE_SYS);

    engineType = (aclopEngineType)1;
    EXPECT_EQ(engineType, ACL_ENGINE_AICORE);

    engineType = (aclopEngineType)2;
    EXPECT_EQ(engineType, ACL_ENGINE_VECTOR);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclprofAicoreMetrics)
{
    aclprofAicoreMetrics aicoreMetrics;
    aicoreMetrics = (aclprofAicoreMetrics)0;
    EXPECT_EQ(aicoreMetrics, ACL_AICORE_ARITHMETIC_UTILIZATION);

    aicoreMetrics = (aclprofAicoreMetrics)1;
    EXPECT_EQ(aicoreMetrics, ACL_AICORE_PIPE_UTILIZATION);

    aicoreMetrics = (aclprofAicoreMetrics)2;
    EXPECT_EQ(aicoreMetrics, ACL_AICORE_MEMORY_BANDWIDTH);

    aicoreMetrics = (aclprofAicoreMetrics)3;
    EXPECT_EQ(aicoreMetrics, ACL_AICORE_L0B_AND_WIDTH);

    aicoreMetrics = (aclprofAicoreMetrics)4;
    EXPECT_EQ(aicoreMetrics, ACL_AICORE_RESOURCE_CONFLICT_RATIO);

    aicoreMetrics = (aclprofAicoreMetrics)0xFF;
    EXPECT_EQ(aicoreMetrics, ACL_AICORE_NONE);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclrtRunMode)
{
    aclrtRunMode runMode;
    runMode = (aclrtRunMode)0;
    EXPECT_EQ(runMode, ACL_DEVICE);

    runMode = (aclrtRunMode)1;
    EXPECT_EQ(runMode, ACL_HOST);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclrtTsId)
{
    aclrtTsId tsId;
    tsId = (aclrtTsId)0;
    EXPECT_EQ(tsId, ACL_TS_ID_AICORE);

    tsId = (aclrtTsId)1;
    EXPECT_EQ(tsId, ACL_TS_ID_AIVECTOR);

    tsId = (aclrtTsId)2;
    EXPECT_EQ(tsId, ACL_TS_ID_RESERVED);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclrtEventStatus)
{
    aclrtEventStatus eventStatus;
    eventStatus = (aclrtEventStatus)0;
    EXPECT_EQ(eventStatus, ACL_EVENT_STATUS_COMPLETE);

    eventStatus = (aclrtEventStatus)1;
    EXPECT_EQ(eventStatus, ACL_EVENT_STATUS_NOT_READY);

    eventStatus = (aclrtEventStatus)2;
    EXPECT_EQ(eventStatus, ACL_EVENT_STATUS_RESERVED);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclrtEventWaitStatus)
{
    aclrtEventStatus eventStatus;
    eventStatus = (aclrtEventStatus)0;
    EXPECT_EQ(eventStatus, ACL_EVENT_WAIT_STATUS_COMPLETE);

    eventStatus = (aclrtEventStatus)1;
    EXPECT_EQ(eventStatus, ACL_EVENT_WAIT_STATUS_NOT_READY);

    eventStatus = (aclrtEventStatus)0xffff;
    EXPECT_EQ(eventStatus, ACL_EVENT_WAIT_STATUS_RESERVED);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclrtCallbackBlockType)
{
    aclrtCallbackBlockType blockType;
    blockType = (aclrtCallbackBlockType)0;
    EXPECT_EQ(blockType, ACL_CALLBACK_NO_BLOCK);

    blockType = (aclrtCallbackBlockType)1;
    EXPECT_EQ(blockType, ACL_CALLBACK_BLOCK);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclrtMemcpyKind)
{
    aclrtMemcpyKind memoryKind;
    memoryKind = (aclrtMemcpyKind)0;
    EXPECT_EQ(memoryKind, ACL_MEMCPY_HOST_TO_HOST);

    memoryKind = (aclrtMemcpyKind)1;
    EXPECT_EQ(memoryKind, ACL_MEMCPY_HOST_TO_DEVICE);

    memoryKind = (aclrtMemcpyKind)2;
    EXPECT_EQ(memoryKind, ACL_MEMCPY_DEVICE_TO_HOST);

    memoryKind = (aclrtMemcpyKind)3;
    EXPECT_EQ(memoryKind, ACL_MEMCPY_DEVICE_TO_DEVICE);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclrtMemMallocPolicy)
{
    aclrtMemMallocPolicy policy;
    policy = (aclrtMemMallocPolicy)0;
    EXPECT_EQ(policy, ACL_MEM_MALLOC_HUGE_FIRST);

    policy = (aclrtMemMallocPolicy)1;
    EXPECT_EQ(policy, ACL_MEM_MALLOC_HUGE_ONLY);

    policy = (aclrtMemMallocPolicy)2;
    EXPECT_EQ(policy, ACL_MEM_MALLOC_NORMAL_ONLY);

    policy = (aclrtMemMallocPolicy)3;
    EXPECT_EQ(policy, ACL_MEM_MALLOC_HUGE_FIRST_P2P);

    policy = (aclrtMemMallocPolicy)4;
    EXPECT_EQ(policy, ACL_MEM_MALLOC_HUGE_ONLY_P2P);

    policy = (aclrtMemMallocPolicy)5;
    EXPECT_EQ(policy, ACL_MEM_MALLOC_NORMAL_ONLY_P2P);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclrtMemAttr)
{
    aclrtMemAttr memAttr;
    memAttr = (aclrtMemAttr)0;
    EXPECT_EQ(memAttr, ACL_DDR_MEM);

    memAttr = (aclrtMemAttr)1;
    EXPECT_EQ(memAttr, ACL_HBM_MEM);

    memAttr = (aclrtMemAttr)2;
    EXPECT_EQ(memAttr, ACL_DDR_MEM_HUGE);

    memAttr = (aclrtMemAttr)3;
    EXPECT_EQ(memAttr, ACL_DDR_MEM_NORMAL);

    memAttr = (aclrtMemAttr)4;
    EXPECT_EQ(memAttr, ACL_HBM_MEM_HUGE);

    memAttr = (aclrtMemAttr)5;
    EXPECT_EQ(memAttr, ACL_HBM_MEM_NORMAL);

    memAttr = (aclrtMemAttr)6;
    EXPECT_EQ(memAttr, ACL_DDR_MEM_P2P_HUGE);

    memAttr = (aclrtMemAttr)7;
    EXPECT_EQ(memAttr, ACL_DDR_MEM_P2P_NORMAL);

    memAttr = (aclrtMemAttr)8;
    EXPECT_EQ(memAttr, ACL_HBM_MEM_P2P_HUGE);

    memAttr = (aclrtMemAttr)9;
    EXPECT_EQ(memAttr, ACL_HBM_MEM_P2P_NORMAL);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclrtGroupAttr)
{
    aclrtGroupAttr groupAttr;
    groupAttr = (aclrtGroupAttr)0;
    EXPECT_EQ(groupAttr, ACL_GROUP_AICORE_INT);

    groupAttr = (aclrtGroupAttr)1;
    EXPECT_EQ(groupAttr, ACL_GROUP_AIV_INT);

    groupAttr = (aclrtGroupAttr)2;
    EXPECT_EQ(groupAttr, ACL_GROUP_AIC_INT);

    groupAttr = (aclrtGroupAttr)3;
    EXPECT_EQ(groupAttr, ACL_GROUP_SDMANUM_INT);

    groupAttr = (aclrtGroupAttr)4;
    EXPECT_EQ(groupAttr, ACL_GROUP_ASQNUM_INT);

    groupAttr = (aclrtGroupAttr)5;
    EXPECT_EQ(groupAttr, ACL_GROUP_GROUPID_INT);
}

TEST_F(UTEST_ACL_compatibility_enum_check, acltdtTensorType)
{
    acltdtTensorType tensorType;
    tensorType = (acltdtTensorType)-1;
    EXPECT_EQ(tensorType, ACL_TENSOR_DATA_UNDEFINED);

    tensorType = (acltdtTensorType)0;
    EXPECT_EQ(tensorType, ACL_TENSOR_DATA_TENSOR);

    tensorType = (acltdtTensorType)1;
    EXPECT_EQ(tensorType, ACL_TENSOR_DATA_END_OF_SEQUENCE);

    tensorType = (acltdtTensorType)2;
    EXPECT_EQ(tensorType, ACL_TENSOR_DATA_ABNORMAL);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclTransType)
{
    aclTransType transType;
    transType = (aclTransType)0;
    EXPECT_EQ(transType, ACL_TRANS_N);

    transType = (aclTransType)1;
    EXPECT_EQ(transType, ACL_TRANS_T);

    transType = (aclTransType)2;
    EXPECT_EQ(transType, ACL_TRANS_NZ);

    transType = (aclTransType)3;
    EXPECT_EQ(transType, ACL_TRANS_NZ_T);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclComputeType)
{
    aclComputeType computeType;
    computeType = (aclComputeType)0;
    EXPECT_EQ(computeType, ACL_COMPUTE_HIGH_PRECISION);

    computeType = (aclComputeType)1;
    EXPECT_EQ(computeType, ACL_COMPUTE_LOW_PRECISION);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclfvSearchType)
{
    aclfvSearchType searchType;
    searchType = (aclfvSearchType)0;
    EXPECT_EQ(searchType, SEARCH_1_N);

    searchType = (aclfvSearchType)1;
    EXPECT_EQ(searchType, SEARCH_N_M);
}

TEST_F(UTEST_ACL_compatibility_enum_check, acldvppStreamFormat)
{
    acldvppStreamFormat format;
    format = (acldvppStreamFormat)0;
    EXPECT_EQ(format, H265_MAIN_LEVEL);

    format = (acldvppStreamFormat)1;
    EXPECT_EQ(format, H264_BASELINE_LEVEL);

    format = (acldvppStreamFormat)2;
    EXPECT_EQ(format, H264_MAIN_LEVEL);

    format = (acldvppStreamFormat)3;
    EXPECT_EQ(format, H264_HIGH_LEVEL);
}

TEST_F(UTEST_ACL_compatibility_enum_check, acldvppChannelMode)
{
    acldvppChannelMode mode;
    mode = (acldvppChannelMode)1;
    EXPECT_EQ(mode, DVPP_CHNMODE_VPC);

    mode = (acldvppChannelMode)2;
    EXPECT_EQ(mode, DVPP_CHNMODE_JPEGD);

    mode = (acldvppChannelMode)4;
    EXPECT_EQ(mode, DVPP_CHNMODE_JPEGE);
}

TEST_F(UTEST_ACL_compatibility_enum_check, acldvppBorderType)
{
    acldvppBorderType type;
    type = (acldvppBorderType)0;
    EXPECT_EQ(type, BORDER_CONSTANT);

    type = (acldvppBorderType)1;
    EXPECT_EQ(type, BORDER_REPLICATE);


    type = (acldvppBorderType)2;
    EXPECT_EQ(type, BORDER_REFLECT);


    type = (acldvppBorderType)3;
    EXPECT_EQ(type, BORDER_REFLECT_101);
}

TEST_F(UTEST_ACL_compatibility_enum_check, aclvencChannelDescParamType)
{
    aclvencChannelDescParamType type;
    type = (aclvencChannelDescParamType)0;
    EXPECT_EQ(type, ACL_VENC_THREAD_ID_UINT64);

    type = (aclvencChannelDescParamType)1;
    EXPECT_EQ(type, ACL_VENC_CALLBACK_PTR);

    type = (aclvencChannelDescParamType)2;
    EXPECT_EQ(type, ACL_VENC_PIXEL_FORMAT_UINT32);

    type = (aclvencChannelDescParamType)3;
    EXPECT_EQ(type, ACL_VENC_ENCODE_TYPE_UINT32);

    type = (aclvencChannelDescParamType)4;
    EXPECT_EQ(type, ACL_VENC_PIC_WIDTH_UINT32);

    type = (aclvencChannelDescParamType)5;
    EXPECT_EQ(type, ACL_VENC_PIC_HEIGHT_UINT32);

    type = (aclvencChannelDescParamType)6;
    EXPECT_EQ(type, ACL_VENC_KEY_FRAME_INTERVAL_UINT32);

    type = (aclvencChannelDescParamType)7;
    EXPECT_EQ(type, ACL_VENC_BUF_ADDR_PTR);

    type = (aclvencChannelDescParamType)8;
    EXPECT_EQ(type, ACL_VENC_BUF_SIZE_UINT32);

    type = (aclvencChannelDescParamType)9;
    EXPECT_EQ(type, ACL_VENC_RC_MODE_UINT32);

    type = (aclvencChannelDescParamType)10;
    EXPECT_EQ(type, ACL_VENC_SRC_RATE_UINT32);

    type = (aclvencChannelDescParamType)11;
    EXPECT_EQ(type, ACL_VENC_MAX_BITRATE_UINT32);

    type = (aclvencChannelDescParamType)12;
    EXPECT_EQ(type, ACL_VENC_MAX_IP_PROP_UINT32);
}

TEST_F(UTEST_ACL_compatibility_enum_check, acldvppJpegFormat)
{
    acldvppJpegFormat format;
    format = (acldvppJpegFormat)0;
    EXPECT_EQ(format, ACL_JPEG_CSS_444);

    format = (acldvppJpegFormat)1;
    EXPECT_EQ(format, ACL_JPEG_CSS_422);

    format = (acldvppJpegFormat)2;
    EXPECT_EQ(format, ACL_JPEG_CSS_420);

    format = (acldvppJpegFormat)3;
    EXPECT_EQ(format, ACL_JPEG_CSS_GRAY);

    format = (acldvppJpegFormat)4;
    EXPECT_EQ(format, ACL_JPEG_CSS_440);

    format = (acldvppJpegFormat)5;
    EXPECT_EQ(format, ACL_JPEG_CSS_411);

    format = (acldvppJpegFormat)1000;
    EXPECT_EQ(format, ACL_JPEG_CSS_UNKNOWN);
}

TEST_F(UTEST_ACL_compatibility_enum_check, acldvppPixelFormat)
{
    acldvppPixelFormat format;
    format = (acldvppPixelFormat)0;
    EXPECT_EQ(format, PIXEL_FORMAT_YUV_400);

    format = (acldvppPixelFormat)1;
    EXPECT_EQ(format, PIXEL_FORMAT_YUV_SEMIPLANAR_420);

    format = (acldvppPixelFormat)2;
    EXPECT_EQ(format, PIXEL_FORMAT_YVU_SEMIPLANAR_420);


    format = (acldvppPixelFormat)3;
    EXPECT_EQ(format, PIXEL_FORMAT_YUV_SEMIPLANAR_422);

    format = (acldvppPixelFormat)4;
    EXPECT_EQ(format, PIXEL_FORMAT_YVU_SEMIPLANAR_422);

    format = (acldvppPixelFormat)5;
    EXPECT_EQ(format, PIXEL_FORMAT_YUV_SEMIPLANAR_444);

    format = (acldvppPixelFormat)6;
    EXPECT_EQ(format, PIXEL_FORMAT_YVU_SEMIPLANAR_444);

    format = (acldvppPixelFormat)7;
    EXPECT_EQ(format, PIXEL_FORMAT_YUYV_PACKED_422);

    format = (acldvppPixelFormat)8;
    EXPECT_EQ(format, PIXEL_FORMAT_UYVY_PACKED_422);

    format = (acldvppPixelFormat)9;
    EXPECT_EQ(format, PIXEL_FORMAT_YVYU_PACKED_422);

    format = (acldvppPixelFormat)10;
    EXPECT_EQ(format, PIXEL_FORMAT_VYUY_PACKED_422);

    format = (acldvppPixelFormat)11;
    EXPECT_EQ(format, PIXEL_FORMAT_YUV_PACKED_444);

    format = (acldvppPixelFormat)12;
    EXPECT_EQ(format, PIXEL_FORMAT_RGB_888);

    format = (acldvppPixelFormat)13;
    EXPECT_EQ(format, PIXEL_FORMAT_BGR_888);

    format = (acldvppPixelFormat)14;
    EXPECT_EQ(format, PIXEL_FORMAT_ARGB_8888);

    format = (acldvppPixelFormat)15;
    EXPECT_EQ(format, PIXEL_FORMAT_ABGR_8888);

    format = (acldvppPixelFormat)16;
    EXPECT_EQ(format, PIXEL_FORMAT_RGBA_8888);

    format = (acldvppPixelFormat)17;
    EXPECT_EQ(format, PIXEL_FORMAT_BGRA_8888);

    format = (acldvppPixelFormat)18;
    EXPECT_EQ(format, PIXEL_FORMAT_YUV_SEMI_PLANNER_420_10BIT);

    format = (acldvppPixelFormat)19;
    EXPECT_EQ(format, PIXEL_FORMAT_YVU_SEMI_PLANNER_420_10BIT);

    format = (acldvppPixelFormat)20;
    EXPECT_EQ(format, PIXEL_FORMAT_YVU_PLANAR_420);

    format = (acldvppPixelFormat)21;
    EXPECT_EQ(format, PIXEL_FORMAT_YVU_PLANAR_422);

    format = (acldvppPixelFormat)22;
    EXPECT_EQ(format, PIXEL_FORMAT_YVU_PLANAR_444);

    format = (acldvppPixelFormat)23;
    EXPECT_EQ(format, PIXEL_FORMAT_RGB_444);

    format = (acldvppPixelFormat)24;
    EXPECT_EQ(format, PIXEL_FORMAT_BGR_444);

    format = (acldvppPixelFormat)25;
    EXPECT_EQ(format, PIXEL_FORMAT_ARGB_4444);

    format = (acldvppPixelFormat)26;
    EXPECT_EQ(format, PIXEL_FORMAT_ABGR_4444);

    format = (acldvppPixelFormat)27;
    EXPECT_EQ(format, PIXEL_FORMAT_RGBA_4444);

    format = (acldvppPixelFormat)28;
    EXPECT_EQ(format, PIXEL_FORMAT_BGRA_4444);

    format = (acldvppPixelFormat)29;
    EXPECT_EQ(format, PIXEL_FORMAT_RGB_555);

    format = (acldvppPixelFormat)30;
    EXPECT_EQ(format, PIXEL_FORMAT_BGR_555);

    format = (acldvppPixelFormat)31;
    EXPECT_EQ(format, PIXEL_FORMAT_RGB_565);

    format = (acldvppPixelFormat)32;
    EXPECT_EQ(format, PIXEL_FORMAT_BGR_565);

    format = (acldvppPixelFormat)33;
    EXPECT_EQ(format, PIXEL_FORMAT_ARGB_1555);

    format = (acldvppPixelFormat)34;
    EXPECT_EQ(format, PIXEL_FORMAT_ABGR_1555);

    format = (acldvppPixelFormat)35;
    EXPECT_EQ(format, PIXEL_FORMAT_RGBA_1555);

    format = (acldvppPixelFormat)36;
    EXPECT_EQ(format, PIXEL_FORMAT_BGRA_1555);

    format = (acldvppPixelFormat)37;
    EXPECT_EQ(format, PIXEL_FORMAT_ARGB_8565);

    format = (acldvppPixelFormat)38;
    EXPECT_EQ(format, PIXEL_FORMAT_ABGR_8565);

    format = (acldvppPixelFormat)39;
    EXPECT_EQ(format, PIXEL_FORMAT_RGBA_8565);

    format = (acldvppPixelFormat)40;
    EXPECT_EQ(format, PIXEL_FORMAT_BGRA_8565);

    format = (acldvppPixelFormat)50;
    EXPECT_EQ(format, PIXEL_FORMAT_RGB_BAYER_8BPP);

    format = (acldvppPixelFormat)51;
    EXPECT_EQ(format, PIXEL_FORMAT_RGB_BAYER_10BPP);

    format = (acldvppPixelFormat)52;
    EXPECT_EQ(format, PIXEL_FORMAT_RGB_BAYER_12BPP);

    format = (acldvppPixelFormat)53;
    EXPECT_EQ(format, PIXEL_FORMAT_RGB_BAYER_14BPP);

    format = (acldvppPixelFormat)54;
    EXPECT_EQ(format, PIXEL_FORMAT_RGB_BAYER_16BPP);

    format = (acldvppPixelFormat)70;
    EXPECT_EQ(format, PIXEL_FORMAT_BGR_888_PLANAR);

    format = (acldvppPixelFormat)71;
    EXPECT_EQ(format, PIXEL_FORMAT_HSV_888_PACKAGE);

    format = (acldvppPixelFormat)72;
    EXPECT_EQ(format, PIXEL_FORMAT_HSV_888_PLANAR);

    format = (acldvppPixelFormat)73;
    EXPECT_EQ(format, PIXEL_FORMAT_LAB_888_PACKAGE);

    format = (acldvppPixelFormat)74;
    EXPECT_EQ(format, PIXEL_FORMAT_LAB_888_PLANAR);

    format = (acldvppPixelFormat)75;
    EXPECT_EQ(format, PIXEL_FORMAT_S8C1);

    format = (acldvppPixelFormat)76;
    EXPECT_EQ(format, PIXEL_FORMAT_S8C2_PACKAGE);

    format = (acldvppPixelFormat)77;
    EXPECT_EQ(format, PIXEL_FORMAT_S8C2_PLANAR);

    format = (acldvppPixelFormat)78;
    EXPECT_EQ(format, PIXEL_FORMAT_S16C1);

    format = (acldvppPixelFormat)79;
    EXPECT_EQ(format, PIXEL_FORMAT_U8C1);

    format = (acldvppPixelFormat)80;
    EXPECT_EQ(format, PIXEL_FORMAT_U16C1);

    format = (acldvppPixelFormat)81;
    EXPECT_EQ(format, PIXEL_FORMAT_S32C1);

    format = (acldvppPixelFormat)82;
    EXPECT_EQ(format, PIXEL_FORMAT_U32C1);

    format = (acldvppPixelFormat)83;
    EXPECT_EQ(format, PIXEL_FORMAT_U64C1);

    format = (acldvppPixelFormat)84;
    EXPECT_EQ(format, PIXEL_FORMAT_S64C1);

    format = (acldvppPixelFormat)1000;
    EXPECT_EQ(format, PIXEL_FORMAT_YUV_SEMIPLANAR_440);

    format = (acldvppPixelFormat)1001;
    EXPECT_EQ(format, PIXEL_FORMAT_YVU_SEMIPLANAR_440);

    format = (acldvppPixelFormat)1002;
    EXPECT_EQ(format, PIXEL_FORMAT_FLOAT32);

    format = (acldvppPixelFormat)10000;
    EXPECT_EQ(format, PIXEL_FORMAT_UNKNOWN);
}