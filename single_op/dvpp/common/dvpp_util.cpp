/**
* @file dvpp_func_internal.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "dvpp_util.h"
#include <set>
#include "runtime/rt.h"
#include "runtime/config.h"
#include "runtime/dev.h"
#include "runtime/mem.h"
#include "driver/ascend_hal.h"
#include "acl/acl_base.h"
#include "common/log_inner.h"
#include "common/error_codes_inner.h"

namespace acl {
    namespace dvpp {
    aclError CopyDvppPicDescAsync(const acldvppPicDesc *aclPicDesc, aclrtMemcpyKind memcpyKind, aclrtStream stream)
    {
        ACL_REQUIRES_NOT_NULL(aclPicDesc);
        ACL_REQUIRES_NOT_NULL(aclPicDesc->dataBuffer.data);

        void *hostData = const_cast<aicpu::dvpp::DvppPicDesc *>(&(aclPicDesc->dvppPicDesc));
        // It's possible that hostSize and devSize are not equal to sizeof(aclPicDesc->dvppPicDesc)
        size_t hostSize = aclPicDesc->dataBuffer.length;
        void *deviceData = const_cast<void *>(aclPicDesc->dataBuffer.data);
        size_t deviceSize = aclPicDesc->dataBuffer.length;

        rtError_t rtMemcpyRet = RT_ERROR_NONE;
        switch (memcpyKind) {
            case ACL_MEMCPY_HOST_TO_DEVICE:
                rtMemcpyRet = rtMemcpyAsync(deviceData, deviceSize, hostData,
                                            hostSize, RT_MEMCPY_HOST_TO_DEVICE, static_cast<rtStream_t>(stream));
                break;
            case ACL_MEMCPY_DEVICE_TO_HOST:
                rtMemcpyRet = rtMemcpyAsync(hostData, hostSize, deviceData,
                                            deviceSize, RT_MEMCPY_DEVICE_TO_HOST, static_cast<rtStream_t>(stream));
                break;
            default:
                ACL_LOG_INNER_ERROR("[Check][Kind]invalid memcpy kind: %d", static_cast<int32_t>(memcpyKind));
                return ACL_ERROR_INVALID_PARAM;
        }

        if (rtMemcpyRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Copy][Mem]memcpy between host and device failed, kind = %d, runtime result = %d",
                static_cast<int32_t>(memcpyKind), rtMemcpyRet);
            return ACL_GET_ERRCODE_RTS(rtMemcpyRet);
        }
        return ACL_SUCCESS;
    }

    aclError CopyDvppBatchPicDescAsync(acldvppBatchPicDesc *aclBatchPicDesc,
                                       aclrtMemcpyKind memcpyKind,
                                       uint32_t batchSize,
                                       aclrtStream stream)
    {
        ACL_REQUIRES_NOT_NULL(aclBatchPicDesc);
        ACL_REQUIRES_NOT_NULL(aclBatchPicDesc->dataBuffer.data);
        if (aclBatchPicDesc->dvppBatchPicDescs.batchSize < batchSize) {
            ACL_LOG_INNER_ERROR("[Check][BatchSize]batchSize[%u] must be larger than or equal to roi batch size[%u]",
                aclBatchPicDesc->dvppBatchPicDescs.batchSize, batchSize);
            return ACL_ERROR_INVALID_PARAM;
        }

        aclBatchPicDesc->dvppBatchPicDescs.batchSize = batchSize;
        for (size_t index = 0; index < batchSize; ++index) {
            aclBatchPicDesc->dvppBatchPicDescs.dvppPicDescs[index] =
                aclBatchPicDesc->aclDvppPicDescs[index].dvppPicDesc;
        }

        void *hostData = &(aclBatchPicDesc->dvppBatchPicDescs);
        size_t hostSize = sizeof(aicpu::dvpp::DvppPicDesc) * batchSize + sizeof(uint32_t);
        void *devData = aclBatchPicDesc->dataBuffer.data;
        size_t devSize = aclBatchPicDesc->dataBuffer.length;
        if (hostSize != devSize) {
            ACL_LOG_INNER_ERROR("[Check][Size]memory size between host [%zu] and device [%zu] not equal",
                hostSize, devSize);
            return ACL_ERROR_INVALID_PARAM;
        }

        rtError_t ret = RT_ERROR_NONE;
        switch (memcpyKind) {
            case ACL_MEMCPY_HOST_TO_DEVICE:
                ret = rtMemcpyAsync(devData, devSize, hostData,
                                    hostSize, RT_MEMCPY_HOST_TO_DEVICE, static_cast<rtStream_t>(stream));
                break;
            case ACL_MEMCPY_DEVICE_TO_HOST:
                ret = rtMemcpyAsync(hostData, hostSize, devData,
                                    devSize, RT_MEMCPY_DEVICE_TO_HOST, static_cast<rtStream_t>(stream));
                break;
            default:
                ACL_LOG_INNER_ERROR("[Check][Kind]invalid memcpy kind: %d", static_cast<int32_t>(memcpyKind));
                return ACL_ERROR_INVALID_PARAM;
        }

        if (ret != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Copy][Mem]memcpy between host and device failed, kind = %d, runtime result = %d",
                static_cast<int32_t>(memcpyKind), ret);
            return ACL_GET_ERRCODE_RTS(ret);
        }
        return ACL_SUCCESS;
    }

    aclError CopyDvppHistDescAsync(acldvppHist *aclDvppHist, aclrtMemcpyKind memcpyKind, aclrtStream stream)
    {
        ACL_REQUIRES_NOT_NULL(aclDvppHist);
        ACL_REQUIRES_NOT_NULL(aclDvppHist->dvppHistDesc.hist);
        ACL_REQUIRES_NOT_NULL(aclDvppHist->dataBuffer.data);

        void *hostData = &(aclDvppHist->dvppHistDesc);
        // It's possible that hostSize and devSize are not equal to sizeof(aclPicDesc->dvppPicDesc)
        size_t hostSize = aclDvppHist->dataBuffer.length;
        void *deviceData = aclDvppHist->dataBuffer.data;
        size_t deviceSize = aclDvppHist->dataBuffer.length;

        aclError aclMemcpyRet = ACL_SUCCESS;
        switch (memcpyKind) {
            case ACL_MEMCPY_HOST_TO_DEVICE:
                aclMemcpyRet = aclrtMemcpyAsync(deviceData, deviceSize, hostData, hostSize,
                                                ACL_MEMCPY_HOST_TO_DEVICE, stream);
                break;
            case ACL_MEMCPY_DEVICE_TO_HOST:
                aclMemcpyRet = aclrtMemcpyAsync(hostData, hostSize, deviceData, deviceSize,
                                                ACL_MEMCPY_DEVICE_TO_HOST, stream);
                break;
            default:
                ACL_LOG_INNER_ERROR("[Copy][Kind]invalid memcpy kind: %d", static_cast<int32_t>(memcpyKind));
                return ACL_ERROR_INVALID_PARAM;
        }

        if (aclMemcpyRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Copy][Mem]memcpy between host and device failed, kind = %d, result = %d",
                static_cast<int32_t>(memcpyKind), aclMemcpyRet);
            return aclMemcpyRet;
        }
        return ACL_SUCCESS;
    }

    void FreeDeviceBuffer(aclDataBuffer &dataBuffer)
    {
        if (dataBuffer.data != nullptr) {
            rtError_t rtErr = rtFree(dataBuffer.data);
            if (rtErr != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Free][Mem]free device mem failed, runtime result = %d",
                    static_cast<int32_t>(rtErr));
            }
            dataBuffer.data = nullptr;
        }
    }

    void FreeDvppDeviceBuffer(aclDataBuffer &dataBuffer)
    {
        if (dataBuffer.data != nullptr) {
            rtError_t rtErr = rtDvppFree(dataBuffer.data);
            if (rtErr != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Free][Mem]free dvpp device mem(4G) failed, "
                    "runtime result = %d", static_cast<int32_t>(rtErr));
            }
            dataBuffer.data = nullptr;
        }
    }

    void FreeDeviceAddr(void *devAddr)
    {
        if (devAddr != nullptr) {
            rtError_t rtErr = rtFree(devAddr);
            if (rtErr != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Free][Mem]free device mem failed, runtime result = %d",
                    static_cast<int32_t>(rtErr));
            }
            devAddr = nullptr;
        }
    }

    void AcldvppLibjpegErrorExit(j_common_ptr cinfo)
    {
        char jpegLastErrorMsg[JMSG_LENGTH_MAX] = {0};
        (*(cinfo->err->format_message))(cinfo, jpegLastErrorMsg);
        ACL_LOG_INNER_ERROR("[Run][LibJpeg]run libjpeg get error : %s", jpegLastErrorMsg);
        throw std::runtime_error(jpegLastErrorMsg);
    }

    bool CalcImageSizeKernel(uint32_t width, uint32_t height, acldvppPixelFormat format, uint32_t *size)
    {
        bool ret = true;
        switch (format) {
            case acldvppPixelFormat::PIXEL_FORMAT_YUV_400:
            case acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_420:
            case acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_420:
                *size = width * height * 3 / 2;  // w*h*3/2
                break;
            case acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_422:
            case acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_422:
                *size = width * height * 2;  // size=w*h*2 if pixel format is YUV422
                break;
            case acldvppPixelFormat::PIXEL_FORMAT_YUYV_PACKED_422:
            case acldvppPixelFormat::PIXEL_FORMAT_UYVY_PACKED_422:
            case acldvppPixelFormat::PIXEL_FORMAT_YVYU_PACKED_422:
            case acldvppPixelFormat::PIXEL_FORMAT_VYUY_PACKED_422:
                *size = width * height;
                break;
            case acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_444:
            case acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_444:
                *size = width * height * 3;  // size=w*h*3 if pixel format is YUV444
                break;
            case acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_440:
                *size = width * height * 2;  // size=w*h*2 if pixel format is YUV440
                break;
            default:
                ACL_LOG_INNER_ERROR("[Unsupported][Format]not support image format %d.", static_cast<int32_t>(format));
                ret = false;
                break;
        }

        return ret;
    }

    bool IsPackedFormat(acldvppPixelFormat pixelFormat)
    {
        const std::set<acldvppPixelFormat> packFormatSet = {
            acldvppPixelFormat::PIXEL_FORMAT_YUYV_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_UYVY_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVYU_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_VYUY_PACKED_422
        };

        auto iter = packFormatSet.find(pixelFormat);
        if (iter != packFormatSet.end()) {
            return true;
        }
        return false;
    }

    bool IsYUV420(const struct jpeg_decompress_struct &libjpegHandler,
                  acldvppPixelFormat pixelFormat)
    {
        const uint32_t yChannel = 0;  // y channal
        const uint32_t uChannel = 1;  // u channal
        // for yuv420 format, the h_samp_factor of uChannel channel is twice that of yChannel
        // and the v_samp_factor of uChannel channel is twice that of yChannel
        if (libjpegHandler.comp_info[yChannel].h_samp_factor ==
            ((libjpegHandler.comp_info[uChannel].h_samp_factor) * 2)) {
            if (libjpegHandler.comp_info[yChannel].v_samp_factor ==
                ((libjpegHandler.comp_info[uChannel].v_samp_factor) * 2)) {
                if ((libjpegHandler.data_precision == 8) &&  // pixel size is 8 bit
                    (pixelFormat == PIXEL_FORMAT_UNKNOWN)) {
                        return true;
                }
            }
        }
        return false;
    }

    bool IsYUV422(const struct jpeg_decompress_struct &libjpegHandler,
                  acldvppPixelFormat pixelFormat)
    {
        const uint32_t yChannel = 0;  // y channal
        const uint32_t uChannel = 1;  // u channal
        // for yuv422 format, the h_samp_factor of uChannel channel is twice that of yChannel
        // and the v_samp_factor of uChannel channel is same as that of yChannel
        if (libjpegHandler.comp_info[yChannel].h_samp_factor ==
            ((libjpegHandler.comp_info[uChannel].h_samp_factor) * 2)) {
            if (libjpegHandler.comp_info[yChannel].v_samp_factor == libjpegHandler.comp_info[uChannel].v_samp_factor) {
                // 422
                if ((pixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422) ||
                    (pixelFormat == PIXEL_FORMAT_YVU_SEMIPLANAR_422) ||
                    (pixelFormat == PIXEL_FORMAT_UNKNOWN)) {
                    return true;
                }
            }
        }

        return false;
    }

    bool IsYUV444(const struct jpeg_decompress_struct &libjpegHandler,
                  acldvppPixelFormat pixelFormat)
    {
        const uint32_t yChannel = 0;  // y channal
        const uint32_t uChannel = 1;  // u channal
        // for yuv422 format, the h_samp_factor of uChannel channel is same as that of yChannel
        // and the v_samp_factor of uChannel channel is same as that of yChannel
        if ((libjpegHandler.comp_info[yChannel].h_samp_factor == libjpegHandler.comp_info[uChannel].h_samp_factor) &&
            (libjpegHandler.comp_info[yChannel].v_samp_factor == libjpegHandler.comp_info[uChannel].v_samp_factor)) {
            // 444
            if ((pixelFormat == PIXEL_FORMAT_YVU_SEMIPLANAR_444) ||
                (pixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_444) ||
                (pixelFormat == PIXEL_FORMAT_UNKNOWN)) {
                return true;
            }
        }
        return false;
    }

    bool IsYUV440(const struct jpeg_decompress_struct &libjpegHandler,
                  acldvppPixelFormat pixelFormat)
    {
        const uint32_t yChannel = 0;  // y channal
        const uint32_t uChannel = 1;  // u channal
        // for yuv420 format, the h_samp_factor of uChannel channel is same as that of yChannel
        // and the v_samp_factor of uChannel channel is same as that of yChannel
        if ((libjpegHandler.comp_info[yChannel].h_samp_factor == libjpegHandler.comp_info[uChannel].h_samp_factor) &&
            (libjpegHandler.comp_info[yChannel].v_samp_factor ==
             2 * libjpegHandler.comp_info[uChannel].v_samp_factor)) {
            // 440
            if ((pixelFormat == PIXEL_FORMAT_YVU_SEMIPLANAR_440) ||
                (pixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_440) ||
                (pixelFormat == PIXEL_FORMAT_UNKNOWN)) {
                return true;
            }
        }
        return false;
    }

    aclError CalcImageSizeForYUV(const struct jpeg_decompress_struct &libjpegHandler, bool needOrientation,
                                 acldvppPixelFormat pixelFormat, uint32_t *size)
    {
        uint32_t width = needOrientation ? static_cast<uint32_t>(libjpegHandler.image_height) :
                static_cast<uint32_t>(libjpegHandler.image_width);
        uint32_t height = needOrientation ? static_cast<uint32_t>(libjpegHandler.image_width) :
                static_cast<uint32_t>(libjpegHandler.image_height);
        width = (width + 127) & (~127);  // align to 128
        height = (height + 15) & (~15);  // align to 16
        const uint32_t uChannel = 1;  // u channal
        const uint32_t vChannel = 2;  // v channal

        if ((libjpegHandler.num_components == 3) &&  // channel number is 3
            (libjpegHandler.comp_info[uChannel].h_samp_factor == libjpegHandler.comp_info[vChannel].h_samp_factor) &&
            (libjpegHandler.comp_info[uChannel].v_samp_factor == libjpegHandler.comp_info[vChannel].v_samp_factor)) {
            if (IsYUV420(libjpegHandler, pixelFormat)) {
                ACL_LOG_INFO("pixel format is YUV420");
                (void) CalcImageSizeKernel(width, height, PIXEL_FORMAT_YVU_SEMIPLANAR_420, size);
                return ACL_SUCCESS;
            } else if (IsYUV422(libjpegHandler, pixelFormat)) {
                ACL_LOG_INFO("pixel format is YUV422");
                (void) CalcImageSizeKernel(width, height, PIXEL_FORMAT_YUV_SEMIPLANAR_422, size);
                return ACL_SUCCESS;
            } else if (IsYUV444(libjpegHandler, pixelFormat)) {
                ACL_LOG_INFO("pixel format is YUV444");
                (void) CalcImageSizeKernel(width, height, PIXEL_FORMAT_YVU_SEMIPLANAR_444, size);
                return ACL_SUCCESS;
            } else if (IsYUV440(libjpegHandler, pixelFormat)) {
                ACL_LOG_INFO("pixel format is YUV440");
                (void) CalcImageSizeKernel(width, height, PIXEL_FORMAT_YVU_SEMIPLANAR_440, size);
                return ACL_SUCCESS;
            }
        }
        ACL_LOG_INNER_ERROR("[Check][Color]The origin jpeg color space is YUV, unknow format, "
            "it can not be decode to format: %d", pixelFormat);

        return ACL_ERROR_FORMAT_NOT_MATCH;
    }

    aclError CalcImageSize(const struct jpeg_decompress_struct &libjpegHandler, bool needOrientation,
                           acldvppPixelFormat outputPixelFormat, uint32_t *size)
    {
        aclError ret = ACL_SUCCESS;
        J_COLOR_SPACE colorSpace = libjpegHandler.jpeg_color_space;
        uint32_t width = needOrientation ? static_cast<uint32_t>(libjpegHandler.image_height) :
                static_cast<uint32_t>(libjpegHandler.image_width);
        uint32_t height = needOrientation ? static_cast<uint32_t>(libjpegHandler.image_width) :
                static_cast<uint32_t>(libjpegHandler.image_height);
        switch (colorSpace) {
            case JCS_GRAYSCALE:
                width = (width + 127) & (~127U);  // align to 128
                height = (height + 15) & (~15U);  // align to 16
                (void) CalcImageSizeKernel(width,
                                           height,
                                           PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                           size);
                break;
            case JCS_YCbCr:
                ret = CalcImageSizeForYUV(libjpegHandler, needOrientation, outputPixelFormat, size);
                break;
            default:
                ACL_LOG_INNER_ERROR("[Check][Color]invalid image color space, the current image color space is %d",
                    static_cast<int32_t>(colorSpace));
                ret = ACL_ERROR_FORMAT_NOT_MATCH;
                break;
        }
        return ret;
    }

    bool IsSupportSuperTask(DvppVersion dvppVersion, uint32_t aiCpuVersion)
    {
        int32_t deviceId = 0;
        rtError_t rtRet = rtGetDevice(&deviceId);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Get][DeviceId]fail to get deviceId, runtime result = %d", rtRet);
            return false;
        }

        int32_t value = 0;
        rtRet = rtGetDeviceCapability(deviceId, static_cast<int32_t>(MODULE_TYPE_AICPU),
            static_cast<int32_t>(FEATURE_TYPE_SCHE), &value);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Get][Feature]fail to get feature information, runtime result = %d", rtRet);
            return false;
        }

        if ((static_cast<tagRtAicpuScheType>(value) == SCHEDULE_SOFTWARE_OPT) &&
            (aiCpuVersion >= acl::dvpp::AICPU_VERSION_NO_NOTIFY) &&
            (dvppVersion == DVPP_KERNELS_V100)) {
            ACL_LOG_INFO("super task is supported");
            return true;
        }

        return false;
    }
    }
}
