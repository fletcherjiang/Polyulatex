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

namespace {
    constexpr uint32_t DVPP_PNGD_MAX_OUTPUT_SIZE = 4096 * 4096 * 4 * 2;
    constexpr uint32_t DVPP_PNGD_MIN_OUTPUT_SIZE = 128 * 1024;
#define GET_PNG_VALUE(a, b, c, d)                                                                       \
    (static_cast<uint32_t>(((static_cast<uint32_t>(a)) << 24) | ((static_cast<uint32_t>(b)) << 16) |    \
        ((static_cast<uint32_t>(c)) << 8) | (static_cast<uint32_t>(d))))
}

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
            return ACL_ERROR_INVALID_PARAM;
        }
        resizeConfig->dvppResizeConfig.interpolation = interpolation;
        return ACL_SUCCESS;
    }

    aclError ImageProcessorV100::acldvppSetChannelDescMode(acldvppChannelDesc *channelDesc, uint32_t mode)
    {
        ACL_LOG_ERROR("Setting mode for channel desc is not supported in this version. Please check.");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError ImageProcessorV100::acldvppPngGetImageInfo(const void *data,
                                                        uint32_t dataSize,
                                                        uint32_t *width,
                                                        uint32_t *height,
                                                        int32_t *components)
    {
        ACL_REQUIRES_NOT_NULL(data);
        ACL_REQUIRES_NOT_NULL(width);
        ACL_REQUIRES_NOT_NULL(height);
        ACL_REQUIRES_NOT_NULL(components);

        aclError ret = GetPngImgInfo(data, dataSize, width, height, components, nullptr);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("get png info failed, result = %d.", ret);
            return ret;
        }

        return ACL_SUCCESS;
    }

    aclError ImageProcessorV100::acldvppPngPredictDecSize(const void *data,
                                                          uint32_t dataSize,
                                                          acldvppPixelFormat outputPixelFormat,
                                                          uint32_t *decSize)
    {
        ACL_REQUIRES_NOT_NULL(data);
        ACL_REQUIRES_NOT_NULL(decSize);
        if ((outputPixelFormat != PIXEL_FORMAT_RGB_888) && (outputPixelFormat != PIXEL_FORMAT_RGBA_8888) &&
            (outputPixelFormat != PIXEL_FORMAT_UNKNOWN)) {
            ACL_LOG_ERROR("the current outputPixelFormat[%d] is not support, "
                          "only support RGB_888, RGBA_8888 and UNKNOWN", static_cast<int32_t>(outputPixelFormat));
            return ACL_ERROR_FORMAT_NOT_MATCH;
        }

        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t bitDepth = 0;
        aclError ret = GetPngImgInfo(data, dataSize, &width, &height, nullptr, &bitDepth);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("get png info failed, result = %d.", ret);
            return ret;
        }

        // param according dvpp capability
        const uint32_t pngdMaxHeight = 4096;
        const uint32_t pngdMaxWidth = 4096;
        const uint32_t pngdMinHeight = 32;
        const uint32_t pngdMinWidth = 32;
        ACL_CHECK_WITH_MESSAGE_AND_RETURN((width >= pngdMinWidth) && (width <= pngdMaxWidth),
            ACL_ERROR_INVALID_PARAM, "png width = %u invalid, it should be between in [%u, %u].", width,
            pngdMinWidth, pngdMaxWidth);
        ACL_CHECK_WITH_MESSAGE_AND_RETURN((height >= pngdMinHeight) && (height <= pngdMaxHeight),
            ACL_ERROR_INVALID_PARAM, "png height = %u invalid, it should be between in [%u, %u].", height,
            pngdMinHeight, pngdMaxHeight);

        width = (width + 127) & (~127U); // width align to 128
        height = (height + 15) & (~15U); // align to 16
        switch (outputPixelFormat) {
            case acldvppPixelFormat::PIXEL_FORMAT_RGB_888:
                width = (width * bitDepth * 3 + 7) >> 3; // 3 represents rgb has 3 byte, 7 is padding
                break;
            case acldvppPixelFormat::PIXEL_FORMAT_UNKNOWN:
            case acldvppPixelFormat::PIXEL_FORMAT_RGBA_8888:
                width = (width * bitDepth * 4 + 7) >> 3; // 4 represents rgba has 4 byte, 7 is padding
                break;
            default:
                ACL_LOG_ERROR("invalid kind of outputPixelFormat[%d]", static_cast<int32_t>(outputPixelFormat));
                return ACL_ERROR_FORMAT_NOT_MATCH;
        }
        *decSize = width * height;
        ACL_CHECK_WITH_MESSAGE_AND_RETURN((*decSize > 0) && (*decSize <= DVPP_PNGD_MAX_OUTPUT_SIZE),
            ACL_ERROR_INVALID_PARAM, "pngd output size = %u invalid, it should be between in (0, %u].",
            *decSize, DVPP_PNGD_MAX_OUTPUT_SIZE);
        return ACL_SUCCESS;
    }

    aclError ImageProcessorV100::acldvppPngDecodeAsync(acldvppChannelDesc *channelDesc,
                                                       const void *data,
                                                       uint32_t size,
                                                       acldvppPicDesc *outputDesc,
                                                       aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppPngDecodeAsync");
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(data);
        ACL_REQUIRES_NOT_NULL(outputDesc);
        ACL_REQUIRES_NOT_NULL(outputDesc->dataBuffer.data);
        ACL_REQUIRES_POSITIVE(size);
        if (channelDesc->isNeedNotify) {
            ACL_REQUIRES_NOT_NULL(channelDesc->notify);
        }

        acldvppPixelFormat outputPixelFormat = static_cast<acldvppPixelFormat>(outputDesc->dvppPicDesc.format);
        if ((outputPixelFormat != PIXEL_FORMAT_RGB_888) &&
            (outputPixelFormat != PIXEL_FORMAT_UNKNOWN)) {
            ACL_LOG_ERROR("the current outputPixelFormat[%d] is not support, only support PIXEL_FORMAT_RGB_888, "
                          "PIXEL_FORMAT_UNKNOWN", static_cast<int32_t>(outputPixelFormat));
            return ACL_ERROR_FORMAT_NOT_MATCH;
        }

        // pngd have 3 input
        constexpr int32_t ioAddrNum = 3;
        constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t) + sizeof(uint32_t);
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        int i = 0;
        ioAddr[i++] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        ioAddr[i++] = reinterpret_cast<uintptr_t>(data);
        ioAddr[i] = reinterpret_cast<uintptr_t>(outputDesc->dataBuffer.data);

        auto sizeVal = reinterpret_cast<uint32_t *>(args.get() + sizeof(aicpu::AicpuParamHead) +
            ioAddrNum * sizeof(uint64_t));
        *sizeVal = size;

        if (aclRunMode_ == ACL_HOST) {
            aclError cpyRet = CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy input pic desc to device failed, result = %d.", cpyRet);
                return cpyRet;
            }
        }

        aclError launchRet = LaunchDvppTask(channelDesc, args.get(), argsSize,
            acl::dvpp::DVPP_KERNELNAME_DECODE_PNG, stream);
        if (launchRet != ACL_SUCCESS) {
            ACL_LOG_ERROR("launch dvpp task failed, result = %d.", launchRet);
            return launchRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            aclError cpyRet = CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_DEVICE_TO_HOST, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("copy output pic desc from device failed, result = %d.", cpyRet);
                return cpyRet;
            }
        }

        ACL_LOG_INFO("launch png decode task success");

        return ACL_SUCCESS;
    }

    aclError ImageProcessorV100::GetPngImgInfo(const void *data,
                                               uint32_t dataSize,
                                               uint32_t *width,
                                               uint32_t *height,
                                               int32_t *components,
                                               uint32_t *bitDepth)
    {
        // 29 represents png min size
        if (dataSize <= 29) {
            ACL_LOG_ERROR("invalid png image size %u, it must be larger than 29", dataSize);
            return ACL_ERROR_INVALID_PARAM;
        }

        const unsigned char *pngData = static_cast<const unsigned char *>(data);
        // 0, 1, 2, 3, 4, 5, 6, 7, 8 are first eight bytes of the png data and must be equal to
        // {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a}
        if ((GET_PNG_VALUE(pngData[0], pngData[1], pngData[2], pngData[3]) != 0x89504e47) ||
            (GET_PNG_VALUE(pngData[4], pngData[5], pngData[6], pngData[7]) != 0x0d0a1a0a)) {
            ACL_LOG_ERROR("invalid png image");
            return ACL_ERROR_INVALID_PARAM;
        }

        if (width != nullptr) {
            // indexs of 16, 17, 18, 19 represent png width
            *width = GET_PNG_VALUE(pngData[16], pngData[17], pngData[18], pngData[19]);
        }
        if (height != nullptr) {
            // indexs of 20, 21, 22, 23 represent png width
            *height = GET_PNG_VALUE(pngData[20], pngData[21], pngData[22], pngData[23]);
        }
        if (components != nullptr) {
            // 25 represents png color type
            // 0 and 2 represents no α channel, 4 and 6 represents that exists α channel
            if (pngData[25] == 0) {
                *components = 1; // 1 represents gray bitmap
            } else if (pngData[25] == 4) {
                *components = 2; // 2 represents gray bitmap with α channel
            } else if (pngData[25] == 2) {
                *components = 3; // 3 represents RGB
            } else if (pngData[25] == 6) {
                *components = 4; // 4 represents RGBA
            } else {
                ACL_LOG_ERROR("invalid png color type %u, only support 0, 2, 4 or 6",
                    static_cast<uint32_t>(pngData[25]));
                return ACL_ERROR_FORMAT_NOT_MATCH;
            }
        }
        if (bitDepth != nullptr) {
            // 24 represents png bit depth
            *bitDepth = static_cast<uint32_t>(pngData[24]);
        }

        return ACL_SUCCESS;
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
