/**
* @file dvpp_util.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef DVPP_UTIL_H
#define DVPP_UTIL_H

#include <map>
#include <string>
#include <typeinfo>
#include "utils/acl_jpeglib.h"
#include "acl/acl_rt.h"
#include "aicpu/dvpp/dvpp_def.h"
#include "single_op/dvpp/common/dvpp_def_internal.h"

namespace acl {
    namespace dvpp {
        // define acl dvpp struct tlv len map
        static const std::map<std::string, uint32_t> aclDvppStructTlvLenMap_ = {
            {typeid(acldvppResizeConfig).name(), DVPP_RESIZE_CONFIG_TLV_LEN},
            {typeid(acldvppJpegeConfig).name(), DVPP_JPEGE_CONFIG_TLV_LEN},
            {typeid(acldvppChannelDesc).name(), DVPP_CHANNEL_DESC_TLV_LEN},
            {typeid(aclvdecChannelDesc).name(), VDEC_CHANNEL_DESC_TLV_LEN},
            {typeid(aclvencChannelDesc).name(), VENC_CHANNEL_DESC_TLV_LEN},
            {typeid(aclvdecFrameConfig).name(), VDEC_FRAME_CONFIG_TLV_LEN},
            {typeid(aclvencFrameConfig).name(), VENC_FRAME_CONFIG_TLV_LEN},
            {typeid(acldvppLutMap).name(), DVPP_LUT_MAP_TLV_LEN},
            {typeid(acldvppBorderConfig).name(), DVPP_BORDER_CONFIG_TLV_LEN},
            {typeid(acldvppHist).name(), DVPP_HIST_DESC_LEN},
            {typeid(aicpu::dvpp::DvppResizeConfig).name(), DVPP_RESIZE_CONFIG_TLV_LEN},
            {typeid(aicpu::dvpp::DvppJpegeConfig).name(), DVPP_JPEGE_CONFIG_TLV_LEN},
            {typeid(aicpu::dvpp::DvppChannelDesc).name(), DVPP_CHANNEL_DESC_TLV_LEN},
            {typeid(aicpu::dvpp::DvppVdecDesc).name(), VDEC_CHANNEL_DESC_TLV_LEN},
            {typeid(aicpu::dvpp::DvppVencDesc).name(), VENC_CHANNEL_DESC_TLV_LEN},
            {typeid(aicpu::dvpp::DvppVdecFrameConfig).name(), VDEC_FRAME_CONFIG_TLV_LEN},
            {typeid(aicpu::dvpp::DvppVencFrameConfig).name(), VENC_FRAME_CONFIG_TLV_LEN},
            {typeid(aicpu::dvpp::DvppLutMap).name(), DVPP_LUT_MAP_TLV_LEN},
            {typeid(aicpu::dvpp::DvppBorderConfig).name(), DVPP_BORDER_CONFIG_TLV_LEN},
            {typeid(aicpu::dvpp::DvppHistDesc).name(), DVPP_HIST_DESC_LEN},
        };

        // defind dev dvpp struct tlv len map
        static const std::map<std::string, uint32_t> devDvppStructTlvLenMap_ = {
            {typeid(aicpu::dvpp::DvppResizeConfig).name(), DVPP_RESIZE_CONFIG_TLV_LEN},
            {typeid(aicpu::dvpp::DvppJpegeConfig).name(), DVPP_JPEGE_CONFIG_TLV_LEN},
            {typeid(aicpu::dvpp::DvppChannelDesc).name(), DVPP_CHANNEL_DESC_TLV_LEN},
            {typeid(aicpu::dvpp::DvppVdecDesc).name(), VDEC_CHANNEL_DESC_TLV_LEN},
            {typeid(aicpu::dvpp::DvppVencDesc).name(), VENC_CHANNEL_DESC_TLV_LEN},
            {typeid(aicpu::dvpp::DvppVdecFrameConfig).name(), VDEC_FRAME_CONFIG_TLV_LEN},
            {typeid(aicpu::dvpp::DvppVencFrameConfig).name(), VENC_FRAME_CONFIG_TLV_LEN},
            {typeid(aicpu::dvpp::DvppLutMap).name(), DVPP_LUT_MAP_TLV_LEN},
            {typeid(aicpu::dvpp::DvppBorderConfig).name(), DVPP_BORDER_CONFIG_TLV_LEN},
            {typeid(aicpu::dvpp::DvppHistDesc).name(), DVPP_HIST_DESC_LEN},
        };

        /**
         * Calculate acl dvpp struct size.
         * @param struct[in] struct type
         * @return size
         */
        template<typename T>
        const uint32_t CalAclDvppStructSize(const T *dvppStruct)
        {
            uint32_t tlvLen = 0;
            auto iter = aclDvppStructTlvLenMap_.find(typeid(T).name());
            if (iter != aclDvppStructTlvLenMap_.end()) {
                tlvLen = iter->second;
            }
            return sizeof(T) + tlvLen;
        };

        /**
         * Calculate dev dvpp struct real used size.
         * @param struct[in] struct type
         * @return size
         */
        template<typename T>
        const uint32_t CalDevDvppStructRealUsedSize(const T *dvppStruct)
        {
            auto iter = devDvppStructTlvLenMap_.find(typeid(T).name());
            if (iter == devDvppStructTlvLenMap_.end()) {
                return sizeof(T);
            }
            return sizeof(T) + dvppStruct->len;
        }

        /**
         * Async copy dvpp date between host and device
         * @param aclPicDesc[in] acl dvpp pic desc
         * @param memcpyKind[in] acl memcpy kind
         * @param stream[in] stream
         * @return ACL_SUCCESS for success, other for failure
         */
        aclError CopyDvppPicDescAsync(const acldvppPicDesc *aclDvppPicDesc,
                                      aclrtMemcpyKind memcpyKind,
                                      aclrtStream stream);

       /**
        * Async copy dvpp batch date between host and device.
        * @param aclPicDesc[in] acl dvpp batch pic desc.
        * @param memcpyKind[in] acl memcpy kind.
        * @param batchSize[in] picture batch size.
        * @param stream[in] stream.
        * @return ACL_SUCCESS for success, other for failure.
        */
        aclError CopyDvppBatchPicDescAsync(acldvppBatchPicDesc *aclBatchPicDesc,
                                           aclrtMemcpyKind memcpyKind,
                                           uint32_t batchSize,
                                           aclrtStream stream);

        /**
         * Async copy dvpp date between host and device
         * @param aclDvppHist[in] acl dvpp hist desc
         * @param memcpyKind[in] acl memcpy kind
         * @param stream[in] stream
         * @return ACL_SUCCESS for success, other for failure
         */
        aclError CopyDvppHistDescAsync(acldvppHist *aclDvppHist, aclrtMemcpyKind memcpyKind, aclrtStream stream);

        /**
         * Free device buffer
         * @param dataBuffer[in] data buffer
         * @return void
         */
        void FreeDeviceBuffer(aclDataBuffer &dataBuffer);

        /**
         * Free dvpp device buffer
         * @param dataBuffer[in] data buffer
         * @return void
         */
        void FreeDvppDeviceBuffer(aclDataBuffer &dataBuffer);

       /**
        * Free device addr (rtFree)
        * @param devAddr[in] device addr
        * @return void
        */
        void FreeDeviceAddr(void *devAddr);

        /**
         * Dvpp libjpeg error exit callback func
         * @param cinfo[in] jpeg common info
         * @return void
         */
        void AcldvppLibjpegErrorExit(j_common_ptr cinfo);

        /**
         * Calculate image size kernel
         * @param width[in] width
         * @param height[in] height
         * @param format[in] acl dvpp pixel format
         * @param size[out] image size
         * @return true for success, false for failed
         */
        bool CalcImageSizeKernel(uint32_t width, uint32_t height, acldvppPixelFormat format, uint32_t *size);

        /**
         * Check wether is packed format
         * @param pixelFoormat[in] acl dvpp pixel format
         * @return true or false
         */
        bool IsPackedFormat(acldvppPixelFormat pixelFormat);

        /**
         * Check wether is yuv420 format
         * @param libjpegHandler[in] jpeg decompress struct
         * @param pixelFormat[in] acl dvpp pixel format
         * @return true or false
         */
        bool IsYUV420(const struct jpeg_decompress_struct &libjpegHandler,
                      acldvppPixelFormat pixelFormat);

        /**
         * Check wether is yuv422 format
         * @param libjpegHandler[in] jpeg decompress struct
         * @param pixelFormat[in] acl dvpp pixel format
         * @return true or false
         */
        bool IsYUV422(const struct jpeg_decompress_struct &libjpegHandler,
                      acldvppPixelFormat pixelFormat);

        /**
         * Check wether is yuv444 format
         * @param libjpegHandler[in] jpeg decompress struct
         * @param pixelFormat[in] acl dvpp pixel format
         * @return true or false
         */
        bool IsYUV444(const struct jpeg_decompress_struct &libjpegHandler,
                      acldvppPixelFormat pixelFormat);

        /**
         * Check wether is yuv440 format
         * @param libjpegHandler[in] jpeg decompress struct
         * @param pixelFormat[in] acl dvpp pixel format
         * @return true or false
         */
        bool IsYUV440(const struct jpeg_decompress_struct &libjpegHandler,
                      acldvppPixelFormat pixelFormat);

        /**
         * Calculate image size for yuv format
         * @param libjpegHandler[in] jpeg decompress struct
         * @param needOrientation[in] orientation flag
         * @param pixelFormat[in] acl dvpp pixel format
         * @param size[out] image size
         * @return ACL_SUCCESS for success, other for failure
         */
        aclError CalcImageSizeForYUV(const struct jpeg_decompress_struct &libjpegHandler,
                                     bool needOrientation,
                                     acldvppPixelFormat pixelFormat,
                                     uint32_t *size);

        /**
         * Calculate image size
         * @param libjpegHandler[in] jpeg decompress struct
         * @param needOrientation[in] orientation flag
         * @param pixelFormat[in] acl dvpp pixel format
         * @param size[out] image size
         * @return ACL_SUCCESS for success, other for failure
         */
        aclError CalcImageSize(const struct jpeg_decompress_struct &libjpegHandler,
                               bool needOrientation,
                               acldvppPixelFormat pixelFormat,
                               uint32_t *size);

        /**
         * Check wether support super task or not
         * @param dvppVersion[in] dvpp version
         * @param aiCpuVersion[in] aicpu version
         * @return true or false
         */
        bool IsSupportSuperTask(DvppVersion dvppVersion, uint32_t aiCpuVersion);
    }
}

#endif //DVPP_UTIL_H
