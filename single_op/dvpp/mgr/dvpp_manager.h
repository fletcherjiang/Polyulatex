/**
* @file dvpp_manager.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef DVPP_MANAGER_H
#define DVPP_MANAGER_H

#include <mutex>
#include <memory>
#include "error_codes_inner.h"
#include "acl/ops/acl_dvpp.h"
#include "single_op/dvpp/base/image_processor.h"
#include "single_op/dvpp/base/video_processor.h"

namespace acl {
    namespace dvpp {
    // image processor shared pointer
    using ImageProcessorPtr = std::unique_ptr<ImageProcessor>;
    // video processor shared pointer
    using VideoProcessorPtr = std::unique_ptr<VideoProcessor>;

    class DvppManager {
    public:
        /**
         * Get dvpp manager instance
         * @return DvppManager reference
         */
        static DvppManager& GetInstance();

        /**
         * Get image processor
         * @return ImageProcessorPtr image processor pointer
         */
        ImageProcessor *GetImageProcessor();

        /**
         * Get video processor
         * @return VideoProcessorPtr video processor pointer
         */
        VideoProcessor *GetVideoProcessor();

        /**
         * Get dvpp version
         * @return dvpp version
         */
        DvppVersion GetDvppVersion();

        /**
         * Get aicpu version
         * @return aicpu version
         */
        uint32_t GetAicpuVersion();

        ~DvppManager() = default;

        // not allow copy constructor and assignment operators
        DvppManager(const DvppManager &) = delete;

        DvppManager &operator=(const DvppManager &) = delete;

        DvppManager(DvppManager &&) = delete;

        DvppManager &&operator=(DvppManager &&) = delete;

    private:
        /**
         * Check dvpp version and run mode
         * @return void
         */
        void CheckRunModeAndDvppVersion();

        /**
         * Get dvpp kernel version
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError GetDvppKernelVersion();

        /**
         * Init dvpp processor
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError InitDvppProcessor();

        DvppManager() = default;

    private:
        // image processor
        ImageProcessorPtr imageProcessor_ = nullptr;
        // video processor
        VideoProcessorPtr videoProcessor_ = nullptr;
        // dvpp version
        DvppVersion dvppVersion_ = DVPP_KERNELS_UNKOWN;
        // aicpu version
        uint32_t aicpuVersion_ = 0;
        // run mode
        aclrtRunMode aclRunMode_ = ACL_HOST;
        // mutex for dvpp version
        std::mutex dvppVersionMutex_;
    };
    }
}
#endif // DVPP_MANAGER_H