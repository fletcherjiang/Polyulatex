/**
* @file png.cpp
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

namespace {
    constexpr uint32_t AICPU_VERSION_PNGD = 1;
}

#ifdef __cplusplus
extern "C" {
#endif

aclError acldvppPngDecodeAsync(acldvppChannelDesc *channelDesc,
                               const void *data,
                               uint32_t size,
                               acldvppPicDesc *outputDesc,
                               aclrtStream stream)

{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_STAGES_REG(acl::ACL_STAGE_DVPP, acl::ACL_STAGE_DEFAULT);
    uint32_t aicpuVersion = acl::dvpp::DvppManager::GetInstance().GetAicpuVersion();
    if (aicpuVersion < AICPU_VERSION_PNGD) {
        ACL_LOG_INNER_ERROR("[Check][aicpuVersion]curVersion[%u], aicpu version must be larger than or "
            "equal to version 1 when using pngd.", aicpuVersion);
        return ACL_ERROR_RESOURCE_NOT_MATCH;
    }
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("[Check][ImageProcessor]image processor is null.");
        const char *argList[] = {"param"};
        const char *argVal[] = {"imageProcessor"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
            argList, argVal, 1);
        return ACL_ERROR_INTERNAL_ERROR;
    }

    return imageProcessor->acldvppPngDecodeAsync(channelDesc, data, size, outputDesc, stream);
}

aclError acldvppPngGetImageInfo(const void *data,
                                uint32_t dataSize,
                                uint32_t *width,
                                uint32_t *height,
                                int32_t *components)

{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    uint32_t aicpuVersion = acl::dvpp::DvppManager::GetInstance().GetAicpuVersion();
    if (aicpuVersion < AICPU_VERSION_PNGD) {
        ACL_LOG_INNER_ERROR("[Check][Version]curVersion[%u], aicpu version must be larger "
            "than or equal to version 1 when using pngd.", aicpuVersion);
        return ACL_ERROR_RESOURCE_NOT_MATCH;
    }
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }

    return imageProcessor->acldvppPngGetImageInfo(data, dataSize, width, height, components);
}

aclError acldvppPngPredictDecSize(const void *data,
                                  uint32_t dataSize,
                                  acldvppPixelFormat outputPixelFormat,
                                  uint32_t *decSize)

{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_STAGES_REG(acl::ACL_STAGE_DVPP, acl::ACL_STAGE_DEFAULT);
    uint32_t aicpuVersion = acl::dvpp::DvppManager::GetInstance().GetAicpuVersion();
    if (aicpuVersion < AICPU_VERSION_PNGD) {
        ACL_LOG_ERROR("curVersion[%u], aicpu version must be larger than or equal to version 1 when using pngd.",
            aicpuVersion);
        return ACL_ERROR_RESOURCE_NOT_MATCH;
    }
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }

    return imageProcessor->acldvppPngPredictDecSize(data, dataSize, outputPixelFormat, decSize);
}

#ifdef __cplusplus
}
#endif

