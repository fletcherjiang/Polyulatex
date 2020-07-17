/**
* @file video_processor_v100.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef VIDEO_PROCESSOR_V100_H
#define VIDEO_PROCESSOR_V100_H

#include "single_op/dvpp/base/video_processor.h"

namespace acl {
    namespace dvpp {
    class VideoProcessorV100 : public VideoProcessor {
    public:
        /**
         * VDEC set channel id for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param channelId[in] channel id
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError aclvdecSetChannelDescChannelId(aclvdecChannelDesc *channelDesc, uint32_t channelId) override;

        /**
         * VDEC set outPicFormat for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param outPicFormat[in] outPicFormat
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError aclvdecSetChannelDescOutPicFormat(aclvdecChannelDesc *channelDesc,
                                                   acldvppPixelFormat outPicFormat) override;

        /**
         * Set ip proportion for venc channel desc.
         * @param channelDesc[OUT] venc channel desc
         * @param ipProp[IN] I frame and P frame proportion
         * @return ACL_SUCCESS for success, other for failure
         */
        aclError aclvencSetChannelDescIPProp(aclvencChannelDesc *channelDesc, uint32_t ipProp) override;

        /**
         * Get ip proportion for venc channel desc.
         * @param channelDesc[IN] venc channel desc
         * @param isSupport[OUT] support flag
         * @return I frame and P frame proportion
         */
        uint32_t aclvencGetChannelDescIPProp(const aclvencChannelDesc *channelDesc, bool &isSupport) override;

        ~VideoProcessorV100() = default;

        // not allow copy constructor and assignment operators
        VideoProcessorV100(const VideoProcessorV100 &) = delete;

        VideoProcessorV100 &operator=(const VideoProcessorV100 &) = delete;

        VideoProcessorV100(VideoProcessorV100 &&) = delete;

        VideoProcessorV100 &&operator=(VideoProcessorV100 &&) = delete;

        // constructor
        explicit VideoProcessorV100(aclrtRunMode aclRunMode) : VideoProcessor(aclRunMode) {};
    };
    }
}
#endif // VIDEO_PROCESSOR_V100_H
