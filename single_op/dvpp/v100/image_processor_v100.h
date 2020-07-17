/**
* @file image_processor_v100.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef IMAGE_PROCESSOR_V100_H
#define IMAGE_PROCESSOR_V100_H

#include "single_op/dvpp/base/image_processor.h"

namespace acl {
    namespace dvpp {
    class ImageProcessorV100 : public ImageProcessor {
    public:

        /**
         * DVPP set interpolation for resize config.
         * @param resizeConfig[in|out] resize config
         * @param interpolation[in] interplation algorithm
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError acldvppSetResizeConfigInterpolation(acldvppResizeConfig *resizeConfig,
                                                     uint32_t interpolation) override;

        /**
         * DVPP set mode for channel desc (not supported).
         * @param channelDesc[in|out] channel desc
         * @param mode[in] mode
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError acldvppSetChannelDescMode(acldvppChannelDesc *channelDesc, uint32_t mode) override;

        /**
         * Get image width and height of png.
         * @param data[in] image data in host memory
         * @param size[in] the size of image data
         * @param width[out] the width of image from image header
         * @param height[out] the height of image from image header
         * @param components [OUT]   the components of image from image header
         * @param bitDepth [OUT]   the bit of image from image header
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError GetPngImgInfo(const void *data,
                               uint32_t dataSize,
                               uint32_t *width,
                               uint32_t *height,
                               int32_t *components,
                               uint32_t *bitDepth);

        /**
         * Get image width, height and format of png.
         * @param data[in] image data in host memory
         * @param size[in] the size of image data
         * @param width[out] the width of image from image header
         * @param height[out] the height of image from image header
         * @param components [OUT]   the components of image from image header
         * @return ACL_SUCCESS for ok, others for fail
         */
         aclError acldvppPngGetImageInfo(const void *data,
                                         uint32_t dataSize,
                                         uint32_t *width,
                                         uint32_t *height,
                                         int32_t *components) override;

        /**
         * Predict decode size of png image.
         * @param data[in] origin image data in host memory
         * @param dataSize[in] the size of origin image data
         * @param outputPixelFormat[in] the pixel format jpeg decode
         * @param decSize[out] the size predicted for decode image
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError acldvppPngPredictDecSize(const void *data,
                                          uint32_t dataSize,
                                          acldvppPixelFormat outputPixelFormat,
                                          uint32_t *decSize) override;

        /**
         * Png decode.
         * @param channelDesc[in] channel desc
         * @param data[in] decode input picture destruction's data
         * @param size[in|out] decode input picture destruction's size
         * @param outputDesc[in|out] decode output picture destruction
         * @param stream[in] runtime stream
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError acldvppPngDecodeAsync(acldvppChannelDesc *channelDesc,
                                       const void *data,
                                       uint32_t size,
                                       acldvppPicDesc *outputDesc,
                                       aclrtStream stream) override;

        ~ImageProcessorV100() = default;

        // not allow copy constructor and assignment operators
        ImageProcessorV100(const ImageProcessorV100 &) = delete;

        ImageProcessorV100 &operator=(const ImageProcessorV100 &) = delete;

        ImageProcessorV100(ImageProcessorV100 &&) = delete;

        ImageProcessorV100 &&operator=(ImageProcessorV100 &&) = delete;

        // constructor
        explicit ImageProcessorV100(aclrtRunMode aclRunMode) : ImageProcessor(aclRunMode) {};

    protected:
        /**
         * Validate input pic format.
         * @param format[in] input pic format.
         * @return ACL_SUCCESS for success, other for failure.
         */
        aclError ValidateVpcInputFormat(acldvppPixelFormat format) override;

        /**
         * Validate vpc output pic format.
         * @param format[in] input pic format.
         * @return ACL_SUCCESS for success, other for failure.
         */
        aclError ValidateVpcOutputFormat(acldvppPixelFormat format) override;

        /**
         * Validate dvpp resize config.
         * @param config[in] dvpp resize config.
         * @return ACL_SUCCESS for success, other for failure.
         */
        aclError ValidateDvppResizeConfig(acldvppResizeConfig *config) override;

        /**
        * Validate jpeg output pic format.
        * @param format[in] input pic format.
        * @return ACL_SUCCESS for success, other for failure.
        */
        aclError ValidateJpegOutputFormat(acldvppPixelFormat format) override;
    };
    }
}
#endif // IMAGE_PROCESSOR_V100_H
