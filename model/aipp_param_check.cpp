/**
* @file aipp_param_check.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "aipp_param_check.h"
#include "utils/math_utils.h"

namespace {
    constexpr uint32_t TWO_CHANNEL = 2;
    constexpr uint32_t THREE_CHANNEL = 3;
    constexpr uint32_t FOUR_CHANNEL = 4;
    constexpr uint32_t MULTIPLE = 16;
    std::set<std::string> socVersionForLhisi = {"Hi3796CV300ES", "Hi3796CV300CS", "SD3403"};
}

aclError AippInputFormatCheck(enum CceAippInputFormat inputFormat, std::string socVersion)
{
    bool flag = false;
    if (inputFormat < CCE_YUV420SP_U8) {
        ACL_LOG_ERROR("[Check][InputFormat]inputFormat must be setted, cceInputFormat = %d",
            static_cast<int32_t>(inputFormat));
        return ACL_ERROR_INVALID_PARAM;
    }

    if ((socVersion == "Ascend310") || (strncmp(socVersion.c_str(), "Ascend910", strlen("Ascend910")) == 0)) {
        flag = ((inputFormat != CCE_YUV420SP_U8) && (inputFormat != CCE_XRGB8888_U8) &&
            (inputFormat != CCE_RGB888_U8) && (inputFormat != CCE_YUV400_U8));
        if (flag) {
            ACL_LOG_ERROR("[Check][InputFormat]Ascend310 Ascend910 only support YUV420SP_U8,XRGB8888_U8,"
                          "RGB888_U8,YUV400_U8, cceInputFormat = %d", static_cast<int32_t>(inputFormat));
            return ACL_ERROR_INVALID_PARAM;
        }
    } else if ((socVersion == "Ascend610") || (socVersion == "Ascend710") || (socVersion == "Ascend615")) {
        flag = ((inputFormat != CCE_YUV420SP_U8) && (inputFormat != CCE_XRGB8888_U8) &&
            (inputFormat != CCE_RGB888_U8) && (inputFormat != CCE_YUV400_U8));
        if (flag) {
            ACL_LOG_ERROR("[Check][InputFormat]Ascend610 Ascend710 and Ascend615 only support YUV420SP_U8,XRGB8888_U8,"
                          "RGB888_U8,YUV400_U8, cce_inputFormat = %d", static_cast<int32_t>(inputFormat));
            return ACL_ERROR_INVALID_PARAM;
        }
    } else if (socVersionForLhisi.find(socVersion) != socVersionForLhisi.end()) {
        flag = ((inputFormat != CCE_YUV420SP_U8) && (inputFormat != CCE_YUV422SP_U8) &&
            (inputFormat != CCE_RGB888_U8) && (inputFormat != CCE_ARGB8888_U8));
        if (flag) {
            ACL_LOG_ERROR("[Check][InputFormat]Hi3796CV300ES, Hi3796CV300CS and SD3403 "
                "only support YUV420SP_U8,YUV422SP_U8, RGB888_U8,ARGB8888_U8, cce_inputFormat = %d",
                static_cast<int32_t>(inputFormat));
            return ACL_ERROR_INVALID_PARAM;
        }
    } else {
        ACL_LOG_ERROR("[Check][Aipp]dynamic aipp not support %s", socVersion.c_str());
        return ACL_ERROR_INVALID_PARAM;
    }
    return ACL_SUCCESS;
}

aclError AippSrcImageSizeCheck(enum CceAippInputFormat inputFormat,
                               int32_t srcImageSizeW, int32_t srcImageSizeH, std::string socVersion)
{
    bool flag = false;
    flag = ((srcImageSizeW == 0) || (srcImageSizeH == 0));
    if (flag) {
        ACL_LOG_ERROR("[Check][Params]srcImageSizeW and srcImageSizeH must be setted!");
        return ACL_ERROR_INVALID_PARAM;
    }

    flag = ((inputFormat == CCE_YUV422SP_U8) || (inputFormat == CCE_YUYV_U8));
    if (inputFormat == CCE_YUV420SP_U8) {
        // determine whether it is even
        flag = ((srcImageSizeW % 2 != 0) || (srcImageSizeH % 2 != 0));
        if (flag) {
            ACL_LOG_ERROR("[Check][Params]srcImageSizeH[%d] and srcImageSizeW[%d] must be even for YUV420SP_U8!",
                srcImageSizeH, srcImageSizeW);
            return ACL_ERROR_INVALID_PARAM;
        }
    } else if (flag) {
        // determine whether it is even
        if (srcImageSizeW % 2 != 0) {
            ACL_LOG_ERROR("[Check][Params]srcImageSizeW[%d] must be even for YUV422SP_U8 and YUYV_U8!", srcImageSizeW);
            return ACL_ERROR_INVALID_PARAM;
        }
    }

    flag = (socVersionForLhisi.find(socVersion) != socVersionForLhisi.end());
    if (flag) {
        flag = ((inputFormat == CCE_YUV420SP_U8) || (inputFormat == CCE_YUV422SP_U8) ||
            (inputFormat == CCE_YUV400_U8) || (inputFormat == CCE_RAW10) ||
            (inputFormat == CCE_RAW12) || (inputFormat == CCE_RAW16));
        if (flag) {
            if (srcImageSizeW % MULTIPLE != 0) {
                ACL_LOG_ERROR("[Check][Params]srcImageSizeW[%d] must be multiples of 16!", srcImageSizeW);
                return ACL_ERROR_INVALID_PARAM;
            }
        }

        flag = ((inputFormat == CCE_ARGB8888_U8) || (inputFormat == CCE_XRGB8888_U8) ||
            (inputFormat == CCE_AYUV444_U8));
        if (flag) {
            if ((srcImageSizeW * FOUR_CHANNEL) % MULTIPLE != 0) {
                ACL_LOG_ERROR("[Check][Params]srcImageSizeW*4 must be multiples of 16!");
                return ACL_ERROR_INVALID_PARAM;
            }
        }

        flag = (inputFormat == CCE_RGB888_U8);
        if (flag) {
            if ((srcImageSizeW * THREE_CHANNEL) % MULTIPLE != 0) {
                ACL_LOG_ERROR("[Check][Params]srcImageSizeW*3 must be multiples of 16!");
                return ACL_ERROR_INVALID_PARAM;
            }
        }

        flag = (inputFormat == CCE_YUYV_U8);
        if (flag) {
            if ((srcImageSizeW * TWO_CHANNEL) % MULTIPLE != 0) {
                ACL_LOG_ERROR("[Check][Params]srcImageSizeW*2 must be multiples of 16!");
                return ACL_ERROR_INVALID_PARAM;
            }
        }
    }

    return ACL_SUCCESS;
}

aclError AippScfSizeCheck(const aclmdlAIPP *aippParmsSet, int32_t batchIndex)
{
    if (aippParmsSet->aippBatchPara.size() == 0) {
        ACL_LOG_ERROR("[Check][Params]the size of aippBatchPara can't be zero");
        return ACL_ERROR_INVALID_PARAM;
    }

    int32_t scfInputSizeW = aippParmsSet->aippBatchPara[batchIndex].scfInputSizeW;
    int32_t scfInputSizeH = aippParmsSet->aippBatchPara[batchIndex].scfInputSizeH;
    int32_t scfOutputSizeW = aippParmsSet->aippBatchPara[batchIndex].scfOutputSizeW;
    int32_t scfOutputSizeH = aippParmsSet->aippBatchPara[batchIndex].scfOutputSizeH;

    int32_t srcImageSizeW = aippParmsSet->aippParms.srcImageSizeW;
    int32_t srcImageSizeH = aippParmsSet->aippParms.srcImageSizeH;

    int8_t cropSwitch = aippParmsSet->aippBatchPara[batchIndex].cropSwitch;
    int32_t cropSizeW = aippParmsSet->aippBatchPara[batchIndex].cropSizeW;
    int32_t cropSizeH = aippParmsSet->aippBatchPara[batchIndex].cropSizeH;

    bool flag = false;
    if (cropSwitch == 1) {
        flag = ((scfInputSizeW != cropSizeW) || (scfInputSizeH != cropSizeH));
        if (flag) {
            ACL_LOG_ERROR("[Check][Params]when enable crop and scf, scfInputSizeW[%d] must be "
                          "equal to cropSizeW[%d] and scfInputSizeH[%d] must be equal to cropSizeH[%d]!",
                          scfInputSizeW, cropSizeW, scfInputSizeH, cropSizeH);
            return ACL_ERROR_INVALID_PARAM;
        }
    } else {
        flag = ((scfInputSizeW != srcImageSizeW) || (scfInputSizeH != srcImageSizeH));
        if (flag) {
            ACL_LOG_ERROR("[Check][Params]when disenable crop and enable scf, scfInputSizeW[%d] "
                "must be equal to srcImageSizeW[%d] and scfInputSizeH[%d] must be equal to srcImageSizeH[%d]!",
                scfInputSizeW, srcImageSizeW, scfInputSizeH, srcImageSizeH);
            return ACL_ERROR_INVALID_PARAM;
        }
    }

    // scfInputSizeH mini value is 16, scfInputSizeH max value is 4096
    flag = ((scfInputSizeH < 16) || (scfInputSizeH > 4096));
    if (flag) {
        ACL_LOG_ERROR("[Check][Params]resize_input_h[%d] should be within [16, 4096]!",
            scfInputSizeH);
        return ACL_ERROR_INVALID_PARAM;
    }

    // scfInputSizeW mini value is 16, scfInputSizeW max value is 4096
    flag = ((scfInputSizeW < 16) || (scfInputSizeW > 4096));
    if (flag) {
        ACL_LOG_ERROR("[Check][Params]resize_input_w[%d] should be within [16, 4096]!", scfInputSizeW);
        return ACL_ERROR_INVALID_PARAM;
    }

    // scfOutputSizeW mini value is 16, scfOutputSizeW max value is 1920
    flag = ((scfOutputSizeW < 16) || (scfOutputSizeW > 1920));
    if (flag) {
        ACL_LOG_ERROR("[Check][Params]resize_output_w[%d] should be within [16, 1920]!", scfOutputSizeW);
        return ACL_ERROR_INVALID_PARAM;
    }

    // scfOutputSizeH mini value is 16, scfOutputSizeH max value is 4096
    flag = ((scfOutputSizeH < 16) || (scfOutputSizeH > 4096));
    if (flag) {
        ACL_LOG_ERROR("[Check][Params]resize_output_h[%d] should be within [16, 4096]!", scfOutputSizeH);
        return ACL_ERROR_INVALID_PARAM;
    }

    float scfRatio = (scfOutputSizeW * 1.0) / scfInputSizeW;
    // scf factor is within [1/16, 16]
    flag = ((scfRatio < (1.0 / 16)) || (scfRatio > 16));
    if (flag) {
        ACL_LOG_ERROR("[Check][Params]resize_output_w/resize_input_w[%f] should be within [1/16, 16]!", scfRatio);
        return ACL_ERROR_INVALID_PARAM;
    }

    scfRatio = (scfOutputSizeH * 1.0) / scfInputSizeH;
    // scf factor is within [1/16, 16]
    flag = ((scfRatio < (1.0 / 16)) || (scfRatio > 16));
    if (flag) {
        ACL_LOG_ERROR("[Check][Params]resize_output_h/resize_input_h[%f] should be within [1/16, 16]!", scfRatio);
        return ACL_ERROR_INVALID_PARAM;
    }

    return ACL_SUCCESS;
}

aclError AippCropSizeCheck(const aclmdlAIPP *aippParmsSet, std::string &socVersion, int32_t batchIndex)
{
    if (aippParmsSet->aippBatchPara.size() == 0) {
        ACL_LOG_ERROR("[Check][Params]the size of aippBatchPara can't be zero");
        return ACL_ERROR_INVALID_PARAM;
    }

    int32_t srcImageSizeW = aippParmsSet->aippParms.srcImageSizeW;
    int32_t srcImageSizeH = aippParmsSet->aippParms.srcImageSizeH;

    int32_t cropStartPosW = aippParmsSet->aippBatchPara[batchIndex].cropStartPosW;
    int32_t cropStartPosH = aippParmsSet->aippBatchPara[batchIndex].cropStartPosH;
    int32_t cropSizeW = aippParmsSet->aippBatchPara[batchIndex].cropSizeW;
    int32_t cropSizeH = aippParmsSet->aippBatchPara[batchIndex].cropSizeH;

    if (cropStartPosW + cropSizeW > srcImageSizeW) {
        ACL_LOG_ERROR("[Check][Params]the sum of cropStartPosW[%d] and cropSizeW[%d] is larger than "
            "srcImageSizeW[%d]", cropStartPosW, cropSizeW, srcImageSizeW);
        return ACL_ERROR_INVALID_PARAM;
    }

    if (cropStartPosH + cropSizeH > srcImageSizeH) {
        ACL_LOG_ERROR("[Check][Params]the sum of cropStartPosH[%d] and cropSizeH[%d] is "
            "larger than or equal to srcImageSizeH[%d]", cropStartPosH, cropSizeH, srcImageSizeH);
        return ACL_ERROR_INVALID_PARAM;
    }

    enum CceAippInputFormat inputFormat = (enum CceAippInputFormat)aippParmsSet->aippParms.inputFormat;
    bool flag = false;
    bool isLhisi = (socVersionForLhisi.find(socVersion) != socVersionForLhisi.end());
    if (inputFormat == CCE_YUV420SP_U8) {
        // determine whether it is even
        flag = ((cropStartPosW % 2 != 0) || (cropStartPosH % 2 != 0));
        if (flag) {
            ACL_LOG_ERROR("[Check][Params]cropStartPosW[%d], cropStartPosH[%d] must be even for YUV420SP_U8!",
                cropStartPosW, cropStartPosH);
            return ACL_ERROR_INVALID_PARAM;
        }
        flag = isLhisi && ((cropSizeW % 2 != 0) || (cropSizeH % 2 != 0));
        if (flag) {
            ACL_LOG_ERROR("[Check][Params]cropSizeW[%d] and cropSizeH[%d] must be even for YUV420SP_U8!",
                cropSizeW, cropSizeH);
            return ACL_ERROR_INVALID_PARAM;
        }
    } else if ((inputFormat == CCE_YUV422SP_U8) || (inputFormat == CCE_YUYV_U8)) {
        // determine whether it is even
        flag = (cropStartPosW % 2 != 0);
        if (flag) {
            ACL_LOG_ERROR("[Check][Params]cropStartPosW[%d] must be even for YUV422SP_U8 and YUYV_U8!", cropStartPosW);
            return ACL_ERROR_INVALID_PARAM;
        }
        flag = isLhisi && (cropSizeW % 2 != 0);
        if (flag) {
            ACL_LOG_ERROR("[Check][Params]cropSizeW[%d] must be even for YUV422SP_U8 and YUYV_U8!", cropSizeW);
            return ACL_ERROR_INVALID_PARAM;
        }
    }

    return ACL_SUCCESS;
}


aclError GetAippOutputHW(const aclmdlAIPP *aippParmsSet, int32_t batchIndex, std::string socVersion,
                         int32_t &aippOutputW, int32_t &aippOutputH)
{
    if (aippParmsSet->aippBatchPara.size() == 0) {
        ACL_LOG_ERROR("[Check][Params]aippParmsSet->aippBatchPara is empty!");
        return ACL_ERROR_INVALID_PARAM;
    }

    int32_t srcImageSizeW = aippParmsSet->aippParms.srcImageSizeW;
    int32_t srcImageSizeH = aippParmsSet->aippParms.srcImageSizeH;

    int8_t cropSwitch = aippParmsSet->aippBatchPara[batchIndex].cropSwitch;
    int32_t cropSizeW = aippParmsSet->aippBatchPara[batchIndex].cropSizeW;
    int32_t cropSizeH = aippParmsSet->aippBatchPara[batchIndex].cropSizeH;

    int8_t scfSwitch = aippParmsSet->aippBatchPara[batchIndex].scfSwitch;
    int32_t scfOutputSizeW = aippParmsSet->aippBatchPara[batchIndex].scfOutputSizeW;
    int32_t scfOutputSizeH = aippParmsSet->aippBatchPara[batchIndex].scfOutputSizeH;

    int8_t paddingSwitch = aippParmsSet->aippBatchPara[batchIndex].paddingSwitch;
    int32_t paddingSizeTop = aippParmsSet->aippBatchPara[batchIndex].paddingSizeTop;
    int32_t paddingSizeBottom = aippParmsSet->aippBatchPara[batchIndex].paddingSizeBottom;
    int32_t paddingSizeLeft = aippParmsSet->aippBatchPara[batchIndex].paddingSizeLeft;
    int32_t paddingSizeRight = aippParmsSet->aippBatchPara[batchIndex].paddingSizeRight;

    aippOutputW = srcImageSizeW;
    aippOutputH = srcImageSizeH;

    if (cropSwitch == 1) {
        aippOutputW = cropSizeW;
        aippOutputH = cropSizeH;

        if (scfSwitch == 1) {
            aippOutputW = scfOutputSizeW;
            aippOutputH = scfOutputSizeH;
        }
    } else if (scfSwitch == 1) {
        aippOutputW = scfOutputSizeW;
        aippOutputH = scfOutputSizeH;
    }

    if (paddingSwitch == 1) {
        aippOutputW += paddingSizeLeft + paddingSizeRight;
        aippOutputH += paddingSizeTop + paddingSizeBottom;

        if ((socVersion == "Ascend310") || (socVersion == "Ascend610") || (socVersion == "Ascend710") ||
            (socVersion == "Ascend615") || (strncmp(socVersion.c_str(), "Ascend910", strlen("Ascend910")) == 0)) {
            if (aippOutputW > 1080) {
                ACL_LOG_ERROR("[Check][Params]after padding, aipp output W[%d] should be less than or equal to 1080 "
                              "for Ascend310, Ascend610, Ascend710, Ascend615, Ascend910", aippOutputW);
                return ACL_ERROR_INVALID_PARAM;
            }
        } else if (socVersionForLhisi.find(socVersion) != socVersionForLhisi.end()) {
            if (aippOutputW > 4096) {
                ACL_LOG_ERROR("[Check][Params]after padding, aipp output W[%d] should be less than or equal to 4096 "
                              "for Hi3796CV300ES, Hi3796CV300CS, SD3403", aippOutputW);
                return ACL_ERROR_INVALID_PARAM;
            }
        }
    }

    return ACL_SUCCESS;
}

aclError AippDynamicBatchParaCheck(const aclmdlAIPP *aippParmsSet, std::string socVersion,
                                   int32_t aippOutputW, int32_t aippOutputH, bool isNewModel)
{
    int8_t scfSwitch = 0;
    int8_t cropSwitch = 0;
    int32_t aippBatchOutputW = 0;
    int32_t aippBatchOutputH = 0;
    int32_t aippFirstOutputW = 0;
    int32_t aippFirstOutputH = 0;

    aclError result = GetAippOutputHW(aippParmsSet, 0, socVersion, aippFirstOutputW, aippFirstOutputH);
    if (result != ACL_SUCCESS) {
        return result;
    }

    uint64_t batchSize = aippParmsSet->batchSize;
    bool flag = false;
    for (uint64_t i = 0; i < batchSize; i++) {
        scfSwitch = aippParmsSet->aippBatchPara[i].scfSwitch;
        if (scfSwitch == 1) {
            flag = (socVersionForLhisi.find(socVersion) == socVersionForLhisi.end());
            if (flag) {
                ACL_LOG_ERROR("[Check][Params]Only Hi3796CV300ES, Hi3796CV300CS and SD3403 support scf!");
                return ACL_ERROR_INVALID_PARAM;
            }

            result = AippScfSizeCheck(aippParmsSet, i);
            if (result != ACL_SUCCESS) {
                return result;
            }
        }

        cropSwitch = aippParmsSet->aippBatchPara[i].cropSwitch;
        if (cropSwitch == 1) {
            result = AippCropSizeCheck(aippParmsSet, socVersion, i);
            if (result != ACL_SUCCESS) {
                return result;
            }
        }

        result = GetAippOutputHW(aippParmsSet, i, socVersion, aippBatchOutputW, aippBatchOutputH);
        if (result != ACL_SUCCESS) {
            return result;
        }

        flag = ((aippBatchOutputW != aippFirstOutputW) || (aippBatchOutputH != aippFirstOutputH));
        if (flag) {
            ACL_LOG_ERROR("[Check][Params]the %d batch output size must be equal to the first "
                "batch aipp output size! aippBatchOutputW = %d, aippBatchOutputH = %d, aippFirstOutputW = %d, "
                "aippFirstOutputH = %d.", (i + 1), aippBatchOutputW, aippBatchOutputH, aippFirstOutputW,
                aippFirstOutputH);
            return ACL_ERROR_INVALID_PARAM;
        }
    }

    if (isNewModel) {
        flag = ((aippFirstOutputW != aippOutputW) || (aippFirstOutputH != aippOutputH));
        if (flag) {
            ACL_LOG_ERROR("[Check][Params]aipp output size by ACL must be equal to aipp output "
                "size in the model! AclAippOutputW = %d, AclAippOutputH = %d, ModelAippOutputW = %d, "
                "ModelAippOutputH = %d.", aippFirstOutputW, aippFirstOutputH, aippOutputW, aippOutputH);
            return ACL_ERROR_INVALID_PARAM;
        }
    } else {
        ACL_LOG_INFO("[Check][Params]isNewModel= %d, current used model is old, user needs to ensure that "
            "aipp output H and W in the model are equal to aipp output H[%d] and W[%d] by acl configure!",
            isNewModel, aippBatchOutputH, aippBatchOutputW);
    }

    return ACL_SUCCESS;
}

aclError AippParamsCheck(const aclmdlAIPP *aippParmsSet, std::string socVersion,
    int32_t aippOutputW, int32_t aippOutputH, bool isNewModel)
{
    ACL_LOG_INFO("start to execute aclAippParamsCheck");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(aippParmsSet);

    enum CceAippInputFormat inputFormat = (enum CceAippInputFormat)aippParmsSet->aippParms.inputFormat;
    aclError result = AippInputFormatCheck(inputFormat, socVersion);
    if (result != ACL_SUCCESS) {
        return result;
    }

    int8_t cscSwitch = aippParmsSet->aippParms.cscSwitch;
    bool flag = false;
    flag = ((inputFormat == CCE_YUV400_U8) || (inputFormat == CCE_RAW10) ||
        (inputFormat == CCE_RAW12) || (inputFormat == CCE_RAW16));
    if (flag == 1) {
        if (cscSwitch == 1) {
            ACL_LOG_ERROR("[Check][Params]YUV400 or raw not support csc switch!");
            return ACL_ERROR_INVALID_PARAM;
        }
    }

    int32_t srcImageSizeW = aippParmsSet->aippParms.srcImageSizeW;
    int32_t srcImageSizeH = aippParmsSet->aippParms.srcImageSizeH;

    result = AippSrcImageSizeCheck(inputFormat, srcImageSizeW, srcImageSizeH, socVersion);
    if (result != ACL_SUCCESS) {
        return result;
    }

    result = AippDynamicBatchParaCheck(aippParmsSet, socVersion, aippOutputW, aippOutputH, isNewModel);
    if (result != ACL_SUCCESS) {
        return result;
    }

    return ACL_SUCCESS;
}


uint64_t GetSrcImageSize(const aclmdlAIPP *aippParmsSet)
{
    ACL_REQUIRES_NOT_NULL(aippParmsSet);

    enum CceAippInputFormat inputFormat = (enum CceAippInputFormat)aippParmsSet->aippParms.inputFormat;
    uint64_t srcImageSizeW = (uint64_t)aippParmsSet->aippParms.srcImageSizeW;
    uint64_t srcImageSizeH = (uint64_t)aippParmsSet->aippParms.srcImageSizeH;
    uint64_t batch = aippParmsSet->batchSize;
    uint64_t size = 0;

    if (inputFormat == CCE_YUV420SP_U8) {
        // YUV420SP_U8, one pixel use uint16, 2 bytes
        ACL_CHECK_ASSIGN_SIZET_MULTI_RET_NUM(batch, THREE_CHANNEL * srcImageSizeH * srcImageSizeW / 2, size);
    } else if (inputFormat == CCE_XRGB8888_U8) {
        ACL_CHECK_ASSIGN_SIZET_MULTI_RET_NUM(batch, FOUR_CHANNEL * srcImageSizeH * srcImageSizeW, size);
    } else if (inputFormat == CCE_RGB888_U8) {
        ACL_CHECK_ASSIGN_SIZET_MULTI_RET_NUM(batch, THREE_CHANNEL * srcImageSizeH * srcImageSizeW, size);
    } else if (inputFormat == CCE_YUV400_U8) {
        ACL_CHECK_ASSIGN_SIZET_MULTI_RET_NUM(batch, srcImageSizeH * srcImageSizeW, size);
    }  else if (inputFormat == CCE_ARGB8888_U8) {
        ACL_CHECK_ASSIGN_SIZET_MULTI_RET_NUM(batch, FOUR_CHANNEL * srcImageSizeH * srcImageSizeW, size);
    } else if (inputFormat == CCE_YUYV_U8) {
        ACL_CHECK_ASSIGN_SIZET_MULTI_RET_NUM(batch, TWO_CHANNEL * srcImageSizeH * srcImageSizeW, size);
    } else if (inputFormat == CCE_YUV422SP_U8) {
        ACL_CHECK_ASSIGN_SIZET_MULTI_RET_NUM(batch, TWO_CHANNEL * srcImageSizeH * srcImageSizeW, size);
    } else if (inputFormat == CCE_AYUV444_U8) {
        ACL_CHECK_ASSIGN_SIZET_MULTI_RET_NUM(batch, FOUR_CHANNEL * srcImageSizeH * srcImageSizeW, size);
    } else if (inputFormat == CCE_RAW10) {
        // RAW10, one pixel use uint16, 2 bytes
        ACL_CHECK_ASSIGN_SIZET_MULTI_RET_NUM(batch, srcImageSizeH * srcImageSizeW * 2, size);
    } else if (inputFormat == CCE_RAW12) {
        // RAW12, one pixel use uint16, 2 bytes
        ACL_CHECK_ASSIGN_SIZET_MULTI_RET_NUM(batch, srcImageSizeH * srcImageSizeW * 2, size);
    } else if (inputFormat == CCE_RAW16) {
        // RAW16, one pixel use uint16, 2 bytes
        ACL_CHECK_ASSIGN_SIZET_MULTI_RET_NUM(batch, srcImageSizeH * srcImageSizeW * 2, size);
    } else if (inputFormat == CCE_RAW24) {
        // RAW24, one pixel use uint32, 4 bytes
        ACL_CHECK_ASSIGN_SIZET_MULTI_RET_NUM(batch, srcImageSizeH * srcImageSizeW * 4, size);
    }

    ACL_LOG_INFO("Input SrcImageSize = %lu, cce_InputFormat = %d", size, static_cast<int32_t>(inputFormat));
    return size;
}




