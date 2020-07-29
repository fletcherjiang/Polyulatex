/**
* @file dvpp_manager.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "dvpp_manager.h"
#include "common/log_inner.h"
#include "aicpu/common/aicpu_task_struct.h"
#include "single_op/dvpp/v100/image_processor_v100.h"
#include "single_op/dvpp/v200/image_processor_v200.h"
#include "single_op/dvpp/v100/video_processor_v100.h"
#include "single_op/dvpp/v200/video_processor_v200.h"
#include "toolchain/profiling_manager.h"

using namespace std;

namespace acl {
    namespace dvpp {
        DvppManager& DvppManager::GetInstance()
        {
            static DvppManager instance;
            // check and init dvpp version
            instance.CheckRunModeAndDvppVersion();
            return instance;
        }

        ImageProcessor *DvppManager::GetImageProcessor()
        {
            return imageProcessor_.get();
        }

        VideoProcessor *DvppManager::GetVideoProcessor()
        {
            return videoProcessor_.get();
        }

        DvppVersion DvppManager::GetDvppVersion()
        {
            return dvppVersion_;
        }

        uint32_t DvppManager::GetAicpuVersion()
        {
            return aicpuVersion_;
        }

        void DvppManager::CheckRunModeAndDvppVersion()
        {
            if ((dvppVersion_ != DVPP_KERNELS_UNKOWN) &&
                (imageProcessor_ != nullptr) &&
                (videoProcessor_ != nullptr)) {
                ACL_LOG_INFO("current dvpp version is %d.", dvppVersion_);
                return;
            }

            std::lock_guard<std::mutex> dvppVersionLock(dvppVersionMutex_);
            if (dvppVersion_ != DVPP_KERNELS_UNKOWN) {
                ACL_LOG_INFO("current dvpp version is %d.", dvppVersion_);
                return;
            }

            // get acl run mode
            aclError getRunModeRet = aclrtGetRunMode(&aclRunMode_);
            if (getRunModeRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("get run mode failed, result = %d.", getRunModeRet);
                return;
            }

            // get dvpp kernel version
            aclError getVersionRet = GetDvppKernelVersion();
            if (getVersionRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("get dvpp kernel version failed, result = %d", getVersionRet);
                return;
            }

            // init dvpp processor
            aclError initProcessorRet = InitDvppProcessor();
            if (initProcessorRet != ACL_SUCCESS) {
                ACL_LOG_ERROR("int dvpp processor failed, result = %d.", initProcessorRet);
                dvppVersion_ = DVPP_KERNELS_UNKOWN;
                return;
            }
        }

        aclError DvppManager::GetDvppKernelVersion()
        {
            // create stream
            rtStream_t rtStream = nullptr;
            rtError_t rtCreate = rtStreamCreate(&rtStream, RT_STREAM_PRIORITY_DEFAULT);
            if (rtCreate != RT_ERROR_NONE) {
                ACL_LOG_ERROR("create stream failed, runtime result = %d.", rtCreate);
                return ACL_GET_ERRCODE_RTS(rtCreate);
            }

            // malloc device memory
            uint32_t size = sizeof(uint32_t);
            void *dvppVersionAddr = nullptr;
            rtError_t rtErr = rtMalloc(&dvppVersionAddr, size,
                RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY);
            if (rtErr != RT_ERROR_NONE) {
                (void) rtStreamDestroy(rtStream);
                ACL_LOG_ERROR("alloc device memory failed, runtime result = %d", rtErr);
                return ACL_GET_ERRCODE_RTS(rtErr);
            }

            // get version have 1 output and 1 input
            constexpr int32_t ioAddrNum = 1;
            constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
            char args[argsSize] = {0};
            auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args);
            paramHead->length = argsSize;
            paramHead->ioAddrNum = ioAddrNum;
            auto ioAddr = reinterpret_cast<uint64_t *>(args + sizeof(aicpu::AicpuParamHead));
            ioAddr[0] = reinterpret_cast<uintptr_t>(dvppVersionAddr);

            // launch get version task
            rtErr = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
                                      acl::dvpp::DVPP_KERNELNAME_GET_VERSION,
                                      1, // blockDim default 1
                                      args,
                                      argsSize,
                                      nullptr, // no need smDesc
                                      rtStream);
            if (rtErr != RT_ERROR_NONE) {
                (void) rtFree(dvppVersionAddr);
                dvppVersionAddr = nullptr;
                (void) rtStreamDestroy(rtStream);
                ACL_LOG_ERROR("dvpp get version call rtCpuKernelLaunch failed, runtime result = %d.", rtErr);
                return ACL_GET_ERRCODE_RTS(rtErr);
            }

            // async memcpy
            uint32_t dvppVersion = DVPP_KERNELS_V100;
            if (aclRunMode_ == ACL_HOST) {
                auto rtRet = rtMemcpyAsync(&dvppVersion, size,
                                           static_cast<const void *>(dvppVersionAddr), size,
                                           RT_MEMCPY_DEVICE_TO_HOST,
                                           rtStream);
                if (rtRet != RT_ERROR_NONE) {
                    (void) rtFree(dvppVersionAddr);
                    dvppVersionAddr = nullptr;
                    (void) rtStreamDestroy(rtStream);
                    ACL_LOG_ERROR("memcpy to host memory failed, size = %u, runtime result = %d.", size, rtRet);
                    return ACL_GET_ERRCODE_RTS(rtRet);
                }
            }

            // stream synchronize
            rtErr = rtStreamSynchronize(rtStream);
            if (rtErr != RT_ERROR_NONE) {
                (void) rtFree(dvppVersionAddr);
                dvppVersionAddr = nullptr;
                (void) rtStreamDestroy(rtStream);
                ACL_LOG_ERROR("synchronize stream failed, runtime result = %d", rtErr);
                return ACL_GET_ERRCODE_RTS(rtErr);
            }

            // get dvpp version
            if (aclRunMode_ == ACL_DEVICE) {
                dvppVersion = *(static_cast<uint32_t *>(dvppVersionAddr));
            }
            string versionStr = to_string(dvppVersion);
            // the length of old dvpp version is 3
            // the length of new dvpp version is 6
            if (versionStr.length() == 3) {
                dvppVersion_ = static_cast<DvppVersion>(dvppVersion);
                aicpuVersion_ = 0;
            } else if (versionStr.length() == 6) {
                dvppVersion_ = static_cast<DvppVersion>(dvppVersion / 1000);
                aicpuVersion_ = dvppVersion % 1000;
            } else {
                (void) rtFree(dvppVersionAddr);
                dvppVersionAddr = nullptr;
                (void) rtStreamDestroy(rtStream);
                ACL_LOG_ERROR("unknown dvpp version length = %zu, dvpp version length must be 3 or 6",
                    versionStr.length());
                return ACL_ERROR_FEATURE_UNSUPPORTED;
            }
            ACL_LOG_INFO("get version success, dvpp version = %d, aicpu version = %u",
                static_cast<int32_t>(dvppVersion_), aicpuVersion_);

            // destroy resource: device memory and stream
            rtErr = rtFree(dvppVersionAddr);
            dvppVersionAddr = nullptr;
            if (rtErr != RT_ERROR_NONE) {
                ACL_LOG_ERROR("free device memory failed, runtime result = %d", rtErr);
            }
            rtErr = rtStreamDestroy(rtStream);
            if (rtErr != RT_ERROR_NONE) {
                ACL_LOG_ERROR("destory stream failed, runtime result = %d", rtErr);
            }
            return ACL_SUCCESS;
        }

        aclError DvppManager::InitDvppProcessor()
        {
            // init image and video processor
            try {
                switch (dvppVersion_) {
                    case DVPP_KERNELS_V100: {
                        imageProcessor_ =
                            std::unique_ptr<ImageProcessorV100>(new (std::nothrow)ImageProcessorV100(aclRunMode_));
                        ACL_CHECK_MALLOC_RESULT(imageProcessor_);
                        videoProcessor_ =
                            std::unique_ptr<VideoProcessorV100>(new (std::nothrow)VideoProcessorV100(aclRunMode_));
                        ACL_CHECK_MALLOC_RESULT(videoProcessor_);
                        break;
                    }
                    case DVPP_KERNELS_V200: {
                        imageProcessor_ =
                            std::unique_ptr<ImageProcessorV200>(new (std::nothrow)ImageProcessorV200(aclRunMode_));
                        ACL_CHECK_MALLOC_RESULT(imageProcessor_);
                        videoProcessor_ =
                            std::unique_ptr<VideoProcessorV200>(new (std::nothrow)VideoProcessorV200(aclRunMode_));
                        ACL_CHECK_MALLOC_RESULT(videoProcessor_);
                        break;
                    }
                    default: {
                        ACL_LOG_ERROR("dvpp kernel is unknown version %d.", dvppVersion_);
                        return ACL_ERROR_FEATURE_UNSUPPORTED;
                    }
                }
            } catch (...) {
                ACL_LOG_ERROR("define object image processor with unique_ptr failed.");
                return ACL_ERROR_BAD_ALLOC;
            }

            return ACL_SUCCESS;
        }
    }
}