/**
* @file image_processor.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "video_processor.h"

#include <cstddef>
#include "securec.h"
#include "runtime/rt.h"
#include "utils/math_utils.h"
#include "common/log_inner.h"
#include "error_codes_inner.h"
#include "aicpu/dvpp/dvpp_def.h"
#include "toolchain/profiling_manager.h"
#include "toolchain/resource_statistics.h"
#include "aicpu/common/aicpu_task_struct.h"
#include "single_op/dvpp/common/dvpp_util.h"

namespace {
    constexpr uint32_t VENC_NO_INIT = 0;
    constexpr uint32_t VENC_SYSTEM_MODE = 1;
    constexpr uint32_t VENC_USER_MODE = 2;
    constexpr uint8_t SEND_FRAME_EOS = 1;
    constexpr int VDEC_CHANNEL_ID_CEILING = 31;
    constexpr uint32_t VDEC_BIT_DEPTH_STRUCT_SIZE = sizeof(aicpu::dvpp::DvppVdecBitDepthConfig);
}

namespace acl {
    namespace dvpp {
    aclrtRunMode VideoProcessor::aclRunMode_(ACL_HOST);

    void VideoProcessor::SetVdecWaitTaskType(aclvdecChannelDesc *channelDesc)
    {
        channelDesc->vdecWaitTaskType = NOTIFY_TASK;
        ACL_LOG_INFO("vdec wait task type is notify");
    }

    aclError VideoProcessor::VdecCreateChannel(aclvdecChannelDesc *channelDesc, bool isNeedNotify,
        const char *kernelName)
    {
        aclError aclRet = CreateNotifyAndStreamForVdecChannel(channelDesc, isNeedNotify);
        if (aclRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Create][Notify]create notify and stream for vdec channel failed, "
                "result = %d", aclRet);
            return aclRet;
        }

        // subscribe report
        rtError_t rtRetVal = rtSubscribeReport(channelDesc->threadId, channelDesc->getFrameStream);
        if (rtRetVal != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Subscribe][Report]subscribe report failed, result = %d", rtRetVal);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, isNeedNotify);
            return ACL_GET_ERRCODE_RTS(rtRetVal);
        }

        // memcpy data to device (both async and sync copy type are OK for this api)
        size_t size = CalDevDvppStructRealUsedSize(&channelDesc->vdecDesc);
        if (aclRunMode_ == ACL_HOST) {
            rtRetVal = rtMemcpyAsync(channelDesc->dataBuffer.data, channelDesc->dataBuffer.length,
                static_cast<const void *>(&channelDesc->vdecDesc), size,
                RT_MEMCPY_HOST_TO_DEVICE, channelDesc->sendFrameStream);
            if (rtRetVal != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Copy][Mem]memcpy to device memory failed, size = %zu, "
                    "result = %d", size, rtRetVal);
                DestroyAllNotifyAndStreamForVdecChannel(channelDesc, isNeedNotify);
                return ACL_GET_ERRCODE_RTS(rtRetVal);
            }
        }

        // create channel have only 2 input
        constexpr int32_t addrNum = 2;
        constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + addrNum * sizeof(uint64_t);
        char args[argsSize] = {0};
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args);
        paramHead->length = argsSize;
        paramHead->ioAddrNum = addrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        ioAddr[1] = reinterpret_cast<uintptr_t>(channelDesc->shareBuffer.data);

        // launch create channel task
        rtRetVal = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
                                     kernelName,
                                     1,
                                     args,
                                     argsSize,
                                     nullptr,
                                     channelDesc->sendFrameStream);
        if (rtRetVal != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Launch][Kernel]create vdec channel call rtCpuKernelLaunch failed, "
                "runtime result = %d", rtRetVal);
            (void) rtUnSubscribeReport(channelDesc->threadId, channelDesc->getFrameStream);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, isNeedNotify);
            return ACL_GET_ERRCODE_RTS(rtRetVal);
        }

        // stream synchronize
        rtRetVal = rtStreamSynchronize(channelDesc->sendFrameStream);
        if (rtRetVal != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Sync][Stream]fail to synchronize sendFrameStream, runtime result = %d", rtRetVal);
            (void) rtUnSubscribeReport(channelDesc->threadId, channelDesc->getFrameStream);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, isNeedNotify);
            return ACL_GET_ERRCODE_RTS(rtRetVal);
        }
        if (isNeedNotify) {
            ACL_LOG_EVENT("success to call aclvdecCreateChannel, channelId=%u, sendFrameNotifyId=%u, "
                "getFrameNotifyId=%u, sendStreamId=%d, getStreamId=%d.",
                channelDesc->vdecDesc.channelId, channelDesc->vdecDesc.sendFrameNotifyId,
                channelDesc->vdecDesc.getFrameNotifyId, channelDesc->sendStreamId, channelDesc->getStreamId);
        } else {
            ACL_LOG_EVENT("success to call aclvdecCreateChannel, channelId=%u, sendStreamId=%d, getStreamId=%d.",
                channelDesc->vdecDesc.channelId, channelDesc->sendStreamId, channelDesc->getStreamId);
        }

        return ACL_SUCCESS;
    }

    aclError VideoProcessor::aclvdecCreateChannel(aclvdecChannelDesc *channelDesc)
    {
        ACL_LOG_INFO("start to execute aclvdecCreateChannel");
        ACL_REQUIRES_NOT_NULL(channelDesc->callback);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        aclError aclRet = SetVdecParamToVdecChannel(channelDesc);
        if (aclRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Set][VdecParam]set vdec parameter failed, result = %d", aclRet);
            return aclRet;
        }
        // record context of current thread
        rtError_t getCtxRet = rtCtxGetCurrent(&channelDesc->vdecMainContext);
        if (getCtxRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Get][Ctx]get ctx failed, result = %d", getCtxRet);
            return ACL_GET_ERRCODE_RTS(getCtxRet);
        }

        if (channelDesc->isNeedNotify) {
            SetVdecWaitTaskType(channelDesc);
            return VdecCreateChannel(channelDesc, true, DVPP_KERNELNAME_CREATE_VDEC_CHANNEL);
        } else {
            return VdecCreateChannel(channelDesc, false, DVPP_KERNELNAME_CREATE_VDEC_CHANNEL_V2);
        }
    }

    aclError VideoProcessor::VdecDestroyChannel(aclvdecChannelDesc *channelDesc, bool isNeedNotify,
        const char *kernelName)
    {
        // send eos to send stream to finish decode all frame
        aclError aclRet = SendEosForVdec(channelDesc);
        if (aclRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Send][Eos]fail to send eos to sendFrameStream, result = %d.", aclRet);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, isNeedNotify);
            return aclRet;
        }

        // wait finish all callback task
        rtError_t rtRetVal = rtStreamSynchronize(channelDesc->getFrameStream);
        if (rtRetVal != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Sync][Stream]fail to synchronize getFrameStream, runtime result = %d.", rtRetVal);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, isNeedNotify);
            return ACL_GET_ERRCODE_RTS(rtRetVal);
        }

        // destroy channel have 1 input
        constexpr int32_t ioAddrNum = 1;
        constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        char args[argsSize] = {0};
        auto cpuParamHead = reinterpret_cast<aicpu::AicpuParamHead *>(args);
        cpuParamHead->length = argsSize;
        cpuParamHead->ioAddrNum = ioAddrNum;

        auto ioAddr = reinterpret_cast<uint64_t *>(args + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);

        rtRetVal = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
                                     kernelName,
                                     1, // blockDim default 1
                                     args,
                                     argsSize,
                                     nullptr, // no need smDesc
                                     channelDesc->sendFrameStream);
        if (rtRetVal != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Launch][Kernel]vdec destory channel call rtCpuKernelLaunch failed, "
                "runtime result = %d.", rtRetVal);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, isNeedNotify);
            return ACL_GET_ERRCODE_RTS(rtRetVal);
        }

        rtRetVal = rtStreamSynchronize(channelDesc->sendFrameStream);
        if (rtRetVal != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Sync][Stream]fail to synchronize sendFrameStream, runtime result = %d", rtRetVal);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, isNeedNotify);
            return ACL_GET_ERRCODE_RTS(rtRetVal);
        }

        ACL_LOG_DEBUG("Begin to unsubscribe report.");
        rtRetVal = rtUnSubscribeReport(channelDesc->threadId, channelDesc->getFrameStream);
        if (rtRetVal != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Unsubscribe][Report]fail to UnSubscribeReport thread, runtime result = %d", rtRetVal);
            // no need to return failed, continue to destory resource
        }
        ACL_LOG_DEBUG("end to unsubscribe report.");

        // destory all notify and stream
        DestroyAllNotifyAndStreamForVdecChannel(channelDesc, isNeedNotify);

        return ACL_SUCCESS;
    }

    aclError VideoProcessor::aclvdecDestroyChannel(aclvdecChannelDesc *channelDesc)
    {
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        ACL_LOG_INFO("start to execute aclvdecDestroyChannel, channelId = %u", channelDesc->vdecDesc.channelId);

        if (channelDesc->isNeedNotify) {
            return VdecDestroyChannel(channelDesc, true, DVPP_KERNELNAME_DESTROY_VDEC_CHANNEL);
        } else {
            return VdecDestroyChannel(channelDesc, false, DVPP_KERNELNAME_DESTROY_VDEC_CHANNEL_V2);
        }
    }

    aclError VideoProcessor::CheckAndCopyVdecInfoData(acldvppStreamDesc *input,
        acldvppPicDesc *output, bool isSkipFlag)
    {
        ACL_REQUIRES_NOT_NULL(input);
        ACL_REQUIRES_NOT_NULL(input->dataBuffer.data);

        // eos frame does not support skipping
        if (input->dvppStreamDesc.eos && isSkipFlag) {
            ACL_LOG_INNER_ERROR("[Check][Params]eos frame does not support skipping");
            return ACL_ERROR_FEATURE_UNSUPPORTED;
        }

        // no skipped data frame will be decoded
        if ((!input->dvppStreamDesc.eos) && (!isSkipFlag)) {
            ACL_REQUIRES_NOT_NULL(output);
            ACL_REQUIRES_NOT_NULL(output->dataBuffer.data);
            // copy outputPicDesc to device
            if (aclRunMode_ == ACL_HOST) {
                size_t size = sizeof(aicpu::dvpp::DvppPicDesc);
                rtError_t memcpyOutputRet = rtMemcpy(output->dataBuffer.data, output->dataBuffer.length,
                                                     static_cast<const void *>(&output->dvppPicDesc), size,
                                                     RT_MEMCPY_HOST_TO_DEVICE);
                if (memcpyOutputRet != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Copy][Mem]memcpy output pic desc to device memory failed, size = %zu,"
                        " runtime result = %d.", size, memcpyOutputRet);
                    return ACL_GET_ERRCODE_RTS(memcpyOutputRet);
                }
            }
        }

        // copy inputStreamDesc to device
        ACL_LOG_INFO("begin to send frame. frame size is %u.", input->dvppStreamDesc.size);
        if (aclRunMode_ == ACL_HOST) {
            size_t size = sizeof(aicpu::dvpp::DvppStreamDesc);
            rtError_t memcpyInputRet = rtMemcpy(input->dataBuffer.data, input->dataBuffer.length,
                                                static_cast<const void *>(&input->dvppStreamDesc), size,
                                                RT_MEMCPY_HOST_TO_DEVICE);
            if (memcpyInputRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Copy][Mem]memcpy input stream desc to device memory failed, "
                    "size = %zu, runtime result = %d.", size, memcpyInputRet);
                return ACL_GET_ERRCODE_RTS(memcpyInputRet);
            }
        }
        return ACL_SUCCESS;
    }

    aclError VideoProcessor::aclvdecSendFrame(aclvdecChannelDesc *channelDesc,
                                              acldvppStreamDesc *input,
                                              acldvppPicDesc *output,
                                              aclvdecFrameConfig *config,
                                              void *userData,
                                              bool isSkipFlag)
    {
        ACL_LOG_INFO("start to execute aclvdecSendFrame, isSkipFlag = %d.", static_cast<int32_t>(isSkipFlag));
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->callback);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(channelDesc->sendFrameStream);
        ACL_REQUIRES_NOT_NULL(channelDesc->getFrameStream);

        aclError memcpyRet = CheckAndCopyVdecInfoData(input, output, isSkipFlag);
        if (memcpyRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Check][Stream]check and copy stream desc or pic desc failed, "
                "result = %d.", memcpyRet);
            return memcpyRet;
        }

        bool eos = input->dvppStreamDesc.eos;
        uint64_t frameId = 0;
        if (!eos) {
            frameId = ++channelDesc->frameId;
            ACL_LOG_DEBUG("vdec process data frame: channelId = %u, frameId = %lu.",
                channelDesc->vdecDesc.channelId, frameId);
        }
        aclError launchRet = LaunchTaskForSendStream(channelDesc, input, output, eos);
        if (launchRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Launch][Task]launch tasks for send stream failed, result = %d.", launchRet);
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
                ACL_LOG_INFO("task queue size is %zu.", channelDesc->taskQueue.size());
            }
        }

        if (needLaunchTaskForGetStream) {
            ACL_LOG_INFO("launch tasks in get stream only for first frame.");
            aclError launchRet = LaunchTaskForGetStream(channelDesc);
            if (launchRet != ACL_SUCCESS) {
                {
                    std::unique_lock<std::mutex> lock{channelDesc->mutexForCallbackMap};
                    channelDesc->callbackMap.erase(frameId);
                }
                ACL_LOG_INNER_ERROR("[Launch][Tasks]launch tasks for get stream failed, result = %d.", launchRet);
                return launchRet;
            }
        }

        // streamSynchronize send frame stream
        rtError_t streamSynRet = rtStreamSynchronize(channelDesc->sendFrameStream);
        if (streamSynRet != RT_ERROR_NONE) {
            {
                std::unique_lock<std::mutex> lock{channelDesc->mutexForCallbackMap};
                channelDesc->callbackMap.erase(frameId);
            }
            ACL_LOG_CALL_ERROR("[Sync][Stream]fail to synchronize sendFrameStream, runtime result = %d.", streamSynRet);
            return ACL_GET_ERRCODE_RTS(streamSynRet);
        }

        if (eos) {
            ACL_LOG_INFO("begin to synchronize get stream for eos, channelId = %u.", channelDesc->vdecDesc.channelId);
            std::unique_lock<std::mutex> lock{channelDesc->mutexForQueue};
            // check eos back flag
            while (!channelDesc->eosBackFlag.load()) {
                ACL_LOG_INFO("eos wait, channelId=%u.", channelDesc->vdecDesc.channelId);
                channelDesc->condVarForEos.wait(lock);
            }
            ACL_LOG_INFO("finish to synchronize get stream for eos, channelId = %u.", channelDesc->vdecDesc.channelId);
            // must clear cached tasks, especially for eos, reset eosBackFlag and queueEmptyFlag
            ClearCachedTasks(channelDesc);
            return ACL_SUCCESS;
        }

        if (channelDesc->isNeedNotify) {
            ACL_LOG_INFO("end to send frame. frame size=%u, channelId=%u, sendFrameNotifyId=%u, getFrameNotifyId=%u, "
                "sendStreamId=%d, getStreamId=%d.",
                input->dvppStreamDesc.size, channelDesc->vdecDesc.channelId,
                channelDesc->vdecDesc.sendFrameNotifyId, channelDesc->vdecDesc.getFrameNotifyId,
                channelDesc->sendStreamId, channelDesc->getStreamId);
        } else {
            ACL_LOG_INFO("end to send frame. frame size=%u, channelId=%u, sendStreamId=%d, getStreamId=%d.",
                input->dvppStreamDesc.size, channelDesc->vdecDesc.channelId,
                channelDesc->sendStreamId, channelDesc->getStreamId);
        }
        return ACL_SUCCESS;
    }

    aclvdecChannelDesc *VideoProcessor::aclvdecCreateChannelDesc()
    {
        aclvdecChannelDesc *aclChannelDesc = nullptr;
        switch (aclRunMode_) {
            case ACL_HOST: {
                aclChannelDesc = CreateVdecChannelDescOnHost();
                break;
            }
            case ACL_DEVICE: {
                aclChannelDesc = CreateVdecChannelDescOnDevice();
                break;
            }
            default: {
                ACL_LOG_INNER_ERROR("[Unknown][Mode]unkown acl run mode %d.", aclRunMode_);
                return nullptr;
            }
        }
        return aclChannelDesc;
    }

    aclError VideoProcessor::aclvdecDestroyChannelDesc(aclvdecChannelDesc *channelDesc)
    {
        if (channelDesc == nullptr) {
            return ACL_SUCCESS;
        }

        {
            std::unique_lock<std::mutex> lock{channelDesc->mutexForTLVMap};
            channelDesc->tlvParamMap.clear();
        }

        switch (aclRunMode_) {
            case ACL_DEVICE: {
                channelDesc->~aclvdecChannelDesc();
                FreeDeviceAddr(static_cast<void *>(channelDesc));
                break;
            }
            case ACL_HOST: {
                channelDesc->~aclvdecChannelDesc();
                FreeDeviceBuffer(channelDesc->dataBuffer);
                ACL_ALIGN_FREE(channelDesc);
                break;
            }
            default: {
                ACL_LOG_INNER_ERROR("[Check][RunMode]vdec destroy channel desc, unkown acl run mode %d.", aclRunMode_);
                return ACL_ERROR_INTERNAL_ERROR;
            }
        }
        return ACL_SUCCESS;
    }

    acldvppStreamDesc *VideoProcessor::acldvppCreateStreamDesc()
    {
        acldvppStreamDesc *aclStreamDesc = nullptr;
        switch (aclRunMode_) {
            case ACL_HOST: {
                aclStreamDesc = CreateDvppStreamDescOnHost();
                break;
            }
            case ACL_DEVICE: {
                aclStreamDesc = CreateDvppStreamDescOnDevice();
                break;
            }
            default: {
                ACL_LOG_INNER_ERROR("[Check][RunMode]unkown acl run mode %d.", aclRunMode_);
                return nullptr;
            }
        }
        return aclStreamDesc;
    }

    aclError VideoProcessor::acldvppDestroyStreamDesc(acldvppStreamDesc *streamDesc)
    {
        ACL_LOG_INFO("start to execute acldvppDestroyStreamDesc");
        if (streamDesc == nullptr) {
            ACL_LOG_WARN("streamDesc is null.");
            return ACL_SUCCESS;
        }

        switch (aclRunMode_) {
            case ACL_HOST: {
                FreeDeviceBuffer(streamDesc->dataBuffer);
                ACL_ALIGN_FREE(streamDesc);
                break;
            }
            case ACL_DEVICE: {
                FreeDeviceAddr(static_cast<void *>(streamDesc));
                break;
            }
            default: {
                ACL_LOG_INNER_ERROR("[Check][RunMode]unkown acl run mode %d.", aclRunMode_);
                return ACL_ERROR_INTERNAL_ERROR;
            }
        }
        ACL_LOG_INFO("successfully execute acldvppDestroyStreamDesc");
        return ACL_SUCCESS;
    }

    aclvdecFrameConfig *VideoProcessor::aclvdecCreateFrameConfig()
    {
        ACL_LOG_INFO("start to execute aclvdecCreateFrameConfig");
        aclvdecFrameConfig *aclVdecFrameConfig = nullptr;
        // malloc memory
        uint32_t aclVdecFrameConfigSize = CalAclDvppStructSize(aclVdecFrameConfig);
        ACL_REQUIRES_POSITIVE_RET_NULL(aclVdecFrameConfigSize);
        void *addr = malloc(aclVdecFrameConfigSize);
        if (addr == nullptr) {
            ACL_LOG_INNER_ERROR("[Malloc][Mem]malloc memory failed. size is %u.", aclVdecFrameConfigSize);
            return nullptr;
        }

        // create aclvdecFrameConfig in memory
        aclVdecFrameConfig = new (addr)aclvdecFrameConfig();
        if (aclVdecFrameConfig == nullptr) {
            ACL_LOG_INNER_ERROR("[Check][FrameConfig]new aclvdecFrameConfig failed");
            ACL_FREE(addr);
            return nullptr;
        }
        return aclVdecFrameConfig;
    }

    aclError VideoProcessor::aclvdecDestroyFrameConfig(aclvdecFrameConfig *vdecFrameConfig)
    {
        ACL_FREE(vdecFrameConfig);
        return ACL_SUCCESS;
    }

    aclvencChannelDesc *VideoProcessor::CreateVencChannelDescOnHost()
    {
        ACL_LOG_INFO("CreateVencChannelDescOnHost begin.");
        aclvencChannelDesc *aclChannelDesc = nullptr;
        // alloc host memory
        uint32_t aclChannelDescSize = CalAclDvppStructSize(aclChannelDesc);
        ACL_REQUIRES_POSITIVE_RET_NULL(aclChannelDescSize);
        size_t pageSize = mmGetPageSize();
        void *hostAddr = mmAlignMalloc(aclChannelDescSize, pageSize);
        if (hostAddr == nullptr) {
            ACL_LOG_INNER_ERROR("[Malloc][ChannelDesc]malloc memory failed. size is %u.", aclChannelDescSize);
            return nullptr;
        }

        // create aclvencChannelDesc in host addr
        aclChannelDesc = new (hostAddr)aclvencChannelDesc;
        if ((aclChannelDesc == nullptr) || (aclChannelDesc->vencDesc.extendInfo == nullptr)) {
            ACL_LOG_INNER_ERROR("create aclvencChannelDesc with function new failed");
            ACL_ALIGN_FREE(hostAddr);
            return nullptr;
        }
        auto err = memset_s(aclChannelDesc->vencDesc.extendInfo, acl::dvpp::VENC_CHANNEL_DESC_TLV_LEN,
            0, acl::dvpp::VENC_CHANNEL_DESC_TLV_LEN);
        if (err != EOK) {
            ACL_LOG_INNER_ERROR("[Set][Mem]set vencDesc extendInfo to 0 failed, dstLen = %u, srclen = %u, "
                "result = %d.", aclChannelDesc->vencDesc.len, aclChannelDesc->vencDesc.len, err);
            ACL_ALIGN_FREE(hostAddr);
            return nullptr;
        }

        // malloc device memory for vencChannelDesc
        void *devPtr = nullptr;
        size_t size = CalAclDvppStructSize(&aclChannelDesc->vencDesc);
        uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
        rtError_t ret = rtMalloc(&devPtr, size, flags);
        if (ret != RT_ERROR_NONE) {
            ACL_LOG_INNER_ERROR("malloc device memory for acl venc channel desc failed, runtime result = %d", ret);
            ACL_ALIGN_FREE(hostAddr);
            return nullptr;
        }

        // set data buffer
        aclChannelDesc->dataBuffer.data = devPtr;
        aclChannelDesc->dataBuffer.length = size;
        ACL_LOG_INFO("CreateVencChannelDescOnHost success.");
        return aclChannelDesc;
    }

    aclvencChannelDesc *VideoProcessor::CreateVencChannelDescOnDevice()
    {
        ACL_LOG_INFO("CreateVencChannelDescOnDevice begin.");
        aclvencChannelDesc *aclChannelDesc = nullptr;
        // alloc device memory
        void *devAddr = nullptr;
        uint32_t aclChannelDescSize = CalAclDvppStructSize(aclChannelDesc);
        uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
        rtError_t rtErr = rtMalloc(&devAddr, aclChannelDescSize, flags);
        if (rtErr != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Malloc][Mem]alloc device memory failed, size = %zu, runtime result = %d",
                aclChannelDescSize, rtErr);
            return nullptr;
        }

        // create aclvencChannelDesc in device addr
        aclChannelDesc = new (devAddr)aclvencChannelDesc;
        if ((aclChannelDesc == nullptr) || (aclChannelDesc->vencDesc.extendInfo == nullptr)) {
            ACL_LOG_INNER_ERROR("[Create][ChannelDesc]create aclvencChannelDesc with function new failed");
            (void) rtFree(devAddr);
            devAddr = nullptr;
            return nullptr;
        }
        auto err = memset_s(aclChannelDesc->vencDesc.extendInfo, acl::dvpp::VENC_CHANNEL_DESC_TLV_LEN,
            0, acl::dvpp::VENC_CHANNEL_DESC_TLV_LEN);
        if (err != EOK) {
            ACL_LOG_INNER_ERROR("[Set][Mem]set vencDesc extendInfo to 0 failed, dstLen = %u, srclen = %u, "
                "result = %d.", aclChannelDesc->vencDesc.len, aclChannelDesc->vencDesc.len, err);
            (void) rtFree(devAddr);
            devAddr = nullptr;
            return nullptr;
        }

        // set data buffer
        auto offset = reinterpret_cast<uintptr_t>(&(reinterpret_cast<aclvencChannelDesc *>(0)->vencDesc));
        aclChannelDesc->dataBuffer.data = reinterpret_cast<aicpu::dvpp::DvppVencDesc *>(
            reinterpret_cast<uintptr_t>(devAddr) + offset);
        aclChannelDesc->dataBuffer.length = CalAclDvppStructSize(&aclChannelDesc->vencDesc);
        ACL_LOG_INFO("CreateVencChannelDescOnDevice success.");
        return aclChannelDesc;
    }

    aclvencChannelDesc *VideoProcessor::aclvencCreateChannelDesc()
    {
        ACL_LOG_INFO("aclvencCreateChannelDesc begin.");
        // alloc device men for outBuffer
        void *outData = nullptr;
        uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
        rtError_t rtErr = rtMalloc(&outData, acl::dvpp::MAX_VENC_OUTPUT_BUFFER_SIZE, flags);
        if (rtErr != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Malloc][Mem]alloc device memory failed, size=%zu, runtime result = %d",
                acl::dvpp::MAX_VENC_OUTPUT_BUFFER_SIZE, rtErr);
            return nullptr;
        }

        aclvencChannelDesc *aclChannelDesc = nullptr;
        switch (aclRunMode_) {
            case ACL_HOST: {
                aclChannelDesc = CreateVencChannelDescOnHost();
                break;
            }
            case ACL_DEVICE: {
                aclChannelDesc = CreateVencChannelDescOnDevice();
                break;
            }
            default: {
                ACL_LOG_INNER_ERROR("[Check][RunMode]unkown acl run mode %d.", aclRunMode_);
            }
        }

        if (aclChannelDesc == nullptr) {
            (void) rtFree(outData);
            outData = nullptr;
            return nullptr;
        }
        aclChannelDesc->bufAddr = reinterpret_cast<uintptr_t>(outData);
        aclChannelDesc->bufSize = acl::dvpp::MAX_VENC_OUTPUT_BUFFER_SIZE;
        aclChannelDesc->vencDesc.bufAddr = aclChannelDesc->bufAddr;
        aclChannelDesc->vencDesc.bufSize = aclChannelDesc->bufSize;
        ACL_LOG_INFO("aclvencCreateChannelDesc success.");
        return aclChannelDesc;
    }

    aclError VideoProcessor::aclvencDestroyChannelDesc(aclvencChannelDesc *channelDesc)
    {
        if (channelDesc == nullptr) {
            return ACL_SUCCESS;
        }

        {
            std::unique_lock<std::mutex> lock{channelDesc->mutexForTLVMap};
            channelDesc->tlvParamMap.clear();
        }
        void *outData = reinterpret_cast<void *>(static_cast<uintptr_t>(channelDesc->bufAddr));
        if (outData != nullptr) {
            rtError_t rtErr = rtFree(outData);
            if (rtErr != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Check][RunMode]free device mem failed, runtime result = %d",
                    static_cast<int32_t>(rtErr));
            }
            outData = nullptr;
        }

        switch (aclRunMode_) {
            case ACL_HOST: {
                FreeDeviceBuffer(channelDesc->dataBuffer);
                ACL_ALIGN_FREE(channelDesc);
                break;
            }
            case ACL_DEVICE: {
                FreeDeviceAddr(static_cast<void *>(channelDesc));
                break;
            }
            default: {
                ACL_LOG_INNER_ERROR("[Destroy][ChannelDesc]venc destroy channel desc failed, "
                    "unkown acl run mode %d.", aclRunMode_);
                return ACL_ERROR_INTERNAL_ERROR;
            }
        }
        return ACL_SUCCESS;
    }

    aclvencFrameConfig *VideoProcessor::aclvencCreateFrameConfig()
    {
        ACL_LOG_INFO("start to execute aclvencCreateFrameConfig");
        aclvencFrameConfig *frameConfig = nullptr;
        // malloc memory
        uint32_t aclVencFrameConfigSize = CalAclDvppStructSize(frameConfig);
        ACL_REQUIRES_POSITIVE_RET_NULL(aclVencFrameConfigSize);
        void *addr = malloc(aclVencFrameConfigSize);
        if (addr == nullptr) {
            ACL_LOG_INNER_ERROR("[Malloc][Mem]malloc memory failed. size is %u.", aclVencFrameConfigSize);
            return nullptr;
        }

        // create aclvencFrameConfig in memory
        frameConfig = new (addr)aclvencFrameConfig();
        if (frameConfig == nullptr) {
            ACL_LOG_INNER_ERROR("[Malloc][FrameConfig]new aclvencFrameConfig failed");
            ACL_FREE(addr);
            return nullptr;
        }
        return frameConfig;
    }

    aclError VideoProcessor::aclvencDestroyFrameConfig(aclvencFrameConfig *vencFrameConfig)
    {
        ACL_FREE(vencFrameConfig);
        return ACL_SUCCESS;
    }

    void VideoProcessor::DestroyVencAllNotify(aclvencChannelDesc *channelDesc)
    {
        if (channelDesc->sendFrameNotify != nullptr) {
            rtError_t rtRet = rtNotifyDestroy(static_cast<rtNotify_t>(channelDesc->sendFrameNotify));
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Destroy][Notify]fail to destroy sendFrameNotify, runtime result = %d", rtRet);
            }
            channelDesc->sendFrameNotify = nullptr;
        }

        if (channelDesc->getFrameNotify != nullptr) {
            rtError_t rtRet = rtNotifyDestroy(static_cast<rtNotify_t>(channelDesc->getFrameNotify));
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Destroy][Notify]fail to destroy getFrameNotify, runtime result = %d", rtRet);
            }
            channelDesc->getFrameNotify = nullptr;
        }
    }

    void VideoProcessor::DestroyVencAllEvent(aclvencChannelDesc *channelDesc)
    {
        if (channelDesc->sendFrameNotify != nullptr) {
            rtError_t rtRetVal = rtEventDestroy(static_cast<rtEvent_t>(channelDesc->sendFrameNotify));
            if (rtRetVal != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Destroy][Event]fail to destroy sendFrameNotify, runtime result = %d", rtRetVal);
            }
            channelDesc->sendFrameNotify = nullptr;
        }

        if (channelDesc->getFrameNotify != nullptr) {
            rtError_t rtRetVal = rtEventDestroy(static_cast<rtEvent_t>(channelDesc->getFrameNotify));
            if (rtRetVal != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Destroy][Event]fail to destroy getFrameNotify, runtime result = %d", rtRetVal);
            }
            channelDesc->getFrameNotify = nullptr;
        }
    }

    /**
     * destroy all notify and stream for venc channel.
     * @param channelDesc[in] venc channel desc
     * @return void
     */
    void VideoProcessor::DestroyVencAllNotifyAndStream(aclvencChannelDesc *channelDesc)
    {
        ACL_LOG_DEBUG("begin to destroy all notify and stream.");
        if (channelDesc == nullptr) {
            ACL_LOG_ERROR("[Check][ChannelDesc]venc channel desc is null.");
            const char *argList[] = {"param"};
            const char *argVal[] = {"channelDesc"};
            acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                argList, argVal, 1);
            return;
        }

        if (channelDesc->vencWaitTaskType == NOTIFY_TASK) {
            DestroyVencAllNotify(channelDesc);
        } else {
            DestroyVencAllEvent(channelDesc);
        }

        if (channelDesc->sendFrameStream != nullptr) {
            rtError_t rtRet = rtStreamDestroy(channelDesc->sendFrameStream);
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Destroy][Stream]fail to destroy sendFrameStream, runtime result = %d", rtRet);
            }
            channelDesc->sendFrameStream = nullptr;
        }

        if (channelDesc->getFrameStream != nullptr) {
            rtError_t rtRet = rtStreamDestroy(channelDesc->getFrameStream);
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Destroy][Stream]fail to destroy getFrameStream, runtime result = %d", rtRet);
            }
            channelDesc->getFrameStream = nullptr;
        }
        ACL_LOG_DEBUG("success to destroy all notify and stream.");
    }

    aclError VideoProcessor::CreateNotifyForVencChannel(aclvencChannelDesc *channelDesc)
    {
        // get deviceId
        int32_t deviceId = 0;
        rtError_t rtRet = rtGetDevice(&deviceId);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Get][Device]fail to get deviceId when create venc channel, result = %d", rtRet);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }

        // create notify for send frame
        rtNotify_t sendFrameNotify = nullptr;
        rtRet = rtNotifyCreate(deviceId, &sendFrameNotify);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Create][Notify]fail to create sendFrameNotify, runtime result = %d", rtRet);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        channelDesc->sendFrameNotify = static_cast<void *>(sendFrameNotify);

        // create notify for get frame
        rtNotify_t getFrameNotify = nullptr;
        rtRet = rtNotifyCreate(deviceId, &getFrameNotify);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Create][Notify]fail to create getFrameNotify, runtime result = %d", rtRet);
            DestroyVencAllNotifyAndStream(channelDesc);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        channelDesc->getFrameNotify = static_cast<void *>(getFrameNotify);

        // get notifyId for send frame
        uint32_t sendFrameNotifyId = 0;
        rtRet = rtGetNotifyID(sendFrameNotify, &sendFrameNotifyId);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Get][NotifyID]fail to get sendFrameNotifyId, runtime result = %d", rtRet);
            DestroyVencAllNotifyAndStream(channelDesc);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        channelDesc->vencDesc.sendFrameNotifyId = sendFrameNotifyId;
        ACL_LOG_INFO("venc channel rtGetNotifyID success, send notify id:%u", sendFrameNotifyId);

        // get notifyId for get frame
        uint32_t getFrameNotifyId = 0;
        rtRet = rtGetNotifyID(getFrameNotify, &getFrameNotifyId);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Get][NotifyID]fail to get getFrameNotifyId, runtime result = %d", rtRet);
            DestroyVencAllNotifyAndStream(channelDesc);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        channelDesc->vencDesc.getFrameNotifyId = getFrameNotifyId;
        ACL_LOG_INFO("venc channel rtGetNotifyID success, get notify id:%u", getFrameNotifyId);

        return ACL_SUCCESS;
    }

    aclError VideoProcessor::CreateEventForVencChannel(aclvencChannelDesc *channelDesc)
    {
        // create event for send frame
        rtEvent_t sendFrameEvent = nullptr;
        rtError_t rtRet = rtEventCreateWithFlag(&sendFrameEvent, RT_EVENT_WITH_FLAG);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Create][Event]fail to create sendFrameEvent, runtime result = %d", rtRet);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        channelDesc->sendFrameNotify = static_cast<void *>(sendFrameEvent);

        // create event for get frame
        rtEvent_t getFrameEvent = nullptr;
        rtRet = rtEventCreateWithFlag(&getFrameEvent, RT_EVENT_WITH_FLAG);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Create][Event]fail to create getFrameEvent, runtime result = %d", rtRet);
            DestroyVencAllNotifyAndStream(channelDesc);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        channelDesc->getFrameNotify = static_cast<void *>(getFrameEvent);

        // get eventId for send frame
        uint32_t sendFrameEventId = 0;
        rtRet = rtGetEventID(sendFrameEvent, &sendFrameEventId);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Get][Event]fail to get sendFrameEventId, runtime result = %d", rtRet);
            DestroyVencAllNotifyAndStream(channelDesc);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        ACL_LOG_INFO("sendFrameEventId is %u", sendFrameEventId);
        channelDesc->vencDesc.sendFrameNotifyId = sendFrameEventId;

        // get eventId for get frame
        uint32_t getFrameEventId = 0;
        rtRet = rtGetEventID(getFrameEvent, &getFrameEventId);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Get][EventID]fail to get getFrameEventId, runtime result = %d", rtRet);
            DestroyVencAllNotifyAndStream(channelDesc);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        ACL_LOG_INFO("getFrameEventId is %u", getFrameEventId);
        channelDesc->vencDesc.getFrameNotifyId = getFrameEventId;

        return ACL_SUCCESS;
    }

    /**
     * create and record all notify and stream for venc.
     * @param channelDesc venc channel desc
     * @return ACL_SUCCESS for success, other for failed
     */
    aclError VideoProcessor::CreateNotifyAndStremForVencChannel(aclvencChannelDesc *channelDesc)
    {
        // Verify the validity of the parameters
        if (channelDesc == nullptr) {
            ACL_LOG_ERROR("[Check][ChannelDesc]venc channel desc is null.");
            const char *argList[] = {"param"};
            const char *argVal[] = {"channelDesc"};
            acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                argList, argVal, 1);
            return ACL_ERROR_INVALID_PARAM;
        }

        aclError aclRet;
        if (channelDesc->vencWaitTaskType == NOTIFY_TASK) {
            aclRet = CreateNotifyForVencChannel(channelDesc);
            if (aclRet != ACL_SUCCESS) {
                return aclRet;
            }
        } else {
            aclRet = CreateEventForVencChannel(channelDesc);
            if (aclRet != ACL_SUCCESS) {
                return aclRet;
            }
        }

        // create stream for send frame
        rtStream_t sendFrameStream = nullptr;
        rtError_t rtRet = rtStreamCreate(&sendFrameStream, RT_STREAM_PRIORITY_DEFAULT);
        if (rtRet != ACL_SUCCESS) {
            ACL_LOG_CALL_ERROR("[Create][Stream]fail to create sendFrameStream, runtime result = %d", rtRet);
            DestroyVencAllNotifyAndStream(channelDesc);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        channelDesc->sendFrameStream = sendFrameStream;

        // create stream for get frame
        rtStream_t getFrameStream = nullptr;
        rtRet = rtStreamCreate(&getFrameStream, RT_STREAM_PRIORITY_DEFAULT);
        if (rtRet != ACL_SUCCESS) {
            ACL_LOG_CALL_ERROR("[Create][Stream]fail to create getFrameStream, result = %d", rtRet);
            DestroyVencAllNotifyAndStream(channelDesc);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        channelDesc->getFrameStream = getFrameStream;

        return ACL_SUCCESS;
    }

    /**
     * memcpy data to device.
     * @param output output dvpp stream desc
     * @return ACL_SUCCESS for success, other for failed
     */
    aclError VideoProcessor::MemcpyDataToDevice(acldvppStreamDesc *output)
    {
        size_t size = sizeof(aicpu::dvpp::DvppStreamDesc);
        rtError_t rtRet = rtMemcpy(output->dataBuffer.data, output->dataBuffer.length,
            static_cast<const void *>(&output->dvppStreamDesc), size,
            RT_MEMCPY_HOST_TO_DEVICE);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Copy][Mem]memcpy to device memory failed, size = %zu, "
                "runtime result = %d", size, rtRet);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        return ACL_SUCCESS;
    }

    /**
     * choose memroy mode.
     * @param channelDesc venc channel desc
     * @param output output dvpp stream desc
     * @return ACL_SUCCESS for success, other for failed
     */
    aclError VideoProcessor::ChooseMemMode(aclvencChannelDesc *channelDesc, acldvppStreamDesc *&output)
    {
        if (channelDesc->outputMemMode == VENC_NO_INIT) {
            if (output == nullptr) {
                channelDesc->outputMemMode = VENC_SYSTEM_MODE;
            } else {
                channelDesc->outputMemMode = VENC_USER_MODE;
            }
        }

        if (channelDesc->outputMemMode != VENC_SYSTEM_MODE) {
            ACL_LOG_INNER_ERROR("[Check][MemMode]venc user mode:%d is not supported and output "
                "should be null.", VENC_USER_MODE);
            return ACL_ERROR_INVALID_PARAM;
        }

        if (output != nullptr) {
            ACL_LOG_WARN("output memery mode is system mode, but output isn't null.");
            output = nullptr;
        }

        output = acldvppCreateStreamDesc();
        ACL_REQUIRES_NOT_NULL(output);
        if (output->dataBuffer.data == nullptr) {
            ACL_LOG_INNER_ERROR("[Check][DataBuffer]output dvpp stream desc dataBuffer data is null.");
            (void)acldvppDestroyStreamDesc(output);
            return ACL_ERROR_RT_FAILURE;
        }
        output->dvppStreamDesc.data = channelDesc->bufAddr;
        output->dvppStreamDesc.size = channelDesc->bufSize;

        // memcpy data to device
        if (aclRunMode_ == ACL_HOST) {
            aclError ret = MemcpyDataToDevice(output);
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][Mem]MemcpyDataToDevice failed, result = %d.", ret);
                (void)acldvppDestroyStreamDesc(output);
                return ret;
            }
        }

        ACL_LOG_INFO("choose memory mode successfully.");
        return ACL_SUCCESS;
    }

    /**
     * dvpp venc launch send frame task.
     * @param channelDesc venc channel desc
     * @param inputPicDesc input picture desc
     * @param outputStreamDesc output stream desc
     * @param config dvpp frame config
     * @return ACL_SUCCESS for ok, others for fail
     */
    aclError VideoProcessor::LaunchVencSendFrameTask(const aclvencChannelDesc *channelDesc,
                                                     acldvppPicDesc *inputPicDesc,
                                                     acldvppStreamDesc *outputStreamDesc,
                                                     aclvencFrameConfig *config)
    {
        constexpr int32_t ioAddrNum = 3;
        uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t) +
            CalDevDvppStructRealUsedSize(&config->vencFrameConfig);
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        int i = 0;
        bool eos = config->vencFrameConfig.eos;
        ioAddr[i++] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        ioAddr[i++] = eos ? 0 : reinterpret_cast<uintptr_t>(inputPicDesc->dataBuffer.data);
        ioAddr[i] = eos ? 0 : reinterpret_cast<uintptr_t>(outputStreamDesc->dataBuffer.data);

        constexpr uint32_t configOffset = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        auto memcpyRet = memcpy_s(args.get() + configOffset, argsSize - configOffset, &(config->vencFrameConfig),
            CalDevDvppStructRealUsedSize(&config->vencFrameConfig));
        if (memcpyRet != EOK) {
            ACL_LOG_INNER_ERROR("[Copy][Frame]copy venc frame config to args failed, result = %d.", memcpyRet);
            return ACL_ERROR_FAILURE;
        }

        ACL_LOG_INFO("begin to send frame to device, eos = %d.", eos);
        rtError_t rtRet = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
                                            acl::dvpp::DVPP_KERNELNAME_VENC_SEND_FRAME,
                                            1, // blockDim default 1
                                            args.get(),
                                            argsSize,
                                            nullptr, // no need smDesc
                                            channelDesc->sendFrameStream);
        ACL_LOG_INFO("end to send frame to device.");
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Launch][Task]venc send frame task call rtCpuKernelLaunch failed, "
                "runtime result = %d.", rtRet);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        return ACL_SUCCESS;
    }


    /**
     * dvpp venc launch release frame task.
     * @param channelDesc venc channel desc
     * @return ACL_SUCCESS for ok, others for fail
     */
    aclError VideoProcessor::LaunchReleaseFrameTask(const aclvencChannelDesc *channelDesc)
    {
        constexpr int32_t ioAddrNum = 1;

        constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t) + sizeof(uint8_t);
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));

        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);

        constexpr uint32_t offset = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        auto memcpyRet = memcpy_s(args.get() + offset,
                                  argsSize - offset,
                                  &(channelDesc->outputMemMode),
                                  sizeof(uint8_t));
        if (memcpyRet != EOK) {
            ACL_LOG_INNER_ERROR("[Copy][Mem]copy output memory mode to args failed, result = %d.", memcpyRet);
            return ACL_ERROR_FAILURE;
        }

        rtError_t rtRet = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
                                            acl::dvpp::DVPP_KERNELNAME_VENC_RELEASE_FRAME,
                                            1, // blockDim default 1
                                            args.get(),
                                            argsSize,
                                            nullptr, // no need smDesc
                                            channelDesc->getFrameStream);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Launch][Task]release venc frame task call rtCpuKernelLaunch failed, "
                "runtime result = %d.", rtRet);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        return ACL_SUCCESS;
    }

    /**
     * get venc frame callback function for runtime.
     * @param callbackData callback user data
     * @return void
     */
    void VideoProcessor::GetVencFrameCallback(void *callbackData)
    {
        ACL_LOG_INFO("start to execute GetVencFrameCallback");
        auto addr = reinterpret_cast<uintptr_t>(callbackData);
        auto info = aicpu::dvpp::CallbackInfoManager<aicpu::dvpp::VencCallbackInfoPtr>::Instance().Take(addr);
        if (info == nullptr) {
            ACL_LOG_CALL_ERROR("[Check][Info]GetVencFrameCallback callbackData = %p, "
                "but no info found in CallbackInfoManager.", callbackData);
            return;
        }

        if (aclRunMode_ == ACL_HOST) {
            // copy outputPicDesc to host
            size_t size = sizeof(aicpu::dvpp::DvppStreamDesc);
            aclError ret = rtMemcpy(&(info->outputStreamDesc->dvppStreamDesc), size,
                                    info->outputStreamDesc->dataBuffer.data, size,
                                    RT_MEMCPY_DEVICE_TO_HOST);
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Copy][Mem]memcpy output stream desc to host memory failed, "
                    "size = %zu, result = %d", size, ret);
                if (info->outputMemMode == VENC_SYSTEM_MODE) {
                    (void)acldvppDestroyStreamDesc(info->outputStreamDesc);
                }
                return;
            }

            ACL_LOG_INFO("rtMemcpy success, timestamp:%llu, size:%u, format:%u, eos:%d, retCode:%u.",
                info->outputStreamDesc->dvppStreamDesc.timestamp,
                info->outputStreamDesc->dvppStreamDesc.size,
                info->outputStreamDesc->dvppStreamDesc.format,
                info->outputStreamDesc->dvppStreamDesc.eos,
                info->outputStreamDesc->dvppStreamDesc.retCode);
        }

        // check user callback func
        if (info->callbackFunc == nullptr) {
            ACL_LOG_INNER_ERROR("[Check][Callback]call back func is null!");
            if (info->outputMemMode == VENC_SYSTEM_MODE) {
                (void)acldvppDestroyStreamDesc(info->outputStreamDesc);
            }
            return;
        }

        // call user callback func
        aclvencCallback callbackFunc = info->callbackFunc;
        (*callbackFunc)(info->inputPicDesc, info->outputStreamDesc, info->callbackData);

        if (info->outputMemMode == VENC_SYSTEM_MODE) {
            (void)acldvppDestroyStreamDesc(info->outputStreamDesc);
        }

        ACL_LOG_INFO("successfully execute GetVencFrameCallback");
        return;
    }

    /**
     * dvpp venc launch send eos frame task.
     * @param channelDesc venc channel desc
     * @param config dvpp frame config
     * @return ACL_SUCCESS for ok, others for fail
     */
    aclError VideoProcessor::aclvencSendEosFrame(aclvencChannelDesc *channelDesc, aclvencFrameConfig *config)
    {
        // launch send frame task to send frame stream
        aclError sendFrameRet = LaunchVencSendFrameTask(channelDesc, nullptr, nullptr, config);
        if (sendFrameRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Launch][Task]launch send frame task failed, result = %d", sendFrameRet);
            return sendFrameRet;
        }

        // launch wait task to send frame stream
        rtError_t sendFrameWaitRet = rtNotifyWait(static_cast<rtNotify_t>(channelDesc->sendFrameNotify),
            channelDesc->sendFrameStream);
        if (sendFrameWaitRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Wait][Notify]wait for a notify to send frame stream failed, "
                "runtime result = %d", sendFrameWaitRet);
            return ACL_GET_ERRCODE_RTS(sendFrameWaitRet);
        }

        // if eos is true, no task in get frame stream
        ACL_LOG_INFO("rtStreamSynchronize send frame begin.");
        rtError_t streamSynRet = rtStreamSynchronize(channelDesc->sendFrameStream);
        if (streamSynRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Sync][Stream]fail to synchronize sendFrameStream, runtime result = %d", streamSynRet);
            (void) rtStreamSynchronize(channelDesc->getFrameStream);
            return ACL_GET_ERRCODE_RTS(streamSynRet);
        }
        ACL_LOG_INFO("rtStreamSynchronize send frame end.");

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

    /**
     * Venc check parameter
     * @param channelDesc venc channel desc
     * @param input input picture desc
     * @return ACL_SUCCESS for ok, others for fail
     */
    aclError VideoProcessor::CheckVencParameter(aclvencChannelDesc *channelDesc, acldvppPicDesc *input)
    {
        // if the verification logic is modified, acl and dvpp must perform the modification at the same time
        uint32_t oneFrameSize = 0;
        ACL_CHECK_ASSIGN_UINT32_MULTI(channelDesc->vencDesc.picWidth, channelDesc->vencDesc.picHeight, oneFrameSize);
        // size of nv12 or nv21 image is equal to width  * height * 3 / 2
        ACL_CHECK_ASSIGN_UINT32_MULTI(oneFrameSize, 3, oneFrameSize);
        oneFrameSize = oneFrameSize / 2;
        // picture size by user setting must be in range [oneFrameSize, oneFrameSize * 2)
        if ((input->dvppPicDesc.size < oneFrameSize) || (input->dvppPicDesc.size >= (oneFrameSize * 2))) {
            ACL_LOG_INNER_ERROR("[Check][Params]invalid picture size %u by user setting, it must be in range [%u, %u)",
                input->dvppPicDesc.size, oneFrameSize, oneFrameSize * 2);
            return ACL_ERROR_INVALID_PARAM;
        }

        return ACL_SUCCESS;
    }

    /**
     * dvpp venc launch send normal frame task.
     * @param channelDesc venc channel desc
     * @param input input picture desc
     * @param config dvpp frame config
     * @param userdata user callback function
     * @return ACL_SUCCESS for ok, others for fail
     */
    aclError VideoProcessor::aclvencSendNomalFrame(aclvencChannelDesc *channelDesc,
                                                   acldvppPicDesc *input,
                                                   void *reserve,
                                                   aclvencFrameConfig *config,
                                                   void *userdata)

    {
        ACL_REQUIRES_NOT_NULL(input);
        ACL_REQUIRES_NOT_NULL(input->dataBuffer.data);

        aclError ret = CheckVencParameter(channelDesc, input);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Check][VencParameter]check venc parameter failed, result = %d", ret);
            return ret;
        }

        // choose memory mode
        acldvppStreamDesc *output = nullptr;
        ret = ChooseMemMode(channelDesc, output);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Choose][MemMode]choose memory mode failed, result = %d", ret);
            return ret;
        }

        if (aclRunMode_ == ACL_HOST) {
            // copy input pic desc to device
            size_t size = sizeof(aicpu::dvpp::DvppPicDesc);
            rtError_t memcpyRet = rtMemcpy(input->dataBuffer.data, input->dataBuffer.length,
                static_cast<const void *>(&input->dvppPicDesc), size, RT_MEMCPY_HOST_TO_DEVICE);
            if (memcpyRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Copy][Mem]memcpy input pic desc to device memory failed, "
                    "size = %zu, runtime result = %d", size, memcpyRet);
                (void)acldvppDestroyStreamDesc(output);
                return ACL_GET_ERRCODE_RTS(memcpyRet);
            }
        }

        // launch send frame task to send frame stream
        aclError sendFrameRet = LaunchVencSendFrameTask(channelDesc, input, output, config);
        if (sendFrameRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Launch][Task]launch send frame task failed, result = %d", sendFrameRet);
            (void)acldvppDestroyStreamDesc(output);
            return sendFrameRet;
        }

        // launch wait task to send frame stream
        rtError_t sendFrameWaitRet = rtNotifyWait(static_cast<rtNotify_t>(channelDesc->sendFrameNotify),
            channelDesc->sendFrameStream);
        if (sendFrameWaitRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Wait][Notify]wait for a notify to send frame stream failed, "
                "runtime result = %d", sendFrameWaitRet);
            (void)acldvppDestroyStreamDesc(output);
            return ACL_GET_ERRCODE_RTS(sendFrameWaitRet);
        }

        // launch wait task to get frame stream
        rtError_t getFrameWaitRet = rtNotifyWait(static_cast<rtNotify_t>(channelDesc->getFrameNotify),
            channelDesc->getFrameStream);
        if (getFrameWaitRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Wait][Notify]wait for a notify to get frame stream failed, "
                "runtime result = %d", getFrameWaitRet);
            (void)acldvppDestroyStreamDesc(output);
            return ACL_GET_ERRCODE_RTS(getFrameWaitRet);
        }

        // create callback info
        aicpu::dvpp::VencCallbackInfoPtr callbackInfoPtr;
        try {
            callbackInfoPtr = std::make_shared<aicpu::dvpp::VencGetFrameCallbackInfo>(
                channelDesc->callback, input, output, channelDesc->outputMemMode, userdata);
        } catch (std::bad_alloc &) {
            ACL_LOG_INNER_ERROR("[Make][Shared]Make shared for get frame callback info failed.");
            (void)acldvppDestroyStreamDesc(output);
            return ACL_ERROR_BAD_ALLOC;
        }
        // save callback info
        auto addr = reinterpret_cast<uintptr_t>(callbackInfoPtr.get());
        // Insert return code is always true, no need to check
        (void)aicpu::dvpp::CallbackInfoManager<aicpu::dvpp::VencCallbackInfoPtr>::
            Instance().Insert(addr, callbackInfoPtr);
        // launch callback task to get frame stream
        rtError_t callbackRet = rtCallbackLaunch(GetVencFrameCallback,
                                                 callbackInfoPtr.get(),
                                                 channelDesc->getFrameStream, true);
        if (callbackRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Launch][Task]launch callback to get frame stream failed, "
                "runtime result = %d", callbackRet);
            (void)aicpu::dvpp::CallbackInfoManager<aicpu::dvpp::VencCallbackInfoPtr>::Instance().Erase(addr);
            return ACL_GET_ERRCODE_RTS(callbackRet);
        }

        // launch release frame task to get frame stream
        aclError releaseFrameRet = LaunchReleaseFrameTask(channelDesc);
        if (releaseFrameRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Launch][Task]launch release frame task failed, result = %d", releaseFrameRet);
            return releaseFrameRet;
        }

        // streamSynchronize send frame stream
        rtError_t streamSynRet = rtStreamSynchronize(channelDesc->sendFrameStream);
        if (streamSynRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Sync][Stream]fail to synchronize sendFrameStream, runtime result = %d", streamSynRet);
            (void)aicpu::dvpp::CallbackInfoManager<aicpu::dvpp::VencCallbackInfoPtr>::Instance().Erase(addr);
            return ACL_GET_ERRCODE_RTS(streamSynRet);
        }

        return ACL_SUCCESS;
    }

    /**
     * send eos for venc.
     * @param channelDesc venc channel desc
     * @return ACL_SUCCESS for success, other for failed
     */
    aclError VideoProcessor::SendEosForVenc(aclvencChannelDesc *channelDesc)
    {
        // create input config desc
        aclvencFrameConfig *config = aclvencCreateFrameConfig();
        if (config == nullptr) {
            ACL_LOG_INNER_ERROR("[Send][Eos]fail to create dvpp config desc for eos.");
            return ACL_ERROR_BAD_ALLOC;
        }

        // set eos
        aclError ret = aclvencSetFrameConfigEos(config, SEND_FRAME_EOS);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Set][FrameConfig]fail to set eos, result = %d", ret);
            (void) aclvencDestroyFrameConfig(config);
            return ret;
        }

        // send eos and sychronize
        ret = aclvencSendEosFrame(channelDesc, config);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Send][EosFrame]fail to send eos, result = %d", ret);
            (void) aclvencDestroyFrameConfig(config);
            return ret;
        }

        // destory input stream desc
        ret = aclvencDestroyFrameConfig(config);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Destroy][FrameConfig]fail to destory dvpp config desc for eos, result = %d", ret);
            return ret;
        }
        return ACL_SUCCESS;
    }

    void VideoProcessor::SetVencWaitTaskType(aclvencChannelDesc *channelDesc)
    {
        channelDesc->vencWaitTaskType = NOTIFY_TASK;
        ACL_LOG_INFO("venc wait task type is notify");
    }

    aclError VideoProcessor::SetVdecParamToVdecChannel(aclvdecChannelDesc *channelDesc)
    {
        uint32_t offset = VDEC_BIT_DEPTH_STRUCT_SIZE;
        std::unique_lock<std::mutex> lock{channelDesc->mutexForTLVMap};
        for (auto &it : channelDesc->tlvParamMap) {
            if (it.first != VDEC_BIT_DEPTH) { // for compatibility, bit depth should be special treated
                ACL_REQUIRES_NOT_NULL(it.second.value.get());
                uint32_t tmpOffset = offset;
                ACL_CHECK_ASSIGN_UINT32T_ADD(tmpOffset, static_cast<uint32_t>(it.second.valueLen), tmpOffset);
                if (tmpOffset > VDEC_CHANNEL_DESC_TLV_LEN) {
                    ACL_LOG_INNER_ERROR("[Check][Offset] offset %u can not be larger than %u",
                        tmpOffset, VDEC_CHANNEL_DESC_TLV_LEN);
                    return ACL_ERROR_FAILURE;
                }
                auto ret = memcpy_s(channelDesc->vdecDesc.extendInfo + offset, it.second.valueLen,
                    it.second.value.get(), it.second.valueLen);
                if (ret != EOK) {
                    ACL_LOG_INNER_ERROR("[Copy][Mem]call memcpy_s failed, result = %d, srcLen = %zu, dstLen = %zu", ret,
                        it.second.valueLen, it.second.valueLen);
                    return ACL_ERROR_FAILURE;
                }
                offset += static_cast<uint32_t>(it.second.valueLen);
            }
        }
        channelDesc->vdecDesc.len = offset;
        // for compatibility, bit depth must be in front of extendInfo
        auto itBitDepth = channelDesc->tlvParamMap.find(VDEC_BIT_DEPTH);
        if (itBitDepth != channelDesc->tlvParamMap.end()) {
            auto ret = memcpy_s(channelDesc->vdecDesc.extendInfo, VDEC_BIT_DEPTH_STRUCT_SIZE,
                itBitDepth->second.value.get(), itBitDepth->second.valueLen);
            if (ret != EOK) {
                ACL_LOG_INNER_ERROR("[Copy][Mem]call memcpy_s failed, result = %d, srcLen = %zu, dstLen = %zu", ret,
                    itBitDepth->second.valueLen, VDEC_BIT_DEPTH_STRUCT_SIZE);
                return ACL_ERROR_FAILURE;
            }
        } else if (offset > VDEC_BIT_DEPTH_STRUCT_SIZE) {
            aicpu::dvpp::DvppVdecBitDepthConfig tmpDvppVdecBitDepthConfig;
            auto ret = memcpy_s(channelDesc->vdecDesc.extendInfo, VDEC_BIT_DEPTH_STRUCT_SIZE,
                &tmpDvppVdecBitDepthConfig, VDEC_BIT_DEPTH_STRUCT_SIZE);
            if (ret != EOK) {
                ACL_LOG_INNER_ERROR("[Copy][Mem]call memcpy_s failed, result = %d, srcLen = %zu, dstLen = %zu", ret,
                    VDEC_BIT_DEPTH_STRUCT_SIZE, VDEC_BIT_DEPTH_STRUCT_SIZE);
                return ACL_ERROR_FAILURE;
            }
        } else {
            channelDesc->vdecDesc.len = 0;
        }
        return ACL_SUCCESS;
    }

    aclError VideoProcessor::SetVencParamToVencChannel(aclvencChannelDesc *channelDesc)
    {
        uint32_t offset = 0;
        std::unique_lock<std::mutex> lock{channelDesc->mutexForTLVMap};
        for (auto &it : channelDesc->tlvParamMap) {
            ACL_REQUIRES_NOT_NULL(it.second.value.get());
            uint32_t tmpOffset = offset;
            ACL_CHECK_ASSIGN_UINT32T_ADD(tmpOffset, static_cast<uint32_t>(it.second.valueLen), tmpOffset);
            if (tmpOffset > VENC_CHANNEL_DESC_TLV_LEN) {
                ACL_LOG_INNER_ERROR("[Check][Offset] offset %u can not be larger than %u",
                    tmpOffset, VENC_CHANNEL_DESC_TLV_LEN);
                return ACL_ERROR_FAILURE;
            }
            auto ret = memcpy_s(channelDesc->vencDesc.extendInfo + offset, it.second.valueLen,
                it.second.value.get(), it.second.valueLen);
            if (ret != EOK) {
                ACL_LOG_INNER_ERROR("[Copy][Mem]call memcpy_s failed, result = %d, srcLen = %zu, dstLen = %zu", ret,
                    it.second.valueLen, it.second.valueLen);
                return ACL_ERROR_FAILURE;
            }
            offset += static_cast<uint32_t>(it.second.valueLen);
        }
        channelDesc->vencDesc.len = offset;

        return ACL_SUCCESS;
    }

    aclError VideoProcessor::aclvencCreateChannel(aclvencChannelDesc *channelDesc)
    {
        ACL_LOG_INFO("aclvencCreateChannel begin");
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);

        aclError aclRet = SetVencParamToVencChannel(channelDesc);
        if (aclRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Set][VencParam]set venc parameter failed, result = %d", aclRet);
            return aclRet;
        }
        SetVencWaitTaskType(channelDesc);

        // create 2 notify and 2 stream for venc channel desc
        aclRet = CreateNotifyAndStremForVencChannel(channelDesc);
        if (aclRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Create][Notify]create notify and stream for venc channel failed, "
                "result = %d", aclRet);
            return aclRet;
        }

        // subscribe report
        rtError_t rtRet = rtSubscribeReport(channelDesc->threadId, channelDesc->getFrameStream);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Subscribe][Report]subscribe report failed, runtime result = %d", rtRet);
            DestroyVencAllNotifyAndStream(channelDesc);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }

        aclRet = aclvencMallocOutMemory(channelDesc);
        if (aclRet != ACL_SUCCESS) {
            (void)rtUnSubscribeReport(channelDesc->threadId, channelDesc->getFrameStream);
            DestroyVencAllNotifyAndStream(channelDesc);
            ACL_LOG_INNER_ERROR("[Malloc][Mem]malloc out buffer for venc channel failed, result = %d", aclRet);
            return aclRet;
        }

        if (aclRunMode_ == ACL_HOST) {
            // memcpy data to device (both async and sync copy type are OK for this api)
            size_t size = CalDevDvppStructRealUsedSize(&channelDesc->vencDesc);
            rtRet = rtMemcpyAsync(channelDesc->dataBuffer.data,
                                  channelDesc->dataBuffer.length,
                                  static_cast<const void *>(&channelDesc->vencDesc),
                                  size,
                                  RT_MEMCPY_HOST_TO_DEVICE,
                                  channelDesc->sendFrameStream);
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Copy][Mem]memcpy to device memory failed, dst size = %lu,"
                    "src size = %zu, runtime result = %d", channelDesc->dataBuffer.length,
                    size, rtRet);
                (void)rtUnSubscribeReport(channelDesc->threadId, channelDesc->getFrameStream);
                DestroyVencAllNotifyAndStream(channelDesc);
                aclvencFreeOutMemory(channelDesc);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }
        }

        // create channel have only 2 input
        constexpr int32_t ioAddrNum = 2;
        constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        ioAddr[1] = channelDesc->outStreamDesc == nullptr ? 0 :
            reinterpret_cast<uintptr_t>(channelDesc->outStreamDesc->dataBuffer.data);

        // launch create channel task
        rtRet = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
            acl::dvpp::DVPP_KERNELNAME_CREATE_VENC_CHANNEL,
            1,
            args.get(),
            argsSize,
            nullptr,
            channelDesc->sendFrameStream);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Launch][Task]create venc channel call rtCpuKernelLaunch failed, "
                "runtime result = %d", rtRet);
            (void)rtUnSubscribeReport(channelDesc->threadId, channelDesc->getFrameStream);
            DestroyVencAllNotifyAndStream(channelDesc);
            aclvencFreeOutMemory(channelDesc);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }

        if (aclRunMode_ == ACL_HOST) {
            size_t size = acl::dvpp::CalAclDvppStructSize(&channelDesc->vencDesc);
            rtRet = rtMemcpyAsync(&channelDesc->vencDesc, size,
                                  static_cast<const void *>(channelDesc->dataBuffer.data),
                                  channelDesc->dataBuffer.length,
                                  RT_MEMCPY_DEVICE_TO_HOST, channelDesc->sendFrameStream);
            if (rtRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Copy][Mem]memcpy to host memory failed, size = %zu, "
                    "runtime result = %d", size, rtRet);
                (void)rtUnSubscribeReport(channelDesc->threadId, channelDesc->getFrameStream);
                DestroyVencAllNotifyAndStream(channelDesc);
                aclvencFreeOutMemory(channelDesc);
                return ACL_GET_ERRCODE_RTS(rtRet);
            }
        }

        // stream synchronize
        rtRet = rtStreamSynchronize(channelDesc->sendFrameStream);
        if (rtRet != ACL_SUCCESS) {
            ACL_LOG_CALL_ERROR("[Sync][Stream]fail to synchronize sendFrameStream, result = %d", rtRet);
            (void)rtUnSubscribeReport(channelDesc->threadId, channelDesc->getFrameStream);
            DestroyVencAllNotifyAndStream(channelDesc);
            aclvencFreeOutMemory(channelDesc);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }

        ACL_LOG_INFO("success to call aclvencCreateChannel, channelId = %u, sendFrameNotifyId = %u, "
            "getFrameNotifyId = %u, bufAddr = %llu, bufSize = %u.",
            channelDesc->vencDesc.channelId, channelDesc->vencDesc.sendFrameNotifyId,
            channelDesc->vencDesc.getFrameNotifyId, channelDesc->bufAddr, channelDesc->bufSize);
        ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_VENC_CHANNEL);
        return ACL_SUCCESS;
    }

    aclError VideoProcessor::aclvencDestroyChannel(aclvencChannelDesc *channelDesc)
    {
        ACL_LOG_INFO("start to execute aclvencDestroyChannel");
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);

        // send eos to send stream to finish decode all frame
        aclError aclRet = SendEosForVenc(channelDesc);
        if (aclRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Send][Eos]fail to send eos to sendFrameStream, result = %d", aclRet);
            DestroyVencAllNotifyAndStream(channelDesc);
            aclvencFreeOutMemory(channelDesc);
            return aclRet;
        }

        // destroy channel have 1 input
        constexpr int32_t ioAddrNum = 1;
        constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto cpuParamHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        cpuParamHead->length = argsSize;
        cpuParamHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);

        rtError_t rtRet = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
            acl::dvpp::DVPP_KERNELNAME_DESTROY_VENC_CHANNEL,
            1, // blockDim default 1
            args.get(),
            argsSize,
            nullptr, // no need smDesc
            channelDesc->sendFrameStream);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Destroy][Venc]destroy venc call rtCpuKernelLaunch failed, runtime result = %d", rtRet);
            DestroyVencAllNotifyAndStream(channelDesc);
            aclvencFreeOutMemory(channelDesc);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }

        rtRet = rtStreamSynchronize(channelDesc->sendFrameStream);
        if (rtRet != ACL_SUCCESS) {
            ACL_LOG_CALL_ERROR("[Sync][Stream]fail to synchronize sendFrameStream, result = %d", rtRet);
            DestroyVencAllNotifyAndStream(channelDesc);
            aclvencFreeOutMemory(channelDesc);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }

        // releasing the memory after channel is destroyed
        aclvencFreeOutMemory(channelDesc);

        rtRet = rtUnSubscribeReport(channelDesc->threadId, channelDesc->getFrameStream);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Unsub][Report]fail to UnSubscribeReport thread, runtime result = %d", rtRet);
            // no need to return failed, continue to destory resource
        }

        // destory all notify and stream
        DestroyVencAllNotifyAndStream(channelDesc);
        return ACL_SUCCESS;
    }

    aclError VideoProcessor::aclvencSetChannelDescPicFormat(aclvencChannelDesc *channelDesc,
        acldvppPixelFormat picFormat)
    {
        ACL_LOG_DEBUG("start to execute aclvencSetChannelDescPicFormat");
        if (channelDesc == nullptr) {
            ACL_LOG_ERROR("[Check][ChannelDesc]channelDesc is null.");
            const char *argList[] = {"param"};
            const char *argVal[] = {"videoProcessor"};
            acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                argList, argVal, 1);
            return ACL_ERROR_INVALID_PARAM;
        }
        // venc only support 2 format
        if ((picFormat != PIXEL_FORMAT_YUV_SEMIPLANAR_420) &&
            (picFormat != PIXEL_FORMAT_YVU_SEMIPLANAR_420)) {
            ACL_LOG_INNER_ERROR("[Check][PicFormat]input format[%u] is not supported in this version, "
                "only support nv12 and nv21", static_cast<uint32_t>(picFormat));
            return ACL_ERROR_INVALID_PARAM;
        }

        channelDesc->vencDesc.picFormat = static_cast<uint32_t>(picFormat);
        return ACL_SUCCESS;
    }

    aclError VideoProcessor::aclvdecSetChannelDescMatrix(aclvdecChannelDesc *channelDesc,
        acldvppCscMatrix matrixFormat)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]Setting descMatrix for channel desc is not "
            "supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"Setting descMatrix for channel desc", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError VideoProcessor::aclvdecGetChannelDescMatrix(const aclvdecChannelDesc *channelDesc,
            acldvppCscMatrix &matrixFormat)
    {
        ACL_LOG_ERROR("[Unsupport][Feature]Getting descMatrix for channel desc is not "
            "supported in this version. Please check.");
        const char *argList[] = {"feature", "reason"};
        const char *argVal[] = {"Getting descMatrix for channel desc", "please check"};
        acl::AclErrorLogManager::ReportInputErrorWithChar(acl::UNSUPPORTED_FEATURE_MSG,
            argList, argVal, 2);
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    aclError VideoProcessor::aclvencSendFrame(aclvencChannelDesc *channelDesc, acldvppPicDesc *input, void *reserve,
        aclvencFrameConfig *config, void *userdata)
    {
        ACL_LOG_INFO("start to execute aclvencSendFrame");
        ACL_REQUIRES_NOT_NULL(channelDesc);
        ACL_REQUIRES_NOT_NULL(channelDesc->dataBuffer.data);
        ACL_REQUIRES_NOT_NULL(channelDesc->sendFrameStream);
        ACL_REQUIRES_NOT_NULL(channelDesc->getFrameStream);
        ACL_REQUIRES_NOT_NULL(config);

        // if eos is true, frame data with true eos will not be decoded
        if (config->vencFrameConfig.eos) {
            return aclvencSendEosFrame(channelDesc, config);
        } else {
            return aclvencSendNomalFrame(channelDesc, input, reserve, config, userdata);
        }

        return ACL_SUCCESS;
    }

    acldvppStreamDesc *VideoProcessor::CreateDvppStreamDescOnHost()
    {
        acldvppStreamDesc *aclStreamDesc = nullptr;
        // alloc host memory
        uint32_t aclStreamDescSize = CalAclDvppStructSize(aclStreamDesc);
        ACL_REQUIRES_POSITIVE_RET_NULL(aclStreamDescSize);
        size_t pageSize = mmGetPageSize();
        void *hostAddr = mmAlignMalloc(aclStreamDescSize, pageSize);
        if (hostAddr == nullptr) {
            ACL_LOG_INNER_ERROR("[Malloc][Mem]malloc memory failed. size is %u.", aclStreamDescSize);
            return nullptr;
        }

        // create acldvppStreamDesc in host memory
        aclStreamDesc = new (hostAddr)acldvppStreamDesc;
        RETURN_NULL_WITH_ALIGN_FREE(aclStreamDesc, hostAddr);

        // malloc device memory for vdecChannelDesc
        void *devPtr = nullptr;
        size_t dataSize = CalAclDvppStructSize(&aclStreamDesc->dvppStreamDesc);
        uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
        rtError_t ret = rtMalloc(&devPtr, dataSize, flags);
        if (ret != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Malloc][Mem]malloc device memory for acl dvpp stream desc failed, "
                "runtime result = %d", ret);
            ACL_ALIGN_FREE(hostAddr);
            return nullptr;
        }

        // set data buffer
        aclStreamDesc->dataBuffer.data = devPtr;
        aclStreamDesc->dataBuffer.length = dataSize;

        return aclStreamDesc;
    }

    acldvppStreamDesc *VideoProcessor::CreateDvppStreamDescOnDevice()
    {
        acldvppStreamDesc *aclStreamDesc = nullptr;
        // alloc device memory
        void *devAddr = nullptr;
        uint32_t aclStreamDescSize = CalAclDvppStructSize(aclStreamDesc);
        uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
        rtError_t rtErr = rtMalloc(&devAddr, aclStreamDescSize, flags);
        if (rtErr != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Malloc][Mem]alloc device memory failed, size = %zu, "
                "runtime result = %d", aclStreamDescSize, rtErr);
            return nullptr;
        }

        // create acldvppStreamDesc in device addr
        aclStreamDesc = new (devAddr)acldvppStreamDesc;
        if (aclStreamDesc == nullptr) {
            ACL_LOG_INNER_ERROR("[Malloc][StreamDesc]new acldvppStreamDesc failed");
            (void) rtFree(devAddr);
            devAddr = nullptr;
            return nullptr;
        }

        // set data buffer
        size_t offset = OFFSET_OF_MEMBER(acldvppStreamDesc, dvppStreamDesc);
        aclStreamDesc->dataBuffer.data = reinterpret_cast<aicpu::dvpp::DvppVdecDesc *>(
            reinterpret_cast<uintptr_t>(devAddr) + offset);
        aclStreamDesc->dataBuffer.length = CalAclDvppStructSize(&aclStreamDesc->dvppStreamDesc);

        return aclStreamDesc;
    }

    aclvdecChannelDesc *VideoProcessor::CreateVdecChannelDescOnHost()
    {
        aclvdecChannelDesc *aclChannelDesc = nullptr;
        // alloc host memory
        uint32_t aclChannelDescSize = CalAclDvppStructSize(aclChannelDesc);
        ACL_REQUIRES_POSITIVE_RET_NULL(aclChannelDescSize);
        size_t pageSize = mmGetPageSize();
        void *hostAddr = mmAlignMalloc(aclChannelDescSize, pageSize);
        if (hostAddr == nullptr) {
            ACL_LOG_INNER_ERROR("[Malloc][Mem]malloc memory failed. size is %u.", aclChannelDescSize);
            return nullptr;
        }

        // create aclvdecChannelDesc in host addr
        aclChannelDesc = new (hostAddr)aclvdecChannelDesc;
        if ((aclChannelDesc == nullptr) || (aclChannelDesc->vdecDesc.extendInfo == nullptr)) {
            ACL_LOG_INNER_ERROR("[Create][VdecChannelDesc]create aclvdecChannelDesc with function new failed");
            ACL_ALIGN_FREE(hostAddr);
            return nullptr;
        }
        auto err = memset_s(aclChannelDesc->vdecDesc.extendInfo, acl::dvpp::VDEC_CHANNEL_DESC_TLV_LEN,
            0, acl::dvpp::VDEC_CHANNEL_DESC_TLV_LEN);
        if (err != EOK) {
            ACL_LOG_INNER_ERROR("[Set][Mem]set vdecDesc extendInfo to 0 failed, dstLen = %u, srclen = %u, "
                "result = %d.", aclChannelDesc->vdecDesc.len, aclChannelDesc->vdecDesc.len, err);
            aclChannelDesc->~aclvdecChannelDesc();
            ACL_ALIGN_FREE(hostAddr);
            return nullptr;
        }
        // malloc device memory for vdecChannelDesc
        void *devPtr = nullptr;
        size_t dataSize = CalAclDvppStructSize(&aclChannelDesc->vdecDesc);
        size_t totalSize = dataSize + acl::dvpp::VDEC_CHANNEL_SHARE_BUFFER_SIZE;
        uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
        rtError_t ret = rtMalloc(&devPtr, totalSize, flags);
        if (ret != RT_ERROR_NONE) {
            ACL_LOG_INNER_ERROR("[Malloc][Mem]malloc device memory for acl vdec channel desc failed, "
                "runtime result = %d", ret);
            aclChannelDesc->~aclvdecChannelDesc();
            ACL_ALIGN_FREE(hostAddr);
            return nullptr;
        }

        // set data buffer
        aclChannelDesc->dataBuffer.data = devPtr;
        aclChannelDesc->dataBuffer.length = dataSize;

        aclChannelDesc->shareBuffer.data = static_cast<char *>(devPtr) + dataSize;
        aclChannelDesc->shareBuffer.length = acl::dvpp::VDEC_CHANNEL_SHARE_BUFFER_SIZE;

        return aclChannelDesc;
    }

    aclvdecChannelDesc *VideoProcessor::CreateVdecChannelDescOnDevice()
    {
        aclvdecChannelDesc *aclChannelDesc = nullptr;
        // alloc device memory
        void *devAddr = nullptr;
        uint32_t aclChannelDescSize = CalAclDvppStructSize(aclChannelDesc);
        uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
        rtError_t rtErr = rtMalloc(&devAddr, aclChannelDescSize, flags);
        if (rtErr != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Malloc][Mem]alloc device memory failed, size = %zu, runtime result = %d",
                aclChannelDescSize, rtErr);
            return nullptr;
        }

        // create aclvdecChannelDesc in device addr
        aclChannelDesc = new (devAddr)aclvdecChannelDesc;
        if ((aclChannelDesc == nullptr) || (aclChannelDesc->vdecDesc.extendInfo == nullptr)) {
            ACL_LOG_INNER_ERROR("[Create][ChannelDesc]create aclvdecChannelDesc with function new failed");
            aclChannelDesc->~aclvdecChannelDesc();
            (void) rtFree(devAddr);
            devAddr = nullptr;
            return nullptr;
        }
        auto err = memset_s(aclChannelDesc->vdecDesc.extendInfo, acl::dvpp::VDEC_CHANNEL_DESC_TLV_LEN,
            0, acl::dvpp::VDEC_CHANNEL_DESC_TLV_LEN);
        if (err != EOK) {
            ACL_LOG_INNER_ERROR("[Set][Mem]set vdecDesc extendInfo to 0 failed, dstLen = %u, srclen = %u, "
                "result = %d.", aclChannelDesc->vdecDesc.len, aclChannelDesc->vdecDesc.len, err);
            aclChannelDesc->~aclvdecChannelDesc();
            (void) rtFree(devAddr);
            devAddr = nullptr;
            return nullptr;
        }

        // set data buffer
        size_t offset = OFFSET_OF_MEMBER(aclvdecChannelDesc, vdecDesc);
        aclChannelDesc->dataBuffer.data = reinterpret_cast<aicpu::dvpp::DvppVdecDesc *>(
            reinterpret_cast<uintptr_t>(devAddr) + offset);
        aclChannelDesc->dataBuffer.length = CalAclDvppStructSize(&aclChannelDesc->vdecDesc);

        // malloc device addr for share buffer
        offset = OFFSET_OF_MEMBER(aclvdecChannelDesc, callbackResult);
        aclChannelDesc->shareBuffer.data = reinterpret_cast<VdecCallbackResultInfo *>(
            reinterpret_cast<uintptr_t>(devAddr) + offset);
        aclChannelDesc->shareBuffer.length = acl::dvpp::VDEC_CHANNEL_SHARE_BUFFER_SIZE;

        return aclChannelDesc;
    }

    aclError VideoProcessor::SendEosForVdec(aclvdecChannelDesc *channelDesc)
    {
        // create input stream desc
        acldvppStreamDesc *eosStreamDesc = acldvppCreateStreamDesc();
        if (eosStreamDesc == nullptr) {
            ACL_LOG_INNER_ERROR("[Create][Stream]fail to create dvpp stream desc for eos.");
            return ACL_ERROR_FAILURE;
        }

        // set eos for eos stream desc
        aclError ret = acldvppSetStreamDescEos(eosStreamDesc, true);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Set][Stream]fail to set eos for stream desc, result = %d", ret);
            (void) acldvppDestroyStreamDesc(eosStreamDesc);
            return ret;
        }

        // send eos and synchronize
        ret = aclvdecSendFrame(channelDesc, eosStreamDesc, nullptr, nullptr, nullptr, false);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Send][Frame]fail to send eos, result = %d", ret);
            (void) acldvppDestroyStreamDesc(eosStreamDesc);
            return ret;
        }

        // destroy input stream desc
        ret = acldvppDestroyStreamDesc(eosStreamDesc);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Destroy][StreamDesc]fail to destory dvpp stream desc for eos, result = %d", ret);
            return ret;
        }
        return ACL_SUCCESS;
    }

    aclError VideoProcessor::GetVdecCallInfo(aclvdecChannelDesc *channelDesc, acldvppStreamDesc *&input,
        acldvppPicDesc *&output, void *&userData, bool &eos)
    {
        rtError_t ret = rtMemcpy(static_cast<void *>(&(channelDesc->callbackResult)),
            sizeof(VdecCallbackResultInfo), channelDesc->shareBuffer.data,
            sizeof(VdecCallbackResultInfo), RT_MEMCPY_DEVICE_TO_HOST);
        if (ret != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Copy][Mem]memcpy frameId to host memory failed, dstSize = %zu, srcSize = %zu, "
                "runtime result = %d", sizeof(VdecCallbackResultInfo), sizeof(VdecCallbackResultInfo), ret);
            return ACL_GET_ERRCODE_RTS(ret);
        }
        uint64_t frameId = channelDesc->callbackResult.frameId;
        aicpu::dvpp::VdecCallbackInfoPtr callbackInfoPtr = nullptr;
        {
            std::unique_lock<std::mutex> lock{channelDesc->mutexForCallbackMap};
            if (channelDesc->callbackMap.find(frameId) == channelDesc->callbackMap.end()) {
                ACL_LOG_INNER_ERROR("[Find][FrameId]the value of frameId[%lu] is invalid, channelId = %u",
                    frameId, channelDesc->vdecDesc.channelId);
                return ACL_ERROR_INTERNAL_ERROR;
            }
            callbackInfoPtr = channelDesc->callbackMap[frameId];
            channelDesc->callbackMap.erase(frameId);
        }
        if (callbackInfoPtr == nullptr) {
            ACL_LOG_INNER_ERROR("[Check][CallbackInfoPtr]callbackInfoPtr is nullptr, frameId = %lu.", frameId);
            return ACL_ERROR_INTERNAL_ERROR;
        }
        if (callbackInfoPtr->outputPicDesc != nullptr) {
            callbackInfoPtr->outputPicDesc->dvppPicDesc = channelDesc->callbackResult.dvppPicDesc;
            output = callbackInfoPtr->outputPicDesc;
        }
        input = callbackInfoPtr->inputStreamDesc;
        userData = callbackInfoPtr->callbackData;
        eos = callbackInfoPtr->eos;
        ACL_LOG_DEBUG("vdec callback: channelId=%u, frameId = %lu, eos = %d",
            channelDesc->vdecDesc.channelId, frameId, eos);

        return ACL_SUCCESS;
    }

    void VideoProcessor::GetVdecFrameCallback(void *callbackData)
    {
        aclvdecChannelDesc *channelDesc =  reinterpret_cast<aclvdecChannelDesc *>(callbackData);
        if ((channelDesc == nullptr) || (channelDesc->shareBuffer.data == nullptr)) {
            ACL_LOG_INNER_ERROR("[Check][Pointer]GetVdecFrameCallback callbackData = %p, "
                "but channelDesc or shareBuffer is null.", callbackData);
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
                ACL_LOG_CALL_ERROR("[Get][Context]get ctx failed, runtime result = %d, "
                    "channelId = %u.", getCtxRet, channelId);
                return;
            }
            // set context of vdec main thread to current callback thread
            rtError_t setCtxRet = rtCtxSetCurrent(channelDesc->vdecMainContext);
            if (setCtxRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Set][Context]set ctx failed, runtime result = %d, "
                    "channelId = %u.", setCtxRet, channelId);
                return;
            }
            // launch task
            aclError launchRet = LaunchTaskForGetStream(channelDesc);
            if (launchRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Get][Context]launch task for get stream failed.");
                (void) rtCtxSetCurrent(curContext);
                return;
            }
            // reset context of current callback thread
            rtError_t resetCtxRet = rtCtxSetCurrent(curContext);
            if (resetCtxRet != RT_ERROR_NONE) {
                ACL_LOG_INNER_ERROR("[Set][Context]set ctx failed, runtime result = %d, channelId = %u.",
                    resetCtxRet, channelId);
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

    aclError VideoProcessor::LaunchTaskForGetStream(aclvdecChannelDesc *channelDesc)
    {
        static uint32_t launchTaskCount[VDEC_CHANNEL_ID_CEILING + 1] = {0};
        if (channelDesc->isNeedNotify) {
            // launch get frame task to get frame stream
            aclError getFrameRet = LaunchGetFrameTask(channelDesc, acl::dvpp::DVPP_KERNELNAME_GET_FRAME);
            if (getFrameRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Launch][Task]launch get frame task failed, result = %d", getFrameRet);
                return getFrameRet;
            }

            // launch wait task to get frame stream
            rtError_t getFrameWaitRet = rtNotifyWait(static_cast<rtNotify_t>(channelDesc->getFrameNotify),
                channelDesc->getFrameStream);
            if (getFrameWaitRet != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Wait][Notify]wait for a notify to get frame stream failed, runtime result = %d",
                    getFrameWaitRet);
                return ACL_GET_ERRCODE_RTS(getFrameWaitRet);
            }
        } else {
            // launch get frame task to get frame stream
            aclError getFrameRet = LaunchGetFrameTask(channelDesc, acl::dvpp::DVPP_KERNELNAME_GET_FRAME_V2);
            if (getFrameRet != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Get][FrameTask]launch get frame task failed, result = %d", getFrameRet);
                return getFrameRet;
            }
        }

        // launch callback task to get frame stream
        rtError_t callbackRet = rtCallbackLaunch(GetVdecFrameCallback,
                                                 static_cast<void *>(channelDesc),
                                                 channelDesc->getFrameStream, true);
        if (callbackRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Launch][Callback]launch callback to get frame stream failed, "
                "runtime result = %d", callbackRet);
            return ACL_GET_ERRCODE_RTS(callbackRet);
        }

        uint32_t channelId = channelDesc->vdecDesc.channelId;
        ACL_LOG_INFO("Launch task for get stream success. launchTaskCount = %u, channelId = %u, getStreamId = %u.",
            ++launchTaskCount[channelId], channelId, channelDesc->getStreamId);
        return ACL_SUCCESS;
    }

    aclError VideoProcessor::LaunchTaskForSendStream(aclvdecChannelDesc *channelDesc,
                                                     acldvppStreamDesc *input,
                                                     acldvppPicDesc *output,
                                                     bool eos)
    {
        if (channelDesc->isNeedNotify) {
            // launch send frame task to send frame stream
            aclError sendFrameRetVal = LaunchSendFrameTask(channelDesc, input, output, eos,
                acl::dvpp::DVPP_KERNELNAME_SEND_FRAME);
            if (sendFrameRetVal != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Launch][Task]launch send frame task failed, result = %d", sendFrameRetVal);
                return sendFrameRetVal;
            }
            rtError_t sendFrameWaitRetVal;
            if (channelDesc->vdecWaitTaskType == NOTIFY_TASK) {
                // launch notify wait to send frame stream
                sendFrameWaitRetVal = rtNotifyWait(static_cast<rtNotify_t>(channelDesc->sendFrameNotify),
                    channelDesc->sendFrameStream);
                if (sendFrameWaitRetVal != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Wait][Notify]wait for a notify to send frame stream failed, "
                        "runtime result = %d", sendFrameWaitRetVal);
                    return ACL_GET_ERRCODE_RTS(sendFrameWaitRetVal);
                }
            } else {
                sendFrameWaitRetVal = rtStreamWaitEvent(channelDesc->sendFrameStream,
                    static_cast<rtEvent_t>(channelDesc->sendFrameNotify));
                if (sendFrameWaitRetVal != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Wait][Event]wait for a event to send frame stream failed, runtime result = %d",
                        sendFrameWaitRetVal);
                    return ACL_GET_ERRCODE_RTS(sendFrameWaitRetVal);
                }
                sendFrameWaitRetVal = rtEventReset(static_cast<rtEvent_t>(channelDesc->sendFrameNotify),
                    channelDesc->sendFrameStream);
                if (sendFrameWaitRetVal != RT_ERROR_NONE) {
                    ACL_LOG_CALL_ERROR("[Reset][Event]reset a event to send frame stream failed, runtime result = %d",
                        sendFrameWaitRetVal);
                    return ACL_GET_ERRCODE_RTS(sendFrameWaitRetVal);
                }
            }
        } else {
            // launch send frame task to send frame stream
            aclError sendFrameRetVal = LaunchSendFrameTask(channelDesc, input, output, eos,
                acl::dvpp::DVPP_KERNELNAME_SEND_FRAME_V2);
            if (sendFrameRetVal != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Launch][Task]launch send frame task failed, result = %d", sendFrameRetVal);
                return sendFrameRetVal;
            }
        }

        return ACL_SUCCESS;
    }

    void VideoProcessor::ClearCachedTasks(aclvdecChannelDesc *channelDesc)
    {
        // frameCounter reset to 0
        ACL_LOG_INFO("task queue size before clear is %zu.", channelDesc->taskQueue.size());
        while (!channelDesc->taskQueue.empty()) {
            channelDesc->taskQueue.pop();
        }
        // reset eos back flag to false
        channelDesc->eosBackFlag.store(false);
        channelDesc->queueEmptyFlag.store(true);
        {
            std::unique_lock<std::mutex> lock{channelDesc->mutexForCallbackMap};
            channelDesc->callbackMap.clear();
        }
        channelDesc->frameId = 0;
    }

    void VideoProcessor::DestroyAllNotifyForVdecChannel(aclvdecChannelDesc *channelDesc)
    {
        if (channelDesc->sendFrameNotify != nullptr) {
            rtError_t rtRetVal = rtNotifyDestroy(static_cast<rtNotify_t>(channelDesc->sendFrameNotify));
            if (rtRetVal != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Destroy][Notify]fail to destroy sendFrameNotify, runtime result = %d", rtRetVal);
            }
            channelDesc->sendFrameNotify = nullptr;
        }

        if (channelDesc->getFrameNotify != nullptr) {
            rtError_t rtRetVal = rtNotifyDestroy(static_cast<rtNotify_t>(channelDesc->getFrameNotify));
            if (rtRetVal != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Destroy][Notify]fail to destroy getFrameNotify, runtime result = %d", rtRetVal);
            }
            channelDesc->getFrameNotify = nullptr;
        }
    }

    void VideoProcessor::DestroyAllEventForVdecChannel(aclvdecChannelDesc *channelDesc)
    {
        if (channelDesc->sendFrameNotify != nullptr) {
            rtError_t rtRetVal = rtEventDestroy(static_cast<rtEvent_t>(channelDesc->sendFrameNotify));
            if (rtRetVal != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Destroy][Event]fail to destroy sendFrameNotify, runtime result = %d", rtRetVal);
            }
            channelDesc->sendFrameNotify = nullptr;
        }

        if (channelDesc->getFrameNotify != nullptr) {
            rtError_t rtRetVal = rtEventDestroy(static_cast<rtEvent_t>(channelDesc->getFrameNotify));
            if (rtRetVal != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Destroy][Event]fail to destroy getFrameNotify, runtime result = %d", rtRetVal);
            }
            channelDesc->getFrameNotify = nullptr;
        }
    }

    void VideoProcessor::DestroyAllNotifyAndStreamForVdecChannel(aclvdecChannelDesc *channelDesc, bool isNeedNotify)
    {
        ACL_LOG_DEBUG("begin to destroy all notify and stream.");
        if (channelDesc == nullptr) {
            ACL_LOG_ERROR("[Check][ChannelDesc]vdec channel desc is null.");
            const char *argList[] = {"param"};
            const char *argVal[] = {"channelDesc"};
            acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                argList, argVal, 1);
            return;
        }

        if (channelDesc->sendFrameStream != nullptr) {
            rtError_t rtRetVal = rtStreamDestroy(channelDesc->sendFrameStream);
            if (rtRetVal != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Destroy][Stream]fail to destroy sendFrameStream, runtime result = %d", rtRetVal);
            }
            channelDesc->sendFrameStream = nullptr;
        }

        if (channelDesc->getFrameStream != nullptr) {
            rtError_t rtRetVal = rtStreamDestroy(channelDesc->getFrameStream);
            if (rtRetVal != RT_ERROR_NONE) {
                ACL_LOG_CALL_ERROR("[Destroy][Stream]fail to destroy getFrameStream, runtime result = %d", rtRetVal);
            }
            channelDesc->getFrameStream = nullptr;
        }

        if (isNeedNotify) {
            if (channelDesc->vdecWaitTaskType == NOTIFY_TASK) {
                DestroyAllNotifyForVdecChannel(channelDesc);
            } else {
                DestroyAllEventForVdecChannel(channelDesc);
            }
        }

        ACL_LOG_DEBUG("success to destroy all notify and stream.");
    }

    aclError VideoProcessor::CreateNotifyForVdecChannel(aclvdecChannelDesc *channelDesc)
    {
        int32_t deviceId = 0;
        rtError_t rtRet = rtGetDevice(&deviceId);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Get][Device]fail to get deviceId when create vdec channel, "
                "runtime result = %d", rtRet);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }

        // create notify for send frame
        rtNotify_t sendFrameNotify = nullptr;
        rtRet = rtNotifyCreate(deviceId, &sendFrameNotify);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Create][Notify]fail to create sendFrameNotify, runtime result = %d", rtRet);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        channelDesc->sendFrameNotify = static_cast<void *>(sendFrameNotify);

        // create notify for get frame
        rtNotify_t getFrameNotify = nullptr;
        rtRet = rtNotifyCreate(deviceId, &getFrameNotify);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Create][Notify]fail to create getFrameNotify, runtime result = %d", rtRet);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, true);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        channelDesc->getFrameNotify = static_cast<void *>(getFrameNotify);

        // get notifyId for send frame
        uint32_t sendFrameNotifyId = 0;
        rtRet = rtGetNotifyID(sendFrameNotify, &sendFrameNotifyId);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Get][NotifyID]fail to get sendFrameNotifyId, runtime result = %d", rtRet);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, true);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        ACL_LOG_INFO("sendFrameNotifyId is %u", sendFrameNotifyId);
        channelDesc->vdecDesc.sendFrameNotifyId = sendFrameNotifyId;

        // get notifyId for get frame
        uint32_t getFrameNotifyId = 0;
        rtRet = rtGetNotifyID(getFrameNotify, &getFrameNotifyId);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Get][NotifyID]fail to get getFrameNotifyId, runtime result = %d", rtRet);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, true);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        ACL_LOG_INFO("getFrameNotifyId is %u", getFrameNotifyId);
        channelDesc->vdecDesc.getFrameNotifyId = getFrameNotifyId;

        return ACL_SUCCESS;
    }

    aclError VideoProcessor::CreateEventForVdecChannel(aclvdecChannelDesc *channelDesc)
    {
        // create event for send frame
        rtEvent_t sendFrameEvent = nullptr;
        rtError_t rtRet = rtEventCreateWithFlag(&sendFrameEvent, RT_EVENT_WITH_FLAG);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Create][Event]fail to create sendFrameEvent, runtime result = %d", rtRet);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        channelDesc->sendFrameNotify = static_cast<void *>(sendFrameEvent);

        // create event for get frame
        rtEvent_t getFrameEvent = nullptr;
        rtRet = rtEventCreateWithFlag(&getFrameEvent, RT_EVENT_WITH_FLAG);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Create][Event]fail to create getFrameEvent, runtime result = %d", rtRet);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, true);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        channelDesc->getFrameNotify = static_cast<void *>(getFrameEvent);

        // get eventId for send frame
        uint32_t sendFrameEventId = 0;
        rtRet = rtGetEventID(sendFrameEvent, &sendFrameEventId);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Get][EventID]fail to get sendFrameEventId, runtime result = %d", rtRet);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, true);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        ACL_LOG_INFO("sendFrameEventId is %u", sendFrameEventId);
        channelDesc->vdecDesc.sendFrameNotifyId = sendFrameEventId;

        // get eventId for get frame
        uint32_t getFrameEventId = 0;
        rtRet = rtGetEventID(getFrameEvent, &getFrameEventId);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Get][EventID]fail to get getFrameEventId, runtime result = %d", rtRet);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, true);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        ACL_LOG_INFO("getFrameEventId is %u", getFrameEventId);
        channelDesc->vdecDesc.getFrameNotifyId = getFrameEventId;

        return ACL_SUCCESS;
    }

    aclError VideoProcessor::CreateNotifyAndStreamForVdecChannel(aclvdecChannelDesc *channelDesc, bool isNeedNotify)
    {
        if (isNeedNotify) {
            aclError aclRet;
            if (channelDesc->vdecWaitTaskType == NOTIFY_TASK) {
                aclRet = CreateNotifyForVdecChannel(channelDesc);
                if (aclRet != ACL_SUCCESS) {
                    return aclRet;
                }
            } else {
                aclRet = CreateEventForVdecChannel(channelDesc);
                if (aclRet != ACL_SUCCESS) {
                    return aclRet;
                }
            }
        }

        // create stream for send frame
        rtStream_t sendFrameStream = nullptr;
        rtError_t rtRet = rtStreamCreate(&sendFrameStream, RT_STREAM_PRIORITY_DEFAULT);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Create][Stream]fail to create sendFrameStream, runtime result = %d", rtRet);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, isNeedNotify);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        channelDesc->sendFrameStream = sendFrameStream;

        // create stream for get frame
        rtStream_t getFrameStream = nullptr;
        rtRet = rtStreamCreate(&getFrameStream, RT_STREAM_PRIORITY_DEFAULT);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Create][Stream]fail to create getFrameStream, runtime result = %d", rtRet);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, isNeedNotify);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        channelDesc->getFrameStream = getFrameStream;

        // get streamId for get frame
        int32_t sendStreamId = 0;
        rtRet = rtGetStreamId(sendFrameStream, &sendStreamId);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Get][StreamId]fail to get sendStreamId, runtime result = %d", rtRet);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, isNeedNotify);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        channelDesc->sendStreamId = sendStreamId;

        // get streamId for send frame
        int32_t getStreamId = 0;
        rtRet = rtGetStreamId(getFrameStream, &getStreamId);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Get][StreamId]fail to get getStreamId, runtime result = %d", rtRet);
            DestroyAllNotifyAndStreamForVdecChannel(channelDesc, isNeedNotify);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        channelDesc->getStreamId = getStreamId;

        return ACL_SUCCESS;
    }

    aclError VideoProcessor::LaunchSendFrameTask(const aclvdecChannelDesc *channelDesc,
                                                 acldvppStreamDesc *inputStreamDesc,
                                                 acldvppPicDesc *outputPicDesc,
                                                 bool eos,
                                                 const char *kernelName)
    {
        // send frame task has 3 input
        constexpr int32_t ioAddrNum = 3;
        constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t) + sizeof(uint64_t);
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        int i = 0;
        ioAddr[i++] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        ioAddr[i++] = reinterpret_cast<uintptr_t>(inputStreamDesc->dataBuffer.data);
        if ((!eos) && (outputPicDesc != nullptr)) {
            ioAddr[i] = reinterpret_cast<uintptr_t>(outputPicDesc->dataBuffer.data);
        } else {
            ioAddr[i] = reinterpret_cast<uintptr_t>(nullptr);
        }
        auto frameId = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead)
            + ioAddrNum * sizeof(uint64_t));
        if (!eos) {
            *frameId = channelDesc->frameId;
        } else {
            *frameId = 0; // 0 represents eos frame id
            ACL_LOG_INFO("vdec process eos frame: channelId = %u, frameId = %lu",
                channelDesc->vdecDesc.channelId, *frameId);
        }

        rtError_t rtRet = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
                                            kernelName,
                                            1, // blockDim default 1
                                            args.get(),
                                            argsSize,
                                            nullptr, // no need smDesc
                                            channelDesc->sendFrameStream);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Launch][Task]vdec send frame task call rtCpuKernelLaunch failed, "
                "runtime result = %d.", rtRet);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        return ACL_SUCCESS;
    }

    aclError VideoProcessor::LaunchGetFrameTask(const aclvdecChannelDesc *channelDesc, const char *kernelName)
    {
        // get frame task has 1 input
        constexpr int32_t ioAddrNum = 1;
        constexpr uint32_t argsSize = sizeof(aicpu::AicpuParamHead) + ioAddrNum * sizeof(uint64_t);
        std::unique_ptr<char[]> args(new (std::nothrow)char[argsSize]);
        ACL_REQUIRES_NOT_NULL(args);
        auto paramHead = reinterpret_cast<aicpu::AicpuParamHead *>(args.get());
        paramHead->length = argsSize;
        paramHead->ioAddrNum = ioAddrNum;
        auto ioAddr = reinterpret_cast<uint64_t *>(args.get() + sizeof(aicpu::AicpuParamHead));
        ioAddr[0] = reinterpret_cast<uintptr_t>(channelDesc->dataBuffer.data);
        rtError_t rtRet = rtCpuKernelLaunch(acl::dvpp::DVPP_KERNELS_SONAME,
                                            kernelName,
                                            1, // blockDim default 1
                                            args.get(),
                                            argsSize,
                                            nullptr, // no need smDesc
                                            channelDesc->getFrameStream);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Launch][Task]vdec get frame task call rtCpuKernelLaunch failed, "
                "runtime result = %d.", rtRet);
            return ACL_GET_ERRCODE_RTS(rtRet);
        }
        return ACL_SUCCESS;
    }

    /**
     * VDEC set outout pic Wdith for channel desc.
     * @param channelDesc[in|out] channel desc
     * @param outPicWidth[in] outPicWidth
     * @return ACL_SUCCESS for ok, others for fail
     */
    aclError VideoProcessor::aclvdecSetChannelDescOutPicWidth(aclvdecChannelDesc *channelDesc,
                                                              uint32_t outPicWidth)
    {
        ACL_LOG_DEBUG("start to execute aclvdecSetChannelDescOutPicWidth");
        ACL_REQUIRES_NOT_NULL(channelDesc);

        channelDesc->vdecDesc.outPicWidth = outPicWidth;
        return ACL_SUCCESS;
    }

    /**
     * VDEC set outout pic Height for channel desc.
     * @param channelDesc[in|out] channel desc
     * @param outHeight[in] outHeight
     * @return ACL_SUCCESS for ok, others for fail
     */
    aclError VideoProcessor::aclvdecSetChannelDescOutPicHeight(aclvdecChannelDesc *channelDesc,
                                                               uint32_t outPicHeight)
    {
        ACL_LOG_DEBUG("start to execute aclvdecSetChannelDescOutPicHeight");
        ACL_REQUIRES_NOT_NULL(channelDesc);

        channelDesc->vdecDesc.outPicHeight = outPicHeight;
        return ACL_SUCCESS;
    }

    /**
     * VDEC set refFrameNum for channel desc.
     * @param channelDesc[in|out] channel desc
     * @param refFrameNum[in] refFrameNum
     * @return ACL_SUCCESS for ok, others for fail
     */
    aclError VideoProcessor::aclvdecSetChannelDescRefFrameNum(aclvdecChannelDesc *channelDesc,
                                                              uint32_t refFrameNum)
    {
        ACL_LOG_INNER_ERROR("[Unsupport][Feature]Setting refFrameNum for channel desc is not "
            "supported in this version. Please check.");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    /**
     * VDEC get refFrameNum for channel desc.
     * @param channelDesc[in] channel desc
     * @retval number of reference frames, default 0.
     */
    uint32_t VideoProcessor::aclvdecGetChannelDescRefFrameNum(const aclvdecChannelDesc *channelDesc)
    {
        ACL_LOG_INNER_ERROR("[Unsupport][Feature]Getting refFrameNum for channel desc is not "
            "supported in this version. Please check.");
        return 0;
    }

    /**
      * VDEC set outPicFormat for channel desc.
      * @param channelDesc[in|out] channel desc
      * @param outPicFormat[in] outPicFormat
      * @return ACL_SUCCESS for ok, others for fail
      */
    aclError VideoProcessor::aclvdecSetChannelDescOutPicFormat(aclvdecChannelDesc *channelDesc,
                                                               acldvppPixelFormat outPicFormat)
    {
        ACL_LOG_INNER_ERROR("[Unsupport][Feature]Setting outPicFormat for channel desc is not "
            "supported in this version. Please check.");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    /**
     * VDEC set channel id for channel desc.
     * @param channelDesc[in|out] channel desc
     * @param channelId[in] channel id
     * @return ACL_SUCCESS for ok, others for fail
     */
    aclError VideoProcessor::aclvdecSetChannelDescChannelId(aclvdecChannelDesc *channelDesc,
                                                            uint32_t channelId)
    {
        ACL_LOG_INNER_ERROR("[Unsupport][Feature]Setting channelId for channel desc is not "
            "supported in this version. Please check.");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    /**
     * VDEC set bit depth for channel desc.
     * @param channelDesc [IN]     vdec channel description.
     * @param bitDepth [IN]     bit depth.
     * @return ACL_SUCCESS for ok, others for fail
     */
    aclError VideoProcessor::aclvdecSetChannelDescBitDepth(aclvdecChannelDesc *channelDesc, uint32_t bitDepth)
    {
        ACL_LOG_INNER_ERROR("[Unsupport][Feature]Setting bit depth for vedc channel desc is "
            "not supported in this version. Please check.");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    /**
     * VDEC get bit depth for channel desc.
     * @param channelDesc [IN]    vdec channel description.
     * @retval bit depth, default 0.
     */
    uint32_t VideoProcessor::aclvdecGetChannelDescBitDepth(const aclvdecChannelDesc *channelDesc)
    {
        ACL_LOG_INNER_ERROR("[Unsupport][Feature]Getting bit depth for vedc channel desc is "
            "not supported in this version. Please check.");
        return 0;
    }

    /**
     * VENC set buf size for channel desc.
     * @param channelDesc[in|out] channel desc
     * @param bufSize[in] buf size
     * @return ACL_SUCCESS for ok, others for fail
     */
    aclError VideoProcessor::aclvencSetChannelDescBufSize(aclvencChannelDesc *channelDesc, uint32_t bufSize)
    {
        ACL_LOG_INNER_ERROR("[Unsupport][Feature]Setting bufsize for channel desc is "
            "not supported in this version. Please check.");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    /**
     * VENC set buf addr for channel desc.
     * @param channelDesc[in|out] channel desc
     * @param bufAddr[in] buf addr
     * @return ACL_SUCCESS for ok, others for fail
     */
    aclError VideoProcessor::aclvencSetChannelDescBufAddr(aclvencChannelDesc *channelDesc, void *bufAddr)
    {
        ACL_LOG_INNER_ERROR("[Unsupport][Feature]Setting bufAddr for channel desc is not "
            "supported in this version. Please check.");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    /**
     * Set rc model for venc channel desc.
     * @param channelDesc[IN/OUT] venc channel desc
     * @param rcMode[IN] venc rc mode(VBR=1, CBR=2)
     * @return ACL_SUCCESS for success, other for failure
     */
    aclError VideoProcessor::aclvencSetChannelDescRcMode(aclvencChannelDesc *channelDesc, uint32_t rcMode)
    {
        ACL_LOG_INFO("start to execute aclvencSetChannelDescRcMode, rcMode %u", rcMode);
        ACL_REQUIRES_NOT_NULL(channelDesc);

        {
            std::unique_lock<std::mutex> lock{channelDesc->mutexForTLVMap};
            auto it = channelDesc->tlvParamMap.find(VENC_RATE_CONTROL);
            if (it == channelDesc->tlvParamMap.end()) {
                VencChannelDescTLVParam vencTLVParam;
                std::shared_ptr<aicpu::dvpp::VencRateControl> vencRateControl =
                    std::make_shared<aicpu::dvpp::VencRateControl>();
                ACL_REQUIRES_NOT_NULL(vencRateControl);
                vencRateControl->rcMode = rcMode;
                vencTLVParam.value = static_cast<std::shared_ptr<void>>(vencRateControl);
                vencTLVParam.valueLen = sizeof(aicpu::dvpp::VencRateControl);
                channelDesc->tlvParamMap[VENC_RATE_CONTROL] = vencTLVParam;
            } else {
                aicpu::dvpp::VencRateControl *vencRateControl =
                    static_cast<aicpu::dvpp::VencRateControl *>(it->second.value.get());
                ACL_REQUIRES_NOT_NULL(vencRateControl);
                vencRateControl->rcMode = rcMode;
            }
        }

        return ACL_SUCCESS;
    }

    /**
     * Set source rate for venc channel desc.
     * @param channelDesc[IN/OUT] venc channel desc
     * @param srcRate[IN] source rate
     * @return ACL_SUCCESS for success, other for failure
     */
    aclError VideoProcessor::aclvencSetChannelDescSrcRate(aclvencChannelDesc *channelDesc, uint32_t srcRate)
    {
        ACL_LOG_INFO("start to execute aclvencSetChannelDescSrcRate, srcRate %u", srcRate);
        ACL_REQUIRES_NOT_NULL(channelDesc);

        {
            std::unique_lock<std::mutex> lock{channelDesc->mutexForTLVMap};
            auto it = channelDesc->tlvParamMap.find(VENC_RATE_CONTROL);
            if (it == channelDesc->tlvParamMap.end()) {
                VencChannelDescTLVParam vencTLVParam;
                std::shared_ptr<aicpu::dvpp::VencRateControl> vencRateControl =
                    std::make_shared<aicpu::dvpp::VencRateControl>();
                ACL_REQUIRES_NOT_NULL(vencRateControl);
                vencRateControl->srcRate = srcRate;
                vencTLVParam.value = static_cast<std::shared_ptr<void>>(vencRateControl);
                vencTLVParam.valueLen = sizeof(aicpu::dvpp::VencRateControl);
                channelDesc->tlvParamMap[VENC_RATE_CONTROL] = vencTLVParam;
            } else {
                aicpu::dvpp::VencRateControl *vencRateControl =
                    static_cast<aicpu::dvpp::VencRateControl *>(it->second.value.get());
                ACL_REQUIRES_NOT_NULL(vencRateControl);
                vencRateControl->srcRate = srcRate;
            }
        }

        return ACL_SUCCESS;
    }

    /**
     * Set max bit rate for venc channel desc.
     * @param channelDesc[IN/OUT] venc channel desc
     * @param maxBitRate[IN] max bit rate
     * @return ACL_SUCCESS for success, other for failure
     */
    aclError VideoProcessor::aclvencSetChannelDescMaxBitRate(aclvencChannelDesc *channelDesc, uint32_t maxBitRate)
    {
        ACL_LOG_INFO("start to execute aclvencSetChannelDescMaxBitRate, maxBitRate %u", maxBitRate);
        ACL_REQUIRES_NOT_NULL(channelDesc);

        {
            std::unique_lock<std::mutex> lock{channelDesc->mutexForTLVMap};
            auto it = channelDesc->tlvParamMap.find(VENC_RATE_CONTROL);
            if (it == channelDesc->tlvParamMap.end()) {
                VencChannelDescTLVParam vencTLVParam;
                std::shared_ptr<aicpu::dvpp::VencRateControl> vencRateControl =
                    std::make_shared<aicpu::dvpp::VencRateControl>();
                ACL_REQUIRES_NOT_NULL(vencRateControl);
                vencRateControl->maxBitRate = maxBitRate;
                vencTLVParam.value = static_cast<std::shared_ptr<void>>(vencRateControl);
                vencTLVParam.valueLen = sizeof(aicpu::dvpp::VencRateControl);
                channelDesc->tlvParamMap[VENC_RATE_CONTROL] = vencTLVParam;
            } else {
                aicpu::dvpp::VencRateControl *vencRateControl =
                    static_cast<aicpu::dvpp::VencRateControl *>(it->second.value.get());
                ACL_REQUIRES_NOT_NULL(vencRateControl);
                vencRateControl->maxBitRate = maxBitRate;
            }
        }

        return ACL_SUCCESS;
    }

    /**
     * Set ip proportion for venc channel desc.
     * @param channelDesc[OUT] venc channel desc
     * @param ipProp[IN] I frame and P frame proportion
     * @return ACL_SUCCESS for success, other for failure
     */
    aclError VideoProcessor::aclvencSetChannelDescIPProp(aclvencChannelDesc *channelDesc, uint32_t ipProp)
    {
        ACL_LOG_INNER_ERROR("[Unsupport][Feature]Setting ipProp for venc channel desc is "
            "not supported in this version. Please check.");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }

    /**
     * Get output buffer size for venc channel desc.
     * @param channelDesc[IN] venc channel desc
     * @param isSupport[OUT] support flag
     * @return output buffer size
     */
    uint32_t VideoProcessor::aclvencGetChannelDescBufSize(const aclvencChannelDesc *channelDesc, bool &isSupport)
    {
        ACL_LOG_INNER_ERROR("[Unsupport][Feature]Getting BufSize for venc channel desc is "
            "not supported in this version. Please check.");
        isSupport = false;
        return 0;
    }

    /**
     * Get output buffer address for venc channel desc.
     * @param channelDesc[IN] venc channel desc
     * @param isSupport[OUT] support flag
     * @return output buffer address
     */
    void *VideoProcessor::aclvencGetChannelDescBufAddr(const aclvencChannelDesc *channelDesc, bool &isSupport)
    {
        ACL_LOG_INNER_ERROR("[Unsupport][Feature]Getting BufAddr for venc channel desc "
            "is not supported in this version. Please check.");
        isSupport = false;
        return nullptr;
    }

    /**
     * Get rc mode for venc channel desc.
     * @param channelDesc[IN] venc channel desc
     * @param isSupport[OUT] support flag
     * @return rc mode, default 0
     */
    uint32_t VideoProcessor::aclvencGetChannelDescRcMode(const aclvencChannelDesc *channelDesc, bool &isSupport)
    {
        ACL_LOG_INFO("start to execute aclvencGetChannelDescRcMode");
        isSupport = true;
        if (channelDesc == nullptr) {
            ACL_LOG_ERROR("[Check][ChannelDesc]venc channelDesc is null.");
            const char *argList[] = {"param"};
            const char *argVal[] = {"channelDesc"};
            acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                argList, argVal, 1);
            return 0;
        }

        {
            std::mutex &mutexMap = const_cast<std::mutex &>(channelDesc->mutexForTLVMap);
            std::unique_lock<std::mutex> lock{mutexMap};
            const auto &it = channelDesc->tlvParamMap.find(VENC_RATE_CONTROL);
            // no set venc rate control para, return 0
            if (it == channelDesc->tlvParamMap.end()) {
                return 0;
            }

            aicpu::dvpp::VencRateControl *vencRateControl =
                static_cast<aicpu::dvpp::VencRateControl *>(it->second.value.get());
            if (vencRateControl == nullptr) {
                ACL_LOG_INNER_ERROR("[Check][VencRateControl]vencRateControl ptr is null.");
                return 0;
            }

            return vencRateControl->rcMode;
        }
    }

    /**
     * Get source rate for venc channel desc.
     * @param channelDesc[IN] venc channel desc
     * @param isSupport[OUT] support flag
     * @return source rate, default 0
     */
    uint32_t VideoProcessor::aclvencGetChannelDescSrcRate(const aclvencChannelDesc *channelDesc, bool &isSupport)
    {
        ACL_LOG_INFO("start to execute aclvencGetChannelDescSrcRate");
        isSupport = true;
        if (channelDesc == nullptr) {
            ACL_LOG_ERROR("[Check][ChannelDesc]venc channelDesc is null.");
            const char *argList[] = {"param"};
            const char *argVal[] = {"channelDesc"};
            acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                argList, argVal, 1);
            return 0;
        }

        {
            std::mutex &mutexMap = const_cast<std::mutex &>(channelDesc->mutexForTLVMap);
            std::unique_lock<std::mutex> lock{mutexMap};
            const auto &it = channelDesc->tlvParamMap.find(VENC_RATE_CONTROL);
            // no set venc rate control para, return 0
            if (it == channelDesc->tlvParamMap.end()) {
                return 0;
            }

            aicpu::dvpp::VencRateControl *vencRateControl =
                static_cast<aicpu::dvpp::VencRateControl *>(it->second.value.get());
            if (vencRateControl == nullptr) {
                ACL_LOG_INNER_ERROR("[Check][VencRateControl]vencRateControl ptr is null.");
                return 0;
            }

            return vencRateControl->srcRate;
        }
    }

    /**
     * Get max bit rate for venc channel desc.
     * @param channelDesc[IN] venc channel desc
     * @param isSupport[OUT] support flag
     * @return max bit rate, default 0
     */
    uint32_t VideoProcessor::aclvencGetChannelDescMaxBitRate(const aclvencChannelDesc *channelDesc, bool &isSupport)
    {
        ACL_LOG_INFO("start to execute aclvencGetChannelDescMaxBitRate");
        isSupport = true;
        if (channelDesc == nullptr) {
            ACL_LOG_ERROR("[Check][ChannelDesc]venc channelDesc is null.");
            const char *argList[] = {"param"};
            const char *argVal[] = {"channelDesc"};
            acl::AclErrorLogManager::ReportInputErrorWithChar(acl::INVALID_NULL_POINTER_MSG,
                argList, argVal, 1);
            return 0;
        }

        {
            std::mutex &mutexMap = const_cast<std::mutex &>(channelDesc->mutexForTLVMap);
            std::unique_lock<std::mutex> lock{mutexMap};
            const auto &it = channelDesc->tlvParamMap.find(VENC_RATE_CONTROL);
            // no set venc rate control para, return 0
            if (it == channelDesc->tlvParamMap.end()) {
                return 0;
            }

            aicpu::dvpp::VencRateControl *vencRateControl =
                static_cast<aicpu::dvpp::VencRateControl *>(it->second.value.get());
            if (vencRateControl == nullptr) {
                ACL_LOG_INNER_ERROR("[Check][VencRateControl]vencRateControl ptr is null.");
                return 0;
            }

            return vencRateControl->maxBitRate;
        }
    }

    /**
     * Get ip proportion for venc channel desc.
     * @param channelDesc[IN] venc channel desc
     * @param isSupport[OUT] support flag
     * @return I frame and P frame proportion
     */
    uint32_t VideoProcessor::aclvencGetChannelDescIPProp(const aclvencChannelDesc *channelDesc, bool &isSupport)
    {
        ACL_LOG_INNER_ERROR("[Get][ChannelDescIPProp]Getting ipProp for venc channel desc is "
            "not supported in this version. Please check.");
        isSupport = false;
        return 0;
    }
    }
}
