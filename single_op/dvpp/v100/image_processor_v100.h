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
