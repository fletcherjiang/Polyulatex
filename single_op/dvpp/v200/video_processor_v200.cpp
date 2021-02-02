/**
* @file image_processor_v200.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "video_processor_v200.h"

#include <set>
#include <cstddef>
#include "securec.h"
#include "runtime/rt.h"
#include "common/log_inner.h"
#include "error_codes_inner.h"
#include "aicpu/dvpp/dvpp_def.h"
#include "toolchain/resource_statistics.h"
#include "toolchain/profiling_manager.h"
#include "aicpu/common/aicpu_task_struct.h"
#include "single_op/dvpp/common/dvpp_util.h"

namespace acl {
    namespace dvpp {
        namespace {
            constexpr int V200_CHANNEL_ID_CEILING = 255;
            constexpr int V200_CHANNEL_ID_FLOOR = 0;
            constexpr uint32_t VENC_USER_MODE = 2;
            constexpr uint32_t VENC_DEFAULT_BUFF_SIZE = 8 * 1024 * 1024;
            constexpr uint32_t MIN_VENC_BUFF_SIZE = 5 * 1024 * 1024;
            constexpr uint32_t VDEC_BIT_DEPTH_CONFIG_LEN = 8;
            constexpr uint16_t VDEC_BIT_DEPTH_CONFIG_DATA_TYPE = 1;
            constexpr uint16_t VDEC_BIT_DEPTH_CONFIG_DATA_LEN = 4;
            constexpr uint32_t VDEC_EXTEND_INFO_OFFSET = sizeof(aicpu::dvpp::DvppVdecBitDepthConfig);
            std::set<acldvppPixelFormat> validOutPicFormatSet = {
                acldvppPixelFormat::PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                acldvppPixelFormat::PIXEL_FORMAT_YUV_SEMIPLANAR_420,
                acldvppPixelFormat::PIXEL_FORMAT_BGR_888,
                acldvppPixelFormat::PIXEL_FORMAT_RGB_888
            };
        }

        aclError VideoProcessorV200::aclvdecSetChannelDescMatrix(aclvdecChannelDesc *channelDesc,
            acldvppCscMatrix matrixFormat)
        {
            ACL_LOG_DEBUG("start to execute aclvdecSetChannelDescMatrix");
            if ((channelDesc == nullptr) || (channelDesc->vdecDesc.extendInfo == nullptr)) {
                ACL_LOG_ERROR("[Check][ChannelDesc]channelDesc is null.");
                const char *argList[] = {"param"};
                const char *argVal[] = {"channelDesc or extendInfo"};
                acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                    argList, argVal, 1);
                return ACL_ERROR_INVALID_PARAM;
            }
            // check if matrix format in range
            if ((matrixFormat < ACL_DVPP_CSC_MATRIX_BT601_WIDE) ||
                (matrixFormat > ACL_DVPP_CSC_MATRIX_BT2020_NARROW)) {
                ACL_LOG_ERROR("[Check][matrixFormat]input format[%u] is not supported in this version",
                    static_cast<uint32_t>(matrixFormat));
                std::string matrixFormatStr = std::to_string(static_cast<uint32_t>(matrixFormat));
                const char *argList[] = {"param", "value", "reason"};
                const char *argVal[] = {"matrixFormat", matrixFormatStr.c_str(), "not supported in this version"};
                acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_PARAM_MSG,
                    argList, argVal, 3);
                return ACL_ERROR_INVALID_PARAM;
            }
            {
                std::unique_lock<std::mutex> lock{channelDesc->mutexForTLVMap};
                auto it = channelDesc->tlvParamMap.find(VDEC_CSC_MATRIX);
                if (it == channelDesc->tlvParamMap.end()) {
                    VdecChannelDescTLVParam vdecTLVParam;
                    std::shared_ptr<aicpu::dvpp::DvppCscMatrixConfig> dvppCscMatrixConfig =
                        std::make_shared<aicpu::dvpp::DvppCscMatrixConfig>();
                    ACL_REQUIRES_NOT_NULL(dvppCscMatrixConfig);
                    dvppCscMatrixConfig->cscMatrix = static_cast<uint32_t>(matrixFormat);
                    vdecTLVParam.value = static_cast<std::shared_ptr<void>>(dvppCscMatrixConfig);
                    vdecTLVParam.valueLen = sizeof(aicpu::dvpp::DvppCscMatrixConfig);
                    channelDesc->tlvParamMap[VDEC_CSC_MATRIX] = vdecTLVParam;
                } else {
                    aicpu::dvpp::DvppCscMatrixConfig *dvppCscMatrixConfig =
                        static_cast<aicpu::dvpp::DvppCscMatrixConfig *>(it->second.value.get());
                    ACL_REQUIRES_NOT_NULL(dvppCscMatrixConfig);
                    dvppCscMatrixConfig->cscMatrix = static_cast<uint32_t>(matrixFormat);
                }
            }
            ACL_LOG_DEBUG("successfully execute aclvdecSetChannelDescMatrix, cscMatrix = %u",
                static_cast<uint32_t>(matrixFormat));
            return ACL_SUCCESS;
        }

        aclError VideoProcessorV200::aclvdecGetChannelDescMatrix(const aclvdecChannelDesc *channelDesc,
            acldvppCscMatrix &matrixFormat)
        {
            ACL_LOG_DEBUG("start to execute aclvdecGetChannelDescMatrix");
            if ((channelDesc == nullptr) || (channelDesc->vdecDesc.extendInfo == nullptr)) {
                ACL_LOG_ERROR("[Check][ChannelDesc]channelDesc is null.");
                const char *argList[] = {"param"};
                const char *argVal[] = {"channelDesc or extendInfo"};
                acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                    argList, argVal, 1);
                return ACL_ERROR_INVALID_PARAM;
            }

            {
                std::mutex &mutexMap = const_cast<std::mutex &>(channelDesc->mutexForTLVMap);
                std::unique_lock<std::mutex> lock{mutexMap};
                const auto &it = channelDesc->tlvParamMap.find(VDEC_CSC_MATRIX);
                if (it == channelDesc->tlvParamMap.end()) {
                    matrixFormat = ACL_DVPP_CSC_MATRIX_BT601_WIDE;
                    return ACL_SUCCESS;
                }

                aicpu::dvpp::DvppCscMatrixConfig *dvppCscMatrixConfig =
                    static_cast<aicpu::dvpp::DvppCscMatrixConfig *>(it->second.value.get());
                if (dvppCscMatrixConfig == nullptr) {
                    ACL_LOG_INNER_ERROR("[Check][VencRateControl]vencRateControl ptr is null.");
                    return ACL_ERROR_INVALID_PARAM;
                }
                matrixFormat = static_cast<acldvppCscMatrix>(dvppCscMatrixConfig->cscMatrix);
                ACL_LOG_DEBUG("successfully execute aclvdecGetChannelDescMatrix, matrixFormat = %d",
                    static_cast<int32_t>(matrixFormat));
            }
            return ACL_SUCCESS;
        }

        aclError VideoProcessorV200::aclvencSetChannelDescBufSize(aclvencChannelDesc *channelDesc, uint32_t bufSize)
        {
            if (channelDesc == nullptr) {
                ACL_LOG_ERROR("[Check][ChannelDesc]channelDesc is null.");
                const char *argList[] = {"param"};
                const char *argVal[] = {"channelDesc"};
                acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                    argList, argVal, 1);
                return ACL_ERROR_INVALID_PARAM;
            }

            if (bufSize < MIN_VENC_BUFF_SIZE) {
                ACL_LOG_ERROR("[Check][BufSize]the value of bufSize[%u] can't be smaller than MIN_VENC_BUFF_SIZE[%u]",
                    bufSize, MIN_VENC_BUFF_SIZE);
                std::string bufSizeStr = std::to_string(bufSize);
                std::string errMsg = acl::AclErrorLogManager::FormatStr("can't be smaller than MIN_VENC_BUFF_SIZE[%u]",
                    MIN_VENC_BUFF_SIZE);
                const char *argList[] = {"param", "value", "reason"};
                const char *argVal[] = {"bufSize", bufSizeStr.c_str(), errMsg.c_str()};
                acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_PARAM_MSG,
                    argList, argVal, 3);
                return ACL_ERROR_INVALID_PARAM;
            }

            channelDesc->outputMemMode = VENC_USER_MODE;
            channelDesc->bufSize = bufSize;
            channelDesc->vencDesc.bufSize = channelDesc->bufSize;
            return ACL_SUCCESS;
        }

        aclError VideoProcessorV200::aclvencSetChannelDescBufAddr(aclvencChannelDesc *channelDesc, void *bufAddr)
        {
            if (channelDesc == nullptr) {
                ACL_LOG_ERROR("[Check][channelDesc]channelDesc is null.");
                const char *argList[] = {"param"};
                const char *argVal[] = {"channelDesc"};
                acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                    argList, argVal, 1);
                return ACL_ERROR_INVALID_PARAM;
            }

            if (bufAddr == nullptr) {
                ACL_LOG_ERROR("[Check][bufAddr]bufAddr is null.");
                const char *argList[] = {"param"};
                const char *argVal[] = {"bufAddr"};
                acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                    argList, argVal, 1);
                return ACL_ERROR_INVALID_PARAM;
            }

            if (channelDesc->bufAddr != 0) {
                ACL_LOG_ERROR("[Check][bufAddr]bufAddr is already set, bufAddr = %lu", channelDesc->bufAddr);
                std::string bufAddr = std::to_string(channelDesc->bufAddr);
                const char *argList[] = {"param", "value", "reason"};
                const char *argVal[] = {"bufAddr", bufAddr.c_str(), "bufAddr is already set"};
                acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_PARAM_MSG,
                    argList, argVal, 3);
                return ACL_ERROR_INVALID_PARAM;
            }

            channelDesc->outputMemMode = VENC_USER_MODE;
            channelDesc->bufAddr = reinterpret_cast<uintptr_t>(bufAddr);
            channelDesc->vencDesc.bufAddr = channelDesc->bufAddr;
            return ACL_SUCCESS;
        }

        uint32_t VideoProcessorV200::aclvencGetChannelDescBufSize(const aclvencChannelDesc *channelDesc,
            bool &isSupport)
        {
            isSupport = true;
            if (channelDesc == nullptr) {
                ACL_LOG_ERROR("[Check][channelDesc]channelDesc is null");
                const char *argList[] = {"param"};
                const char *argVal[] = {"channelDesc"};
                acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                    argList, argVal, 1);
                return 0;  // default 0
            }

            if (channelDesc->outputMemMode == VENC_USER_MODE) {
                return channelDesc->bufSize;
            }
            return 0;
        }

        void *VideoProcessorV200::aclvencGetChannelDescBufAddr(const aclvencChannelDesc *channelDesc, bool &isSupport)
        {
            isSupport = true;
            if (channelDesc == nullptr) {
                ACL_LOG_ERROR("[Check][channelDesc]channelDesc is null");
                const char *argList[] = {"param"};
                const char *argVal[] = {"channelDesc"};
                acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                    argList, argVal, 1);
                return nullptr;
            }

            if (channelDesc->outputMemMode == VENC_USER_MODE) {
                return reinterpret_cast<void *>(static_cast<uintptr_t>(channelDesc->bufAddr));
            }
            return nullptr;
        }

        aclvencChannelDesc *VideoProcessorV200::aclvencCreateChannelDesc()
        {
            ACL_LOG_INFO("aclvencCreateChannelDesc begin.");
            aclvencChannelDesc *vencChannelDesc = nullptr;
            switch (aclRunMode_) {
                case ACL_HOST: {
                    vencChannelDesc = CreateVencChannelDescOnHost();
                    break;
                }
                case ACL_DEVICE: {
                    vencChannelDesc = CreateVencChannelDescOnDevice();
                    break;
                }
                default: {
                    ACL_LOG_INNER_ERROR("[Check][RunMode]aclvencCreateChannelDesc failed, "
                        "unkown acl run mode %d.", aclRunMode_);
                    return nullptr;
                }
            }

            ACL_LOG_INFO("aclvencCreateChannelDesc success.");
            return vencChannelDesc;
        }

        aclError VideoProcessorV200::aclvencDestroyChannelDesc(aclvencChannelDesc *vencChannelDesc)
        {
            ACL_LOG_INFO("aclvencDestroyChannelDesc begin.");
            if (vencChannelDesc == nullptr) {
                return ACL_SUCCESS;
            }

            {
                std::unique_lock<std::mutex> lock{vencChannelDesc->mutexForTLVMap};
                vencChannelDesc->tlvParamMap.clear();
            }
            switch (aclRunMode_) {
                case ACL_HOST: {
                    FreeDeviceBuffer(vencChannelDesc->dataBuffer);
                    ACL_FREE(vencChannelDesc);
                    break;
                }
                case ACL_DEVICE: {
                    FreeDeviceAddr(static_cast<void *>(vencChannelDesc));
                    break;
                }
                default: {
                    ACL_LOG_INNER_ERROR("[Check][ChannelDesc]aclvencDestroyChannelDesc failed, "
                        "unkown acl run mode %d.", aclRunMode_);
                    return ACL_ERROR_INTERNAL_ERROR;
                }
            }

            ACL_LOG_INFO("aclvencDestroyChannelDesc success.");
            return ACL_SUCCESS;
        }

        aclError VideoProcessorV200::aclvencMallocOutMemory(aclvencChannelDesc *channelDesc)
        {
            ACL_LOG_INFO("aclvencMallocOutMemory begin.");

            acldvppStreamDesc *output = acldvppCreateStreamDesc();
            ACL_REQUIRES_NOT_NULL(output);
            channelDesc->outStreamDesc = output;
            if (output->dataBuffer.data == nullptr) {
                ACL_LOG_INNER_ERROR("[Check][DataBuffer]output dvpp stream desc dataBuffer data is null.");
                (void)acldvppDestroyStreamDesc(output);
                channelDesc->outStreamDesc = nullptr;
                return ACL_ERROR_RT_FAILURE;
            }

            if ((channelDesc->bufAddr == 0 && channelDesc->bufSize != 0) ||
                (channelDesc->bufAddr != 0 && channelDesc->bufSize == 0)) {
                ACL_LOG_INNER_ERROR("[Check][DataBuffer]bufAddr and bufSize should be set together.");
                (void)acldvppDestroyStreamDesc(output);
                channelDesc->outStreamDesc = nullptr;
                return ACL_ERROR_RT_FAILURE;
            }

            if (channelDesc->bufAddr != 0) {
                ACL_LOG_INFO("bufAddr is already set, bufAddr = %llu", channelDesc->bufAddr);
                return ACL_SUCCESS;
            }

            void *outData = nullptr;
            aclError rtErr = acldvppMalloc(&outData, VENC_DEFAULT_BUFF_SIZE);
            if (rtErr != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Malloc][Mem]alloc device memory failed, size = %u, runtime result = %d",
                    VENC_DEFAULT_BUFF_SIZE, rtErr);
                (void)acldvppDestroyStreamDesc(output);
                channelDesc->outStreamDesc = nullptr;
                return ACL_GET_ERRCODE_RTS(rtErr);
            }

            channelDesc->bufAddr = reinterpret_cast<uintptr_t>(outData);
            channelDesc->bufSize = VENC_DEFAULT_BUFF_SIZE;
            channelDesc->vencDesc.bufAddr = channelDesc->bufAddr;
            channelDesc->vencDesc.bufSize = channelDesc->bufSize;
            ACL_LOG_INFO("aclvencMallocOutMemory end");
            return ACL_SUCCESS;
        }

        void VideoProcessorV200::aclvencFreeOutMemory(aclvencChannelDesc *channelDesc)
        {
            ACL_LOG_INFO("aclvencFreeOutMemory begin");
            void *outData = reinterpret_cast<void *>(static_cast<uintptr_t>(channelDesc->bufAddr));

            if (outData != nullptr && channelDesc->outputMemMode != VENC_USER_MODE) {
                ACL_LOG_INFO("aclrtFree begin");
                aclError ret = aclrtFree(outData);
                if (ret != ACL_SUCCESS) {
                    ACL_LOG_CALL_ERROR("[Free][Mem]free device mem failed, result = %d", ret);
                }
                outData = nullptr;
                ACL_LOG_INFO("aclrtFree end");
            }

            (void)acldvppDestroyStreamDesc(channelDesc->outStreamDesc);
            channelDesc->outStreamDesc = nullptr;
            ACL_LOG_INFO("aclvencFreeOutMemory end");
        }

        void VideoProcessorV200::GetVencFrameCallbackV200(void *callbackData)
        {
            ACL_LOG_INFO("GetVencFrameCallbackV200 begin");
            auto addr = reinterpret_cast<uintptr_t>(callbackData);
            auto info = aicpu::dvpp::CallbackInfoManager<aicpu::dvpp::VencCallbackInfoPtr>::Instance().Take(addr);
            if (info == nullptr) {
                ACL_LOG_INNER_ERROR("[Check][Info]GetVencFrameCallbackV200 callbackData = %p, but no info found in "
                    "CallbackInfoManager.", callbackData);
                return;
            }

            if (aclRunMode_ == ACL_HOST) {
                // copy outputPicDesc to host
                size_t size = sizeof(aicpu::dvpp::DvppStreamDesc);
                aclError ret = rtMemcpy(&(info->outputStreamDesc->dvppStreamDesc), size,
                    info->outputStreamDesc->dataBuffer.data,
                    info->outputStreamDesc->dataBuffer.length, RT_MEMCPY_DEVICE_TO_HOST);
                if (ret != ACL_SUCCESS) {
                    ACL_LOG_INNER_ERROR("[Copy][Mem]memcpy output stream desc to host memory failed, "
                        "size = %zu, result = %d", size, ret);
                    return;
                }

                ACL_LOG_INFO("rtMemcpy success, timestamp:%llu, data:%llu, size:%u, format:%u, retCode:%u.",
                    info->outputStreamDesc->dvppStreamDesc.timestamp, info->outputStreamDesc->dvppStreamDesc.data,
                    info->outputStreamDesc->dvppStreamDesc.size, info->outputStreamDesc->dvppStreamDesc.format,
                    info->outputStreamDesc->dvppStreamDesc.retCode);
            }

            // check user callback func
            if (info->callbackFunc == nullptr) {
                ACL_LOG_INNER_ERROR("[Check][callbackFunc]call back func is null!");
                return;
            }
            // call user callback func
            aclvencCallback callbackFunc = info->callbackFunc;
            (*callbackFunc)(info->inputPicDesc, info->outputStreamDesc, info->callbackData);

            ACL_LOG_INFO("GetVencFrameCallbackV200 success");
            return;
        }

        aclError VideoProcessorV200::LaunchVencSendFrameTask(const aclvencChannelDesc *channelDesc,
            acldvppPicDesc *inputPicDesc, aclvencFrameConfig *config)
        {
            constexpr int32_t ioAddrNum = 2;
            uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t) +
                acl::dvpp::CalAclDvppStructSize(&config->vencFrameConfig);
            std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
            ACL_REQUIRES_NOT_NULL(args);
            auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
            paramHead->length = argsSize;
            paramHead->ioAddrNum = ioAddrNum;
            auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
            bool eos = config->vencFrameConfig.eos;
            ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
            ioAddr[1] = eos ? 0 : reinterpret_cast<uintptr_t>(inputPicDesc->dataBuffer.data);

            constexpr uint32_t configOffset = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
            auto memcpyRet = memcpy_s(args.get() + configOffset, argsSize - configOffset, &(config->vencFrameConfig),
                acl::dvpp::CalAclDvppStructSize(&config->vencFrameConfig));
            if (memcpyRet != EOK) {
                ACL_LOG_INNER_ERROR("[Copy][FrameConfig]copy venc frame config to args failed, "
                    "result = %d.", memcpyRet);
                return ACL_ERROR_FAILURE;
            }

            ACL_LOG_INFO("begin to send frame to device, eos = %d.", eos);
            rtError_t rtRet = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
                acl::dvpp::DVPP_KERNELNAME_VENC_SEND_FRAME,
                1, // blockDim default 1
                args.get(), argsSize,
                nullptr, // no need smDesc
                channelDesc->sendFrameStream);
            ACL_LOG_INFO("end to send frame to device.");
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Launch][Kernel]venc send frame task call rtCpuKernelLaunch failed, "
                    "runtime result = %d.", rtRet);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }
            return ACL_SUCCESS;
        }

        aclError VideoProcessorV200::LaunchReleaseFrameTask(const aclvencChannelDesc *channelDesc)
        {
            constexpr int32_t ioAddrNum = 1;

            constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
            std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
            auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
            ACL_REQUIRES_NOT_NULL(args);
            paramHead->length = argsSize;
            paramHead->ioAddrNum = ioAddrNum;
            auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
            ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);

            rtError_t rtRet = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
                acl::dvpp::DVPP_KERNELNAME_VENC_RELEASE_FRAME,
                1, // blockDim default 1
                args.get(), argsSize,
                nullptr, // no need smDesc
                channelDesc->getFrameStream);
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Launch][Task]release venc frame task call rtCpuKernelLaunch failed, "
                    "runtime result = %d.", rtRet);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }
            return ACL_SUCCESS;
        }

        aclError VideoProcessorV200::LaunchVencWaitTask(aclvencChannelDesc *channelDesc)
        {
            rtError_t sendFrameWaitRet;
            rtError_t getFrameWaitRet;
            if (channelDesc->vencWaitTaskType == NOTIFY_TASK) {
                // launch wait task to send frame stream
                sendFrameWaitRet = rtNotifyWait(static_cast<rtNotify_t>(channelDesc->sendFrameNotify),
                    channelDesc->sendFrameStream);
                if (sendFrameWaitRet != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Wait][Notify]wait for a notify to send frame stream failed, "
                        "runtime result = %d", sendFrameWaitRet);
                    return ACL_GET_ERRCODE_RTS(sendFrameWaitRet);
                }

                // launch wait task to get frame stream
                getFrameWaitRet = rtNotifyWait(static_cast<rtNotify_t>(channelDesc->getFrameNotify),
                    channelDesc->getFrameStream);
                if (getFrameWaitRet != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Wait][Notify]wait for a notify to get frame stream failed, "
                        "runtime result = %d", getFrameWaitRet);
                    return ACL_GET_ERRCODE_RTS(getFrameWaitRet);
                }
            } else {
                sendFrameWaitRet = rtStreamWaitEvent(channelDesc->sendFrameStream,
                    static_cast<rtEvent_t>(channelDesc->sendFrameNotify));
                if (sendFrameWaitRet != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Wait][Event]wait for a event to send frame stream failed, runtime result = %d",
                        sendFrameWaitRet);
                    return ACL_GET_ERRCODE_RTS(sendFrameWaitRet);
                }
                sendFrameWaitRet = rtEventReset(static_cast<rtEvent_t>(channelDesc->sendFrameNotify),
                    channelDesc->sendFrameStream);
                if (sendFrameWaitRet != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Reset][Event]reset a event to send frame stream failed, runtime result = %d",
                        sendFrameWaitRet);
                    return ACL_GET_ERRCODE_RTS(sendFrameWaitRet);
                }

                getFrameWaitRet = rtStreamWaitEvent(channelDesc->getFrameStream,
                    static_cast<rtEvent_t>(channelDesc->getFrameNotify));
                if (getFrameWaitRet != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Wait][Event]wait for a event to get frame stream failed, runtime result = %d",
                        getFrameWaitRet);
                    return ACL_GET_ERRCODE_RTS(getFrameWaitRet);
                }
                getFrameWaitRet = rtEventReset(static_cast<rtEvent_t>(channelDesc->getFrameNotify),
                    channelDesc->getFrameStream);
                if (getFrameWaitRet != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Reset][Event]reset a event to get frame stream failed, runtime result = %d",
                        getFrameWaitRet);
                    return ACL_GET_ERRCODE_RTS(getFrameWaitRet);
                }
            }

            return ACL_SUCCESS;
        }

        aclError VideoProcessorV200::aclvencSendNomalFrame(aclvencChannelDesc *channelDesc, acldvppPicDesc *input,
            void *reserve, aclvencFrameConfig *config, void *userdata)

        {
            ACL_REQUIRES_NOT_NULL(input);
            ACL_REQUIRES_NOT_NULL(input->dataBuffer.data);
            // choose memory mode
            if (aclRunMode_ == ACL_HOST) {
                // copy input pic desc to device
                size_t size = sizeof(aicpu::dvpp::DvppPicDesc);
                rtError_t memcpyRet = rtMemcpy(input->dataBuffer.data, input->dataBuffer.length,
                    static_cast<const void *>(&input->dvppPicDesc), size, RT_MEMCPY_HOST_TO_DEVICE);
                if (memcpyRet != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Copy][Mem]memcpy input pic desc to device memory failed, "
                        "size = %zu, runtime result = %d", size, memcpyRet);
                    return ACL_GET_ERRCODE_RTS(memcpyRet);
                }
            }

            // launch send frame task to send frame stream
            aclError sendFrameRet = LaunchVencSendFrameTask(channelDesc, input, config);
            if (sendFrameRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Launch][Task]launch send frame task failed, result = %d", sendFrameRet);
                return sendFrameRet;
            }

            aclError launchWaitTaskRet = LaunchVencWaitTask(channelDesc);
            if (launchWaitTaskRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Launch][Task]launch wait task failed, result = %d", launchWaitTaskRet);
                return launchWaitTaskRet;
            }

            // create callback info
            aicpu::dvpp::VencCallbackInfoPtr callbackInfoPtr;
            try {
                callbackInfoPtr = std::make_shared<aicpu::dvpp::VencGetFrameCallbackInfo>(channelDesc->callback, input,
                    channelDesc->outStreamDesc, channelDesc->outputMemMode, userdata);
            } catch (...) {
                ACL_LOG_INNER_ERROR("[Make][Shared]Make shared for get frame callback info failed.");
                return ACL_ERROR_BAD_ALLOC;
            }
            // save callback info
            auto addr = reinterpret_cast<uintptr_t>(callbackInfoPtr.get());
            // Insert return code is always true, no need to check
            (void)aicpu::dvpp::CallbackInfoManager<aicpu::dvpp::VencCallbackInfoPtr>::Instance().Insert(addr,
                callbackInfoPtr);
            // launch callback task to get frame stream
            rtError_t callbackRet =
                rtCallbackLaunch(GetVencFrameCallbackV200, callbackInfoPtr.get(), channelDesc->getFrameStream, true);
            if (callbackRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Launch][Callback]launch venc callback to get frame stream failed, "
                    "runtime result = %d", callbackRet);
                (void)aicpu::dvpp::CallbackInfoManager<aicpu::dvpp::VencCallbackInfoPtr>::Instance().Erase(addr);
                return ACL_GET_ERRCODE_RTS(callbackRet);
            }

            // launch release frame task to get frame stream
            aclError releaseFrameRet = LaunchReleaseFrameTask(channelDesc);
            if (releaseFrameRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Launch][Venc]launch venc release frame task failed, "
                    "result = %d", releaseFrameRet);
                return releaseFrameRet;
            }

            // streamSynchronize send frame stream
            rtError_t streamSynRet = rtStreamSynchronize(channelDesc->sendFrameStream);
            if (streamSynRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Sync][Stream]fail to synchronize venc sendFrameStream, "
                    "runtime result = %d", streamSynRet);
                (void)aicpu::dvpp::CallbackInfoManager<aicpu::dvpp::VencCallbackInfoPtr>::Instance().Erase(addr);
                return ACL_GET_ERRCODE_RTS(streamSynRet);
            }
            ACL_LOG_INFO("aclvencSendFrame success.");
            return ACL_SUCCESS;
        }

        aclError VideoProcessorV200::aclvencSendEosFrame(aclvencChannelDesc *channelDesc, aclvencFrameConfig *config)
        {
            // synchronize getFrameStream
            ACL_LOG_INFO("rtStreamSynchronize get frame begin.");
            rtError_t rtRet = rtStreamSynchronize(channelDesc->getFrameStream);
            if (rtRet != ACL_SUCCESS) {
                ACL_LOG_CALL_ERROR("[Sync][Stream]fail to synchronize getFrameStream, result = %d", rtRet);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }
            ACL_LOG_INFO("rtStreamSynchronize get frame end.");
            return ACL_SUCCESS;
        }

        aclError VideoProcessorV200::SendEosForVenc(aclvencChannelDesc *channelDesc)
        {
            // send eos and sychronize
            aclError ret = aclvencSendEosFrame(channelDesc, nullptr);
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Send][EosFrame]fail to send eos, result = %d", ret);
                return ret;
            }
            return ACL_SUCCESS;
        }

        void VideoProcessorV200::SetVencWaitTaskType(aclvencChannelDesc *channelDesc)
        {
            channelDesc->vencWaitTaskType = EVENT_TASK;
            ACL_LOG_INFO("venc wait task type is event");
        }

        void VideoProcessorV200::SetVdecWaitTaskType(aclvdecChannelDesc *channelDesc)
        {
            channelDesc->vdecWaitTaskType = EVENT_TASK;
            ACL_LOG_INFO("vdec wait task type is event");
        }

        /**
         * Set rc model for venc channel desc.
         * @param channelDesc[IN/OUT] venc channel desc
         * @param rcMode[IN] venc rc mode(VBR=1, CBR=2)
         * @return ACL_SUCCESS for success, other for failure
         */
        aclError VideoProcessorV200::aclvencSetChannelDescRcMode(aclvencChannelDesc *channelDesc,
                                                                 uint32_t rcMode)
        {
            // rcMode value
            if (rcMode > 2) {
                ACL_LOG_INNER_ERROR("rcMode %u, rcMode must be set to 0, 1, or 2.", rcMode);
                return ACL_ERROR_INVALID_PARAM;
            }
            return VideoProcessor::aclvencSetChannelDescRcMode(channelDesc, rcMode);
        }

        /**
         * VDEC set refFrameNum for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param refFrameNum[in] refFrameNum
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError VideoProcessorV200::aclvdecSetChannelDescRefFrameNum(aclvdecChannelDesc *channelDesc,
                                                                      uint32_t refFrameNum)
        {
            ACL_LOG_DEBUG("start to execute aclvdecSetChannelDescRefFrameNum");
            ACL_REQUIRES_NOT_NULL(channelDesc);

            channelDesc->vdecDesc.refFrameNum = refFrameNum;
            return ACL_SUCCESS;
        }

        /**
         * VDEC get refFrameNum for channel desc.
         * @param channelDesc[in] channel desc
         * @retval number of reference frames, default 0.
         */
        uint32_t VideoProcessorV200::aclvdecGetChannelDescRefFrameNum(const aclvdecChannelDesc *channelDesc)
        {
            ACL_LOG_DEBUG("start to execute aclvdecGetChannelDescRefFrameNum");
            if (channelDesc == nullptr) {
                ACL_LOG_ERROR("[Check][channelDesc]channelDesc is null");
                const char *argList[] = {"param"};
                const char *argVal[] = {"channelDesc"};
                acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                    argList, argVal, 1);
                return 0; // default 0
            }

            return channelDesc->vdecDesc.refFrameNum;
        }

        /**
         * VDEC set outPicFormat for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param outPicFormat[in] outPicFormat
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError VideoProcessorV200::aclvdecSetChannelDescOutPicFormat(aclvdecChannelDesc *channelDesc,
                                                                       acldvppPixelFormat outPicFormat)
        {
            ACL_LOG_DEBUG("start to execute aclvdecSetChannelDescOutPicFormat");
            if (channelDesc == nullptr) {
                ACL_LOG_ERROR("[Check][ChannelDesc]channelDesc is null.");
                const char *argList[] = {"param"};
                const char *argVal[] = {"channelDesc"};
                acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                    argList, argVal, 1);
                return ACL_ERROR_INVALID_PARAM;
            }
            if (validOutPicFormatSet.find(outPicFormat) == validOutPicFormatSet.end()) {
                ACL_LOG_INNER_ERROR("[Find][outPicFormat]the value of outPicFormat[%d] is "
                    "not supported in this version", static_cast<int32_t>(outPicFormat));
                return ACL_ERROR_INVALID_PARAM;
            }

            channelDesc->vdecDesc.outPicFormat = static_cast<uint32_t>(outPicFormat);
            return ACL_SUCCESS;
        }

        /**
         * VDEC set channel id for channel desc.
         * @param channelDesc[in|out] channel desc
         * @param channelId[in] channel id
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError VideoProcessorV200::aclvdecSetChannelDescChannelId(aclvdecChannelDesc *channelDesc,
                                                                    uint32_t channelId)
        {
            ACL_LOG_DEBUG("start to execute aclvdecSetChannelDescChannelId");
            if (channelDesc == nullptr) {
                ACL_LOG_ERROR("[Check][channelDesc]channelDesc is null.");
                const char *argList[] = {"param"};
                const char *argVal[] = {"channelDesc"};
                acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                    argList, argVal, 1);
                return ACL_ERROR_INVALID_PARAM;
            }

            if ((channelId < V200_CHANNEL_ID_FLOOR) || (channelId > V200_CHANNEL_ID_CEILING)) {
                ACL_LOG_INNER_ERROR("[Check][channelId]the value of channelId[%u] is invalid, "
                    "it should be between in [%d, %d]", channelId, V200_CHANNEL_ID_FLOOR,
                    V200_CHANNEL_ID_CEILING);
                return ACL_ERROR_INVALID_PARAM;
            }

            channelDesc->vdecDesc.channelId = channelId;
            return ACL_SUCCESS;
        }

        /**
         * VDEC set bit depth for channel desc.
         * @param channelDesc [IN]     vdec channel description.
         * @param bitDepth [IN]     bit depth.
         * @return ACL_SUCCESS for ok, others for fail
         */
        aclError VideoProcessorV200::aclvdecSetChannelDescBitDepth(aclvdecChannelDesc *channelDesc, uint32_t bitDepth)
        {
            ACL_LOG_DEBUG("start to execute aclvdecSetChannelDescBitDepth, bitDepth %u", bitDepth);
            ACL_REQUIRES_NOT_NULL(channelDesc);
            ACL_REQUIRES_NOT_NULL(channelDesc->vdecDesc.extendInfo);
            {
                std::unique_lock<std::mutex> lock{channelDesc->mutexForTLVMap};
                auto it = channelDesc->tlvParamMap.find(VDEC_BIT_DEPTH);
                if (it == channelDesc->tlvParamMap.end()) {
                    VdecChannelDescTLVParam vdecTLVParam;
                    std::shared_ptr<aicpu::dvpp::DvppVdecBitDepthConfig> dvppVdecBitDepthConfig =
                        std::make_shared<aicpu::dvpp::DvppVdecBitDepthConfig>();
                    ACL_REQUIRES_NOT_NULL(dvppVdecBitDepthConfig);
                    dvppVdecBitDepthConfig->bitDepth = bitDepth;
                    vdecTLVParam.value = static_cast<std::shared_ptr<void>>(dvppVdecBitDepthConfig);
                    vdecTLVParam.valueLen = sizeof(aicpu::dvpp::DvppVdecBitDepthConfig);
                    channelDesc->tlvParamMap[VDEC_BIT_DEPTH] = vdecTLVParam;
                } else {
                    aicpu::dvpp::DvppVdecBitDepthConfig *dvppVdecBitDepthConfig =
                        static_cast<aicpu::dvpp::DvppVdecBitDepthConfig *>(it->second.value.get());
                    ACL_REQUIRES_NOT_NULL(dvppVdecBitDepthConfig);
                    dvppVdecBitDepthConfig->bitDepth = bitDepth;
                }
            }
            return ACL_SUCCESS;
        }

        /**
         * VDEC get bit depth for channel desc.
         * @param channelDesc [IN]    vdec channel description.
         * @retval bit depth， default 0.
         */
        uint32_t VideoProcessorV200::aclvdecGetChannelDescBitDepth(const aclvdecChannelDesc *channelDesc)
        {
            ACL_LOG_DEBUG("start to execute aclvdecGetChannelDescBitDepth");
            if ((channelDesc == nullptr) || (channelDesc->vdecDesc.extendInfo == nullptr)) {
                ACL_LOG_INNER_ERROR("[Check][channelDesc]vdec channelDesc or vdec channelDesc extendInfo is null.");
                return 0;
            }
            {
                std::mutex &mutexMap = const_cast<std::mutex &>(channelDesc->mutexForTLVMap);
                std::unique_lock<std::mutex> lock{mutexMap};
                const auto &it = channelDesc->tlvParamMap.find(VDEC_BIT_DEPTH);
                if (it == channelDesc->tlvParamMap.end()) {
                    return 0;
                }

                aicpu::dvpp::DvppVdecBitDepthConfig *dvppVdecBitDepthConfig =
                    static_cast<aicpu::dvpp::DvppVdecBitDepthConfig *>(it->second.value.get());
                if (dvppVdecBitDepthConfig == nullptr) {
                    ACL_LOG_INNER_ERROR("[Check][VencRateControl]vencRateControl ptr is null.");
                    return 0;
                }
                ACL_LOG_DEBUG("successfully execute aclvdecGetChannelDescBitDepth");
                return dvppVdecBitDepthConfig->bitDepth;
            }
        }

        aclError VideoProcessorV200::LaunchVdecReleaseFrameTask(const aclvdecChannelDesc *channelDesc)
        {
            // release frame task has 1 input
            constexpr uint32_t ioAddrNum = 1;
            constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
            char args[argsSize] = {0};
            auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args);
            paramHead->length = argsSize;
            paramHead->ioAddrNum = ioAddrNum;
            auto ioAddr = reinterpret_cast<uint64_t *>(args + sizeof(aicpu::AicpuParamHead));
            ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
            rtError_t rtRet = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
                                                acl::dvpp::DVPP_KERNELNAME_VDEC_RELEASE_FRAME,
                                                1, // blockDim default 1
                                                args,
                                                argsSize,
                                                nullptr, // no need smDesc
                                                channelDesc->getFrameStream);
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Launch][Kernel]vdec release frame task call rtCpuKernelLaunch failed, "
                    "runtime result = %d.", rtRet);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }
            return ACL_SUCCESS;
        }

        void VideoProcessorV200::GetVdecFrameCallbackV200(void *callbackData)
        {
            aclvdecChannelDesc *channelDesc =  reinterpret_cast<aclvdecChannelDesc *>(callbackData);
            if ((channelDesc == nullptr) || (channelDesc->shareBuffer.data == nullptr)) {
                ACL_LOG_ERROR("[Check][Params]GetVdecFrameCallback callbackData = %p, but channelDesc "
                    "or shareBuffer is null.", callbackData);
                const char *argList[] = {"param"};
                const char *argVal[] = {"channelDesc or shareBuffer"};
                acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                    argList, argVal, 1);
                return;
            }

            uint32_t channelId = channelDesc->vdecDesc.channelId;
            // get vdec callback info
            acldvppStreamDesc *input = nullptr;
            acldvppPicDesc *output = nullptr;
            void *userData = nullptr;
            bool eos = false;
            aclError aclRet = GetVdecCallInfo(channelDesc, input, output, userData, eos);
            if (aclRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Get][VdecCallInfo]get vdec callback info failed, channelId = %u.", channelId);
                return;
            }

            if (eos) {
                std::unique_lock<std::mutex> lock{channelDesc->mutexForQueue};
                ACL_LOG_INFO("Begin to process callback for eos, channelId = %u.", channelId);
                channelDesc->eosBackFlag.store(true);
                channelDesc->condVarForEos.notify_one();
                ACL_LOG_INFO("Finish to process callback for eos, channelId = %u.", channelId);
                return;
            }

            // launch task for get stream
            aicpu::dvpp::VdecCallbackInfoPtr nextCallbackInfo = nullptr;
            {
                std::lock_guard<std::mutex> queueLock(channelDesc->mutexForQueue);
                ACL_LOG_INFO("task queue size is %zu, channelId = %u.", channelDesc->taskQueue.size(), channelId);
                if (channelDesc->taskQueue.empty()) {
                    channelDesc->queueEmptyFlag.store(true);
                } else {
                    nextCallbackInfo = channelDesc->taskQueue.front();
                    channelDesc->taskQueue.pop();
                }
            }

            // launch task to get stream
            if (nextCallbackInfo != nullptr) {
                rtContext_t curContext = nullptr;
                // get current context
                rtError_t getCtxRet = rtCtxGetCurrent(&curContext);
                if (getCtxRet != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Get][curContext]get ctx failed, runtime result = %d, "
                        "channelId = %u.", getCtxRet, channelId);
                    return;
                }
                // set context of vdec main thread to current callback thread
                rtError_t setCtxRet = rtCtxSetCurrent(channelDesc->vdecMainContext);
                if (setCtxRet != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Set][curContext]set ctx failed, runtime result = %d, "
                        "channelId = %u.", setCtxRet, channelId);
                    return;
                }

                aclError launchRet = LaunchTaskForGetStream(channelDesc, nextCallbackInfo);
                if (launchRet != ACL_SUCCESS) {
                    ACL_LOG_INNER_ERROR("[Launch][Task]launch task for get stream failed.");
                    (void) rtCtxSetCurrent(curContext);
                    return;
                }
                // reset context of current callback thread
                rtError_t resetCtxRet = rtCtxSetCurrent(curContext);
                if (resetCtxRet != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Set][curContext]set ctx failed, runtime result = %d, "
                        "channelId = %u.", resetCtxRet, channelId);
                    return;
                }
            }

            // check user callback func
            if (channelDesc->callback == nullptr) {
                ACL_LOG_INNER_ERROR("[Check][Callback]call back func is null!");
                return;
            }
            // call user callback func
            (*channelDesc->callback)(input, output, userData);
        }

        aclError VideoProcessorV200::LaunchTaskForGetStream(aclvdecChannelDesc *channelDesc,
            aicpu::dvpp::VdecCallbackInfoPtr callbackInfoPtr)
        {
            static uint32_t launchTaskCount[V200_CHANNEL_ID_CEILING + 1] = {0};
            rtError_t getFrameWaitRet;
            if (channelDesc->vdecWaitTaskType == NOTIFY_TASK) {
                // launch notify task to release frame stream
                getFrameWaitRet = rtNotifyWait(static_cast<rtNotify_t>(channelDesc->getFrameNotify),
                    channelDesc->getFrameStream);
                if (getFrameWaitRet != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Wait][Notify]wait for a notify to release frame stream failed, "
                        "runtime result = %d", getFrameWaitRet);
                    return ACL_GET_ERRCODE_RTS(getFrameWaitRet);
                }
            } else {
                // launch event task to release frame stream
                getFrameWaitRet = rtStreamWaitEvent(channelDesc->getFrameStream,
                    static_cast<rtEvent_t>(channelDesc->getFrameNotify));
                if (getFrameWaitRet != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Wait][Event]wait for a event to release frame stream failed, "
                        "runtime result = %d", getFrameWaitRet);
                    return ACL_GET_ERRCODE_RTS(getFrameWaitRet);
                }
                getFrameWaitRet = rtEventReset(static_cast<rtEvent_t>(channelDesc->getFrameNotify),
                    channelDesc->getFrameStream);
                if (getFrameWaitRet != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Reset][Event]reset a event to release frame stream failed, "
                        "runtime result = %d", getFrameWaitRet);
                    return ACL_GET_ERRCODE_RTS(getFrameWaitRet);
                }
            }

            // launch callback task to release frame stream
            rtError_t callbackRet = rtCallbackLaunch(GetVdecFrameCallbackV200,
                                                     static_cast<void *>(channelDesc),
                                                     channelDesc->getFrameStream, true);
            if (callbackRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Launch][Callback]launch callback to release frame stream failed, "
                    "runtime result = %d", callbackRet);
                return ACL_GET_ERRCODE_RTS(callbackRet);
            }

            if (!callbackInfoPtr->eos) {
                // launch release frame task to release frame stream
                aclError releaseFrameRet = LaunchVdecReleaseFrameTask(channelDesc);
                if (releaseFrameRet != ACL_SUCCESS) {
                    ACL_LOG_INNER_ERROR("[Launch][Task]launch release frame task failed, result = %d", releaseFrameRet);
                    return releaseFrameRet;
                }
            }

            uint32_t channelId = channelDesc->vdecDesc.channelId;
            ACL_LOG_INFO("Launch task for get stream success. launchTaskCount = %u, channelId = %u, get streamId = %u.",
                ++launchTaskCount[channelId], channelId, channelDesc->getStreamId);
            return ACL_SUCCESS;
        }

        aclError VideoProcessorV200::aclvdecSendFrame(aclvdecChannelDesc *channelDesc,
                                                      acldvppStreamDesc *input,
                                                      acldvppPicDesc *output,
                                                      aclvdecFrameConfig *config,
                                                      void *userData,
                                                      bool isSkipFlag)
        {
            ACL_LOG_INFO("start to execute aclvdecSendFrame, channelId=%u, isSkipFlag=%d.",
                channelDesc->vdecDesc.channelId, static_cast<int32_t>(isSkipFlag));
            ACL_REQUIRES_NOT_NULL(channelDesc);
            ACL_REQUIRES_NOT_NULL(channelDesc->callback);
            ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
            ACL_REQUIRES_NOT_NULL(channelDesc->sendFrameStream);
            ACL_REQUIRES_NOT_NULL(channelDesc->getFrameStream);

            // channel range : [0, V200_CHANNEL_ID_CEILING]
            static uint32_t sendFrameCount[V200_CHANNEL_ID_CEILING + 1];
            auto channelId = channelDesc->vdecDesc.channelId;
            sendFrameCount[channelId]++;

            aclError checkRet = CheckAndCopyVdecInfoData(input, output, isSkipFlag);
            if (checkRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[CheckAndCopy][VdecInfoData]check and copy stream desc "
                    "or pic desc failed, result = %d", checkRet);
                return checkRet;
            }

            bool eos = input->dvppStreamDesc.eos;
            uint64_t frameId = 0;
            if (!eos) {
                frameId = ++channelDesc->frameId;
                ACL_LOG_DEBUG("vdec process data frame: channelId = %u, frameId = %lu",
                    channelDesc->vdecDesc.channelId, frameId);
            }
            aclError launchRet = LaunchTaskForSendStream(channelDesc, input, output, eos);
            if (launchRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Launch][Task]launch tasks for send stream failed, result = %d", launchRet);
                return launchRet;
            }

            aicpu::dvpp::VdecCallbackInfoPtr callbackInfoPtr = nullptr;
            try {
                callbackInfoPtr = std::make_shared<aicpu::dvpp::VdecGetFrameCallbackInfo>(
                    input, output, userData, eos);
            } catch (std::bad_alloc &) {
                ACL_LOG_INNER_ERROR("[Make][Shared]Make shared for get frame callback info failed.");
                return ACL_ERROR_BAD_ALLOC;
            }

            {
                std::unique_lock<std::mutex> lock{channelDesc->mutexForCallbackMap};
                channelDesc->callbackMap[frameId] = callbackInfoPtr;
            }

            // only launch task for get stream when frame counter != 0
            bool needLaunchTaskForGetStream = false;
            {
                std::lock_guard<std::mutex> queueLock(channelDesc->mutexForQueue);
                if (channelDesc->queueEmptyFlag.load()) {
                    needLaunchTaskForGetStream = true;
                    channelDesc->queueEmptyFlag.store(false);
                } else {
                    // save get frame task info to queue of current channel
                    channelDesc->taskQueue.push(callbackInfoPtr);
                    ACL_LOG_INFO("channelId=%u, task queue size is %zu.",
                        channelDesc->vdecDesc.channelId, channelDesc->taskQueue.size());
                }
            }

            if (needLaunchTaskForGetStream) {
                ACL_LOG_INFO("launch tasks in get stream only for first frame.");
                aclError launchRet = LaunchTaskForGetStream(channelDesc, callbackInfoPtr);
                if (launchRet != ACL_SUCCESS) {
                    {
                        std::unique_lock<std::mutex> lock{channelDesc->mutexForCallbackMap};
                        channelDesc->callbackMap.erase(frameId);
                    }
                    ACL_LOG_INNER_ERROR("[Launch][Tasks]launch tasks for get stream failed, result = %d", launchRet);
                    return launchRet;
                }
            }

            // streamSynchronize vdec send frame stream
            rtError_t streamSynRet = rtStreamSynchronize(channelDesc->sendFrameStream);
            if (streamSynRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Sync][Stream]vdec fail to synchronize sendFrameStream, "
                    "runtime result = %d", streamSynRet);
                {
                    std::unique_lock<std::mutex> lock{channelDesc->mutexForCallbackMap};
                    channelDesc->callbackMap.erase(frameId);
                }
                return ACL_GET_ERRCODE_RTS(streamSynRet);
            }

            // eos has no callback task
            if (eos) {
                ACL_LOG_INFO("begin to synchronize get stream for eos, channelId = %u.",
                    channelDesc->vdecDesc.channelId);
                std::unique_lock<std::mutex> lock{channelDesc->mutexForQueue};
                // check eos back flag
                while (!channelDesc->eosBackFlag.load()) {
                    ACL_LOG_INFO("eos wait, channelId=%u.", channelDesc->vdecDesc.channelId);
                    channelDesc->condVarForEos.wait(lock);
                }
                ACL_LOG_INFO("finish to synchronize get stream for eos, channelId = %u.",
                             channelDesc->vdecDesc.channelId);
                // must clear cached tasks, especially for eos, reset eosBackFlag and queueEmptyFlag
                ClearCachedTasks(channelDesc);
                return ACL_SUCCESS;
            }

            ACL_LOG_INFO("end to send frame. channelId = %u, frame number = %u, sendNtfId = %u, "
                "getNtfId = %u, sendStreamId = %d, getStreamId = %d.", channelId, sendFrameCount[channelId],
                channelDesc->vdecDesc.sendFrameNotifyId, channelDesc->vdecDesc.getFrameNotifyId,
                channelDesc->sendStreamId, channelDesc->getStreamId);
            return ACL_SUCCESS;
        }
    }
}
