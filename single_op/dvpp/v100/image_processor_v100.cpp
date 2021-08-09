/**
* @file image_processor_v100.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "image_processor_v100.h"

#include <set>
#include <atomic>
#include <memory>
#include <cstddef>
#include "securec.h"
#include "runtime/rt.h"
#include "common/log_inner.h"
#include "aicpu/dvpp/dvpp_def.h"
#include "aicpu/common/aicpu_task_struct.h"
#include "single_op/dvpp/common/dvpp_util.h"

namespace acl {
namespace dvpp {
    aclError ImageProcessorV100::acldvppSetResizeConfigInterpolation(acldvppResizeConfig *resizeConfig,
                                                                     uint32_t interpolation)
    {
        ACL_LOG_DEBUG("start to execute acldvppSetResizeConfigInterpolation.");
        ACL_REQUIRES_NOT_NULL(resizeConfig);
        // 0 self developed interpolation algorithm; 1 bilinear; 2 Nearest neighbor(opencv);
        // 3 Bilinear; 4 Nearest neighbor(tf)
        if (interpolation > 4) {
            ACL_LOG_ERROR("the current interpolation[%u] is not support, only supporte [0,4]", interpolation);
            std::string valueStr = std::to_string(interpolation);
            const char *argList[] = {"param", "value", "reason"};
            const char *argVal[] = {"interpolation", valueStr.c_str(), "only supporte [0,4]"};
            acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_PARAM_MSG,
                argList, argVal, 3);
            return ACL_ERROR_INVALID_PARAM;
        }
        resizeConfig->dvppResizeConfig.interpolation = interpolation;
        return ACL_SUCCESS;
    }

    aclError ImageProcessorV100::acldvppSetChannelDescMode(acldvppChannelDesc *channelDesc, uint32_t mode)
    {
        ACL_LOG_INNER_ERROR("[Set][Mode]Setting mode for channel desc is not supported in this version. Please check.");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError ImageProcessorV100::ValidateVpcInputFormat(acldvppPixelFormat format)
    {
        static const std::set<acldvppPixelFormat> inputFormatSet = {
            acldvppPixelFormat::PIXEL_FORMAT_YUV_400,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_444,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_444,
            acldvppPixelFormat::PIXEL_FORMAT_YVYU_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_VYUY_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_YUYV_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_UYVY_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_PACKED_444,
            acldvppPixelFormat::PIXEL_FORMAT_BGR_888,
            acldvppPixelFormat::PIXEL_FORMAT_RGB_888,
            acldvppPixelFormat::PIXEL_FORMAT_ARGB_8888,
            acldvppPixelFormat::PIXEL_FORMAT_ABGR_8888,
            acldvppPixelFormat::PIXEL_FORMAT_RGBA_8888,
            acldvppPixelFormat::PIXEL_FORMAT_BGRA_8888,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMI_PLANNER_420_10BIT,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMI_PLANNER_420_10BIT
        };

        auto iter = inputFormatSet.find(format);
        if (iter != inputFormatSet.end()) {
            return ACL_SUCCESS;
        }
        return ACL_ERROR_FORMAT_NOT_MATCH;
    }

    aclError ImageProcessorV100::ValidateVpcOutputFormat(acldvppPixelFormat format)
    {
        static const std::set<acldvppPixelFormat> outputFormatSet = {
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_420
        };

        auto iter = outputFormatSet.find(format);
        if (iter != outputFormatSet.end()) {
            return ACL_SUCCESS;
        }
        return ACL_ERROR_FORMAT_NOT_MATCH;
    }

    aclError ImageProcessorV100::ValidateDvppResizeConfig(acldvppResizeConfig *config)
    {
        ACL_REQUIRES_NOT_NULL(config);
        // 1910: 0 self developed interpolation algorithm; 1 bilinear; 2 Nearest neighbor(opencv);
        // 3 Bilinear; 4 Nearest neighbor(tf)
        if (config->dvppResizeConfig.interpolation > 4) {
            ACL_LOG_ERROR("the current interpolation[%u] is not support, interpolation only can be set [0,4]",
                config->dvppResizeConfig.interpolation);
            const char *argList[] = {"feature", "reason"};
            const char *argVal[] = {"interpolation", "interpolation only can be set [0,4]"};
            acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG, argList, argVal, 2);
            return ACL_ERROR_FEATURE_UNSUPPORTED;
        }
        return ACL_SUCCESS;
    }

    aclError ImageProcessorV100::ValidateJpegOutputFormat(acldvppPixelFormat format)
    {
        static const std::set<acldvppPixelFormat> outputFormatSet = {
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_444,
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_440,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_440,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_422,
            acldvppPixelFormat::PIXEL_FORMAT_UNKNOWN
        };

        auto iter = outputFormatSet.find(format);
        if (iter != outputFormatSet.end()) {
            return ACL_SUCCESS;
        }
        return ACL_ERROR_FORMAT_NOT_MATCH;
    }
}
}
