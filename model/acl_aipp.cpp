/**
* @file acl_aipp.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <vector>
#include <string>
#include <map>
#include <functional>
#include <sstream>
#include "log_inner.h"
#include "securec.h"
#include "acl/acl_mdl.h"
#include "common/dynamic_aipp.h"
#include "executor/ge_executor.h"
#include "common/ge_inner_error_codes.h"
#include "common/ge_types.h"
#include "model_desc_internal.h"
#include "error_codes_inner.h"
#include "common_inner.h"
#include "toolchain/profiling_manager.h"
#include "toolchain/resource_statistics.h"
#include "runtime/mem.h"
#include "aipp_param_check.h"
#include "model_desc_internal.h"


namespace {
#define FP16_IF_BOOL_EXEC(expr, exec_expr) \
    { \
        if (expr) { \
            exec_expr; \
        } \
    }

const int16_t FP16_MAX_EXP = 0x001F;
const int16_t FP16_MAX_MAN = 0x03FF;
const int16_t FP16_MAN_HIDE_BIT = 0x0400;
const int16_t FP16_SIGN_INDEX = 15;
const uint32_t FP32_SIGN_MASK = 0x80000000u;
const uint32_t FP32_SIGN_INDEX = 31;
const uint32_t FP32_EXP_MASK = 0x7F800000u;
const uint32_t FP32_MAN_LEN = 23;
const uint32_t FP16_MAN_LEN = 10;
const uint32_t FP32_MAN_MASK = 0x007FFFFFu;
const uint32_t FP32_MAN_HIDE_BIT = 0x00800000u;

constexpr int8_t AIPP_SWITCH_ON = 1;
constexpr int8_t AIPP_SWITCH_OFF = 0;
constexpr int16_t CSC_MATRIX_MIN = -32677;
constexpr int16_t CSC_MATRIX_MAX = 32676;
constexpr uint8_t BIAS_MIN = 0;
constexpr uint8_t BIAS_MAX = 255;
constexpr int32_t IMAGE_SIZE_MIN = 1;
constexpr int32_t IMAGE_SIZE_MAX = 4096;
constexpr int32_t SCF_SIZE_MIN = 16;
constexpr int32_t SCF_SIZEW_MAX = 1920;
constexpr int32_t PADDING_MIN = 0;
constexpr int32_t PADDING_MAX = 32;
constexpr int16_t MEAN_CHN_MIN = 0;
constexpr int16_t MEAN_CHN_MAX = 255;
constexpr float MIN_CHN_MIN = 0;
constexpr float MIN_CHN_MAX = 255;
constexpr float VR_CHN_MIN = -65504;
constexpr float VR_CHN_MAX = 65504;
}

static bool IsRoundOne(uint64_t man, uint16_t truncLen)
{
    uint16_t shiftOut = truncLen - 2; // shift 2 byte
    uint64_t mask = 0x4;
    uint64_t mask1 = 0x2;
    uint64_t mask2;

    mask = mask << shiftOut;
    mask1 = mask1 << shiftOut;
    mask2 = mask1 - 1;
    bool lastBit = ((man & mask) > 0);
    bool truncHigh = ((man & mask1) > 0);
    bool truncLeft = ((man & mask2) > 0);
    return (truncHigh && (truncLeft || lastBit));
}

static void Fp16Normalize(int16_t &exp, uint16_t &man)
{
    FP16_IF_BOOL_EXEC(exp >= FP16_MAX_EXP,
        exp = FP16_MAX_EXP - 1;
        man = FP16_MAX_MAN;
    )
    FP16_IF_BOOL_EXEC(exp == 0 && man == FP16_MAN_HIDE_BIT,
        exp++;
        man = 0;
    )
}

struct Fp16Type {
    uint16_t val;

    Fp16Type(void)
    {
        val = 0x0u;
    }

    explicit Fp16Type(const float &fVal)
    {
        val = 0x0u;
        this->operator=(fVal);
    }

    ~Fp16Type() = default;

    Fp16Type &operator=(const float &fVal)
    {
        uint16_t sRet, mRet;
        int16_t eRet;
        uint32_t eF, mF;
        void *pV = const_cast<float *>(&fVal); // 1:8:23bit sign:exp:man
        uint32_t ui32V = *(static_cast<uint32_t *>(pV));
        uint32_t mLenDelta;

        sRet = static_cast<uint16_t>((ui32V & FP32_SIGN_MASK) >> FP32_SIGN_INDEX); // 4Byte->2Byte
        eF = (ui32V & FP32_EXP_MASK) >> FP32_MAN_LEN; // 8 bit exponent
        mF = (ui32V & FP32_MAN_MASK); // 23 bit mantissa dont't need to care about denormal
        mLenDelta = FP32_MAN_LEN - FP16_MAN_LEN;

        bool needRound = false;
        // Exponent overflow/NaN converts to signed inf/NaN
        FP16_IF_BOOL_EXEC(eF > 0x8Fu,
            // 0x8Fu:142=127+15
            eRet = FP16_MAX_EXP - 1;
            mRet = FP16_MAX_MAN;
        )
        FP16_IF_BOOL_EXEC(eF <= 0x70u,
            // 0x70u:112=127-15 Exponent underflow converts to denormalized half or signed zero
            eRet = 0;
            if (eF >= 0x67) {
                // 0x67:103=127-24 Denormal
                mF = (mF | FP32_MAN_HIDE_BIT);
                uint16_t shiftOut = FP32_MAN_LEN;
                uint64_t m_tmp = (static_cast<uint64_t>(mF)) << (eF - 0x67);

                needRound = IsRoundOne(m_tmp, shiftOut);
                mRet = static_cast<uint16_t>(m_tmp >> shiftOut);
                if (needRound) {
                    mRet++;
                }
            } else if (eF == 0x66 && mF > 0) {
                // 0x66:102 Denormal 0<f_v<min(Denormal)
                mRet = 1;
            } else {
                mRet = 0;
            }
        )
        FP16_IF_BOOL_EXEC(0x8Fu >= eF && eF > 0x70u,
            // Regular case with no overflow or underflow
            eRet = (int16_t) (eF - 0x70u);
            needRound = IsRoundOne(mF, mLenDelta);
            mRet = static_cast<uint16_t>(mF >> mLenDelta);
            if (needRound) {
                mRet++;
            }
            if (mRet & FP16_MAN_HIDE_BIT) {
                eRet++;
            }
        )

        Fp16Normalize(eRet, mRet);
        val =  (((sRet) << FP16_SIGN_INDEX) |
            ((static_cast<uint16_t>(eRet)) << FP16_MAN_LEN) | ((mRet) & FP16_MAX_MAN));
        return *this;
    }
};

static aclError SetIODims(const ge::InputOutputDims oriDims, aclmdlIODims &dstDims)
{
    ACL_LOG_DEBUG("start to execute SetIODims");
    dstDims.dimCount = oriDims.dim_num;
    if (oriDims.dims.size() > ACL_MAX_DIM_CNT) {
        ACL_LOG_ERROR("[Check][Params]size of dims[%zu] must be smaller than ACL_MAX_DIM_CNT(128)",
            oriDims.dims.size());
        return ACL_ERROR_GE_FAILURE;
    }
    for (size_t i = 0; i < oriDims.dims.size(); ++i) {
        dstDims.dims[i] = oriDims.dims[i];
    }
    if (oriDims.name.empty()) {
        ACL_LOG_WARN("the name of oriDims is empty");
        return ACL_SUCCESS;
    }
    auto ret = strncpy_s(dstDims.name, sizeof(dstDims.name), oriDims.name.c_str(), oriDims.name.size());
    if (ret != EOK) {
        ACL_LOG_ERROR("[Copy][Str]call strncpy_s failed");
        return ACL_ERROR_FAILURE;
    }
    return ACL_SUCCESS;
}

aclmdlAIPP *aclmdlCreateAIPP(uint64_t batchSize)
{
    aclmdlAIPP *aippParmsSet = nullptr;
    try {
        ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_AIPP);
        ACL_LOG_INFO("start to execute aclmdlCreateAIPP, batchSize[%lu]", batchSize);
        if (batchSize == 0) {
            ACL_LOG_ERROR("[Check][BatchSize]the batchSize can't be zero");
            return nullptr;
        }
        aippParmsSet = new(std::nothrow) aclmdlAIPP();
        if (aippParmsSet == nullptr) {
            ACL_LOG_ERROR("[Check][ParmsSet]new aclmdlAIPP fail");
            return nullptr;
        }

        auto ret = memset_s(aippParmsSet, sizeof(aclmdlAIPP), 0, sizeof(aclmdlAIPP));
        if (ret != EOK) {
            ACL_LOG_ERROR("[Set][Mem]memset failed, result[%d]", ret);
            ACL_DELETE(aippParmsSet);
            return nullptr;
        }

        aippParmsSet->batchSize = batchSize;
        aippParmsSet->aippParms.batchNum = static_cast<int8_t>(batchSize);
        aippParmsSet->aippBatchPara.resize(batchSize);
        ACL_LOG_INFO("the size of aippBatchPara is [%zu]", aippParmsSet->aippBatchPara.size());
        for (uint64_t i = 0; i < batchSize; i++) {
            aippParmsSet->aippBatchPara[i].dtcPixelVarReciChn0 = Fp16Type(1.0).val;
            aippParmsSet->aippBatchPara[i].dtcPixelVarReciChn1 = Fp16Type(1.0).val;
            aippParmsSet->aippBatchPara[i].dtcPixelVarReciChn2 = Fp16Type(1.0).val;
            aippParmsSet->aippBatchPara[i].dtcPixelVarReciChn3 = Fp16Type(1.0).val;
        }
        ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_AIPP);
        return aippParmsSet;
    } catch (std::bad_alloc &) {
        ACL_LOG_ERROR("[Create][AIPP]aclmdlCreateAIPP fail, bad memory allocation, batchSize[%lu]", batchSize);
    } catch (std::length_error &) {
        ACL_LOG_ERROR("[Create][AIPP]aclmdlCreateAIPP fail with length error, batchSize[%lu]", batchSize);
    }

    ACL_DELETE(aippParmsSet);
    return nullptr;
}

aclError aclmdlDestroyAIPP(const aclmdlAIPP *aippParmsSet)
{
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_AIPP);
    ACL_LOG_INFO("start to execute aclmdlDestroyAIPP");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(aippParmsSet);
    ACL_DELETE_AND_SET_NULL(aippParmsSet);
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_AIPP);
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPInputFormat(aclmdlAIPP *aippParmsSet, aclAippInputFormat inputFormat)
{
    ACL_LOG_INFO("start to execute aclmdlSetAIPPInputFormat");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(aippParmsSet);
    static std::map<aclAippInputFormat, CceAippInputFormat> inputFormatMap = {
        {ACL_YUV420SP_U8, CCE_YUV420SP_U8},
        {ACL_XRGB8888_U8, CCE_XRGB8888_U8},
        {ACL_RGB888_U8,   CCE_RGB888_U8},
        {ACL_YUV400_U8,   CCE_YUV400_U8},
        {ACL_NC1HWC0DI_FP16, CCE_NC1HWC0DI_FP16},
        {ACL_NC1HWC0DI_S8, CCE_NC1HWC0DI_S8},
        {ACL_ARGB8888_U8, CCE_ARGB8888_U8},
        {ACL_YUYV_U8, CCE_YUYV_U8},
        {ACL_YUV422SP_U8, CCE_YUV422SP_U8},
        {ACL_AYUV444_U8, CCE_AYUV444_U8},
        {ACL_RAW10, CCE_RAW10},
        {ACL_RAW12, CCE_RAW12},
        {ACL_RAW16, CCE_RAW16},
        {ACL_RAW24, CCE_RAW24}
    };

    auto it = inputFormatMap.find(inputFormat);
    if (it == inputFormatMap.end()) {
        ACL_LOG_ERROR("[Unsupported][Format]unsupported inputFormat[%d]", static_cast<int32_t>(inputFormat));
        return ACL_ERROR_INVALID_PARAM;
    }

    aippParmsSet->aippParms.inputFormat = static_cast<uint8_t>(it->second);
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPCscParams(aclmdlAIPP *aippParmsSet, int8_t cscSwitch,
                                int16_t cscMatrixR0C0, int16_t cscMatrixR0C1, int16_t cscMatrixR0C2,
                                int16_t cscMatrixR1C0, int16_t cscMatrixR1C1, int16_t cscMatrixR1C2,
                                int16_t cscMatrixR2C0, int16_t cscMatrixR2C1, int16_t cscMatrixR2C2,
                                uint8_t cscOutputBiasR0, uint8_t cscOutputBiasR1, uint8_t cscOutputBiasR2,
                                uint8_t cscInputBiasR0, uint8_t cscInputBiasR1, uint8_t cscInputBiasR2)
{
    ACL_LOG_INFO("start to execute aclmdlSetAIPPCscParams");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(aippParmsSet);

    ACL_CHECK_RANGE_INT(cscSwitch, AIPP_SWITCH_OFF, AIPP_SWITCH_ON);
    aippParmsSet->aippParms.cscSwitch = cscSwitch;
    if (cscSwitch == 0) {
        ACL_LOG_INFO("cscSwitch[%d] is off", cscSwitch);
        return ACL_SUCCESS;
    }
    ACL_CHECK_RANGE_INT(cscMatrixR0C0, CSC_MATRIX_MIN, CSC_MATRIX_MAX);
    ACL_CHECK_RANGE_INT(cscMatrixR0C1, CSC_MATRIX_MIN, CSC_MATRIX_MAX);
    ACL_CHECK_RANGE_INT(cscMatrixR0C2, CSC_MATRIX_MIN, CSC_MATRIX_MAX);

    ACL_CHECK_RANGE_INT(cscMatrixR1C0, CSC_MATRIX_MIN, CSC_MATRIX_MAX);
    ACL_CHECK_RANGE_INT(cscMatrixR1C1, CSC_MATRIX_MIN, CSC_MATRIX_MAX);
    ACL_CHECK_RANGE_INT(cscMatrixR1C2, CSC_MATRIX_MIN, CSC_MATRIX_MAX);

    ACL_CHECK_RANGE_INT(cscMatrixR2C0, CSC_MATRIX_MIN, CSC_MATRIX_MAX);
    ACL_CHECK_RANGE_INT(cscMatrixR2C1, CSC_MATRIX_MIN, CSC_MATRIX_MAX);
    ACL_CHECK_RANGE_INT(cscMatrixR2C2, CSC_MATRIX_MIN, CSC_MATRIX_MAX);

    ACL_CHECK_RANGE_UINT(cscOutputBiasR0, BIAS_MIN, BIAS_MAX);
    ACL_CHECK_RANGE_UINT(cscOutputBiasR1, BIAS_MIN, BIAS_MAX);
    ACL_CHECK_RANGE_UINT(cscOutputBiasR2, BIAS_MIN, BIAS_MAX);

    ACL_CHECK_RANGE_UINT(cscInputBiasR0, BIAS_MIN, BIAS_MAX);
    ACL_CHECK_RANGE_UINT(cscInputBiasR1, BIAS_MIN, BIAS_MAX);
    ACL_CHECK_RANGE_UINT(cscInputBiasR2, BIAS_MIN, BIAS_MAX);
    aippParmsSet->aippParms.cscMatrixR0C0 = cscMatrixR0C0;
    aippParmsSet->aippParms.cscMatrixR0C1 = cscMatrixR0C1;
    aippParmsSet->aippParms.cscMatrixR0C2 = cscMatrixR0C2;
    aippParmsSet->aippParms.cscMatrixR1C0 = cscMatrixR1C0;
    aippParmsSet->aippParms.cscMatrixR1C1 = cscMatrixR1C1;
    aippParmsSet->aippParms.cscMatrixR1C2 = cscMatrixR1C2;
    aippParmsSet->aippParms.cscMatrixR2C0 = cscMatrixR2C0;
    aippParmsSet->aippParms.cscMatrixR2C1 = cscMatrixR2C1;
    aippParmsSet->aippParms.cscMatrixR2C2 = cscMatrixR2C2;
    aippParmsSet->aippParms.cscOutputBiasR0 = cscOutputBiasR0;
    aippParmsSet->aippParms.cscOutputBiasR1 = cscOutputBiasR1;
    aippParmsSet->aippParms.cscOutputBiasR2 = cscOutputBiasR2;
    aippParmsSet->aippParms.cscInputBiasR0 = cscInputBiasR0;
    aippParmsSet->aippParms.cscInputBiasR1 = cscInputBiasR1;
    aippParmsSet->aippParms.cscInputBiasR2 = cscInputBiasR2;
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPRbuvSwapSwitch(aclmdlAIPP *aippParmsSet, int8_t rbuvSwapSwitch)
{
    ACL_LOG_INFO("start to execute aclmdlSetAIPPRbuvSwapSwitch");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(aippParmsSet);
    ACL_CHECK_RANGE_INT(rbuvSwapSwitch, AIPP_SWITCH_OFF, AIPP_SWITCH_ON);
    aippParmsSet->aippParms.rbuvSwapSwitch = rbuvSwapSwitch;
    ACL_LOG_INFO("successfully execute aclmdlSetAIPPRbuvSwapSwitch");
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPAxSwapSwitch(aclmdlAIPP *aippParmsSet, int8_t axSwapSwitch)
{
    ACL_LOG_INFO("start to execute aclmdlSetAIPPAxSwapSwitch");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(aippParmsSet);
    ACL_CHECK_RANGE_INT(axSwapSwitch, AIPP_SWITCH_OFF, AIPP_SWITCH_ON);
    aippParmsSet->aippParms.axSwapSwitch = axSwapSwitch;
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPSrcImageSize(aclmdlAIPP *aippParmsSet, int32_t srcImageSizeW, int32_t srcImageSizeH)
{
    ACL_LOG_INFO("start to execute aclmdlSetAIPPSrcImageSize");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(aippParmsSet);
    ACL_CHECK_RANGE_INT(srcImageSizeW, IMAGE_SIZE_MIN + 1, IMAGE_SIZE_MAX);
    ACL_CHECK_RANGE_INT(srcImageSizeH, IMAGE_SIZE_MIN, IMAGE_SIZE_MAX);
    aippParmsSet->aippParms.srcImageSizeW = srcImageSizeW;
    aippParmsSet->aippParms.srcImageSizeH = srcImageSizeH;
    ACL_LOG_INFO("successfully execute aclmdlSetAIPPSrcImageSize");
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPScfParams(aclmdlAIPP *aippParmsSet, int8_t scfSwitch,
                                int32_t scfInputSizeW, int32_t scfInputSizeH,
                                int32_t scfOutputSizeW, int32_t scfOutputSizeH,
                                uint64_t batchIndex)
{
    ACL_LOG_INFO("start to execute aclmdlSetAIPPScfParams");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(aippParmsSet);
    uint64_t aippBatchParaSize = static_cast<uint64_t>(aippParmsSet->aippBatchPara.size());
    if (batchIndex >= aippBatchParaSize) {
        ACL_LOG_ERROR("[Check][Param]Set batch parameter Failed, batch_index (%lu) is greater than or "
            "equal to batch_number (%lu)", batchIndex, aippBatchParaSize);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("batch_index (%lu) is greater than or "
            "equal to batch_number (%lu)", batchIndex, aippBatchParaSize);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_AIPP_MSG, std::vector<std::string>({"param", "reason"}),
            std::vector<std::string>({"batch_index", errMsg}));
        return ACL_ERROR_INVALID_PARAM;
    }
    ACL_CHECK_RANGE_INT(scfSwitch, AIPP_SWITCH_OFF, AIPP_SWITCH_ON);
    aippParmsSet->aippBatchPara[batchIndex].scfSwitch = scfSwitch;
    if (scfSwitch == 0) {
        ACL_LOG_INFO("scfSwitch[%d] is off", scfSwitch);
        return ACL_SUCCESS;
    }
    ACL_CHECK_RANGE_INT(scfInputSizeW, SCF_SIZE_MIN, IMAGE_SIZE_MAX);
    ACL_CHECK_RANGE_INT(scfInputSizeH, SCF_SIZE_MIN, IMAGE_SIZE_MAX);
    ACL_CHECK_RANGE_INT(scfOutputSizeW, SCF_SIZE_MIN, SCF_SIZEW_MAX);
    ACL_CHECK_RANGE_INT(scfOutputSizeH, SCF_SIZE_MIN, IMAGE_SIZE_MAX);
    aippParmsSet->aippBatchPara[batchIndex].scfInputSizeW = scfInputSizeW;
    aippParmsSet->aippBatchPara[batchIndex].scfInputSizeH = scfInputSizeH;
    aippParmsSet->aippBatchPara[batchIndex].scfOutputSizeW = scfOutputSizeW;
    aippParmsSet->aippBatchPara[batchIndex].scfOutputSizeH = scfOutputSizeH;

    ACL_LOG_INFO("successfully execute aclmdlSetAIPPScfParams");
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPCropParams(aclmdlAIPP *aippParmsSet, int8_t cropSwitch,
                                 int32_t cropStartPosW, int32_t cropStartPosH,
                                 int32_t cropSizeW, int32_t cropSizeH,
                                 uint64_t batchIndex)
{
    ACL_LOG_INFO("start to execute aclmdlSetAIPPCropParams");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(aippParmsSet);
    uint64_t aippBatchParaSize = static_cast<uint64_t>(aippParmsSet->aippBatchPara.size());
    if (batchIndex >= aippBatchParaSize) {
        ACL_LOG_ERROR("[Check][Param]Set batch parameter Failed, batch_index (%lu) is greater than or "
            "equal to batch_number (%lu)", batchIndex, aippBatchParaSize);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("batch_index (%lu) is greater than or "
            "equal to batch_number (%lu)", batchIndex, aippBatchParaSize);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_AIPP_MSG, std::vector<std::string>({"param", "reason"}),
            std::vector<std::string>({"batch_index", errMsg}));
        return ACL_ERROR_INVALID_PARAM;
    }
    ACL_CHECK_RANGE_INT(cropSwitch, AIPP_SWITCH_OFF, AIPP_SWITCH_ON);
    aippParmsSet->aippBatchPara[batchIndex].cropSwitch = cropSwitch;
    if (cropSwitch == 0) {
        ACL_LOG_INFO("cropSwitch[%d] is off", cropSwitch);
        return ACL_SUCCESS;
    }
    ACL_CHECK_RANGE_INT(cropStartPosW, IMAGE_SIZE_MIN - 1, IMAGE_SIZE_MAX - 1);
    ACL_CHECK_RANGE_INT(cropStartPosH, IMAGE_SIZE_MIN - 1, IMAGE_SIZE_MAX - 1);
    ACL_CHECK_RANGE_INT(cropSizeW, IMAGE_SIZE_MIN, IMAGE_SIZE_MAX);
    ACL_CHECK_RANGE_INT(cropSizeH, IMAGE_SIZE_MIN, IMAGE_SIZE_MAX);
    aippParmsSet->aippBatchPara[batchIndex].cropStartPosW = cropStartPosW;
    aippParmsSet->aippBatchPara[batchIndex].cropStartPosH = cropStartPosH;
    aippParmsSet->aippBatchPara[batchIndex].cropSizeW = cropSizeW;
    aippParmsSet->aippBatchPara[batchIndex].cropSizeH = cropSizeH;

    ACL_LOG_INFO("successfully execute aclmdlSetAIPPCropParams");
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPPaddingParams(aclmdlAIPP *aippParmsSet, int8_t paddingSwitch,
                                    int32_t paddingSizeTop, int32_t paddingSizeBottom,
                                    int32_t paddingSizeLeft, int32_t paddingSizeRight,
                                    uint64_t batchIndex)
{
    ACL_LOG_INFO("start to execute aclmdlSetAIPPPaddingParams");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(aippParmsSet);
    uint64_t aippBatchParaSize = static_cast<uint64_t>(aippParmsSet->aippBatchPara.size());
    if (batchIndex >= aippBatchParaSize) {
        ACL_LOG_ERROR("[Check][Param]Set batch parameter Failed, batch_index (%lu) is greater "
            "than or equal to batch_number (%lu)", batchIndex, aippBatchParaSize);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("batch_index (%lu) is greater "
            "than or equal to batch_number (%lu)", batchIndex, aippBatchParaSize);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_AIPP_MSG, std::vector<std::string>({"param",
            "reason"}), std::vector<std::string>({"batch_index", errMsg}));
        return ACL_ERROR_INVALID_PARAM;
    }
    ACL_CHECK_RANGE_INT(paddingSwitch, AIPP_SWITCH_OFF, AIPP_SWITCH_ON);
    aippParmsSet->aippBatchPara[batchIndex].paddingSwitch = paddingSwitch;
    if (paddingSwitch == 0) {
        ACL_LOG_INFO("paddingSwitch[%d] is off", paddingSwitch);
        return ACL_SUCCESS;
    }
    ACL_CHECK_RANGE_INT(paddingSizeTop, PADDING_MIN, PADDING_MAX);
    ACL_CHECK_RANGE_INT(paddingSizeBottom, PADDING_MIN, PADDING_MAX);
    ACL_CHECK_RANGE_INT(paddingSizeLeft, PADDING_MIN, PADDING_MAX);
    ACL_CHECK_RANGE_INT(paddingSizeRight, PADDING_MIN, PADDING_MAX);
    aippParmsSet->aippBatchPara[batchIndex].paddingSizeTop = paddingSizeTop;
    aippParmsSet->aippBatchPara[batchIndex].paddingSizeBottom = paddingSizeBottom;
    aippParmsSet->aippBatchPara[batchIndex].paddingSizeLeft = paddingSizeLeft;
    aippParmsSet->aippBatchPara[batchIndex].paddingSizeRight = paddingSizeRight;

    ACL_LOG_INFO("successfully execute aclmdlSetAIPPPaddingParams");
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPDtcPixelMean(aclmdlAIPP *aippParmsSet,
                                   int16_t dtcPixelMeanChn0,
                                   int16_t dtcPixelMeanChn1,
                                   int16_t dtcPixelMeanChn2,
                                   int16_t dtcPixelMeanChn3,
                                   uint64_t batchIndex)
{
    ACL_LOG_INFO("start to execute aclmdlSetAIPPDtcPixelMean");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(aippParmsSet);
    uint64_t aippBatchParaSize = static_cast<uint64_t>(aippParmsSet->aippBatchPara.size());
    if (batchIndex >= aippBatchParaSize) {
        ACL_LOG_ERROR("[Check][Param]Set batch parameter Failed, batch_index (%lu) is greater than or "
            "equal to batch_number (%lu)", batchIndex, aippBatchParaSize);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_AIPP_MSG, std::vector<std::string>({"param", "reason"}),
            std::vector<std::string>({"InputAIPP", "parameters verification failed"}));
        return ACL_ERROR_INVALID_PARAM;
    }
    ACL_CHECK_RANGE_INT(dtcPixelMeanChn0, MEAN_CHN_MIN, MEAN_CHN_MAX);
    ACL_CHECK_RANGE_INT(dtcPixelMeanChn1, MEAN_CHN_MIN, MEAN_CHN_MAX);
    ACL_CHECK_RANGE_INT(dtcPixelMeanChn2, MEAN_CHN_MIN, MEAN_CHN_MAX);
    ACL_CHECK_RANGE_INT(dtcPixelMeanChn3, MEAN_CHN_MIN, MEAN_CHN_MAX);
    aippParmsSet->aippBatchPara[batchIndex].dtcPixelMeanChn0 = dtcPixelMeanChn0;
    aippParmsSet->aippBatchPara[batchIndex].dtcPixelMeanChn1 = dtcPixelMeanChn1;
    aippParmsSet->aippBatchPara[batchIndex].dtcPixelMeanChn2 = dtcPixelMeanChn2;
    aippParmsSet->aippBatchPara[batchIndex].dtcPixelMeanChn3 = dtcPixelMeanChn3;

    ACL_LOG_INFO("successfully execute aclmdlSetAIPPDtcPixelMean");
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPDtcPixelMin(aclmdlAIPP *aippParmsSet,
                                  float dtcPixelMinChn0,
                                  float dtcPixelMinChn1,
                                  float dtcPixelMinChn2,
                                  float dtcPixelMinChn3,
                                  uint64_t batchIndex)
{
    ACL_LOG_INFO("start to execute aclmdlSetAIPPDtcPixelMin");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(aippParmsSet);
    uint64_t aippBatchParaSize = static_cast<uint64_t>(aippParmsSet->aippBatchPara.size());
    if (batchIndex >= aippBatchParaSize) {
        ACL_LOG_ERROR("[Check][Param]Set batch parameter Failed, batch_index (%lu) is greater than or "
            "equal to batch_number (%lu)", batchIndex, aippBatchParaSize);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("batch_index (%lu) is greater than or "
            "equal to batch_number (%lu)", batchIndex, aippBatchParaSize);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_AIPP_MSG, std::vector<std::string>({"param", "reason"}),
            std::vector<std::string>({"batch_index", errMsg}));
        return ACL_ERROR_INVALID_PARAM;
    }
    ACL_CHECK_RANGE_FLOAT(dtcPixelMinChn0, MIN_CHN_MIN, MIN_CHN_MAX);
    ACL_CHECK_RANGE_FLOAT(dtcPixelMinChn1, MIN_CHN_MIN, MIN_CHN_MAX);
    ACL_CHECK_RANGE_FLOAT(dtcPixelMinChn2, MIN_CHN_MIN, MIN_CHN_MAX);
    ACL_CHECK_RANGE_FLOAT(dtcPixelMinChn3, MIN_CHN_MIN, MIN_CHN_MAX);
    aippParmsSet->aippBatchPara[batchIndex].dtcPixelMinChn0 = Fp16Type(dtcPixelMinChn0).val;
    aippParmsSet->aippBatchPara[batchIndex].dtcPixelMinChn1 = Fp16Type(dtcPixelMinChn1).val;
    aippParmsSet->aippBatchPara[batchIndex].dtcPixelMinChn2 = Fp16Type(dtcPixelMinChn2).val;
    aippParmsSet->aippBatchPara[batchIndex].dtcPixelMinChn3 = Fp16Type(dtcPixelMinChn3).val;
    ACL_LOG_INFO("successfully execute aclmdlSetAIPPDtcPixelMin, input dtcPixelMinChn0:%f,"
                 "the dtcPixelMinChn0:%u of batchIndex:%lu", dtcPixelMinChn0,
                 aippParmsSet->aippBatchPara[batchIndex].dtcPixelMinChn0, batchIndex);
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPPixelVarReci(aclmdlAIPP *aippParmsSet,
                                   float dtcPixelVarReciChn0,
                                   float dtcPixelVarReciChn1,
                                   float dtcPixelVarReciChn2,
                                   float dtcPixelVarReciChn3,
                                   uint64_t batchIndex)
{
    ACL_LOG_INFO("start to execute aclmdlSetAIPPPixelVarReci");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(aippParmsSet);
    uint64_t aippBatchParaSize = static_cast<uint64_t>(aippParmsSet->aippBatchPara.size());
    if (batchIndex >= aippBatchParaSize) {
        ACL_LOG_ERROR("[Check][Param]Set batch parameter Failed, batch_index (%lu) is greater than or "
        "equal to batch_number (%lu)", batchIndex, aippBatchParaSize);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("batch_index (%lu) is greater than or "
            "equal to batch_number (%lu)", batchIndex, aippBatchParaSize);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_AIPP_MSG, std::vector<std::string>({"param", "reason"}),
            std::vector<std::string>({"batch_index", errMsg}));
        return ACL_ERROR_INVALID_PARAM;
    }
    ACL_CHECK_RANGE_FLOAT(dtcPixelVarReciChn0, VR_CHN_MIN, VR_CHN_MAX);
    ACL_CHECK_RANGE_FLOAT(dtcPixelVarReciChn1, VR_CHN_MIN, VR_CHN_MAX);
    ACL_CHECK_RANGE_FLOAT(dtcPixelVarReciChn2, VR_CHN_MIN, VR_CHN_MAX);
    ACL_CHECK_RANGE_FLOAT(dtcPixelVarReciChn3, VR_CHN_MIN, VR_CHN_MAX);
    aippParmsSet->aippBatchPara[batchIndex].dtcPixelVarReciChn0 = Fp16Type(dtcPixelVarReciChn0).val;
    aippParmsSet->aippBatchPara[batchIndex].dtcPixelVarReciChn1 = Fp16Type(dtcPixelVarReciChn1).val;
    aippParmsSet->aippBatchPara[batchIndex].dtcPixelVarReciChn2 = Fp16Type(dtcPixelVarReciChn2).val;
    aippParmsSet->aippBatchPara[batchIndex].dtcPixelVarReciChn3 = Fp16Type(dtcPixelVarReciChn3).val;
    ACL_LOG_INFO("input dtcPixelVarReciChn0:%f, the dtcPixelVarReciChn0:%u of batchIndex:%lu",
                 dtcPixelVarReciChn0, aippParmsSet->aippBatchPara[batchIndex].dtcPixelVarReciChn0, batchIndex);
    return ACL_SUCCESS;
}

static std::string AippBatchParaDebugString(const kAippDynamicBatchPara &aippBatchPara)
{
    std::stringstream ss;
    ss << "kAippDynamicBatchPara[";
    ss << " cropSwitch:" << static_cast<int32_t>(aippBatchPara.cropSwitch);
    ss << " cropStartPosW:" << aippBatchPara.cropStartPosW;
    ss << " cropStartPosH:" << aippBatchPara.cropStartPosH;
    ss << " cropSizeW:" << aippBatchPara.cropSizeW;
    ss << " cropSizeH:" << aippBatchPara.cropSizeH;
    ss << " scfSwitch:" << static_cast<int32_t>(aippBatchPara.scfSwitch);
    ss << " scfInputSizeW:" << aippBatchPara.scfInputSizeW;
    ss << " scfInputSizeH:" << aippBatchPara.scfInputSizeH;
    ss << " scfOutputSizeW:" << aippBatchPara.scfOutputSizeW;
    ss << " scfOutputSizeH:" << aippBatchPara.scfOutputSizeH;
    ss << " paddingSwitch:" << static_cast<int32_t>(aippBatchPara.paddingSwitch);
    ss << " paddingSizeTop:" << aippBatchPara.paddingSizeTop;
    ss << " paddingSizeBottom:" << aippBatchPara.paddingSizeBottom;
    ss << " paddingSizeLeft:" << aippBatchPara.paddingSizeLeft;
    ss << " paddingSizeRight:" << aippBatchPara.paddingSizeRight;
    ss << " rotateSwitch:" << static_cast<int32_t>(aippBatchPara.rotateSwitch);
    ss << " dtcPixelMeanChn0:" << static_cast<int32_t>(aippBatchPara.dtcPixelMeanChn0);
    ss << " dtcPixelMeanChn1:" << static_cast<int32_t>(aippBatchPara.dtcPixelMeanChn1);
    ss << " dtcPixelMeanChn2:" << static_cast<int32_t>(aippBatchPara.dtcPixelMeanChn2);
    ss << " dtcPixelMeanChn3:" << static_cast<int32_t>(aippBatchPara.dtcPixelMeanChn3);
    ss << " dtcPixelMinChn0:" << static_cast<uint32_t>(aippBatchPara.dtcPixelMinChn0);
    ss << " dtcPixelMinChn1:" << static_cast<uint32_t>(aippBatchPara.dtcPixelMinChn1);
    ss << " dtcPixelMinChn2:" << static_cast<uint32_t>(aippBatchPara.dtcPixelMinChn2);
    ss << " dtcPixelMinChn3:" << static_cast<uint32_t>(aippBatchPara.dtcPixelMinChn3);
    ss << " dtcPixelVarReciChn0:" << aclFloat16ToFloat(aippBatchPara.dtcPixelVarReciChn0);
    ss << " dtcPixelVarReciChn1:" << aclFloat16ToFloat(aippBatchPara.dtcPixelVarReciChn1);
    ss << " dtcPixelVarReciChn2:" << aclFloat16ToFloat(aippBatchPara.dtcPixelVarReciChn2);
    ss << " dtcPixelVarReciChn3:" << aclFloat16ToFloat(aippBatchPara.dtcPixelVarReciChn3);
    ss << " ]";

    return ss.str();
}

static std::string AippParmsDebugString(const kAippDynamicPara &aippParms)
{
    std::stringstream ss;
    ss << "kAippDynamicPara[";
    ss << " inputFormat:" << static_cast<uint32_t>(aippParms.inputFormat);
    ss << " cscSwitch:" << static_cast<int32_t>(aippParms.cscSwitch);
    ss << " rbuvSwapSwitch:" << static_cast<int32_t>(aippParms.rbuvSwapSwitch);
    ss << " axSwapSwitch:" << static_cast<int32_t>(aippParms.axSwapSwitch);
    ss << " batchNum:" << static_cast<int32_t>(aippParms.batchNum);
    ss << " srcImageSizeW:" << aippParms.srcImageSizeW;
    ss << " srcImageSizeH:" << aippParms.srcImageSizeH;
    ss << " cscMatrixR0C0:" << static_cast<int32_t>(aippParms.cscMatrixR0C0);
    ss << " cscMatrixR0C1:" << static_cast<int32_t>(aippParms.cscMatrixR0C1);
    ss << " cscMatrixR0C2:" << static_cast<int32_t>(aippParms.cscMatrixR0C2);
    ss << " cscMatrixR1C0:" << static_cast<int32_t>(aippParms.cscMatrixR1C0);
    ss << " cscMatrixR1C1:" << static_cast<int32_t>(aippParms.cscMatrixR1C1);
    ss << " cscMatrixR1C2:" << static_cast<int32_t>(aippParms.cscMatrixR1C2);
    ss << " cscMatrixR2C0:" << static_cast<int32_t>(aippParms.cscMatrixR2C0);
    ss << " cscMatrixR2C1:" << static_cast<int32_t>(aippParms.cscMatrixR2C1);
    ss << " cscMatrixR2C2:" << static_cast<int32_t>(aippParms.cscMatrixR2C2);
    ss << " cscOutputBiasR0:" << static_cast<uint32_t>(aippParms.cscOutputBiasR0);
    ss << " cscOutputBiasR1:" << static_cast<uint32_t>(aippParms.cscOutputBiasR1);
    ss << " cscOutputBiasR2:" << static_cast<uint32_t>(aippParms.cscOutputBiasR2);
    ss << " cscInputBiasR0:" << static_cast<uint32_t>(aippParms.cscInputBiasR0);
    ss << " cscInputBiasR1:" << static_cast<uint32_t>(aippParms.cscInputBiasR1);
    ss << " cscInputBiasR2:" << static_cast<uint32_t>(aippParms.cscInputBiasR2);
    ss << " ]";

    return ss.str();
}

static bool GetDynamicAippInfo(uint32_t modelId, size_t index, ge::AippConfigInfo &aippParams)
{
    // get dynamic aipp config
    ge::GeExecutor executor;
    ACL_LOG_DEBUG("call ge interface executor.GetAIPPInfo");
    auto ret = executor.GetAIPPInfo(modelId, index, aippParams);
    if (ret != ge::SUCCESS) {
        ACL_LOG_WARN("Get dynamic aippInfo fail, the model may be old model, ge result[%u]", ret);
        return false;
    }

    ACL_LOG_INFO("GetAIPPInfo success");
    return true;
}

static size_t GetMaxShapeIndex(const std::vector<ge::InputOutputDims> &inputDims)
{
    size_t maxShapeIndex = 0;
    uint32_t shapeSize = 0;
    for (size_t i = 0; i < inputDims.size(); ++i) {
        if (inputDims[i].size > shapeSize) {
            shapeSize = inputDims[i].size;
            maxShapeIndex = i;
        }
    }
    ACL_LOG_INFO("GetMaxShapeIndex success, maxShapeIndex[%zu]", maxShapeIndex);
    return maxShapeIndex;
}

static aclError GetModelOriDims(uint32_t modelId, uint32_t relatedInputRank, bool &isGetDim,
                                int64_t &mdlOriH, int64_t &mdlOriW, int64_t &mdlOriN)
{
    // get model origin input info
    ge::GeExecutor executor;
    ge::OriginInputInfo inputInfo;
    ACL_LOG_DEBUG("call ge interface executor.GetOrigInputInfo");
    auto ret = executor.GetOrigInputInfo(modelId, relatedInputRank, inputInfo);
    if (ret != ge::SUCCESS) {
        ACL_LOG_ERROR("[Get][OrigInputInfo]GetOrigInputInfo failed, modelId[%u], index[%zu], "
                                       "ge result[%u]", modelId, relatedInputRank, ret);
        return ACL_GET_ERRCODE_GE(ret);
    }
    aclFormat srcFormat = static_cast<aclFormat>(inputInfo.format);

    // get model origin input dims
    std::vector<ge::InputOutputDims> inputDims;
    std::vector<ge::InputOutputDims> outputDims;
    ACL_LOG_DEBUG("call ge interface executor.GetAllAippInputOutputDims");
    ret = executor.GetAllAippInputOutputDims(modelId, relatedInputRank, inputDims, outputDims);
    if (ret != ge::SUCCESS) {
        ACL_LOG_ERROR("[Get][AllAippInputOutputDims]GetAllAippInputOutputDims failed, modelId[%u], "
            "index[%zu], ge result[%u]", modelId, relatedInputRank, ret);
        return ACL_GET_ERRCODE_GE(ret);
    }

    // Parse NHW from input Dims when inputDims is not empty
    if (inputDims.empty()) {
        ACL_LOG_ERROR("[Check][InputDims]get model origin input dims fail, origin input dims is empty");
        return ACL_ERROR_GE_FAILURE;
    }
    // Get the index of the maximum gear
    size_t maxShapeIndex = GetMaxShapeIndex(inputDims);
    aclmdlIODims srcDims;
    aclError ioRet = SetIODims(inputDims[maxShapeIndex], srcDims);
    if (ioRet != ACL_SUCCESS) {
        ACL_LOG_ERROR("[Set][IODims]srcDims SetIODims failed, modelId[%u], result[%d]", modelId, ioRet);
        return ioRet;
    }
    switch (srcFormat) {
        case ACL_FORMAT_NCHW:
            if (srcDims.dimCount == 4) {
                mdlOriH = srcDims.dims[2];
                mdlOriW = srcDims.dims[3];
                mdlOriN = srcDims.dims[0];
            }
            isGetDim = true;
            break;
        case ACL_FORMAT_NHWC:
            if (srcDims.dimCount == 4) {
                mdlOriH = srcDims.dims[1];
                mdlOriW = srcDims.dims[2];
                mdlOriN = srcDims.dims[0];
            }
            isGetDim = true;
            break;
        default :
            ACL_LOG_WARN("the model origin format[%d] is invalid, only support ACL_FORMAT_NHWC or ACL_FORMAT_NCHW",
                static_cast<int32_t>(srcFormat));
            isGetDim = false;
            break;
    }
    return ACL_SUCCESS;
}

static aclError GetAndCheckAippParams(uint32_t modelId, size_t index, const aclmdlAIPP *aippParmsSet)
{
    // check dynamic aipp parameters
    ge::AippConfigInfo aippParams;
    bool isNewAippModel = GetDynamicAippInfo(modelId, index, aippParams);
    int64_t mdlOriH = 0;
    int64_t mdlOriW = 0;
    int64_t mdlOriN = 0;
    if (isNewAippModel) {
        uint32_t relatedInputRank = aippParams.related_input_rank;
        uint64_t maxSrcImageSize = static_cast<uint64_t>(aippParams.max_src_image_size);
        // check max_src_image_size
        uint64_t size = GetSrcImageSize(aippParmsSet);
        ACL_LOG_INFO("Input SrcImageSize = %lu", size);
        if (size > maxSrcImageSize) {
            ACL_LOG_ERROR("[Check][Size]the dynamic aipp size[%lu] is bigger than max_src_image_size[%lu]",
                size, maxSrcImageSize);
            std::string errMsg = acl::AclErrorLogManager::FormatStr("bigger than max_src_image_size[%lu]",
                maxSrcImageSize);
            acl::AclErrorLogManager::ReportInputError(acl::INVALID_AIPP_MSG,
                std::vector<std::string>({"param", "reason"}),
                std::vector<std::string>({"dynamic aipp size", errMsg}));
            return ACL_ERROR_INVALID_PARAM;
        }

        bool isGetDim = false;
        aclError mdlRet = GetModelOriDims(modelId, relatedInputRank, isGetDim, mdlOriH, mdlOriW, mdlOriN);
        if (mdlRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("[Get][ModelOriDims]get model original dims fail");
            return mdlRet;
        }

        if (isGetDim) {
            ACL_LOG_INFO("relatedInputRank[%u], maxSrcImageSize[%u], mdlOriH[%ld], mdlOriW[%ld], mdlOriN[%ld]",
                relatedInputRank, maxSrcImageSize, mdlOriH, mdlOriW, mdlOriN);
            // check batchSize
            if (mdlOriN != static_cast<int64_t>(aippParmsSet->batchSize)) {
                ACL_LOG_ERROR("[Check][mdlOriN]the dynamic aipp batchSize[%lu] is not equal to model origin batch[%ld]",
                    aippParmsSet->batchSize, mdlOriN);
                std::string errMsg = acl::AclErrorLogManager::FormatStr("batchSize[%lu] is not equal to "
                    "model origin batch[%ld]", aippParmsSet->batchSize, mdlOriN);
                acl::AclErrorLogManager::ReportInputError(acl::INVALID_AIPP_MSG,
                    std::vector<std::string>({"param", "reason"}),
                    std::vector<std::string>({"dynamic aipp batchSize", errMsg}));
                return ACL_ERROR_INVALID_PARAM;
            }
        } else {
            ACL_LOG_INFO("can not get model H W N");
            isNewAippModel = false;
        }
    }

    return AippParamsCheck(aippParmsSet, GetSocVersion(), mdlOriW, mdlOriH, isNewAippModel);
}

static aclError CheckAippDataIndex(uint32_t modelId, size_t index, aclmdlDesc* modelDesc)
{
    ACL_LOG_INFO("call ge interface executor.GetAippType, modelId[%u]", modelId);
    ge::GeExecutor executor;
    ge::InputAippType type;
    size_t aippIndex = 0;
    auto ret = executor.GetAippType(modelId, index, type, aippIndex);
    if (ret != ge::SUCCESS) {
        ACL_LOG_ERROR("[Get][AippType]Get aipp type failed, ge result[%u]", ret);
        return ACL_GET_ERRCODE_GE(ret);
    }
    if (type == ge::DYNAMIC_AIPP_NODE) {
        ACL_LOG_INFO("Index [%zu] entered by the user is dynamic aipp data", index);
        return ACL_SUCCESS;
    } else if (type == ge::DATA_WITHOUT_AIPP) {
        // maybe this is old om when getaipptype interface is unsupported, ensure compatibility
        size_t indexInModel;
        auto mdlRet = aclmdlGetInputIndexByName(modelDesc, ACL_DYNAMIC_AIPP_NAME, &indexInModel);
        if (mdlRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("[Get][InputIndex]the model is not a dynamic aipp model, there is no dynamic aipp node");
            return mdlRet;
        }
        if (indexInModel != index) {
            ACL_LOG_ERROR("[Check][indexInModel]index[%zu] entered by the user is not dynamic aipp index[%zu]",
                index, indexInModel);
            return ACL_ERROR_INVALID_PARAM;
        }
        return ACL_SUCCESS;
    } else {
        ACL_LOG_ERROR("[Check][Index]index[%zu] entered by the user is not dynamic aipp data index.", index);
        return ACL_ERROR_INVALID_PARAM;
    }
}

aclError aclmdlSetInputAIPP(uint32_t modelId,
                            aclmdlDataset *dataset,
                            size_t index,
                            const aclmdlAIPP *aippParmsSet)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_LOG_DEBUG("start to execute aclmdlSetInputAIPP, modelId[%u], index[%zu]", modelId, index);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dataset);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(aippParmsSet);
    // check dynamic aipp index
    aclmdlDesc modelDesc;
    aclError mdlRet = aclmdlGetDesc(&modelDesc, modelId);
    if (mdlRet != ACL_SUCCESS) {
        ACL_LOG_ERROR("[Get][ModelDesc]get modelDesc fail, modelId[%u]", modelId);
        return mdlRet;
    }
    mdlRet = CheckAippDataIndex(modelId, index, &modelDesc);
    if (mdlRet != ACL_SUCCESS) {
        ACL_LOG_ERROR("[Check][AippData]Dynamic AIPP data index %zu is invalid, parameters verification failed", index);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("index %zu is invalid", index);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_AIPP_MSG, std::vector<std::string>({"param", "reason"}),
            std::vector<std::string>({"Dynamic AIPP data index", errMsg}));
        return mdlRet;
    }

    mdlRet = GetAndCheckAippParams(modelId, index, aippParmsSet);
    if (mdlRet != ACL_SUCCESS) {
        ACL_LOG_ERROR("[Check][AippParams]Dynamic AIPP parameters is invalid, parameters verification failed");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_AIPP_MSG, std::vector<std::string>({"param", "reason"}),
            std::vector<std::string>({"parameters", "parameters verification failed"}));
        return mdlRet;
    }

    aclDataBuffer *buff = aclmdlGetDatasetBuffer(dataset, index);
    if (buff == nullptr) {
        ACL_LOG_ERROR("[Check][Buff]failed to get data buffer by index[%zu]", index);
        return ACL_ERROR_INVALID_PARAM;
    }

    void *devPtr = aclGetDataBufferAddr(buff);
    if (devPtr == nullptr) {
        ACL_LOG_ERROR("[Check][DevPtr]failed to get addr by index[%zu]", index);
        return ACL_ERROR_INVALID_PARAM;
    }
    uint64_t memSize = aclGetDataBufferSizeV2(buff);
    ACL_LOG_DEBUG("aippParmsSet->aippParms: %s .", AippParmsDebugString(aippParmsSet->aippParms).c_str());
    for (size_t i = 0; i < aippParmsSet->aippBatchPara.size(); ++i) {
        ACL_LOG_DEBUG("batchIndex[%lu] aippParmsSet->aippBatchPara: %s .",
            i, AippBatchParaDebugString(aippParmsSet->aippBatchPara[i]).c_str());
    }
    // send dynamic aipp to GE
    ACL_LOG_INFO("call ge interface executor.SetDynamicAippData, modelId[%u]", modelId);
    ge::GeExecutor executor;
    auto ret = executor.SetDynamicAippData(modelId, devPtr, memSize,
        aippParmsSet->aippBatchPara, aippParmsSet->aippParms);
    if (ret != ge::SUCCESS) {
        ACL_LOG_ERROR("[Set][DynamicAippData]SetDynamicImageSize failed, ge result[%u]", ret);
        return ACL_GET_ERRCODE_GE(ret);
    }
    return ACL_SUCCESS;
}

aclError aclmdlGetAippType(uint32_t modelId, size_t index, aclmdlInputAippType *type, size_t *dynamicAttachedDataIndex)
{
    ACL_LOG_INFO("start to execute aclmdlGetAippType, modelId[%u], index[%zu]", modelId, index);
    *dynamicAttachedDataIndex = ACL_INVALID_NODE_INDEX;
    ge::GeExecutor executor;
    ge::InputAippType typeTmp;
    auto ret = executor.GetAippType(modelId, index, typeTmp, *dynamicAttachedDataIndex);
    if (ret != ge::SUCCESS) {
        ACL_LOG_ERROR("[Get][AippType]Get aipp type failed, ge result[%u]", ret);
        return ACL_GET_ERRCODE_GE(ret);
    }
    *type = (aclmdlInputAippType)typeTmp;
    ACL_LOG_INFO("successfully execute aclmdlGetAippType, modelId[%u], index[%zu]", modelId, index);
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPByInputIndex(uint32_t modelId,
                                   aclmdlDataset *dataset,
                                   size_t index,
                                   const aclmdlAIPP *aippParmsSet)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_LOG_INFO("start to execute aclmdlSetAIPPByInputIndex, modelId[%u], index[%zu]", modelId, index);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(aippParmsSet);
    if ((dataset == nullptr) || (index >= dataset->blobs.size())) {
        ACL_LOG_ERROR("[Check][Dataset]input param is invalid, dataset[%p], index[%zu]", dataset, index);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("dataset[%p], index[%zu]", dataset, index);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_AIPP_MSG, std::vector<std::string>({"param", "reason"}),
            std::vector<std::string>({"params", errMsg}));
        return ACL_ERROR_INVALID_PARAM;
    }
    aclmdlInputAippType type;
    size_t dynamicAttachedDataIndex = 0;
    auto ret = aclmdlGetAippType(modelId, index, &type, &dynamicAttachedDataIndex);
    if (ret != ACL_SUCCESS) {
        return ret;
    }
    if (type != ACL_DATA_WITH_DYNAMIC_AIPP) {
        ACL_LOG_ERROR("[Check][Type]This %zu input has no dynamic aipp linked, modelId[%u]", index, modelId);
        return ACL_ERROR_FAILURE;
    }
    ACL_LOG_INFO("successfully execute aclmdlSetAIPPByInputIndex, modelId[%u], index[%zu]", modelId, index);
    return aclmdlSetInputAIPP(modelId, dataset, dynamicAttachedDataIndex, aippParmsSet);
}

static std::string AippInfoDebugString(aclAippInfo *aippInfo)
{
    if (aippInfo == nullptr) {
        ACL_LOG_ERROR("[Check][aippInfo]param aippInfo must not be null");
        return "";
    }
    std::stringstream ss;
    ss << "aclAippInfo[";
    ss << " inputFormat:" << static_cast<int32_t>(aippInfo->inputFormat);
    ss << " srcImageSizeW:" << aippInfo->srcImageSizeW;
    ss << " srcImageSizeH:" << aippInfo->srcImageSizeH;

    ss << " cropSwitch:" << static_cast<int32_t>(aippInfo->cropSwitch);
    ss << " loadStartPosW:" << aippInfo->loadStartPosW;
    ss << " loadStartPosH:" << aippInfo->loadStartPosH;
    ss << " cropSizeW:" << aippInfo->cropSizeW;
    ss << " cropSizeH:" << aippInfo->cropSizeH;

    ss << " resizeSwitch:" << static_cast<int32_t>(aippInfo->resizeSwitch);
    ss << " resizeOutputW:" << aippInfo->resizeOutputW;
    ss << " resizeOutputH:" << aippInfo->resizeOutputH;

    ss << " paddingSwitch:" << static_cast<int32_t>(aippInfo->paddingSwitch);
    ss << " leftPaddingSize:" << aippInfo->leftPaddingSize;
    ss << " rightPaddingSize:" << aippInfo->rightPaddingSize;
    ss << " topPaddingSize:" << aippInfo->topPaddingSize;
    ss << " bottomPaddingSize:" << aippInfo->bottomPaddingSize;

    ss << " cscSwitch:" << static_cast<int32_t>(aippInfo->cscSwitch);
    ss << " rbuvSwapSwitch:" << static_cast<int32_t>(aippInfo->rbuvSwapSwitch);
    ss << " axSwapSwitch:" << static_cast<int32_t>(aippInfo->axSwapSwitch);
    ss << " singleLineMode:" << static_cast<int32_t>(aippInfo->singleLineMode);

    ss << " matrixR0C0:" << aippInfo->matrixR0C0;
    ss << " matrixR0C1:" << aippInfo->matrixR0C1;
    ss << " matrixR0C2:" << aippInfo->matrixR0C2;
    ss << " matrixR1C0:" << aippInfo->matrixR1C0;
    ss << " matrixR1C1:" << aippInfo->matrixR1C1;
    ss << " matrixR1C2:" << aippInfo->matrixR1C2;
    ss << " matrixR2C0:" << aippInfo->matrixR2C0;
    ss << " matrixR2C1:" << aippInfo->matrixR2C1;
    ss << " matrixR2C2:" << aippInfo->matrixR2C2;

    ss << " outputBias0:" << aippInfo->outputBias0;
    ss << " outputBias1:" << aippInfo->outputBias1;
    ss << " outputBias2:" << aippInfo->outputBias2;
    ss << " inputBias0:" << aippInfo->inputBias0;
    ss << " inputBias1:" << aippInfo->inputBias1;
    ss << " inputBias2:" << aippInfo->inputBias2;

    ss << " meanChn0:" << aippInfo->meanChn0;
    ss << " meanChn1:" << aippInfo->meanChn1;
    ss << " meanChn2:" << aippInfo->meanChn2;
    ss << " meanChn3:" << aippInfo->meanChn3;
    ss << " minChn0:" << aippInfo->minChn0;
    ss << " minChn1:" << aippInfo->minChn1;
    ss << " minChn2:" << aippInfo->minChn2;
    ss << " minChn3:" << aippInfo->minChn3;
    ss << " varReciChn0:" << aippInfo->varReciChn0;
    ss << " varReciChn1:" << aippInfo->varReciChn1;
    ss << " varReciChn2:" << aippInfo->varReciChn2;
    ss << " varReciChn3:" << aippInfo->varReciChn3;

    ss << " shapeCount:" << aippInfo->shapeCount;
    ss << " srcFormat:" << aippInfo->srcFormat;
    ss << " srcDatatype:" << aippInfo->srcDatatype;
    ss << " srcDimNum:" << aippInfo->srcDimNum;
    ss << " ]";
    return ss.str();
}

static std::string DimsDebugString(aclmdlIODims &ioDims)
{
    std::stringstream ss;
    ss << "[" << " tensorName:" << ioDims.name;
    ss << " dimcount:" << static_cast<int32_t>(ioDims.dimCount);
    ss << " dims:";
    for (size_t i = 0; i < ioDims.dimCount; i++) {
        ss << " " << ioDims.dims[i];
    }
    ss << "]; ";
    return ss.str();
}

static std::string AippDimsDebugString(aclAippDims *aippDims, size_t shapeCount)
{
    std::stringstream ssDims;
    for (size_t i = 0; i < shapeCount; i++) {
        ssDims << " aclAippDims[" << i << "]: ";
        ssDims << DimsDebugString(aippDims[i].srcDims);
        ssDims << " srcSize:"<< aippDims[i].srcSize;
        ssDims << DimsDebugString(aippDims[i].aippOutdims);
        ssDims << " aippOutSize:"<< aippDims[i].aippOutSize;
    }
    return ssDims.str();
}

static void SetAippInfo(aclAippInfo *aippInfo, const ge::AippConfigInfo &aippParams)
{
    ACL_LOG_DEBUG("start to execute SetAippInfo");
    if (aippInfo == nullptr) {
        ACL_LOG_ERROR("[Check][AippInfo]param aippInfo must not be null");
        return;
    }
    aippInfo->inputFormat = static_cast<aclAippInputFormat>(aippParams.input_format);
    aippInfo->srcImageSizeW = aippParams.src_image_size_w;
    aippInfo->srcImageSizeH = aippParams.src_image_size_h;

    aippInfo->cropSwitch = aippParams.crop;
    aippInfo->loadStartPosW = aippParams.load_start_pos_w;
    aippInfo->loadStartPosH = aippParams.load_start_pos_h;
    aippInfo->cropSizeW = aippParams.crop_size_w;
    aippInfo->cropSizeH = aippParams.crop_size_h;

    aippInfo->resizeSwitch = aippParams.resize;
    aippInfo->resizeOutputW = aippParams.resize_output_w;
    aippInfo->resizeOutputH = aippParams.resize_output_h;

    aippInfo->paddingSwitch = aippParams.padding;
    aippInfo->leftPaddingSize = aippParams.left_padding_size;
    aippInfo->rightPaddingSize = aippParams.right_padding_size;
    aippInfo->topPaddingSize = aippParams.top_padding_size;
    aippInfo->bottomPaddingSize = aippParams.bottom_padding_size;

    aippInfo->cscSwitch = aippParams.csc_switch;
    aippInfo->rbuvSwapSwitch = aippParams.rbuv_swap_switch;
    aippInfo->axSwapSwitch = aippParams.ax_swap_switch;
    aippInfo->singleLineMode = aippParams.single_line_mode;

    aippInfo->matrixR0C0 = aippParams.matrix_r0c0;
    aippInfo->matrixR0C1 = aippParams.matrix_r0c1;
    aippInfo->matrixR0C2 = aippParams.matrix_r0c2;
    aippInfo->matrixR1C0 = aippParams.matrix_r1c0;
    aippInfo->matrixR1C1 = aippParams.matrix_r1c1;
    aippInfo->matrixR1C2 = aippParams.matrix_r1c2;
    aippInfo->matrixR2C0 = aippParams.matrix_r2c0;
    aippInfo->matrixR2C1 = aippParams.matrix_r2c1;
    aippInfo->matrixR2C2 = aippParams.matrix_r2c2;

    aippInfo->outputBias0 = aippParams.output_bias_0;
    aippInfo->outputBias1 = aippParams.output_bias_1;
    aippInfo->outputBias2 = aippParams.output_bias_2;
    aippInfo->inputBias0 = aippParams.input_bias_0;
    aippInfo->inputBias1 = aippParams.input_bias_1;
    aippInfo->inputBias2 = aippParams.input_bias_2;

    aippInfo->meanChn0 = aippParams.mean_chn_0;
    aippInfo->meanChn1 = aippParams.mean_chn_1;
    aippInfo->meanChn2 = aippParams.mean_chn_2;
    aippInfo->meanChn3 = aippParams.mean_chn_3;
    aippInfo->minChn0 = aippParams.min_chn_0;
    aippInfo->minChn1 = aippParams.min_chn_1;
    aippInfo->minChn2 = aippParams.min_chn_2;
    aippInfo->minChn3 = aippParams.min_chn_3;

    aippInfo->varReciChn0 = aippParams.var_reci_chn_0;
    aippInfo->varReciChn1 = aippParams.var_reci_chn_1;
    aippInfo->varReciChn2 = aippParams.var_reci_chn_2;
    aippInfo->varReciChn3 = aippParams.var_reci_chn_3;
    ACL_LOG_DEBUG("end to execute SetAippInfo");
}

aclError aclmdlGetFirstAippInfo(uint32_t modelId, size_t index, aclAippInfo *aippInfo)
{
    ACL_LOG_DEBUG("start to execute aclmdlGetFirstAippInfo, modelId[%u], index[%zu]", modelId, index);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(aippInfo);
    ge::AippConfigInfo aippParams;
    ge::GeExecutor executor;
    ACL_LOG_DEBUG("call ge interface executor.GetAIPPInfo");
    auto ret = executor.GetAIPPInfo(modelId, index, aippParams);
    if (ret == ACL_ERROR_GE_AIPP_NOT_EXIST) {
        ACL_LOG_WARN("the tensor index[%lu] is not configured with aipp, modelId[%u], index[%zu], ge result[%u]",
            index, modelId, index, ret);
        return ACL_GET_ERRCODE_GE(ret);
    } else if (ret != ge::SUCCESS) {
        ACL_LOG_ERROR("[Get][AIPPInfo]GetAIPPInfo failed, modelId[%u], index[%zu], ge result[%u]",
            modelId, index, ret);
        return ACL_GET_ERRCODE_GE(ret);
    }
    SetAippInfo(aippInfo, aippParams);

    size_t shapeCount;
    ACL_LOG_DEBUG("call ge interface executor.GetBatchInfoSize");
    ret = executor.GetBatchInfoSize(modelId, shapeCount);
    if (ret != ge::SUCCESS) {
        ACL_LOG_ERROR("[Get][BatchInfo]GetBatchInfoSize failed, modelId[%u], index[%zu], ge result[%u]",
            modelId, index, ret);
        return ACL_GET_ERRCODE_GE(ret);
    }
    ACL_LOG_DEBUG("get shapeCount[%zu]", shapeCount);
    aippInfo->shapeCount = shapeCount;

    ge::OriginInputInfo inputInfo;
    ACL_LOG_DEBUG("call ge interface executor.GetOrigInputInfo");
    ret = executor.GetOrigInputInfo(modelId, index, inputInfo);
    if (ret != ge::SUCCESS) {
        ACL_LOG_ERROR("[Get][OrigInputInfo]GetOrigInputInfo failed, modelId[%u], index[%zu], ge result[%u]",
            modelId, index, ret);
        return ACL_GET_ERRCODE_GE(ret);
    }
    aippInfo->srcFormat = static_cast<aclFormat>(inputInfo.format);
    aippInfo->srcDatatype = static_cast<aclDataType>(inputInfo.data_type);
    aippInfo->srcDimNum = inputInfo.dim_num;
    ACL_LOG_DEBUG("aclAippInfo: %s .", AippInfoDebugString(aippInfo).c_str());

    std::vector<ge::InputOutputDims> inputDims;
    std::vector<ge::InputOutputDims> outputDims;
    ACL_LOG_DEBUG("call ge interface executor.GetAllAippInputOutputDims");
    ret = executor.GetAllAippInputOutputDims(modelId, index, inputDims, outputDims);
    if (ret != ge::SUCCESS) {
        ACL_LOG_ERROR("[Get][AllAippInputOutputDims]GetAllAippInputOutputDims failed, modelId[%u], index[%zu], "
            "ge result[%u]", modelId, index, ret);
        return ACL_GET_ERRCODE_GE(ret);
    }

    ACL_LOG_DEBUG("GetAllAippInputOutputDims success");
    if ((shapeCount > ACL_MAX_SHAPE_COUNT) || (shapeCount != inputDims.size()) || (shapeCount != outputDims.size())) {
        ACL_LOG_ERROR("[Check][Params]shapeCount[%zu] should be smaller than ACL_MAX_SHAPE_COUNT(128) and it should "
                      "be equal to size of inputDims[%zu], size of outputDims[%zu]",
                      shapeCount, inputDims.size(), outputDims.size());
        return ACL_ERROR_GE_FAILURE;
    }
    for (size_t i = 0 ; i < shapeCount; i++) {
        aclError ioRet = SetIODims(inputDims[i], aippInfo->outDims[i].srcDims);
        if (ioRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("[Set][IODims]srcDims SetIODims failed, modelId[%u], index[%zu], result[%d]",
                modelId, index, ioRet);
            return ioRet;
        }
        aippInfo->outDims[i].srcSize = inputDims[i].size;
        ioRet = SetIODims(outputDims[i], aippInfo->outDims[i].aippOutdims);
        if (ioRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("[Set][IODims]aippOutdims SetIODims failed, modelId[%u], index[%zu], result[%d]",
                modelId, index, ioRet);
            return ioRet;
        }
        aippInfo->outDims[i].aippOutSize = outputDims[i].size;
    }
    ACL_LOG_DEBUG("successfully execute aclmdlGetFirstAippInfo, aclAippDims: %s",
        AippDimsDebugString(aippInfo->outDims, aippInfo->shapeCount).c_str());
    return ACL_SUCCESS;
}
