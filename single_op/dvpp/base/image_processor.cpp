/**
* @file image_processor.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "image_processor.h"

#include <set>
#include <atomic>
#include <memory>
#include <cstddef>
#include "securec.h"
#include "runtime/rt.h"
#include "common/log_inner.h"
#include "error_codes_inner.h"
#include "aicpu/dvpp/dvpp_def.h"
#include "aicpu/common/aicpu_task_struct.h"
#include "single_op/dvpp/common/dvpp_util.h"

namespace acl {
    namespace dvpp {
        namespace {
            const char EXIF_HEADER[6] = {'E', 'x', 'i', 'f', 0, 0};
            const char TIFF_HEADER_LE[4] = {0x49, 0x49, 0x2a, 0x00};
            const char TIFF_HEADER_BE[4] = {0x4d, 0x4d, 0x00, 0x2a};
            #define EXIF_MARKER (JPEG_APP0 + 1)
            #define TIFF_LITTLE_ENDIAN 0x4949
            #define TIFF_BIG_ENDIAN 0x4d4d
            #define TIFF_ORIENTATION_TAG 0x0112
            #define JPEG_RAW_FORMAT_UNSUPPORT (-1)

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
            bool GetImageFormat(jpeg_decompress_struct &libjpegHandler, acldvppJpegFormat *format)
            {
                bool isValidFormat = false;
                size_t y = 0;
                size_t u = 1;
                size_t v = 2;
                // just support colorspace JCS_GRAYSCALE or JCS_YCbCr
                if (libjpegHandler.jpeg_color_space == JCS_YCbCr ||
                    libjpegHandler.jpeg_color_space == JCS_GRAYSCALE) {
                    // yuv400: component 1 1x1
                    // yuv420: component 3 2x2 1x1 1x1
                    // yuv422: component 3 2x1 1x1 1x1
                    // yuv440: component 3 1x2 1x1 1x1
                    // yuv444: component 3 1x1 1x1 1x1
                    if ((libjpegHandler.num_components == 1) &&
                        (libjpegHandler.comp_info[y].h_samp_factor == libjpegHandler.comp_info[y].v_samp_factor)) {
                            isValidFormat = true;
                            *format = ACL_JPEG_CSS_GRAY;
                    } else if ((libjpegHandler.num_components == 3) &&
                        (libjpegHandler.comp_info[u].h_samp_factor == libjpegHandler.comp_info[v].h_samp_factor) &&
                        (libjpegHandler.comp_info[u].v_samp_factor == libjpegHandler.comp_info[v].v_samp_factor)) {
                        if (libjpegHandler.comp_info[y].h_samp_factor ==
                            ((libjpegHandler.comp_info[u].h_samp_factor) * 2)) {
                            if (libjpegHandler.comp_info[y].v_samp_factor ==
                                ((libjpegHandler.comp_info[u].v_samp_factor) * 2)) {
                                isValidFormat = true;
                                *format = ACL_JPEG_CSS_420;
                            } else if (libjpegHandler.comp_info[y].v_samp_factor ==
                                       libjpegHandler.comp_info[u].v_samp_factor) {
                                isValidFormat = true;
                                *format = ACL_JPEG_CSS_422;
                            }
                        } else if (libjpegHandler.comp_info[y].h_samp_factor ==
                                   libjpegHandler.comp_info[u].h_samp_factor) {
                            if (libjpegHandler.comp_info[y].v_samp_factor ==
                                libjpegHandler.comp_info[u].v_samp_factor) {
                                isValidFormat = true;
                                *format = ACL_JPEG_CSS_444;
                            } else if (libjpegHandler.comp_info[y].v_samp_factor ==
                                       (libjpegHandler.comp_info[u].v_samp_factor * 2)) {
                                isValidFormat = true;
                                *format = ACL_JPEG_CSS_440;
                            }
                        }
                    }
                }
                return isValidFormat;
            }
            uint16_t GetU16(uint8_t *data, uint16_t endian)
            {
                if (endian == TIFF_BIG_ENDIAN) {
                    return ((uint16_t)data[0]) << 8 | data[1];
                } else {
                    return ((uint16_t)data[1]) << 8 | data[0];
                }
            }
            uint32_t GetU32(uint8_t *data, uint16_t endian)
            {
                if (endian == TIFF_BIG_ENDIAN) {
                    return ((uint32_t)data[0]) << 24 | ((uint32_t)data[1]) << 16 | ((uint32_t)data[2]) << 8 | data[3];
                } else {
                    return ((uint32_t)data[3]) << 24 | ((uint32_t)data[2]) << 16 | ((uint32_t)data[1]) << 8 | data[0];
                }
            }
            jpeg_saved_marker_ptr JpegdFindExifMarker(j_decompress_ptr cinfo)
            {
                jpeg_saved_marker_ptr tmpMarker = cinfo->marker_list;
                while (tmpMarker != nullptr) {
                    // 6 means ignore EXIF heder
                    if ((tmpMarker->marker == EXIF_MARKER) && (tmpMarker->data_length >= 6) &&
                       !memcmp(tmpMarker->data, EXIF_HEADER, 6)) {
                        return tmpMarker;
                    }
                    tmpMarker = tmpMarker->next;
                }
                return nullptr;
            }
            bool JudgeNeedOrientation(j_decompress_ptr cinfo)
            {
                jpeg_saved_marker_ptr exifMarker = JpegdFindExifMarker(cinfo);
                if (exifMarker == nullptr) {
                    return false;
                }
                uint16_t orientation = 0;
                uint32_t pos = 6; // 6 means ignore EXIF heder
                uint16_t endian = 0;

                // 8 means TIFF header
                if ((pos + 8) > exifMarker->data_length) {
                    return false;
                }
                if (!memcmp(&exifMarker->data[pos], TIFF_HEADER_LE, 4)) {
                    endian = TIFF_LITTLE_ENDIAN;
                } else if (!memcmp(&exifMarker->data[pos], TIFF_HEADER_BE, 4)) {
                    endian = TIFF_BIG_ENDIAN;
                } else {
                    return false;
                }

                pos += GetU32(&exifMarker->data[pos] + 4, endian);
                if ((pos + 2) > exifMarker->data_length) {
                    return false;
                }
                uint16_t entries = GetU16(&exifMarker->data[pos], endian);
                pos += 2;

                if ((pos + entries * 12) > exifMarker->data_length) {
                    return false;
                }

                while (entries--) {
                    uint16_t tag = GetU16(&exifMarker->data[pos], endian);
                    if (tag == TIFF_ORIENTATION_TAG) {
                        uint16_t format = GetU16(&exifMarker->data[pos + 2], endian);
                        uint16_t count = GetU32(&exifMarker->data[pos + 4], endian);
                        if ((format != 3) || (count != 1)) {
                            return false;
                        }
                        orientation = GetU16(&exifMarker->data[pos + 8], endian);
                    }
                    pos += 12;
                }
                ACL_LOG_INFO("orientation is %d", orientation);
                if ((orientation >= ORIENTATION_5) && (orientation <= ORIENTATION_8)) {
                    ACL_LOG_INFO("This image need orientation");
                    return true;
                }
                return false;
            }
        }

    void ImageProcessor::SetDvppWaitTaskType(acldvppChannelDesc *channelDesc)
    {
        channelDesc->dvppWaitTaskType = NOTIFY_TASK;
        ACL_LOG_INFO("dvpp wait task type is notify");
    }

    void ImageProcessor::ResetEvent(acldvppChannelDesc *channelDesc, rtStream_t rtStream)
    {
        if (channelDesc->dvppWaitTaskType == EVENT_TASK) {
            (void)rtEventReset(static_cast<rtEvent_t>(channelDesc->notify), rtStream);
        }
    }

    aclError ImageProcessor::DvppCreateChannelWithNotify(acldvppChannelDesc *channelDesc)
    {
        SetDvppWaitTaskType(channelDesc);

        // create notify for dvpp channel desc
        aclError aclRet = CreateNotifyForDvppChannel(channelDesc);
        if (aclRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Create][Notify]create notify for dvpp channel failed, result = %d", aclRet);
            return aclRet;
        }

        rtStream_t rtStream = nullptr;
        rtError_t rtCreate = rtStreamCreate(&rtStream, RT_STREAM_PRIORITY_DEFAULT);
        if (rtCreate != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Create][Stream]create stream failed, result = %d", rtCreate);
            if (channelDesc->dvppWaitTaskType == NOTIFY_TASK) {
                (void)rtNotifyDestroy(static_cast<rtNotify_t>(channelDesc->notify));
            } else {
                (void)rtEventDestroy(static_cast<rtEvent_t>(channelDesc->notify));
            }
            channelDesc->notify = nullptr;
            return ACL_GET_ERRCODE_RTS(rtCreate);
        }

        // clear event task, prevent resource residue
        ResetEvent(channelDesc, rtStream);

        size_t size = CalDevDvppStructRealUsedSize(&channelDesc->dvppDesc);
        if (aclRunMode_ == ACL_HOST) {
            // if online memcpy data to device (both async and sync copy type are OK for this api)
            rtError_t rtRet = rtMemcpyAsync(channelDesc->dataBuffer.data, channelDesc->dataBuffer.length,
                                            static_cast<const void *>(&channelDesc->dvppDesc), size,
                                            RT_MEMCPY_HOST_TO_DEVICE, rtStream);
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Copy][Mem]memcpy to device memory failed, size = %zu, "
                    "runtime result = %d", size, rtRet);
                DestroyNotifyAndStream(channelDesc, rtStream);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }
        }

        // create channel have 1 output and 1 input
        constexpr int32_t ioAddrNum = 2;
        constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        char args[argsSize] = {0};
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args);
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->cmdListBuffer.data);
        ioAddr[1] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);

        rtError_t rtRet = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
                                            acl::dvpp::DVPP_KERNELNAME_CREATE_CHANNEL,
                                            1, // blockDim default 1
                                            args,
                                            argsSize,
                                            nullptr, // no need smDesc
                                            rtStream);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Create][Channel]dvpp create channel call rtCpuKernelLaunch failed, "
                "runtime result = %d.", rtRet);
            DestroyNotifyAndStream(channelDesc, rtStream);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }

        if (aclRunMode_ == ACL_HOST) {
            rtError_t rtRet = rtMemcpyAsync(&channelDesc->dvppDesc, size,
                                            static_cast<const void *>(channelDesc->dataBuffer.data), size,
                                            RT_MEMCPY_DEVICE_TO_HOST, rtStream);
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Copy][Mem]memcpy to host memory failed, size = %zu, "
                    "runtime result = %d", size, rtRet);
                DestroyNotifyAndStream(channelDesc, rtStream);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }
        }

        rtError_t rtSynchronize = rtStreamSynchronize(rtStream);
        if (rtSynchronize != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Copy][Stream]synchronize stream failed, result = %d", rtSynchronize);
            DestroyNotifyAndStream(channelDesc, rtStream);
            return ACL_GET_ERRCODE_RTS(rtSynchronize);
        }

        // retCode = 0 : success
        if (channelDesc->dvppDesc.retCode != 0) {
            ACL_LOG_INNER_ERROR("[Create][Channel]create channel failed, result = %d", channelDesc->dvppDesc.retCode);
            DestroyNotifyAndStream(channelDesc, rtStream);
            return ACL_ERROR_INTERNAL_ERROR;
        }

        rtError_t rtDestroy = rtStreamDestroy(rtStream);
        if (rtDestroy != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Destroy][Stream]destroy stream failed, runtime result = %d", rtDestroy);
            if (channelDesc->dvppWaitTaskType == NOTIFY_TASK) {
                (void)rtNotifyDestroy(static_cast<rtNotify_t>(channelDesc->notify));
            } else {
                (void)rtEventDestroy(static_cast<rtEvent_t>(channelDesc->notify));
            }
            channelDesc->notify = nullptr;
            return ACL_GET_ERRCODE_RTS(rtDestroy);
        }

        ACL_LOG_INFO("create dvpp channel with notify success, notifyId = %u", channelDesc->dvppDesc.notifyId);
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::DvppCreateChannelWithoutNotify(acldvppChannelDesc *channelDesc)
    {
        rtStream_t rtStream = nullptr;
        rtError_t rtCreate = rtStreamCreate(&rtStream, RT_STREAM_PRIORITY_DEFAULT);
        if (rtCreate != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Create][Stream]create stream failed, result = %d", rtCreate);
            return ACL_GET_ERRCODE_RTS(rtCreate);
        }

        size_t size = CalDevDvppStructRealUsedSize(&channelDesc->dvppDesc);
        if (aclRunMode_ == ACL_HOST) {
            // if online memcpy data to device (both async and sync copy type are OK for this api)
            rtError_t rtRet = rtMemcpyAsync(channelDesc->dataBuffer.data, channelDesc->dataBuffer.length,
                                            static_cast<const void *>(&channelDesc->dvppDesc), size,
                                            RT_MEMCPY_HOST_TO_DEVICE, rtStream);
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Copy][Mem]memcpy to device memory failed, size = %zu, "
                    "runtime result = %d", size, rtRet);
                DestroyStream(rtStream);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }
        }

        // create channel have 1 output and 1 input
        constexpr int32_t ioAddrNum = 2;
        constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        char args[argsSize] = {0};
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args);
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->cmdListBuffer.data);
        ioAddr[1] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);

        rtError_t rtRet = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
                                            acl::dvpp::DVPP_KERNELNAME_CREATE_CHANNEL_V2,
                                            1, // blockDim default 1
                                            args,
                                            argsSize,
                                            nullptr, // no need smDesc
                                            rtStream);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Create][Channel]dvpp create channel call rtCpuKernelLaunch failed, "
                "runtime result = %d.", rtRet);
            DestroyStream(rtStream);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }

        if (aclRunMode_ == ACL_HOST) {
            rtError_t rtRet = rtMemcpyAsync(&channelDesc->dvppDesc, size,
                                            static_cast<const void *>(channelDesc->dataBuffer.data), size,
                                            RT_MEMCPY_DEVICE_TO_HOST, rtStream);
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Copy][Mem]memcpy to host memory failed, size = %zu, "
                    "runtime result = %d", size, rtRet);
                DestroyStream(rtStream);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }
        }

        rtError_t rtSynchronize = rtStreamSynchronize(rtStream);
        if (rtSynchronize != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Sync][Stream]synchronize stream failed, runtime result = %d", rtSynchronize);
            DestroyStream(rtStream);
            return ACL_GET_ERRCODE_RTS(rtSynchronize);
        }

        // retCode = 0 : success
        if (channelDesc->dvppDesc.retCode != 0) {
            ACL_LOG_INNER_ERROR("[Create][Channel]create channel failed, result = %d", channelDesc->dvppDesc.retCode);
            DestroyStream(rtStream);
            return ACL_ERROR_INTERNAL_ERROR;
        }

        rtError_t rtDestroy = rtStreamDestroy(rtStream);
        if (rtDestroy != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Destroy][Stream]destroy stream failed, runtime result = %d", rtDestroy);
            return ACL_GET_ERRCODE_RTS(rtDestroy);
        }

        ACL_LOG_INFO("create dvpp channel without notify success");
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::acldvppCreateChannel(acldvppChannelDesc *channelDesc)
    {
        ACL_LOG_INFO("start to execute acldvppCreateChannel");
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        if (channelDesc->isNeedNotify) {
            return DvppCreateChannelWithNotify(channelDesc);
        } else {
            return DvppCreateChannelWithoutNotify(channelDesc);
        }
    }

    aclError ImageProcessor::DvppDestroyChannelWithNotify(acldvppChannelDesc *channelDesc)
    {
        // destroy channel have 1 input
        constexpr int32_t ioAddrNum = 1;
        constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        char args[argsSize] = {0};
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args);
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);

        rtStream_t rtStream = nullptr;
        rtError_t rtCreate = rtStreamCreate(&rtStream, RT_STREAM_PRIORITY_DEFAULT);
        if (rtCreate != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Create][Stream]create stream failed, runtime result = %d", rtCreate);
            if (channelDesc->dvppWaitTaskType == NOTIFY_TASK) {
                (void)rtNotifyDestroy(static_cast<rtNotify_t>(channelDesc->notify));
            } else {
                (void)rtEventDestroy(static_cast<rtEvent_t>(channelDesc->notify));
            }
            channelDesc->notify = nullptr;
            return ACL_GET_ERRCODE_RTS(rtCreate);
        }
        rtError_t rtRetVal = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
            acl::dvpp::DVPP_KERNELNAME_DESTROY_CHANNEL,
            1, // blockDim default 1
            args,
            argsSize,
            nullptr, // no need smDesc
            rtStream);
        if (rtRetVal != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Destroy][Channel]dvpp destroy channel call rtCpuKernelLaunch failed, "
                "runtime result = %d.", rtRetVal);
            DestroyNotifyAndStream(channelDesc, rtStream);
            return ACL_GET_ERRCODE_RTS(rtRetVal);
        }

        rtError_t rtSynchronizeVal = rtStreamSynchronize(rtStream);
        if (rtSynchronizeVal != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Sync][Stream]synchronize stream failed, runtime result = %d.", rtSynchronizeVal);
            DestroyNotifyAndStream(channelDesc, rtStream);
            return ACL_GET_ERRCODE_RTS(rtSynchronizeVal);
        }

        rtError_t rtDestroyVal = rtStreamDestroy(rtStream);
        if (rtDestroyVal != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Destroy][Stream]destroy stream failed, runtime result = %d.", rtDestroyVal);
            if (channelDesc->dvppWaitTaskType == NOTIFY_TASK) {
                (void)rtNotifyDestroy(static_cast<rtNotify_t>(channelDesc->notify));
            } else {
                (void)rtEventDestroy(static_cast<rtEvent_t>(channelDesc->notify));
            }
            channelDesc->notify = nullptr;
            return ACL_GET_ERRCODE_RTS(rtDestroyVal);
        }

        if (channelDesc->dvppWaitTaskType == NOTIFY_TASK) {
            rtRetVal = rtNotifyDestroy(static_cast<rtNotify_t>(channelDesc->notify));
        } else {
            rtRetVal = rtEventDestroy(static_cast<rtEvent_t>(channelDesc->notify));
        }
        if (rtRetVal != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Destroy][Notify]fail to destroy Notify, runtime result = %d.", rtRetVal);
            return ACL_GET_ERRCODE_RTS(rtRetVal);
        }
        channelDesc->notify = nullptr;
        ACL_LOG_INFO("destroy dvpp channel with notify success, notifyId = %u.", channelDesc->dvppDesc.notifyId);
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::DvppDestroyChannelWithoutNotify(acldvppChannelDesc *channelDesc)
    {
        // destroy channel have 1 input
        constexpr int32_t ioAddrNum = 1;
        constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        char args[argsSize] = {0};
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args);
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);

        rtStream_t rtStream = nullptr;
        rtError_t rtCreate = rtStreamCreate(&rtStream, RT_STREAM_PRIORITY_DEFAULT);
        if (rtCreate != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Create][Stream]create stream failed, result = %d", rtCreate);
            return ACL_GET_ERRCODE_RTS(rtCreate);
        }
        rtError_t rtRetVal = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
            acl::dvpp::DVPP_KERNELNAME_DESTROY_CHANNEL_V2,
            1, // blockDim default 1
            args,
            argsSize,
            nullptr, // no need smDesc
            rtStream);
        if (rtRetVal != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Destroy][Channel]dvpp destroy channel call "
                "rtCpuKernelLaunch failed, ret=%d.", rtRetVal);
            DestroyStream(rtStream);
            return ACL_GET_ERRCODE_RTS(rtRetVal);
        }

        rtError_t rtSynchronizeVal = rtStreamSynchronize(rtStream);
        if (rtSynchronizeVal != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Sync][Stream]synchronize stream failed, result = %d", rtSynchronizeVal);
            DestroyStream(rtStream);
            return ACL_GET_ERRCODE_RTS(rtSynchronizeVal);
        }

        rtError_t rtDestroyVal = rtStreamDestroy(rtStream);
        if (rtDestroyVal != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Destroy][Stream]destroy stream failed, result = %d", rtDestroyVal);
            return ACL_GET_ERRCODE_RTS(rtDestroyVal);
        }

        ACL_LOG_INFO("destroy dvpp channel without notify success");
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::acldvppDestroyChannel(acldvppChannelDesc *channelDesc)
    {
        ACL_LOG_INFO("start to execute acldvppDestroyChannel");
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);

        if (channelDesc->isNeedNotify) {
            return DvppDestroyChannelWithNotify(channelDesc);
        } else {
            return DvppDestroyChannelWithoutNotify(channelDesc);
        }
    }

    aclError ImageProcessor::LaunchDvppWaitTask(const acldvppChannelDesc *channelDesc, aclrtStream stream)
    {
        rtError_t rtRetCodeVal;
        if (channelDesc->dvppWaitTaskType == NOTIFY_TASK) {
            rtRetCodeVal = rtNotifyWait(static_cast<rtNotify_t>(channelDesc->notify), stream);
            if (rtRetCodeVal != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Wait][Notify]wait for a notify to stream failed, "
                    "runtime result = %d", rtRetCodeVal);
                return ACL_GET_ERRCODE_RTS(rtRetCodeVal);
            }
        } else {
            rtRetCodeVal = rtStreamWaitEvent(stream, static_cast<rtEvent_t>(channelDesc->notify));
            if (rtRetCodeVal != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Wait][Event]wait for a event to stream failed, "
                    "runtime result = %d", rtRetCodeVal);
                return ACL_GET_ERRCODE_RTS(rtRetCodeVal);
            }
            rtRetCodeVal = rtEventReset(static_cast<rtEvent_t>(channelDesc->notify), stream);
            if (rtRetCodeVal != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Reset][Event]reset a event to stream failed, "
                    "runtime result = %d", rtRetCodeVal);
                return ACL_GET_ERRCODE_RTS(rtRetCodeVal);
            }
        }

        return ACL_SUCCESS;
    }

    aclError ImageProcessor::LaunchDvppTask(const acldvppChannelDesc *channelDesc, const char *args, uint32_t argsSize,
        const char *kernelName, aclrtStream stream)
    {
        if (channelDesc->isNeedNotify) {
            rtError_t rtRetCodeVal = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
                                                       kernelName,
                                                       1, // blockDim default 1
                                                       args,
                                                       argsSize,
                                                       nullptr, // no need smDesc
                                                       stream);
            if (rtRetCodeVal != RT_ERROR_NONE) {
                ACL_LOG_INNER_ERROR("[Call][KernelLaunch]dvpp call rtCpuKernelLaunch failed, "
                    "runtime result = %d.", rtRetCodeVal);
                return ACL_GET_ERRCODE_RTS(rtRetCodeVal);
            }

            aclError launchTaskRet = LaunchDvppWaitTask(channelDesc, stream);
            if (launchTaskRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Launch][WaitTask]launch dvpp wait task failed, result = %d.", launchTaskRet);
                return launchTaskRet;
            }
            ACL_LOG_INFO("Launch dvpp tasks success, notifyId = %u", channelDesc->dvppDesc.notifyId);
        } else {
            std::string kernelNameV2 = kernelName;
            kernelNameV2.append("V2");
            rtError_t rtRetCodeVal = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
                                                       kernelNameV2.c_str(),
                                                       1, // blockDim default 1
                                                       args,
                                                       argsSize,
                                                       nullptr, // no need smDesc
                                                       stream);
            if (rtRetCodeVal != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Call][Kernel]dvpp call rtCpuKernelLaunch failed, "
                    "runtime result = %d.", rtRetCodeVal);
                return ACL_GET_ERRCODE_RTS(rtRetCodeVal);
            }
        }

        return ACL_SUCCESS;
    }

    aclError ImageProcessor::acldvppVpcResizeAsync(acldvppChannelDesc *channelDesc,
                                                   acldvppPicDesc *inputDesc,
                                                   acldvppPicDesc *outputDesc,
                                                   acldvppResizeConfig *resizeConfig,
                                                   aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppVpcResizeAsync");
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(inputDesc);
        ACL_REQUIRES_NOT_NULL(inputDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(outputDesc);
        ACL_REQUIRES_NOT_NULL(outputDesc->dataBuffer.data);
        if (channelDesc->isNeedNotify) {
            ACL_REQUIRES_NOT_NULL(channelDesc->notify);
        }

        // check vpc input format
        aclError validResizeInputRet = ValidateVpcInputFormat(
            static_cast<acldvppPixelFormat>(inputDesc->dvppPicDesc.format));
        if (validResizeInputRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Verify][Format]input picture describe format verify failed, "
                "result = %d, format = %u.", validResizeInputRet, inputDesc->dvppPicDesc.format);
            return validResizeInputRet;
        }

        // check vpc output format
        aclError validResizeOutputRet = ValidateVpcOutputFormat(
            static_cast<acldvppPixelFormat>(outputDesc->dvppPicDesc.format));
        if (validResizeOutputRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Verify][Format]output picture describe format verify failed, "
                "result = %d, format = %u.", validResizeOutputRet, outputDesc->dvppPicDesc.format);
            return validResizeOutputRet;
        }
        // check dvpp resize config
        aclError validResizeConfigRet = ValidateDvppResizeConfig(resizeConfig);
        if (validResizeConfigRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Verify][ResizeConfig]resize config acldvppResizeConfig verify failed, "
                "result = %d.", validResizeConfigRet);
            return validResizeConfigRet;
        }

        // CropAndPaste have 3 inputs
        constexpr int32_t ioAddrNum = 3;
        uint32_t resizeConfigSize = CalDevDvppStructRealUsedSize(&resizeConfig->dvppResizeConfig);
        uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t) + resizeConfigSize;
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        ioAddr[1] = reinterpret_cast<uintptr_t>(inputDesc->dataBuffer.data);
        ioAddr[2] = reinterpret_cast<uintptr_t>(outputDesc->dataBuffer.data);
        constexpr uint32_t configOffset = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        auto memcpyRet = memcpy_s(args.get() + configOffset, argsSize - configOffset, &(resizeConfig->dvppResizeConfig),
            resizeConfigSize);
        if (memcpyRet != EOK) {
            ACL_LOG_INNER_ERROR("[Copy][Mem]copy resize config to args failed, result = %d.", memcpyRet);
            return ACL_ERROR_FAILURE;
        }

        if (aclRunMode_ == ACL_HOST) {
            aclError cpyRet = CopyDvppPicDescAsync(inputDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][PicDesc]copy resize input picture desc failed, "
                    "result = %d.", cpyRet);
                return cpyRet;
            }
            cpyRet = CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][PicDesc]copy resize output picture desc to device failed, "
                    "result = %d.", cpyRet);
                return cpyRet;
            }
        }

        aclError launchRet = LaunchDvppTask(channelDesc, args.get(), argsSize,
            acl::dvpp::DVPP_KERNELNAME_RESIZE, stream);
        if (launchRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Launch][DvppTask]launch dvpp task failed, result = %d.", launchRet);
            return launchRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            aclError cpyRet = CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_DEVICE_TO_HOST, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][PicDesc]copy resize output pic desc from device failed, "
                    "result = %d.", cpyRet);
                return cpyRet;
            }
        }

        ACL_LOG_INFO("Launch vpc resize tasks success");
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::ValidateParamForDvppVpcCropResizePaste(const acldvppChannelDesc *channelDesc,
                                                                    const acldvppPicDesc *inputDesc,
                                                                    const acldvppPicDesc *outputDesc,
                                                                    const acldvppRoiConfig *cropArea,
                                                                    const acldvppRoiConfig *pasteArea,
                                                                    const bool &pasteAreaSwitch,
                                                                    acldvppResizeConfig *resizeConfig,
                                                                    const bool &resizeConfigSwitch,
                                                                    uint32_t &resizeConfigSize)
    {
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(inputDesc);
        ACL_REQUIRES_NOT_NULL(inputDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(outputDesc);
        ACL_REQUIRES_NOT_NULL(outputDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(cropArea);
        if (pasteAreaSwitch) {
            ACL_REQUIRES_NOT_NULL(pasteArea);
        }
        if (channelDesc->isNeedNotify) {
            ACL_REQUIRES_NOT_NULL(channelDesc->notify);
        }

        if (resizeConfigSwitch) {
            ACL_REQUIRES_NOT_NULL(resizeConfig);
            aclError validResizeConfigRet = ValidateDvppResizeConfig(resizeConfig);
            if (validResizeConfigRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Verify][DvppResizeConfig]resize config acldvppResizeConfig "
                    "verify failed, result = %d.", validResizeConfigRet);
                return validResizeConfigRet;
            }
            resizeConfigSize = CalDevDvppStructRealUsedSize(&resizeConfig->dvppResizeConfig);
        }

        // check vpc input format
        aclError validCropInputRet = ValidateVpcInputFormat(
            static_cast<acldvppPixelFormat>(inputDesc->dvppPicDesc.format));
        if (validCropInputRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Verify][VpcInputFormat]input acldvppPicDesc format verify failed, "
                "result = %d, format = %u.", validCropInputRet, inputDesc->dvppPicDesc.format);
            return validCropInputRet;
        }

        // check vpc output format
        aclError validCropOutputRet = ValidateVpcOutputFormat(
            static_cast<acldvppPixelFormat>(outputDesc->dvppPicDesc.format));
        if (validCropOutputRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Verify][VpcOutputFormat]output acldvppPicDesc format verify failed,"
                "result = %d, format = %u.", validCropOutputRet, outputDesc->dvppPicDesc.format);
            return validCropOutputRet;
        }
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::DvppVpcCropResizeAsync(acldvppChannelDesc *channelDesc,
                                                    acldvppPicDesc *inputDesc,
                                                    acldvppPicDesc *outputDesc,
                                                    acldvppRoiConfig *cropArea,
                                                    acldvppResizeConfig *resizeConfig,
                                                    const bool &resizeConfigSwitch,
                                                    aclrtStream stream)
    {
        uint32_t resizeConfigSize = 0;
        aclError validateRet = ValidateParamForDvppVpcCropResizePaste(channelDesc, inputDesc, outputDesc, cropArea,
            nullptr, false, resizeConfig, resizeConfigSwitch, resizeConfigSize);
        if (validateRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Verify][Param]verify params for DvppVpcCropResize failed, ret = %d", validateRet);
            return validateRet;
        }
        // CropAndPaste have 3 inputs
        constexpr int32_t ioAddrNum = 3;
        constexpr uint32_t roiConfigSize = sizeof(aicpu::dvpp::DvppRoiConfig);
        uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t) + roiConfigSize +
            resizeConfigSize;
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto cpuParamHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        cpuParamHead->length = argsSize;
        cpuParamHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        ioAddr[1] = reinterpret_cast<uintptr_t>(inputDesc->dataBuffer.data);
        ioAddr[2] = reinterpret_cast<uintptr_t>(outputDesc->dataBuffer.data);

        // copy roi config
        constexpr uint32_t cropAreaOffset = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        auto cpyRet = memcpy_s(args.get() + cropAreaOffset, argsSize - cropAreaOffset, &(cropArea->dvppRoiConfig),
            roiConfigSize);
        if (cpyRet != EOK) {
            ACL_LOG_INNER_ERROR("[Copy][CropArea]copy crop area to args failed, result = %d.", cpyRet);
            return ACL_ERROR_FAILURE;
        }

        if (aclRunMode_ == ACL_HOST) {
            aclError copyRet = CopyDvppPicDescAsync(inputDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (copyRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][PicDesc]copy input pic desc to device failed, result = %d.", copyRet);
                return copyRet;
            }

            copyRet = CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (copyRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][PicDesc]copy output pic desc to device failed, result = %d.", copyRet);
                return copyRet;
            }
        }
        const char *kernelName = acl::dvpp::DVPP_KERNELNAME_CROP;
        // copy resize config
        if (resizeConfigSwitch) {
            constexpr uint32_t resizeConfigOffset = cropAreaOffset + roiConfigSize;
            cpyRet = memcpy_s(args.get() + resizeConfigOffset, argsSize - resizeConfigOffset,
                &(resizeConfig->dvppResizeConfig), resizeConfigSize);
            if (cpyRet != EOK) {
                ACL_LOG_INNER_ERROR("[Cpoy][ResizeConfig]copy resize config to args failed, result = %d.", cpyRet);
                return ACL_ERROR_FAILURE;
            }
            kernelName = acl::dvpp::DVPP_KERNELNAME_CROP_RESIZE;
        }
        aclError launchRet = LaunchDvppTask(channelDesc, args.get(), argsSize,
            kernelName, stream);
        if (launchRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Launch][Task]launch dvpp task failed, result = %d.", launchRet);
            return launchRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            aclError copyRet = CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_DEVICE_TO_HOST, stream);
            if (copyRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][PicDesc]copy output pic desc from device failed, result = %d.", copyRet);
                return copyRet;
            }
        }
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::acldvppVpcCropResizeAsync(acldvppChannelDesc *channelDesc,
                                                       acldvppPicDesc *inputDesc,
                                                       acldvppPicDesc *outputDesc,
                                                       acldvppRoiConfig *cropArea,
                                                       acldvppResizeConfig *resizeConfig,
                                                       aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppVpcCropResizeAsync");
        auto ret = DvppVpcCropResizeAsync(channelDesc, inputDesc, outputDesc, cropArea, resizeConfig, true, stream);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Exec][DvppVpcCropResizeAsync]execute acldvppVpcCropResizeAsync failed, "
                "result = %d.", ret);
            return ret;
        }
        ACL_LOG_INFO("Launch vpc crop and resize tasks success");
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::acldvppVpcCropAsync(acldvppChannelDesc *channelDesc,
                                                 acldvppPicDesc *inputDesc,
                                                 acldvppPicDesc *outputDesc,
                                                 acldvppRoiConfig *cropArea,
                                                 aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppVpcCropAsync");
        auto ret = DvppVpcCropResizeAsync(channelDesc, inputDesc, outputDesc, cropArea, nullptr, false, stream);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Exec][DvppVpcCrop]execute acldvppVpcCropAsync failed, result = %d.", ret);
            return ret;
        }
        ACL_LOG_INFO("Launch vpc crop tasks success");
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::DvppVpcBatchCropResizeAsync(acldvppChannelDesc *channelDesc,
                                                         acldvppBatchPicDesc *srcBatchPicDescs,
                                                         uint32_t *roiNums,
                                                         uint32_t size,
                                                         acldvppBatchPicDesc *dstBatchPicDescs,
                                                         acldvppRoiConfig *cropAreas[],
                                                         acldvppResizeConfig *resizeConfig,
                                                         const bool &resizeConfigSwitch,
                                                         aclrtStream stream)
    {
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(srcBatchPicDescs);
        ACL_REQUIRES_NOT_NULL(srcBatchPicDescs->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(dstBatchPicDescs);
        ACL_REQUIRES_NOT_NULL(dstBatchPicDescs->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(cropAreas);
        ACL_REQUIRES_NOT_NULL(roiNums);
        ACL_REQUIRES_POSITIVE(size);
        if (channelDesc->isNeedNotify) {
            ACL_REQUIRES_NOT_NULL(channelDesc->notify);
        }

        if (resizeConfigSwitch) {
            ACL_REQUIRES_NOT_NULL(resizeConfig);
        }

        // valid input param
        std::unique_ptr<uint16_t[]> roiNumsPtr(new (std::nothrow)uint16_t[size]);
        if (roiNumsPtr == nullptr) {
            ACL_LOG_INNER_ERROR("[Create][BatchCrop]create batch crop roiNums pointer failed, "
                "roiNums size = %u.", size);
            return ACL_ERROR_INVALID_PARAM;
        }
        uint32_t totalRoiNums = 0;
        aclError validParamRet = ValidateAndConvertVpcBatchParams(srcBatchPicDescs,
                                                                  dstBatchPicDescs,
                                                                  roiNums,
                                                                  size,
                                                                  roiNumsPtr.get(),
                                                                  totalRoiNums,
                                                                  BATCH_ROI_MAX_SIZE,
                                                                  resizeConfig);
        if (validParamRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Verify][CropParam]verify batch crop param failed, result = %d.", validParamRet);
            return validParamRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            // copy input batch pic desc data
            aclError cpyAsyncRet = CopyDvppBatchPicDescAsync(srcBatchPicDescs,
                                                             ACL_MEMCPY_HOST_TO_DEVICE,
                                                             size,
                                                             stream);
            if (cpyAsyncRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][BatchPicDesc]async copy input batch pic desc to device failed, "
                    "result = %d.", cpyAsyncRet);
                return cpyAsyncRet;
            }

            // copy output batch pic desc data
            cpyAsyncRet = CopyDvppBatchPicDescAsync(dstBatchPicDescs,
                                                    ACL_MEMCPY_HOST_TO_DEVICE,
                                                    totalRoiNums,
                                                    stream);
            if (cpyAsyncRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][BatchPicDesc]async copy output batch pic desc to device failed, "
                    "result = %d.", cpyAsyncRet);
                return cpyAsyncRet;
            }
        } else {
            // set data buffer for input batch pic desc
            aclError setDataRet = SetDataBufferForBatchPicDesc(srcBatchPicDescs, size);
            if (setDataRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Set][DataBuffer]dvpp batch crop set data buffer for src batch pic desc "
                    "failed, result = %d.", setDataRet);
                return setDataRet;
            }

            setDataRet = SetDataBufferForBatchPicDesc(dstBatchPicDescs, totalRoiNums);
            if (setDataRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Set][DataBuffer]dvpp batch crop set data buffer for dst batch pic desc "
                    "failed, result = %d.", setDataRet);
                return setDataRet;
            }
        }

        void *roiNumsAddress = static_cast<void *>(roiNumsPtr.get());
        VpcBatchParams batchParams = {srcBatchPicDescs,
                                      dstBatchPicDescs,
                                      cropAreas,
                                      nullptr,
                                      totalRoiNums,
                                      roiNumsAddress,
                                      size};

        // launch task
        aclError launchTaskRet = LaunchTaskForVpcBatchCrop(channelDesc, batchParams, resizeConfig, stream);
        if (launchTaskRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Launch][Task]launch task for vpc batch crop failed, result = %d.", launchTaskRet);
            return launchTaskRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            // copy output data
            aclError validCpyRet = CopyDvppBatchPicDescAsync(dstBatchPicDescs,
                                                             ACL_MEMCPY_DEVICE_TO_HOST,
                                                             totalRoiNums,
                                                             stream);
            if (validCpyRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][BatchPicDesc]copy output batch pic desc to host failed, "
                    "result = %d.", validCpyRet);
                return validCpyRet;
            }
        }
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::acldvppVpcBatchCropResizeAsync(acldvppChannelDesc *channelDesc,
                                                            acldvppBatchPicDesc *srcBatchPicDescs,
                                                            uint32_t *roiNums,
                                                            uint32_t size,
                                                            acldvppBatchPicDesc *dstBatchPicDescs,
                                                            acldvppRoiConfig *cropAreas[],
                                                            acldvppResizeConfig *resizeConfig,
                                                            aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppVpcBatchCropResizeAsync");
        auto ret = DvppVpcBatchCropResizeAsync(channelDesc, srcBatchPicDescs, roiNums, size, dstBatchPicDescs,
            cropAreas, resizeConfig, true, stream);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Exec][BatchCropResize]execute acldvppVpcBatchCropResizeAsync failed, "
                "result = %d.", ret);
            return ret;
        }
        ACL_LOG_INFO("Launch vpc batch crop and resize tasks success");
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::acldvppVpcBatchCropAsync(acldvppChannelDesc *channelDesc,
                                                      acldvppBatchPicDesc *srcBatchPicDescs,
                                                      uint32_t *roiNums,
                                                      uint32_t size,
                                                      acldvppBatchPicDesc *dstBatchPicDescs,
                                                      acldvppRoiConfig *cropAreas[],
                                                      aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppVpcBatchCropAsync.");
        auto ret = DvppVpcBatchCropResizeAsync(channelDesc, srcBatchPicDescs, roiNums, size, dstBatchPicDescs,
            cropAreas, nullptr, false, stream);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Exec][BatchCrop]execute acldvppVpcBatchCropAsync failed, result = %d.", ret);
            return ret;
        }
        ACL_LOG_INFO("Launch vpc batch crop tasks success");
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::DvppVpcCropResizePasteAsync(acldvppChannelDesc *channelDesc,
                                                         acldvppPicDesc *inputDesc,
                                                         acldvppPicDesc *outputDesc,
                                                         acldvppRoiConfig *cropArea,
                                                         acldvppRoiConfig *pasteArea,
                                                         acldvppResizeConfig *resizeConfig,
                                                         const bool &resizeConfigSwitch,
                                                         aclrtStream stream)
    {
        uint32_t resizeConfigSize = 0;
        aclError validateRet = ValidateParamForDvppVpcCropResizePaste(channelDesc, inputDesc, outputDesc, cropArea,
            pasteArea, true, resizeConfig, resizeConfigSwitch, resizeConfigSize);
        if (validateRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Verify][Params]verify params for DvppVpcCropResizePaste failed, "
                "ret = %d", validateRet);
            return validateRet;
        }
        // CropAndPaste have 3 inputs
        constexpr int32_t ioAddrNum = 3;
        uint32_t cropAreaConfigSize = sizeof(aicpu::dvpp::DvppRoiConfig);
        uint32_t pasteAreaConfigSize = sizeof(aicpu::dvpp::DvppRoiConfig);
        uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t) +
            cropAreaConfigSize + pasteAreaConfigSize + resizeConfigSize;
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        ioAddr[1] = reinterpret_cast<uintptr_t>(inputDesc->dataBuffer.data);
        ioAddr[2] = reinterpret_cast<uintptr_t>(outputDesc->dataBuffer.data);

        constexpr uint32_t cropAreaOffset = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        auto memcpyRet = memcpy_s(args.get() + cropAreaOffset, argsSize - cropAreaOffset, &(cropArea->dvppRoiConfig),
                                  cropAreaConfigSize);
        if (memcpyRet != EOK) {
            ACL_LOG_INNER_ERROR("[Copy][Area]copy crop area to args failed, result = %d.", memcpyRet);
            return ACL_ERROR_FAILURE;
        }

        uint32_t pasteAreaOffset = cropAreaOffset + cropAreaConfigSize;
        memcpyRet = memcpy_s(args.get() + pasteAreaOffset, argsSize - pasteAreaOffset, &(pasteArea->dvppRoiConfig),
            pasteAreaConfigSize);
        if (memcpyRet != EOK) {
            ACL_LOG_INNER_ERROR("[Copy][Area]copy paste area to args failed, result = %d.", memcpyRet);
            return ACL_ERROR_FAILURE;
        }

        if (resizeConfigSwitch) {
            uint32_t resizeConfigOffset = pasteAreaOffset + pasteAreaConfigSize;
            memcpyRet = memcpy_s(args.get() + resizeConfigOffset, argsSize - resizeConfigOffset,
                &(resizeConfig->dvppResizeConfig), resizeConfigSize);
            if (memcpyRet != EOK) {
                ACL_LOG_INNER_ERROR("[Copy][ResizeConfig]copy resize config to args failed, result = %d.", memcpyRet);
                return ACL_ERROR_FAILURE;
            }
        }

        if (aclRunMode_ == ACL_HOST) {
            aclError cpyRet = CopyDvppPicDescAsync(inputDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][PicDesc]copy input pic desc to device failed, result = %d.", cpyRet);
                return cpyRet;
            }

            cpyRet = CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][PicDesc]copy output pic desc to device failed, result = %d.", cpyRet);
                return cpyRet;
            }
        }
        const char *kernelName = acl::dvpp::DVPP_KERNELNAME_CROP_AND_PASTE;
        if (resizeConfigSwitch) {
            kernelName = acl::dvpp::DVPP_KERNELNAME_CROP_RESIZE_PASTE;
        }
        aclError launchRet = LaunchDvppTask(channelDesc, args.get(), argsSize,
            kernelName, stream);
        if (launchRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Launch][Task]launch dvpp task failed, result = %d.", launchRet);
            return launchRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            aclError cpyRet = acl::dvpp::CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_DEVICE_TO_HOST, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][PicDesc]copy output pic desc from device failed, result = %d.", cpyRet);
                return cpyRet;
            }
        }
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::acldvppVpcCropResizePasteAsync(acldvppChannelDesc *channelDesc,
                                                            acldvppPicDesc *inputDesc,
                                                            acldvppPicDesc *outputDesc,
                                                            acldvppRoiConfig *cropArea,
                                                            acldvppRoiConfig *pasteArea,
                                                            acldvppResizeConfig *resizeConfig,
                                                            aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppVpcCropResizePasteAsync.");
        auto ret = DvppVpcCropResizePasteAsync(channelDesc, inputDesc, outputDesc, cropArea,
            pasteArea, resizeConfig, true, stream);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Exec][CropResizePaste]execute acldvppVpcCropResizePasteAsync "
                "failed, result = %d.", ret);
            return ret;
        }
        ACL_LOG_INFO("Launch vpc crop, resize and paste tasks success");
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::acldvppVpcCropAndPasteAsync(acldvppChannelDesc *channelDesc,
                                                         acldvppPicDesc *inputDesc,
                                                         acldvppPicDesc *outputDesc,
                                                         acldvppRoiConfig *cropArea,
                                                         acldvppRoiConfig *pasteArea,
                                                         aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppVpcCropAndPasteAsync.");
        auto ret = DvppVpcCropResizePasteAsync(channelDesc, inputDesc, outputDesc, cropArea,
            pasteArea, nullptr, false, stream);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Exec][CropAndPaste]execute acldvppVpcCropAndPasteAsync failed, result = %d.", ret);
            return ret;
        }
        ACL_LOG_INFO("Launch vpc crop and paste tasks success.");
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::DvppVpcBatchCropResizePasteAsync(acldvppChannelDesc *channelDesc,
                                                              acldvppBatchPicDesc *srcBatchPicDescs,
                                                              uint32_t *roiNums,
                                                              uint32_t size,
                                                              acldvppBatchPicDesc *dstBatchPicDescs,
                                                              acldvppRoiConfig *cropAreas[],
                                                              acldvppRoiConfig *pasteAreas[],
                                                              acldvppResizeConfig *resizeConfig,
                                                              const bool &resizeConfigSwitch,
                                                              aclrtStream stream)
    {
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(srcBatchPicDescs);
        ACL_REQUIRES_NOT_NULL(srcBatchPicDescs->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(dstBatchPicDescs);
        ACL_REQUIRES_NOT_NULL(dstBatchPicDescs->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(cropAreas);
        ACL_REQUIRES_NOT_NULL(pasteAreas);
        ACL_REQUIRES_NOT_NULL(roiNums);
        ACL_REQUIRES_POSITIVE(size);
        if (channelDesc->isNeedNotify) {
            ACL_REQUIRES_NOT_NULL(channelDesc->notify);
        }

        if (resizeConfigSwitch) {
            ACL_REQUIRES_NOT_NULL(resizeConfig);
        }

        // valid input param
        std::unique_ptr<uint16_t[]> devRoiNums(new (std::nothrow)uint16_t[size]);
        if (devRoiNums == nullptr) {
            ACL_LOG_INNER_ERROR("[Check][devRoiNums]create device roiNums failed, "
                "roiNums size = %u.", size);
            return ACL_ERROR_INVALID_PARAM;
        }
        uint32_t totalRoiNums = 0;
        aclError validParamRet = ValidateAndConvertVpcBatchParams(srcBatchPicDescs,
                                                                  dstBatchPicDescs,
                                                                  roiNums,
                                                                  size,
                                                                  devRoiNums.get(),
                                                                  totalRoiNums,
                                                                  BATCH_ROI_MAX_SIZE,
                                                                  resizeConfig);
        if (validParamRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Check][ValidParam]verify batch crop and paste param failed, "
                "result = %d.", validParamRet);
            return validParamRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            // copy batch picDesc data
            aclError cpyRet = CopyDvppBatchPicDescAsync(srcBatchPicDescs,
                                                        ACL_MEMCPY_HOST_TO_DEVICE,
                                                        size,
                                                        stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][BatchPicDesc]copy input batch pic desc to device failed, "
                    "result = %d.", cpyRet);
                return cpyRet;
            }

            cpyRet = CopyDvppBatchPicDescAsync(dstBatchPicDescs,
                                               ACL_MEMCPY_HOST_TO_DEVICE,
                                               totalRoiNums,
                                               stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][BatchPicDesc]copy output batch pic desc to device failed, "
                    "result = %d.", cpyRet);
                return cpyRet;
            }
        } else {
            // set data buffer for input batch pic desc
            aclError setDataRet = SetDataBufferForBatchPicDesc(srcBatchPicDescs, size);
            if (setDataRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Set][DataBuffer]set data buffer for src batch pic desc failed, "
                    "result = %d.", setDataRet);
                return setDataRet;
            }

            setDataRet = SetDataBufferForBatchPicDesc(dstBatchPicDescs, totalRoiNums);
            if (setDataRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Set][DataBuffer]set data buffer for dst batch pic desc failed, "
                    "result = %d.", setDataRet);
                return setDataRet;
            }
        }

        void *roiNumsAddr = static_cast<void *>(devRoiNums.get());
        VpcBatchParams batchParams = {srcBatchPicDescs,
                                      dstBatchPicDescs,
                                      cropAreas,
                                      pasteAreas,
                                      totalRoiNums,
                                      roiNumsAddr,
                                      size};

        // launch task
        aclError launchTaskRet = LaunchTaskForVpcBatchCropAndPaste(channelDesc, batchParams, resizeConfig, stream);
        if (launchTaskRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Launch][Task]launch task for vpc batch crop and paste failed, "
                "result = %d.", launchTaskRet);
            return launchTaskRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            // copy output data
            aclError cpyRet = acl::dvpp::CopyDvppBatchPicDescAsync(dstBatchPicDescs,
                                                                   ACL_MEMCPY_DEVICE_TO_HOST,
                                                                   totalRoiNums,
                                                                   stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][BatchPicDesc]copy output batch pic desc to host failed, "
                    "result = %d.", cpyRet);
                return cpyRet;
            }
        }
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::acldvppVpcBatchCropResizePasteAsync(acldvppChannelDesc *channelDesc,
                                                                 acldvppBatchPicDesc *srcBatchPicDescs,
                                                                 uint32_t *roiNums,
                                                                 uint32_t size,
                                                                 acldvppBatchPicDesc *dstBatchPicDescs,
                                                                 acldvppRoiConfig *cropAreas[],
                                                                 acldvppRoiConfig *pasteAreas[],
                                                                 acldvppResizeConfig *resizeConfig,
                                                                 aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppVpcBatchCropResizePasteAsync.");
        auto ret = DvppVpcBatchCropResizePasteAsync(channelDesc, srcBatchPicDescs, roiNums, size, dstBatchPicDescs,
            cropAreas, pasteAreas, resizeConfig, true, stream);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Exec][BatchCropResizePaste]execute acldvppVpcBatchCropResizePasteAsync failed, "
                "result = %d.", ret);
            return ret;
        }
        ACL_LOG_INFO("Launch vpc batch crop, resize and paste tasks success");
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::acldvppVpcBatchCropAndPasteAsync(acldvppChannelDesc *channelDesc,
                                                              acldvppBatchPicDesc *srcBatchPicDescs,
                                                              uint32_t *roiNums,
                                                              uint32_t size,
                                                              acldvppBatchPicDesc *dstBatchPicDescs,
                                                              acldvppRoiConfig *cropAreas[],
                                                              acldvppRoiConfig *pasteAreas[],
                                                              aclrtStream stream)
    {
        // validate parametes
        ACL_LOG_INFO("start to execute acldvppVpcBatchCropAndPasteAsync.");
        auto ret = DvppVpcBatchCropResizePasteAsync(channelDesc, srcBatchPicDescs, roiNums, size, dstBatchPicDescs,
            cropAreas, pasteAreas, nullptr, false, stream);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Exec][BatchCropAndPaste]execute acldvppVpcBatchCropAndPasteAsync failed, "
                "result = %d.", ret);
            return ret;
        }
        ACL_LOG_INFO("Launch vpc batch crop and paste tasks success");
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::acldvppVpcConvertColorAsync(acldvppChannelDesc *channelDesc,
                                                         acldvppPicDesc *inputDesc,
                                                         acldvppPicDesc *outputDesc,
                                                         aclrtStream stream)
    {
        ACL_LOG_ERROR("[Unsupport][Version]This version can not support vpc convert color api. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"vpc convert color api", "Please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError ImageProcessor::acldvppVpcPyrDownAsync(acldvppChannelDesc *channelDesc,
                                                    acldvppPicDesc *inputDesc,
                                                    acldvppPicDesc *outputDesc,
                                                    void* reserve,
                                                    aclrtStream stream)
    {
        ACL_LOG_ERROR("[Unsupport][Version]This version can not support vpc pyramid down api. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"vpc pyramid down api", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError ImageProcessor::acldvppVpcEqualizeHistAsync(const acldvppChannelDesc *channelDesc,
                                                         const acldvppPicDesc *inputDesc,
                                                         acldvppPicDesc *outputDesc,
                                                         const acldvppLutMap *lutMap,
                                                         aclrtStream stream)
    {
        ACL_LOG_ERROR("[Unsupport][Version]This version can not support vpc equalize "
            "hist api. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"vpc equalize hist api", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError ImageProcessor::acldvppVpcMakeBorderAsync(const acldvppChannelDesc *channelDesc,
                                                       const acldvppPicDesc *inputDesc,
                                                       acldvppPicDesc *outputDesc,
                                                       const acldvppBorderConfig *borderConfig,
                                                       aclrtStream stream)
    {
        ACL_LOG_ERROR("[Unsupport][Version]This version can not support "
            "vpc make border api. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"vpc make border api", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError ImageProcessor::acldvppVpcBatchCropResizeMakeBorderAsync(acldvppChannelDesc *channelDesc,
                                                                      acldvppBatchPicDesc *srcBatchPicDescs,
                                                                      uint32_t *roiNums,
                                                                      uint32_t size,
                                                                      acldvppBatchPicDesc *dstBatchPicDescs,
                                                                      acldvppRoiConfig *cropAreas[],
                                                                      acldvppBorderConfig *borderCfgs[],
                                                                      acldvppResizeConfig *resizeConfig,
                                                                      aclrtStream stream)
    {
        ACL_LOG_ERROR("[Unsupport][Version]This version can not support "
            "vpc batch crop, resize and make border api. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"vpc make border api", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError ImageProcessor::acldvppVpcCalcHistAsync(acldvppChannelDesc *channelDesc,
                                                     acldvppPicDesc *srcPicDesc,
                                                     acldvppHist *hist,
                                                     void *reserve,
                                                     aclrtStream stream)
    {
        ACL_LOG_ERROR("[Unsupport][Version]This version can not support vpc "
            "calculate hist api. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"vpc calculate hist api", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    acldvppHist* ImageProcessor::acldvppCreateHist()
    {
        ACL_LOG_ERROR("[Unsupport][Version]This version can not support "
            "vpc hist api. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"vpc hist api", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return nullptr;
    }

    aclError ImageProcessor::acldvppDestroyHist(acldvppHist *hist)
    {
        ACL_LOG_ERROR("[Unsupport][Version]This version can not support "
            "vpc hist api. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"vpc hist api", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError ImageProcessor::acldvppClearHist(acldvppHist *hist)
    {
        ACL_LOG_ERROR("[Unsupport][Version]This version can not support "
            "vpc hist api. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"vpc hist api", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    uint32_t ImageProcessor::acldvppGetHistDims(acldvppHist *hist)
    {
        ACL_LOG_ERROR("[Unsupport][Version]This version can not support "
            "vpc hist api. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"vpc hist api", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return 0;
    }

    aclError ImageProcessor::acldvppGetHistData(acldvppHist *hist, uint32_t dim, uint32_t **data, uint16_t *len)
    {
        ACL_LOG_ERROR("[Unsupport][Version]This version can not support "
            "vpc hist api. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"vpc hist api", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    uint32_t ImageProcessor::acldvppGetHistRetCode(acldvppHist* hist)
    {
        ACL_LOG_ERROR("[Unsupport][Version]This version can not support "
            "vpc hist api. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"vpc hist api", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return 0;
    }

    aclError ImageProcessor::acldvppJpegDecodeAsync(acldvppChannelDesc *channelDesc,
                                                    const void *data,
                                                    uint32_t size,
                                                    acldvppPicDesc *outputDesc,
                                                    aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppJpegDecodeAsync.");
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(data);
        ACL_REQUIRES_NOT_NULL(outputDesc);
        ACL_REQUIRES_NOT_NULL(outputDesc->dataBuffer.data);
        ACL_REQUIRES_POSITIVE(size);
        if (channelDesc->isNeedNotify) {
            ACL_REQUIRES_NOT_NULL(channelDesc->notify);
        }

        aclError validOutputRet = ValidateJpegOutputFormat(
                static_cast<acldvppPixelFormat>(outputDesc->dvppPicDesc.format));
        if (validOutputRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Verify][JpegOutputFormat]output acldvppPicDesc format "
                "verify failed, result = %d, format = %u.", validOutputRet, outputDesc->dvppPicDesc.format);
            return validOutputRet;
        }

        // jpegd have 3 input
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
                ACL_LOG_INNER_ERROR("[Copy][PicDesc]copy input pic desc to device failed, result = %d.", cpyRet);
                return cpyRet;
            }
        }

        aclError launchRet = LaunchDvppTask(channelDesc, args.get(), argsSize,
            acl::dvpp::DVPP_KERNELNAME_DECODE_JPEG, stream);
        if (launchRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Launch][Task]launch dvpp task failed, result = %d.", launchRet);
            return launchRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            aclError cpyRet = CopyDvppPicDescAsync(outputDesc, ACL_MEMCPY_DEVICE_TO_HOST, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][PicDesc]copy output pic desc from device failed, result = %d.", cpyRet);
                return cpyRet;
            }
        }

        ACL_LOG_INFO("Launch jpeg decode tasks success.");
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::acldvppJpegEncodeAsync(acldvppChannelDesc *channelDesc,
                                                    acldvppPicDesc *inputDesc,
                                                    const void *data,
                                                    uint32_t *size,
                                                    acldvppJpegeConfig *config,
                                                    aclrtStream stream)
    {
        ACL_LOG_INFO("start to execute acldvppJpegEncodeAsync.");
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(channelDesc->shareBuffer.data);
        ACL_REQUIRES_NOT_NULL(inputDesc);
        ACL_REQUIRES_NOT_NULL(inputDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(data);
        ACL_REQUIRES_NOT_NULL(size);
        ACL_REQUIRES_NOT_NULL(config);
        ACL_REQUIRES_POSITIVE(*size);
        if (channelDesc->isNeedNotify) {
            ACL_REQUIRES_NOT_NULL(channelDesc->notify);
        }

        aclError validOutputRet = ValidateJpegInputFormat(
            static_cast<acldvppPixelFormat>(inputDesc->dvppPicDesc.format));
        if (validOutputRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Verify][PicDesc]input acldvppPicDesc format verify failed, result = %d, format = %u.",
                validOutputRet, inputDesc->dvppPicDesc.format);
            return validOutputRet;
        }

        // jpeg encode have 4 input
        constexpr int32_t ioAddrNum = 4;
        uint32_t dvppJpegeConfigSize = CalDevDvppStructRealUsedSize(&config->dvppJpegeConfig);
        uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t) +
            dvppJpegeConfigSize + sizeof(uint32_t);
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        int i = 0;
        ioAddr[i++] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        ioAddr[i++] = reinterpret_cast<uintptr_t>(inputDesc->dataBuffer.data);
        ioAddr[i++] = reinterpret_cast<uintptr_t>(data);
        // use share buffer transfer size
        ioAddr[i] = reinterpret_cast<uintptr_t>(channelDesc->shareBuffer.data);

        // copy jpeg encode config size to device by args.
        constexpr uint32_t configOffset = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        auto memcpyRet = memcpy_s(args.get() + configOffset, argsSize - configOffset,
                                  &(config->dvppJpegeConfig), dvppJpegeConfigSize);
        if (memcpyRet != EOK) {
            ACL_LOG_INNER_ERROR("[Copy][JpegeConfig]copy jpege config to args failed, result = %d.", memcpyRet);
            return ACL_ERROR_FAILURE;
        }

        // copy output buffer size to device by args.
        uint32_t bufferSizeOffset = configOffset + dvppJpegeConfigSize;
        memcpyRet = memcpy_s(args.get() + bufferSizeOffset, argsSize - bufferSizeOffset, size,
            sizeof(uint32_t));
        if (memcpyRet != EOK) {
            ACL_LOG_INNER_ERROR("[Copy][Buffer]copy output buffer size to args failed, result = %d.", memcpyRet);
            return ACL_ERROR_FAILURE;
        }

        if (aclRunMode_ == ACL_HOST) {
            aclError cpyRet = CopyDvppPicDescAsync(inputDesc, ACL_MEMCPY_HOST_TO_DEVICE, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][PicDesc]copy input pic desc to device failed, result = %d.", cpyRet);
                return cpyRet;
            }
        }

        aclError launchRet = LaunchDvppTask(channelDesc, args.get(), argsSize,
            acl::dvpp::DVPP_KERNELNAME_ENCODE_JPEG, stream);
        if (launchRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Launch][Task]launch dvpp task failed, result = %d.", launchRet);
            return launchRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            aclError cpyRet = CopyDvppPicDescAsync(inputDesc, ACL_MEMCPY_DEVICE_TO_HOST, stream);
            if (cpyRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][Config]copy jpege config from device failed, result = %d.", cpyRet);
                return cpyRet;
            }

            rtError_t rtRet = rtMemcpyAsync(size, sizeof(uint32_t),
                                            channelDesc->shareBuffer.data, sizeof(uint32_t),
                                            RT_MEMCPY_DEVICE_TO_HOST, stream);
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_INNER_ERROR("[Copy][JpegeSize]copy jpege size back to host failed, "
                    "runtime result = %d.", rtRet);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }
        } else {
            // copy size to host
            auto rtRet = rtMemcpyAsync(size, sizeof(uint32_t),
                                       channelDesc->shareBuffer.data, sizeof(uint32_t),
                                       RT_MEMCPY_DEVICE_TO_HOST_EX, stream);
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_INNER_ERROR("[Copy][JpegeSize]copy jpege size to host failed, runtime result = %d.", rtRet);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }
        }

        ACL_LOG_INFO("Launch jpeg encode tasks success");
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::acldvppPngDecodeAsync(acldvppChannelDesc *channelDesc,
                                                   const void *data,
                                                   uint32_t size,
                                                   acldvppPicDesc *outputDesc,
                                                   aclrtStream stream)
    {
        ACL_LOG_INNER_ERROR("[Unsupport][Decode]png decode is not supported in this version. Please check.");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError ImageProcessor::acldvppJpegGetImageInfo(const void *data,
                                                     uint32_t size,
                                                     uint32_t *width,
                                                     uint32_t *height,
                                                     int32_t *components,
                                                     acldvppJpegFormat *format)
    {
        ACL_REQUIRES_NOT_NULL(data);
        bool noNeedGetImageInfo = (width == nullptr) && (height == nullptr) &&
                                  (components == nullptr) && (format == nullptr);
        if (noNeedGetImageInfo) {
            ACL_LOG_INFO("no need to get jpeg info");
            return ACL_SUCCESS;
        }
        struct jpeg_decompress_struct libjpegHandler;
        struct jpeg_error_mgr libjpegErrorMsg;
        libjpegHandler.err = jpeg_std_error(&libjpegErrorMsg);
        libjpegErrorMsg.error_exit = AcldvppLibjpegErrorExit;
        jpeg_create_decompress(&libjpegHandler);
        jpeg_save_markers(&libjpegHandler, EXIF_MARKER, 0xffff);
        try {
            jpeg_mem_src(&libjpegHandler, reinterpret_cast<const unsigned char *>(data), size);
            jpeg_read_header(&libjpegHandler, TRUE);
        } catch (...) {
            jpeg_destroy_decompress(&libjpegHandler);
            return ACL_ERROR_FAILURE;
        }
        bool needOrientation = JudgeNeedOrientation(&libjpegHandler);
        if (width != nullptr) {
            *width = needOrientation ? static_cast<uint32_t>(libjpegHandler.image_height) :
                static_cast<uint32_t>(libjpegHandler.image_width);
        }
        if (height != nullptr) {
            *height = needOrientation ? static_cast<uint32_t>(libjpegHandler.image_width) :
                static_cast<uint32_t>(libjpegHandler.image_height);
        }
        if (components != nullptr) {
            *components = static_cast<int32_t>(libjpegHandler.num_components);
        }
        if (format != nullptr) {
            *format = ACL_JPEG_CSS_UNKNOWN;
            if (!GetImageFormat(libjpegHandler, format)) {
                ACL_LOG_WARN("Get image format failed, maybe unsupported.");
            }
        }
        // here jpeg_destroy_decompress does not throw exception according souce code in jcomapi.c
        jpeg_destroy_decompress(&libjpegHandler);
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::acldvppJpegPredictEncSize(const acldvppPicDesc *inputDesc,
                                                       const acldvppJpegeConfig *config,
                                                       uint32_t *size)
    {
        ACL_REQUIRES_NOT_NULL(inputDesc);
        ACL_REQUIRES_NOT_NULL(config);
        ACL_REQUIRES_NOT_NULL(size);
        const uint32_t &width = inputDesc->dvppPicDesc.width;
        const uint32_t &height = inputDesc->dvppPicDesc.height;
        const uint32_t &widthStride = inputDesc->dvppPicDesc.widthStride;
        const uint32_t &heightStride = inputDesc->dvppPicDesc.heightStride;

        // param according dvpp capability
        const uint32_t jpegeMaxHeight     = 8192;
        const uint32_t jpegeMaxWidth      = 8192;
        const uint32_t jpegeMinHeight     = 32;
        const uint32_t jpegeMinWidth      = 32;

        ACL_CHECK_WITH_MESSAGE_AND_RETURN((width >= jpegeMinWidth) && (width <= jpegeMaxWidth),
            ACL_ERROR_INVALID_PARAM, "width = %u invalid, it should be between in [%u, %u].", width,
            jpegeMinWidth, jpegeMaxWidth);
        ACL_CHECK_WITH_MESSAGE_AND_RETURN((height >= jpegeMinHeight) && (height <= jpegeMaxHeight),
            ACL_ERROR_INVALID_PARAM, "height = %u invalid, it should be between in [%u, %u].", height,
            jpegeMinHeight, jpegeMaxHeight);

        acldvppPixelFormat format = static_cast<acldvppPixelFormat>(inputDesc->dvppPicDesc.format);
        ACL_CHECK_WITH_MESSAGE_AND_RETURN(ValidateJpegInputFormat(format) == ACL_SUCCESS,
            ACL_ERROR_FORMAT_NOT_MATCH, "input acldvppPicDesc format verify failed, format = %u.",
            inputDesc->dvppPicDesc.format);

        const uint32_t alignSize = 16;
        if (IsPackedFormat(format)) {
            // widthStride should be twice as width and aligned to 16
            ACL_CHECK_WITH_MESSAGE_AND_RETURN(((width * 2 + (alignSize - 1)) & (~(alignSize - 1))) == widthStride,
                ACL_ERROR_INVALID_PARAM,
                "stride width = %u invalid, it should be twice as width = %u and aligned to 16.", widthStride, width);
        } else {
            // widthStride align to 16 or align to multiple of 16.
            bool widthCondition = (widthStride >= width) && (widthStride % alignSize == 0);
            ACL_CHECK_WITH_MESSAGE_AND_RETURN(widthCondition, ACL_ERROR_INVALID_PARAM,
                "stride width = %u invalid, it should be greater than or equal width = %u and aligned to 16.",
                widthStride, width);
        }

        bool heightCondition = (heightStride == height) ||
                                ((heightStride > height) &&
                                (heightStride % alignSize == 0)); // heightStride must be aligned to 16
        ACL_CHECK_WITH_MESSAGE_AND_RETURN(heightCondition, ACL_ERROR_INVALID_PARAM,
            "stride height = %u invalid, it should be equal to height = %u or greater than height and aligned to 16.",
            heightStride, height);

        ACL_CHECK_WITH_MESSAGE_AND_RETURN(CalcImageSizeKernel(widthStride, heightStride, format, size),
            ACL_ERROR_FORMAT_NOT_MATCH, "predict jpeg encode size failed.");

        const uint32_t jpegeHeaderSize = 640;
        *size += jpegeHeaderSize;
        const uint32_t startAlignBytes = 128;
        *size += startAlignBytes;
        // align to 2M bytes
        const uint32_t memAlignSize = 2097152;  // 2M
        *size = (*size + memAlignSize - 1) & (~(memAlignSize - 1));

        return ACL_SUCCESS;
    }

    aclError ImageProcessor::acldvppJpegPredictDecSize(const void *data,
                                                       uint32_t dataSize,
                                                       acldvppPixelFormat outputPixelFormat,
                                                       uint32_t *decSize)
    {
        ACL_REQUIRES_NOT_NULL(data);
        ACL_REQUIRES_NOT_NULL(decSize);
        ACL_CHECK_WITH_MESSAGE_AND_RETURN(ValidateJpegOutputFormat(outputPixelFormat) == ACL_SUCCESS,
            ACL_ERROR_FORMAT_NOT_MATCH, "output acldvppPicDesc format verify failed, format = %d.",
            static_cast<int32_t>(outputPixelFormat));
        aclError ret = ACL_SUCCESS;
        struct jpeg_decompress_struct libjpegHandler;
        struct jpeg_error_mgr libjpegErrorMsg;
        libjpegHandler.err = jpeg_std_error(&libjpegErrorMsg);
        libjpegErrorMsg.error_exit = AcldvppLibjpegErrorExit;
        jpeg_create_decompress(&libjpegHandler);
        jpeg_save_markers(&libjpegHandler, EXIF_MARKER, 0xffff);
        try {
            jpeg_mem_src(&libjpegHandler, reinterpret_cast<const unsigned char *>(data), dataSize);
            jpeg_read_header(&libjpegHandler, TRUE);
            bool needOrientation = JudgeNeedOrientation(&libjpegHandler);
            uint32_t width = needOrientation ? static_cast<uint32_t>(libjpegHandler.image_height) :
                static_cast<uint32_t>(libjpegHandler.image_width);
            uint32_t height = needOrientation ? static_cast<uint32_t>(libjpegHandler.image_width) :
                static_cast<uint32_t>(libjpegHandler.image_height);
            ACL_LOG_INFO("width = %u, height = %u", width, height);
            // param according dvpp capability
            const uint32_t jpegdMaxHeight     = 8192;
            const uint32_t jpegdMaxWidth      = 8192;
            const uint32_t jpegdMinHeight     = 32;
            const uint32_t jpegdMinWidth      = 32;
            ACL_CHECK_WITH_MESSAGE_AND_RETURN((width >= jpegdMinWidth) && (width <= jpegdMaxWidth),
                ACL_ERROR_INVALID_PARAM, "stride width = %u invalid, it should be between in [%u, %u].", width,
                jpegdMinWidth, jpegdMaxWidth);
            ACL_CHECK_WITH_MESSAGE_AND_RETURN((height >= jpegdMinHeight) && (height <= jpegdMaxHeight),
                ACL_ERROR_INVALID_PARAM, "stride height = %u invalid, it should be between in [%u, %u].", height,
                jpegdMinHeight, jpegdMaxHeight);

            switch (outputPixelFormat) {
                case acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_420:
                case acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_420:
                    width = (width + 127) & (~127U);  // align to 128
                    height = (height + 15) & (~15U);  // align to 16
                    (void)CalcImageSizeKernel(width, height, outputPixelFormat, decSize);
                    break;
                default:
                    // output pixel format as same as input pixel format
                    ret = CalcImageSize(libjpegHandler, needOrientation, outputPixelFormat, decSize);
                    if (ret != ACL_SUCCESS) {
                        ACL_LOG_INNER_ERROR("[Predict][Decode]predict decode size failed, output pixel format = %d",
                            static_cast<int32_t>(outputPixelFormat));
                    }
                    break;
            }
        } catch (...) {
            jpeg_destroy_decompress(&libjpegHandler);
            return ACL_ERROR_FAILURE;
        }
        // here jpeg_destroy_decompress does not throw exception according souce code in jcomapi.c
        jpeg_destroy_decompress(&libjpegHandler);

        return ret;
    }

    acldvppChannelDesc *ImageProcessor::acldvppCreateChannelDesc()
    {
        ACL_LOG_INFO("start to execute acldvppCreateChannelDesc");

        acldvppChannelDesc *aclDvppChannelDesc = nullptr;
        switch (aclRunMode_) {
            case ACL_HOST: {
                aclDvppChannelDesc = CreateChannelDescOnHost();
                break;
            }
            case ACL_DEVICE: {
                aclDvppChannelDesc = CreateChannelDescOnDevice();
                break;
            }
            default: {
                ACL_LOG_INNER_ERROR("[Unknown][Mode]unknown acl run mode %d.", aclRunMode_);
                return nullptr;
            }
        }

        // check result
        if (aclDvppChannelDesc == nullptr) {
            ACL_LOG_INNER_ERROR("[Check][ChannelDesc]create channel desc failed. Please check.");
            return nullptr;
        }

        static std::atomic<std::uint64_t> nextChannelIndex(1);
        aclDvppChannelDesc->channelIndex = nextChannelIndex++;
        // channelMode is not used in v100
        aclDvppChannelDesc->dvppDesc.channelMode =
            aicpu::dvpp::DVPP_CHNMODE_VPC | aicpu::dvpp::DVPP_CHNMODE_JPEGD | aicpu::dvpp::DVPP_CHNMODE_JPEGE;

        ACL_LOG_INFO("create DvppChannelDesc : channeldesc addr = %p, cmd addr = %p, share addr = %p,"
            "support type = %d", aclDvppChannelDesc->dataBuffer.data, aclDvppChannelDesc->cmdListBuffer.data,
            aclDvppChannelDesc->shareBuffer.data, aclDvppChannelDesc->dvppDesc.channelMode);
        return aclDvppChannelDesc;
    }

    aclError ImageProcessor::acldvppDestroyChannelDesc(acldvppChannelDesc *channelDesc)
    {
        if (channelDesc == nullptr) {
            return ACL_SUCCESS;
        }

        ACL_LOG_INFO("destroy DvppChannelDesc info: channelIndex = %lu, dataLen = %u, cmdListLen = %u.",
            channelDesc->channelIndex, channelDesc->dataBuffer.length, channelDesc->cmdListBuffer.length);
        switch (aclRunMode_) {
            case ACL_HOST: {
                FreeDeviceBuffer(channelDesc->dataBuffer);
                FreeDvppDeviceBuffer(channelDesc->cmdListBuffer);
                // shareBuffer is freed with cmdListBuffer together.
                ACL_ALIGN_FREE(channelDesc);
                break;
            }
            case ACL_DEVICE: {
                FreeDvppDeviceBuffer(channelDesc->cmdListBuffer);
                FreeDeviceAddr(channelDesc->shareBuffer.data);
                FreeDeviceAddr(static_cast<void *>(channelDesc));
                break;
            }
            default: {
                ACL_LOG_INNER_ERROR("[Unknown][Mode]unkown acl run mode %d.", aclRunMode_);
                return ACL_ERROR_INTERNAL_ERROR;
            }
        }
        ACL_LOG_DEBUG("destroy channelDesc successful");
        return ACL_SUCCESS;
    }

    acldvppPicDesc *ImageProcessor::acldvppCreatePicDesc()
    {
        acldvppPicDesc *aclPicDesc = nullptr;
        switch (aclRunMode_) {
            case ACL_HOST: {
                aclPicDesc = CreatePicDescOnHost();
                break;
            }
            case ACL_DEVICE: {
                aclPicDesc = CreatePicDescOnDevice();
                break;
            }
            default: {
                ACL_LOG_INNER_ERROR("[Unknown][Mode]unkown acl run mode %d.", aclRunMode_);
                return nullptr;
            }
        }
        return aclPicDesc;
    }

    aclError ImageProcessor::acldvppDestroyPicDesc(acldvppPicDesc *picDesc)
    {
        if (picDesc == nullptr) {
            return ACL_SUCCESS;
        }

        switch (aclRunMode_) {
            case ACL_HOST: {
                FreeDeviceBuffer(picDesc->dataBuffer);
                ACL_ALIGN_FREE(picDesc);
                break;
            }
            case ACL_DEVICE: {
                FreeDeviceAddr(static_cast<void *>(picDesc));
                break;
            }
            default: {
                ACL_LOG_INNER_ERROR("[Unknown][Mode]unkown acl run mode %d.", aclRunMode_);
                return ACL_ERROR_INTERNAL_ERROR;
            }
        }
        return ACL_SUCCESS;
    }

    acldvppRoiConfig *ImageProcessor::acldvppCreateRoiConfig(uint32_t left,
                                                             uint32_t right,
                                                             uint32_t top,
                                                             uint32_t bottom)
    {
        ACL_LOG_INFO("start to execute acldvppCreateRoiConfig, left[%u], right[%u], top[%u], bottom[%u]",
            left, right, top, bottom);
        acldvppRoiConfig *aclRoiConfig = nullptr;
        // alloc memory
        uint32_t aclDvppRoiConfigSize = CalAclDvppStructSize(aclRoiConfig);
        ACL_REQUIRES_POSITIVE_RET_NULL(aclDvppRoiConfigSize);
        void *addr = malloc(aclDvppRoiConfigSize);
        if (addr == nullptr) {
            ACL_LOG_INNER_ERROR("[Malloc][Mem]malloc memory failed. size is %u.", aclDvppRoiConfigSize);
            return nullptr;
        }

        // create acldvppRoiConfig in memory
        aclRoiConfig = new (addr)acldvppRoiConfig();
        if (aclRoiConfig == nullptr) {
            ACL_LOG_INNER_ERROR("[Create][DvppRoiConfig]create aclDvppRoiConfig with function new failed");
            ACL_FREE(addr);
            return nullptr;
        }

        // set value
        aclRoiConfig->dvppRoiConfig.leftOffset = left;
        aclRoiConfig->dvppRoiConfig.rightOffset = right;
        aclRoiConfig->dvppRoiConfig.upOffset = top;
        aclRoiConfig->dvppRoiConfig.downOffset = bottom;
        return aclRoiConfig;
    }

    aclError ImageProcessor::acldvppDestroyRoiConfig(acldvppRoiConfig *roiConfig)
    {
        ACL_FREE(roiConfig);
        return ACL_SUCCESS;
    }

    acldvppResizeConfig *ImageProcessor::acldvppCreateResizeConfig()
    {
        ACL_LOG_INFO("start to execute acldvppCreateResizeConfig");
        acldvppResizeConfig *aclResizeConfig = nullptr;
        // malloc memory
        uint32_t aclResizeConfigSize = CalAclDvppStructSize(aclResizeConfig);
        ACL_REQUIRES_POSITIVE_RET_NULL(aclResizeConfigSize);
        void *addr = malloc(aclResizeConfigSize);
        if (addr == nullptr) {
            ACL_LOG_INNER_ERROR("[Malloc][Mem]malloc memory failed. size is %u.", aclResizeConfigSize);
            return nullptr;
        }

        // create acldvppResizeConfig in memory
        aclResizeConfig = new (addr)acldvppResizeConfig();
        if (aclResizeConfig == nullptr) {
            ACL_LOG_INNER_ERROR("[Create][ResizeConfig]create acldvppResizeConfig with function new failed");
            ACL_FREE(addr);
            return nullptr;
        }
        return aclResizeConfig;
    }

    aclError ImageProcessor::acldvppDestroyResizeConfig(acldvppResizeConfig *resizeConfig)
    {
        ACL_FREE(resizeConfig);
        return ACL_SUCCESS;
    }

    acldvppJpegeConfig *ImageProcessor::acldvppCreateJpegeConfig()
    {
        acldvppJpegeConfig *aclJpegeConfig = nullptr;
        // malloc memory
        uint32_t aclJpegeConfigSize = CalAclDvppStructSize(aclJpegeConfig);
        ACL_REQUIRES_POSITIVE_RET_NULL(aclJpegeConfigSize);
        void *addr = malloc(aclJpegeConfigSize);
        if (addr == nullptr) {
            ACL_LOG_INNER_ERROR("[Malloc][Mem]malloc memory failed. size is %u.", aclJpegeConfigSize);
            return nullptr;
        }

        // create acldvppResizeConfig in memory
        aclJpegeConfig = new (addr)acldvppJpegeConfig();
        if (aclJpegeConfig == nullptr) {
            ACL_LOG_INNER_ERROR("[Create][JpegeConfig]create acldvppJpegeConfig with function new failed");
            ACL_FREE(addr);
            return nullptr;
        }
        return aclJpegeConfig;
    }

    aclError ImageProcessor::acldvppDestroyJpegeConfig(acldvppJpegeConfig *jpegeConfig)
    {
        ACL_FREE(jpegeConfig);
        return ACL_SUCCESS;
    }

    acldvppPicDesc *ImageProcessor::CreatePicDescOnHost()
    {
        acldvppPicDesc *aclPicDesc = nullptr;
        // alloc host memory
        uint32_t aclPicDescSize = CalAclDvppStructSize(aclPicDesc);
        ACL_REQUIRES_POSITIVE_RET_NULL(aclPicDescSize);
        size_t pageSize = mmGetPageSize();
        void *hostAddr = mmAlignMalloc(aclPicDescSize, pageSize);
        if (hostAddr == nullptr) {
            ACL_LOG_INNER_ERROR("[Malloc][Mem]malloc memory failed. size is %u.", aclPicDescSize);
            return nullptr;
        }

        // create acldvppPicDesc in host addr
        aclPicDesc = new (hostAddr)acldvppPicDesc;
        RETURN_NULL_WITH_ALIGN_FREE(aclPicDesc, hostAddr);

        // malloc device memory for dvppPicDesc
        void *devPtr = nullptr;
        size_t size = CalAclDvppStructSize(&aclPicDesc->dvppPicDesc);
        uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
        rtError_t ret = rtMalloc(&devPtr, size, flags);
        if (ret != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Malloc][Mem]malloc device memory for acl dvpp pic desc failed, "
                "runtime result = %d", ret);
            ACL_ALIGN_FREE(hostAddr);
            return nullptr;
        }

        // set data buffer
        aclPicDesc->dataBuffer.data = devPtr;
        aclPicDesc->dataBuffer.length = size;

        return aclPicDesc;
    }

    acldvppPicDesc *ImageProcessor::CreatePicDescOnDevice()
    {
        acldvppPicDesc *aclPicDesc = nullptr;
        // alloc device memory
        void *devAddr = nullptr;
        uint32_t aclPicDescSize = CalAclDvppStructSize(aclPicDesc);
        uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
        rtError_t rtErr = rtMalloc(&devAddr, aclPicDescSize, flags);
        if (rtErr != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Malloc][Mem]alloc device memory failed, size = %zu, "
                "runtime result = %d", aclPicDescSize, rtErr);
            return nullptr;
        }

        // create acldvppPicDesc in device addr
        aclPicDesc = new (devAddr)acldvppPicDesc;
        if (aclPicDesc == nullptr) {
            ACL_LOG_INNER_ERROR("[Create][PicDesc]create acldvppPicDesc with function new failed");
            (void) rtFree(devAddr);
            devAddr = nullptr;
            return nullptr;
        }

        // set data buffer
        auto offset = offsetof(acldvppPicDesc, dvppPicDesc);
        aclPicDesc->dataBuffer.data = reinterpret_cast<aicpu::dvpp::DvppPicDesc *>(
            reinterpret_cast<uintptr_t>(devAddr) + offset);
        aclPicDesc->dataBuffer.length = CalAclDvppStructSize(&aclPicDesc->dvppPicDesc);

        return aclPicDesc;
    }

    size_t ImageProcessor::GetCmdlistBuffSize()
    {
        return acl::dvpp::DVPP_CMDLIST_BUFFER_SIZE_V100;
    }

    acldvppChannelDesc *ImageProcessor::CreateChannelDescOnHost()
    {
        acldvppChannelDesc *aclDvppChannelDesc = nullptr;
        // alloc host memory
        uint32_t aclDvppChannelDescSize = CalAclDvppStructSize(aclDvppChannelDesc);
        ACL_REQUIRES_POSITIVE_RET_NULL(aclDvppChannelDescSize);
        size_t pageSize = mmGetPageSize();
        void *hostAddr = mmAlignMalloc(aclDvppChannelDescSize, pageSize);
        if (hostAddr == nullptr) {
            ACL_LOG_INNER_ERROR("[Malloc][Mem]malloc memory failed. size is %u.", aclDvppChannelDescSize);
            return nullptr;
        }
        // create acldvppChannelDesc in host addr
        aclDvppChannelDesc = new (hostAddr)acldvppChannelDesc;
        RETURN_NULL_WITH_ALIGN_FREE(aclDvppChannelDesc, hostAddr);

        // alloc device mem for dataBuffer and share buffer
        size_t dataBufferSize = CalAclDvppStructSize(&aclDvppChannelDesc->dvppDesc);
        size_t totalSize =  dataBufferSize + acl::dvpp::DVPP_CHANNEL_SHARE_BUFFER_SIZE;
        uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
        rtError_t rtErr = rtMalloc(&aclDvppChannelDesc->dataBuffer.data, totalSize, flags);
        if (rtErr != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Malloc][Mem]alloc device memory failed, size = %zu, "
                "runtime result = %d", totalSize, rtErr);
            ACL_ALIGN_FREE(hostAddr);
            return nullptr;
        }
        aclDvppChannelDesc->dataBuffer.length = dataBufferSize;

        // set share buffer
        aclDvppChannelDesc->shareBuffer.data =
            static_cast<char *>(aclDvppChannelDesc->dataBuffer.data) + dataBufferSize;
        aclDvppChannelDesc->shareBuffer.length = acl::dvpp::DVPP_CHANNEL_SHARE_BUFFER_SIZE;

        // alloc device mem for cmdListBuffer
        void *tmpData = nullptr;
        size_t tmpDataSize = GetCmdlistBuffSize();
        rtErr = rtDvppMalloc(&tmpData, tmpDataSize);
        if (rtErr != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Alloc][Mem]alloc device memory for channel tmp buffer failed, "
                "size = %zu, runtime result = %d", tmpDataSize, rtErr);
            (void) rtFree(aclDvppChannelDesc->dataBuffer.data);
            aclDvppChannelDesc->dataBuffer.data = nullptr;
            ACL_ALIGN_FREE(hostAddr);
            return nullptr;
        }
        aclDvppChannelDesc->cmdListBuffer.data = tmpData;
        aclDvppChannelDesc->cmdListBuffer.length = tmpDataSize;

        return aclDvppChannelDesc;
    }

    acldvppChannelDesc *ImageProcessor::CreateChannelDescOnDevice()
    {
        acldvppChannelDesc *aclDvppChannelDesc = nullptr;
        // alloc memory
        void *devAddr = nullptr;
        uint32_t aclDvppChannelDescSize = CalAclDvppStructSize(aclDvppChannelDesc);
        uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
        rtError_t rtErr = rtMalloc(&devAddr, aclDvppChannelDescSize, flags);
        if (rtErr != RT_ERROR_NONE) {
            ACL_LOG_INNER_ERROR("[Alloc][Mem]alloc device memory failed, size = %zu, runtime result = %d",
                aclDvppChannelDescSize, rtErr);
            return nullptr;
        }

        // create acldvppChannelDesc in device addr
        aclDvppChannelDesc = new (devAddr)acldvppChannelDesc;
        if (aclDvppChannelDesc == nullptr) {
            ACL_LOG_INNER_ERROR("[Create][ChannelDesc]create acldvppChannelDesc with function new failed");
            (void) rtFree(devAddr);
            devAddr = nullptr;
            return nullptr;
        }

        // set data buffer
        auto offset = offsetof(acldvppChannelDesc, dvppDesc);
        aclDvppChannelDesc->dataBuffer.data = reinterpret_cast<aicpu::dvpp::DvppChannelDesc *>(
            reinterpret_cast<uintptr_t>(devAddr) + offset);
        aclDvppChannelDesc->dataBuffer.length = CalAclDvppStructSize(&aclDvppChannelDesc->dvppDesc);

        // malloc device addr for share buffer
        uint32_t shareBufferSize = acl::dvpp::DVPP_CHANNEL_SHARE_BUFFER_SIZE;
        rtErr = rtMalloc(&aclDvppChannelDesc->shareBuffer.data, shareBufferSize, flags);
        if (rtErr != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Malloc][Mem]alloc device memory failed, size = %zu, runtime result = %d",
                shareBufferSize, rtErr);
            (void) rtFree(devAddr);
            devAddr = nullptr;
            return nullptr;
        }
        aclDvppChannelDesc->shareBuffer.length = shareBufferSize;

        // alloc device mem for cmdListBuffer
        void *tmpData = nullptr;
        size_t tmpDataSize = GetCmdlistBuffSize();
        rtErr = rtDvppMalloc(&tmpData, tmpDataSize); // 2M align
        if (rtErr != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Alloc][Mem]alloc device memory for channel tmp buffer failed, "
                "size = %zu, runtime result = %d", tmpDataSize, rtErr);
            (void) rtFree(aclDvppChannelDesc->shareBuffer.data);
            (void) rtFree(devAddr);
            aclDvppChannelDesc->shareBuffer.data = nullptr;
            devAddr = nullptr;
            return nullptr;
        }

        aclDvppChannelDesc->cmdListBuffer.data = tmpData;
        aclDvppChannelDesc->cmdListBuffer.length = tmpDataSize;

        return aclDvppChannelDesc;
    }

    aclError ImageProcessor::CreateNotifyForDvppChannel(acldvppChannelDesc *channelDesc)
    {
        rtError_t rtRet;
        if (channelDesc->dvppWaitTaskType == NOTIFY_TASK) {
            int32_t deviceId = 0;
            rtRet = rtGetDevice(&deviceId);
            if (rtRet != ACL_SUCCESS) {
                ACL_LOG_CALL_ERROR("[Get][DeviceId]fail to get deviceId when create dvpp channel, result = %d", rtRet);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }

            // create notify for dvpp channel.
            rtNotify_t notify = nullptr;
            rtRet = rtNotifyCreate(deviceId, &notify);
            if (rtRet != ACL_SUCCESS) {
                ACL_LOG_CALL_ERROR("[Create][Notify]fail to create notify, result = %d", rtRet);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }
            channelDesc->notify = static_cast<void *>(notify);

            // get notifyId for dvpp channel.
            uint32_t notifyId = 0;
            rtRet = rtGetNotifyID(notify, &notifyId);
            if (rtRet != ACL_SUCCESS) {
                ACL_LOG_CALL_ERROR("[Get][Notify]fail to get NotifyId, result = %d", rtRet);
                (void)rtNotifyDestroy(channelDesc->notify);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }
            ACL_LOG_INFO("notifyId is %u", notifyId);
            channelDesc->dvppDesc.notifyId = notifyId;
        } else {
            // create event for dvpp channel.
            rtEvent_t event = nullptr;
            rtRet = rtEventCreateWithFlag(&event, RT_EVENT_WITH_FLAG);
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Create][Event]fail to create sendFrameEvent, runtime result = %d", rtRet);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }
            channelDesc->notify = static_cast<void *>(event);

            // get eventId for dvpp channel
            uint32_t eventId = 0;
            rtRet = rtGetEventID(event, &eventId);
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Get][EventID]fail to get eventId, runtime result = %d", rtRet);
                (void)rtEventDestroy(event);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }
            ACL_LOG_INFO("eventId is %u", eventId);
            channelDesc->dvppDesc.notifyId = eventId;
        }
        return ACL_SUCCESS;
    }

    void ImageProcessor::DestroyNotifyAndStream(acldvppChannelDesc *channelDesc, rtStream_t stream)
    {
        ACL_LOG_DEBUG("begin to destroy notify and stream.");
        if (channelDesc == nullptr) {
            ACL_LOG_ERROR("[Check][ChannelDesc]dvpp channel desc is null.");
            const char *argList[] = {"param"};
            const char *argVal[] = {"channelDesc"};
            acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                argList, argVal, 1);
            return;
        }

        if (stream != nullptr) {
            rtError_t rtRet = rtStreamDestroy(stream);
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Destroy][Stream]fail to destroy stream, runtime result = %d", rtRet);
            }
        }

        if (channelDesc->notify != nullptr) {
            rtError_t rtRet;
            if (channelDesc->dvppWaitTaskType == NOTIFY_TASK) {
                rtRet = rtNotifyDestroy(static_cast<rtNotify_t>(channelDesc->notify));
                if (rtRet != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Destroy][Notify]fail to destroy notify, runtime result = %d", rtRet);
                }
            } else {
                rtRet = rtEventDestroy(static_cast<rtEvent_t>(channelDesc->notify));
                if (rtRet != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Destroy][Event]fail to destroy event, runtime result = %d", rtRet);
                }
            }
            channelDesc->notify = nullptr;
        }

        ACL_LOG_DEBUG("success to destroy notify and stream.");
    }

    void ImageProcessor::DestroyStream(rtStream_t stream)
    {
        ACL_LOG_DEBUG("begin to destroy stream.");

        if (stream != nullptr) {
            rtError_t rtRet = rtStreamDestroy(stream);
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Destroy][Stream]fail to destroy stream, runtime result = %d", rtRet);
            }
        }

        ACL_LOG_DEBUG("success to destroy stream.");
    }

    aclError ImageProcessor::ValidateJpegInputFormat(acldvppPixelFormat format)
    {
        static const std::set<acldvppPixelFormat> jpegInputFormatSet = {
            acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_420,
            acldvppPixelFormat::PIXEL_FORMAT_YUYV_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_UYVY_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_YVYU_PACKED_422,
            acldvppPixelFormat::PIXEL_FORMAT_VYUY_PACKED_422
        };

        auto iter = jpegInputFormatSet.find(format);
        if (iter != jpegInputFormatSet.end()) {
            return ACL_SUCCESS;
        }
        return ACL_ERROR_FORMAT_NOT_MATCH;
    }

    aclError ImageProcessor::ValidateAndConvertVpcBatchParams(const acldvppBatchPicDesc *srcBatchPicDescs,
                                                              const acldvppBatchPicDesc *dstBatchPicDescs,
                                                              const uint32_t *roiNums,
                                                              uint32_t size,
                                                              uint16_t *deviceRoiNums,
                                                              uint32_t &totalRoiNums,
                                                              uint32_t maxRoiNums,
                                                              acldvppResizeConfig *resizeConfig)
    {
        // valid size
        bool validParam = (size == 0) || (srcBatchPicDescs->dvppBatchPicDescs.batchSize < size);
        if (validParam) {
            ACL_LOG_ERROR("[Check][BatchSize]srcBatchPicDescs batchSize less than roiNums size or size = 0, "
                "batch size = %u, size = %u.", srcBatchPicDescs->dvppBatchPicDescs.batchSize, size);
            std::string convertedStr = std::to_string(srcBatchPicDescs->dvppBatchPicDescs.batchSize);
            std::string errMsg = acl::AclErrorLogManager::FormatStr("less than roiNums size[%u] or size = 0", size);
            const char *argList[] = {"param", "value", "reason"};
            const char *argVal[] = {"srcBatchPicDescs batchSize", convertedStr.c_str(), errMsg.c_str()};
            acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_PARAM_MSG,
                argList, argVal, 3);
            return ACL_ERROR_INVALID_PARAM;
        }

        if (resizeConfig != nullptr) {
            aclError validResizeConfigRet = ValidateDvppResizeConfig(resizeConfig);
            if (validResizeConfigRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Check][ResizeConfig]resize config acldvppResizeConfig verify failed, "
                    "result = %d.", validResizeConfigRet);
                return validResizeConfigRet;
            }
        }

        // validate input format
        aclError validFormat = ACL_SUCCESS;
        for (uint32_t index = 0; index < srcBatchPicDescs->dvppBatchPicDescs.batchSize; ++index) {
            validFormat = ValidateVpcInputFormat(
                static_cast<acldvppPixelFormat>(srcBatchPicDescs->aclDvppPicDescs[index].dvppPicDesc.format));
            if (validFormat != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Verify][InputFormat]input acldvppPicDesc format verify failed, "
                    "result = %d, format = %u.", validFormat,
                    srcBatchPicDescs->aclDvppPicDescs[index].dvppPicDesc.format);
                return validFormat;
            }
        }

        // validate output format
        for (uint32_t index = 0; index < dstBatchPicDescs->dvppBatchPicDescs.batchSize; ++index) {
            validFormat = ValidateVpcOutputFormat(
                static_cast<acldvppPixelFormat>(dstBatchPicDescs->aclDvppPicDescs[index].dvppPicDesc.format));
            if (validFormat != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Verify][OutputFormat]output acldvppPicDesc format verify failed, "
                    "result = %d, format = %u.", validFormat,
                    dstBatchPicDescs->aclDvppPicDescs[index].dvppPicDesc.format);
                return validFormat;
            }
        }

        // calculate roi nums
        for (uint32_t index = 0; index < size; ++index) {
            validParam = (roiNums[index] > UINT16_MAX) || (roiNums[index] == 0);
            if (validParam) {
                ACL_LOG_INNER_ERROR("[Check][Params]roiNums index[%u] great than UINT16_MAX[65535] or its value is %u",
                    index, roiNums[index]);
                return ACL_ERROR_INVALID_PARAM;
            }
            deviceRoiNums[index] = static_cast<uint16_t>(roiNums[index]);
            totalRoiNums += roiNums[index];
        }

        // validate total roi nums: min value is 1
        validParam = (totalRoiNums < 1) || (totalRoiNums > maxRoiNums);
        if (validParam) {
            ACL_LOG_INNER_ERROR("[Check][Params]the value of totalRoiNums[%d] is invalid, "
                "it must be between in [%d, %d]", totalRoiNums, 1, maxRoiNums);
            return ACL_ERROR_INVALID_PARAM;
        }
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::SetDataBufferForBatchPicDesc(acldvppBatchPicDesc *batchPicDesc, uint32_t batchSize)
    {
        ACL_REQUIRES_NOT_NULL(batchPicDesc);
        ACL_REQUIRES_NOT_NULL(batchPicDesc->dataBuffer.data);
        if (batchPicDesc->dvppBatchPicDescs.batchSize < batchSize) {
            ACL_LOG_INNER_ERROR("[Set][DataBuffer]batchSize[%u] from batchPicDesc must be greater "
                "or equal roi batch size[%u].", batchSize, batchPicDesc->dvppBatchPicDescs.batchSize);
            return ACL_ERROR_INVALID_PARAM;
        }

        aicpu::dvpp::DvppBatchPicDesc *devBatchPicPtr = reinterpret_cast<aicpu::dvpp::DvppBatchPicDesc *>(
            batchPicDesc->dataBuffer.data);
        devBatchPicPtr->batchSize = batchSize;
        for (size_t index = 0; index < batchSize; ++index) {
            devBatchPicPtr->dvppPicDescs[index] = batchPicDesc->aclDvppPicDescs[index].dvppPicDesc;
        }
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::LaunchTaskForVpcBatchCrop(acldvppChannelDesc *channelDesc,
                                                       VpcBatchParams &batchParams,
                                                       const acldvppResizeConfig *resizeConfig,
                                                       rtStream_t stream)
    {
        // check corp area
        for (uint32_t index = 0; index < batchParams.totalRoiNums_; ++index) {
            ACL_REQUIRES_NOT_NULL(batchParams.cropAreas_[index]);
        }

        // calculate crop config total size
        uint32_t cropAreaSize = sizeof(aicpu::dvpp::DvppRoiConfig);
        uint32_t totalCropConfigSize = cropAreaSize * batchParams.totalRoiNums_;
        uint32_t resizeConfigSize = 0;
        if (resizeConfig != nullptr) {
            resizeConfigSize = CalDevDvppStructRealUsedSize(&resizeConfig->dvppResizeConfig);
        }

        // BatchCrop have 3 inputs
        constexpr int32_t addrNum = 3;
        uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + addrNum * sizeof(uint64_t) +
            totalCropConfigSize + batchParams.batchSize_ * sizeof(uint16_t) + resizeConfigSize;
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = addrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        ioAddr[1] = reinterpret_cast<uintptr_t>(batchParams.srcBatchPicDescs_->dataBuffer.data);
        ioAddr[2] = reinterpret_cast<uintptr_t>(batchParams.dstBatchPicDescs_->dataBuffer.data);

        uint32_t cropAreaOffset = sizeof(aicpu::AicpuParamHead) + addrNum * sizeof(uint64_t);
        // copy crop config
        for (size_t index = 0; index < batchParams.totalRoiNums_; ++index) {
            auto memcpyRet = memcpy_s(args.get() + cropAreaOffset, argsSize - cropAreaOffset,
                                      &(batchParams.cropAreas_[index]->dvppRoiConfig), cropAreaSize);
            if (memcpyRet != EOK) {
                ACL_LOG_INNER_ERROR("[Copy][Crop]copy crop area to args failed, result = %d.", memcpyRet);
                return ACL_ERROR_FAILURE;
            }
            cropAreaOffset += cropAreaSize;
        }
        // copy roiNums
        auto memcpyRet = memcpy_s(args.get() + cropAreaOffset, argsSize - cropAreaOffset,
                                  batchParams.roiNums_, sizeof(uint16_t) * batchParams.batchSize_);
        if (memcpyRet != EOK) {
            ACL_LOG_INNER_ERROR("[Copy][RoiNums]copy roiNums to args failed, result = %d.", memcpyRet);
            return ACL_ERROR_FAILURE;
        }

        // copy resize config
        const char *kernelName = acl::dvpp::DVPP_KERNELNAME_BATCH_CROP;
        if (resizeConfig != nullptr) {
            kernelName = acl::dvpp::DVPP_KERNELNAME_BATCH_CROP_RESIZE;
            cropAreaOffset += (sizeof(uint16_t) * batchParams.batchSize_);
            memcpyRet = memcpy_s(args.get() + cropAreaOffset, argsSize - cropAreaOffset,
                &(resizeConfig->dvppResizeConfig), resizeConfigSize);
            if (memcpyRet != EOK) {
                ACL_LOG_INNER_ERROR("[Copy][ResizeConfig]copy resize config to args failed, "
                    "result = %d.", memcpyRet);
                return ACL_ERROR_FAILURE;
            }
        }
        aclError launchRet = LaunchDvppTask(channelDesc, args.get(), argsSize,
            kernelName, stream);
        if (launchRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Launch][Task]launch dvpp task failed, result = %d.", launchRet);
            return launchRet;
        }
        return ACL_SUCCESS;
    }

    aclError ImageProcessor::LaunchTaskForVpcBatchCropAndPaste(acldvppChannelDesc *channelDesc,
                                                               VpcBatchParams &batchParams,
                                                               const acldvppResizeConfig *resizeConfig,
                                                               rtStream_t stream)
    {
        // check crop area and paste area
        for (uint32_t index = 0; index < batchParams.totalRoiNums_; ++index) {
            ACL_REQUIRES_NOT_NULL(batchParams.cropAreas_[index]);
            ACL_REQUIRES_NOT_NULL(batchParams.pasteAreas_[index]);
        }
        // 2 areas: crop area and paste area
        int32_t roiAreaSize = sizeof(aicpu::dvpp::DvppRoiConfig);
        uint32_t totalRoiConfigSize = roiAreaSize * 2 * batchParams.totalRoiNums_;
        uint32_t resizeConfigSize = 0;
        if (resizeConfig != nullptr) {
            resizeConfigSize = CalDevDvppStructRealUsedSize(&resizeConfig->dvppResizeConfig);
        }

        // BatchCropAndPaste have 3 inputs
        constexpr int32_t ioAddrNum = 3;
        uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t) +
            totalRoiConfigSize + batchParams.batchSize_ * sizeof(uint16_t) + resizeConfigSize;
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        ioAddr[1] = reinterpret_cast<uintptr_t>(batchParams.srcBatchPicDescs_->dataBuffer.data);
        ioAddr[2] = reinterpret_cast<uintptr_t>(batchParams.dstBatchPicDescs_->dataBuffer.data);

        uint32_t roiAreaOffset = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        // copy crop config
        for (uint32_t index = 0; index < batchParams.totalRoiNums_; ++index) {
            auto memcpyRet = memcpy_s(args.get() + roiAreaOffset, argsSize - roiAreaOffset,
                                      &(batchParams.cropAreas_[index]->dvppRoiConfig), roiAreaSize);
            if (memcpyRet != EOK) {
                ACL_LOG_INNER_ERROR("[Copy][CropArea]copy crop area to args failed, result = %d.", memcpyRet);
                return ACL_ERROR_FAILURE;
            }
            roiAreaOffset += roiAreaSize;

            memcpyRet = memcpy_s(args.get() + roiAreaOffset, argsSize - roiAreaOffset,
                                 &(batchParams.pasteAreas_[index]->dvppRoiConfig), roiAreaSize);
            if (memcpyRet != EOK) {
                ACL_LOG_INNER_ERROR("[Copy][PasteArea]copy paste area to args failed, result = %d.", memcpyRet);
                return ACL_ERROR_FAILURE;
            }
            roiAreaOffset += roiAreaSize;
        }
        // copy roiNums
        auto memcpyRet = memcpy_s(args.get() + roiAreaOffset, argsSize - roiAreaOffset,
                                  batchParams.roiNums_, sizeof(uint16_t) * batchParams.batchSize_);
        if (memcpyRet != EOK) {
            ACL_LOG_INNER_ERROR("[Copy][RoiNums]copy roiNums to args failed, result = %d.", memcpyRet);
            return ACL_ERROR_FAILURE;
        }

        // copy resize config
        const char *kernelName = acl::dvpp::DVPP_KERNELNAME_BATCH_CROP_AND_PASTE;
        if (resizeConfig != nullptr) {
            uint32_t resizeConfigOffset = roiAreaOffset + sizeof(uint16_t) * batchParams.batchSize_;
            memcpyRet = memcpy_s(args.get() + resizeConfigOffset, argsSize - resizeConfigOffset,
                &(resizeConfig->dvppResizeConfig), resizeConfigSize);
            if (memcpyRet != EOK) {
                ACL_LOG_INNER_ERROR("[Copy][ResizeConfig]copy resize config to args failed, "
                    "result = %d.", memcpyRet);
                return ACL_ERROR_FAILURE;
            }
            kernelName = acl::dvpp::DVPP_KERNELNAME_BATCH_CROP_RESIZE_PASTE;
        }

        aclError launchRet = LaunchDvppTask(channelDesc, args.get(), argsSize, kernelName, stream);
        if (launchRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Launch][Task]launch dvpp task failed, result = %d.", launchRet);
            return launchRet;
        }
        return ACL_SUCCESS;
    }

    acldvppLutMap *ImageProcessor::acldvppCreateLutMap()
    {
        ACL_LOG_INNER_ERROR("[Create][LutMap]acl dvpp create lut map is not supported "
            "in this version. Please check.");
        return nullptr;
    }

    aclError ImageProcessor::acldvppDestroyLutMap(acldvppLutMap *lutMap)
    {
        ACL_LOG_INNER_ERROR("[Destroy][LutMap]acl dvpp destroy lut map is not supported "
            "in this version. Please check.");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    uint32_t ImageProcessor::acldvppGetLutMapDims(const acldvppLutMap *lutMap)
    {
        ACL_LOG_INNER_ERROR("[Get][LutMapDims]acl dvpp get lut map dims is not supported "
            "in this version. Please check.");
        return 0;
    }

    aclError ImageProcessor::acldvppGetLutMapData(const acldvppLutMap *lutMap,
                                                  uint32_t dim,
                                                  uint8_t **data,
                                                  uint32_t *len)
    {
        ACL_LOG_INNER_ERROR("[Get][LutMapData]acl dvpp get lut map data is not supported "
            "in this version. Please check.");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    acldvppBorderConfig *ImageProcessor::acldvppCreateBorderConfig()
    {
        ACL_LOG_INNER_ERROR("[Create][BorderConfig]acl dvpp create border config is not supported "
            "in this version. Please check.");
        return nullptr;
    }

    aclError ImageProcessor::acldvppDestroyBorderConfig(acldvppBorderConfig *borderConfig)
    {
        ACL_LOG_INNER_ERROR("[Destroy][BorderConfig]acl dvpp destroy border config is not supported "
            "in this version. Please check.");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError ImageProcessor::AvoidDependOnLibjpeg8d(void *compressInfoPtr)
    {
        ACL_REQUIRES_NOT_NULL(compressInfoPtr);
        struct jpeg_compress_struct *cinfoPtr = static_cast<jpeg_compress_struct *>(compressInfoPtr);
        jpeg_start_compress(cinfoPtr, TRUE);

        return ACL_SUCCESS;
    }
    }
}
