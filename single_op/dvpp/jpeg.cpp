/**
* @file jpeg.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/ops/acl_dvpp.h"
#include "common/log_inner.h"
#include "single_op/dvpp/mgr/dvpp_manager.h"
#include "toolchain/profiling_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

aclError acldvppJpegDecodeAsync(acldvppChannelDesc *channelDesc,
                                const void *data,
                                uint32_t size,
                                acldvppPicDesc *outputDesc,
                                aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_STAGES_REG(acl::ACL_STAGE_DVPP, acl::ACL_STAGE_DEFAULT);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("[Check][ImageProcessor]image processor is null.");
        const char *argList[] = {"param"};
        const char *argVal[] = {"imageProcessor"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
            argList, argVal, 1);
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppJpegDecodeAsync(channelDesc, data, size, outputDesc, stream);
}

aclError acldvppJpegEncodeAsync(acldvppChannelDesc *channelDesc,
                                acldvppPicDesc *inputDesc,
                                const void *data,
                                uint32_t *size,
                                acldvppJpegeConfig *config,
                                aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_STAGES_REG(acl::ACL_STAGE_DVPP, acl::ACL_STAGE_DEFAULT);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("[Check][ImageProcessor]image processor is null.");
        const char *argList[] = {"param"};
        const char *argVal[] = {"imageProcessor"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
            argList, argVal, 1);
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppJpegEncodeAsync(channelDesc, inputDesc, data, size, config, stream);
}

aclError acldvppJpegGetImageInfo(const void *data,
                                 uint32_t size,
                                 uint32_t *width,
                                 uint32_t *height,
                                 int32_t *components)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("[Check][ImageProcessor]image processor is null.");
        const char *argList[] = {"param"};
        const char *argVal[] = {"imageProcessor"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
            argList, argVal, 1);
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppJpegGetImageInfo(data, size, width, height, components, nullptr);
}

aclError acldvppJpegGetImageInfoV2(const void *data,
                                   uint32_t size,
                                   uint32_t *width,
                                   uint32_t *height,
                                   int32_t *components,
                                   acldvppJpegFormat *format)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("[Check][ImageProcessor]image processor is null.");
        const char *argList[] = {"param"};
        const char *argVal[] = {"imageProcessor"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
            argList, argVal, 1);
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppJpegGetImageInfo(data, size, width, height, components, format);
}


// The size calculated is used by dvpp.
// You may not understand the details, which is normal, because the logic of dvpp is unreasonable.
aclError acldvppJpegPredictEncSize(const acldvppPicDesc *inputDesc, const acldvppJpegeConfig *config, uint32_t *size)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_STAGES_REG(acl::ACL_STAGE_DVPP, acl::ACL_STAGE_DEFAULT);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("[Check][ImageProcessor]image processor is null.");
        const char *argList[] = {"param"};
        const char *argVal[] = {"imageProcessor"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
            argList, argVal, 1);
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppJpegPredictEncSize(inputDesc, config, size);
}

aclError acldvppJpegPredictDecSize(const void *data,
                                   uint32_t dataSize,
                                   acldvppPixelFormat outputPixelFormat,
                                   uint32_t *decSize)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_STAGES_REG(acl::ACL_STAGE_DVPP, acl::ACL_STAGE_DEFAULT);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("[Check][ImageProcessor]image processor is null.");
        const char *argList[] = {"param"};
        const char *argVal[] = {"imageProcessor"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
            argList, argVal, 1);
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppJpegPredictDecSize(data, dataSize, outputPixelFormat, decSize);
}

#ifdef __cplusplus
}
#endif
