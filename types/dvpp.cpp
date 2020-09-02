/**
* @file dvpp.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/ops/acl_dvpp.h"
#include "common/log_inner.h"
#include "error_codes_inner.h"
#include "single_op/dvpp/common/dvpp_util.h"
#include "single_op/dvpp/mgr/dvpp_manager.h"
#include "single_op/dvpp/common/dvpp_def_internal.h"
#include "toolchain/profiling_manager.h"
#include "toolchain/resource_statistics.h"

namespace {
    constexpr int CHANNEL_ID_CEILING = 31;
    constexpr int CHANNEL_ID_FLOOR = 0;
    const int DATA_MEMORY_ALIGN_SIZE = 32;
    constexpr uint32_t BATCH_MAX_SIZE = 256;
    constexpr uint32_t DVPP_BORDER_CONFIG_INVALID_VALUE = 0;
    constexpr uint32_t DVPP_MAKE_BORDER_MAX_COMPONENT = 4;
    constexpr uint32_t VENC_USER_MODE = 2;

    aclError CopyVencChannelDescParam(const void *src, size_t srcLen, void *dst, size_t dstLen)
    {
        ACL_REQUIRES_NOT_NULL(src);
        ACL_REQUIRES_NOT_NULL(dst);
        if (srcLen > dstLen) {
            ACL_LOG_ERROR("src length=%zu is larger than dst length=%zu when memcpy", srcLen, dstLen);
            return ACL_ERROR_INVALID_PARAM;
        }
        auto ret = memcpy_s(dst, dstLen, src, srcLen);
        if (ret != EOK) {
            ACL_LOG_ERROR("call memcpy failed, result=%d, srcLen=%zu, dstLen=%zu", ret, srcLen, dstLen);
            return ACL_ERROR_FAILURE;
        }

        return ACL_SUCCESS;
    }

    aclError SetVencChannelDescThreadId(aclvencChannelDesc *channelDesc, size_t length,
        const void *param)
    {
        uint64_t threadId = 0;
        aclError ret = CopyVencChannelDescParam(param, length, static_cast<void *>(&threadId), sizeof(uint64_t));
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        return aclvencSetChannelDescThreadId(channelDesc, threadId);
    }

    aclError SetVencChannelDescCallback(aclvencChannelDesc *channelDesc, size_t length,
        const void *param)
    {
        aclvencCallback callback = nullptr;
        aclError ret = CopyVencChannelDescParam(param, length, static_cast<void *>(&callback), sizeof(size_t));
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        return aclvencSetChannelDescCallback(channelDesc, callback);
    }

    aclError SetVencChannelDescPicFormat(aclvencChannelDesc *channelDesc, size_t length,
        const void *param)
    {
        acldvppPixelFormat picFormat = PIXEL_FORMAT_YUV_400;
        aclError ret = CopyVencChannelDescParam(param, length, static_cast<void *>(&picFormat),
            sizeof(acldvppPixelFormat));
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        return aclvencSetChannelDescPicFormat(channelDesc, picFormat);
    }

    aclError SetVencChannelDescEnType(aclvencChannelDesc *channelDesc, size_t length,
        const void *param)
    {
        acldvppStreamFormat enType = H265_MAIN_LEVEL;
        aclError ret = CopyVencChannelDescParam(param, length, static_cast<void *>(&enType),
            sizeof(acldvppStreamFormat));
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        return aclvencSetChannelDescEnType(channelDesc, enType);
    }

    aclError SetVencChannelDescPicWidth(aclvencChannelDesc *channelDesc, size_t length,
        const void *param)
    {
        uint32_t picWidth = 0;
        aclError ret = CopyVencChannelDescParam(param, length, static_cast<void *>(&picWidth), sizeof(uint32_t));
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        return aclvencSetChannelDescPicWidth(channelDesc, picWidth);
    }

    aclError SetVencChannelDescPicHeight(aclvencChannelDesc *channelDesc, size_t length,
        const void *param)
    {
        uint32_t picHeight = 0;
        aclError ret = CopyVencChannelDescParam(param, length, static_cast<void *>(&picHeight), sizeof(uint32_t));
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        return aclvencSetChannelDescPicHeight(channelDesc, picHeight);
    }

    aclError SetVencChannelDescKeyFrameInterval(aclvencChannelDesc *channelDesc, size_t length,
        const void *param)
    {
        uint32_t keyFrameInterval = 0;
        aclError ret = CopyVencChannelDescParam(param, length, static_cast<void *>(&keyFrameInterval),
            sizeof(uint32_t));
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        return aclvencSetChannelDescKeyFrameInterval(channelDesc, keyFrameInterval);
    }

    aclError SetVencChannelDescBufAddr(aclvencChannelDesc *channelDesc, size_t length,
        const void *param)
    {
        void *bufAddr = nullptr;
        aclError ret = CopyVencChannelDescParam(param, length, static_cast<void *>(&bufAddr), sizeof(size_t));
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        return aclvencSetChannelDescBufAddr(channelDesc, bufAddr);
    }

    aclError SetVencChannelDescBufSize(aclvencChannelDesc *channelDesc, size_t length,
        const void *param)
    {
        uint32_t bufSize = 0;
        aclError ret = CopyVencChannelDescParam(param, length, static_cast<void *>(&bufSize), sizeof(uint32_t));
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        return aclvencSetChannelDescBufSize(channelDesc, bufSize);
    }

    aclError SetVencChannelDescRcMode(aclvencChannelDesc *channelDesc, size_t length,
        const void *param)
    {
        uint32_t rcMode = 0;
        aclError ret = CopyVencChannelDescParam(param, length, static_cast<void *>(&rcMode), sizeof(uint32_t));
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        return aclvencSetChannelDescRcMode(channelDesc, rcMode);
    }

    aclError SetVencChannelDescSrcRate(aclvencChannelDesc *channelDesc, size_t length,
        const void *param)
    {
        uint32_t srcRate = 0;
        aclError ret = CopyVencChannelDescParam(param, length, static_cast<void *>(&srcRate), sizeof(uint32_t));
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        return aclvencSetChannelDescSrcRate(channelDesc, srcRate);
    }

    aclError SetVencChannelDescMaxBitRate(aclvencChannelDesc *channelDesc, size_t length,
        const void *param)
    {
        uint32_t maxBitRate = 0;
        aclError ret = CopyVencChannelDescParam(param, length, static_cast<void *>(&maxBitRate), sizeof(uint32_t));
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        return aclvencSetChannelDescMaxBitRate(channelDesc, maxBitRate);
    }

    aclError SetVencChannelDescIpProp(aclvencChannelDesc *channelDesc, size_t length,
        const void *param)
    {
        uint32_t ipProp = 0;
        aclError ret = CopyVencChannelDescParam(param, length, static_cast<void *>(&ipProp), sizeof(uint32_t));
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
        if (videoProcessor == nullptr) {
            ACL_LOG_ERROR("video processor is null.");
            return ret;
        }
        return videoProcessor->aclvencSetChannelDescIPProp(channelDesc, ipProp);
    }

    aclError GetVencChannelDescThreadId(const aclvencChannelDesc *channelDesc, size_t length,
        size_t *paramRetSize, void *param)
    {
        uint64_t threadId = aclvencGetChannelDescThreadId(channelDesc);
        aclError ret = CopyVencChannelDescParam(static_cast<const void *>(&threadId), sizeof(uint64_t), param, length);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        *paramRetSize = sizeof(uint64_t);

        return ACL_SUCCESS;
    }

    aclError GetVencChannelDescCallback(const aclvencChannelDesc *channelDesc, size_t length,
        size_t *paramRetSize, void *param)
    {
        aclvencCallback callback = aclvencGetChannelDescCallback(channelDesc);
        aclError ret = CopyVencChannelDescParam(static_cast<const void *>(&callback), sizeof(size_t), param, length);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        *paramRetSize = sizeof(size_t);

        return ACL_SUCCESS;
    }

    aclError GetVencChannelDescPicFormat(const aclvencChannelDesc *channelDesc, size_t length,
        size_t *paramRetSize, void *param)
    {
        acldvppPixelFormat picFormat = aclvencGetChannelDescPicFormat(channelDesc);
        aclError ret = CopyVencChannelDescParam(static_cast<const void *>(&picFormat), sizeof(acldvppPixelFormat),
            param, length);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        *paramRetSize = sizeof(acldvppPixelFormat);

        return ACL_SUCCESS;
    }

    aclError GetVencChannelDescEnType(const aclvencChannelDesc *channelDesc, size_t length,
        size_t *paramRetSize, void *param)
    {
        acldvppStreamFormat enType = aclvencGetChannelDescEnType(channelDesc);
        aclError ret = CopyVencChannelDescParam(static_cast<const void *>(&enType), sizeof(acldvppStreamFormat),
            param, length);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        *paramRetSize = sizeof(acldvppStreamFormat);

        return ACL_SUCCESS;
    }

    aclError GetVencChannelDescPicWidth(const aclvencChannelDesc *channelDesc, size_t length,
        size_t *paramRetSize, void *param)
    {
        uint32_t picWidth = aclvencGetChannelDescPicWidth(channelDesc);
        aclError ret = CopyVencChannelDescParam(static_cast<const void *>(&picWidth), sizeof(uint32_t), param, length);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        *paramRetSize = sizeof(uint32_t);

        return ACL_SUCCESS;
    }

    aclError GetVencChannelDescPicHeight(const aclvencChannelDesc *channelDesc, size_t length,
        size_t *paramRetSize, void *param)
    {
        uint32_t picHeight = aclvencGetChannelDescPicHeight(channelDesc);
        aclError ret = CopyVencChannelDescParam(static_cast<const void *>(&picHeight), sizeof(uint32_t), param, length);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        *paramRetSize = sizeof(uint32_t);

        return ACL_SUCCESS;
    }

    aclError GetVencChannelDescKeyFrameInterval(const aclvencChannelDesc *channelDesc, size_t length,
        size_t *paramRetSize, void *param)
    {
        uint32_t keyFrameInterval = aclvencGetChannelDescKeyFrameInterval(channelDesc);
        aclError ret = CopyVencChannelDescParam(static_cast<const void *>(&keyFrameInterval), sizeof(uint32_t),
            param, length);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        *paramRetSize = sizeof(uint32_t);

        return ACL_SUCCESS;
    }

    aclError GetVencChannelDescBufAddr(const aclvencChannelDesc *channelDesc, size_t length,
        size_t *paramRetSize, void *param)
    {
        auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
        if (videoProcessor == nullptr) {
            ACL_LOG_ERROR("video processor is null.");
            return ACL_ERROR_INTERNAL_ERROR;
        }
        bool isSupport = false;
        void *bufAddr = videoProcessor->aclvencGetChannelDescBufAddr(channelDesc, isSupport);
        if (isSupport == false) {
            return ACL_ERROR_FEATURE_UNSUPPORTED;
        }

        aclError ret = CopyVencChannelDescParam(static_cast<const void *>(&bufAddr), sizeof(size_t), param, length);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        *paramRetSize = sizeof(size_t);

        return ACL_SUCCESS;
    }

    aclError GetVencChannelDescBufSize(const aclvencChannelDesc *channelDesc, size_t length,
        size_t *paramRetSize, void *param)
    {
        auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
        if (videoProcessor == nullptr) {
            ACL_LOG_ERROR("video processor is null.");
            return ACL_ERROR_INTERNAL_ERROR;
        }
        bool isSupport = false;
        uint32_t bufSize = videoProcessor->aclvencGetChannelDescBufSize(channelDesc, isSupport);
        if (isSupport == false) {
            return ACL_ERROR_FEATURE_UNSUPPORTED;
        }

        aclError ret = CopyVencChannelDescParam(static_cast<const void *>(&bufSize), sizeof(uint32_t), param, length);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        *paramRetSize = sizeof(uint32_t);

        return ACL_SUCCESS;
    }

    aclError GetVencChannelDescRcMode(const aclvencChannelDesc *channelDesc, size_t length,
        size_t *paramRetSize, void *param)
    {
        auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
        if (videoProcessor == nullptr) {
            ACL_LOG_ERROR("video processor is null.");
            return ACL_ERROR_INTERNAL_ERROR;
        }
        bool isSupport = false;
        uint32_t rcMode = videoProcessor->aclvencGetChannelDescRcMode(channelDesc, isSupport);
        if (isSupport == false) {
            return ACL_ERROR_FEATURE_UNSUPPORTED;
        }

        aclError ret = CopyVencChannelDescParam(static_cast<const void *>(&rcMode), sizeof(uint32_t), param, length);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        *paramRetSize = sizeof(uint32_t);

        return ACL_SUCCESS;
    }

    aclError GetVencChannelDescSrcRate(const aclvencChannelDesc *channelDesc, size_t length,
        size_t *paramRetSize, void *param)
    {
        auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
        if (videoProcessor == nullptr) {
            ACL_LOG_ERROR("video processor is null.");
            return ACL_ERROR_INTERNAL_ERROR;
        }
        bool isSupport = false;
        uint32_t srcRate = videoProcessor->aclvencGetChannelDescSrcRate(channelDesc, isSupport);
        if (isSupport == false) {
            return ACL_ERROR_FEATURE_UNSUPPORTED;
        }

        aclError ret = CopyVencChannelDescParam(static_cast<const void *>(&srcRate), sizeof(uint32_t), param, length);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        *paramRetSize = sizeof(uint32_t);

        return ACL_SUCCESS;
    }

    aclError GetVencChannelDescMaxBitRate(const aclvencChannelDesc *channelDesc, size_t length,
        size_t *paramRetSize, void *param)
    {
        auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
        if (videoProcessor == nullptr) {
            ACL_LOG_ERROR("video processor is null.");
            return ACL_ERROR_INTERNAL_ERROR;
        }
        bool isSupport = false;
        uint32_t maxBitRate = videoProcessor->aclvencGetChannelDescMaxBitRate(channelDesc, isSupport);
        if (isSupport == false) {
            return ACL_ERROR_FEATURE_UNSUPPORTED;
        }
        aclError ret = CopyVencChannelDescParam(static_cast<const void *>(&maxBitRate), sizeof(uint32_t),
            param, length);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        *paramRetSize = sizeof(uint32_t);

        return ACL_SUCCESS;
    }

    aclError GetVencChannelDescIpProp(const aclvencChannelDesc *channelDesc, size_t length,
        size_t *paramRetSize, void *param)
    {
        auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
        if (videoProcessor == nullptr) {
            ACL_LOG_ERROR("video processor is null.");
            return ACL_ERROR_INTERNAL_ERROR;
        }
        bool isSupport = false;
        uint32_t ipProp = videoProcessor->aclvencGetChannelDescIPProp(channelDesc, isSupport);
        if (isSupport == false) {
            return ACL_ERROR_FEATURE_UNSUPPORTED;
        }

        aclError ret = CopyVencChannelDescParam(static_cast<const void *>(&ipProp), sizeof(uint32_t), param, length);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_ERROR("memcpy value failed, ret=%d.", ret);
            return ret;
        }
        *paramRetSize = sizeof(uint32_t);

        return ACL_SUCCESS;
    }

    typedef aclError (*SetVencParamFunc)(aclvencChannelDesc *, size_t, const void *);
    std::map<aclvencChannelDescParamType, SetVencParamFunc> g_vencSetParamFuncMap = {
        {ACL_VENC_THREAD_ID_UINT64, SetVencChannelDescThreadId},
        {ACL_VENC_CALLBACK_PTR, SetVencChannelDescCallback},
        {ACL_VENC_PIXEL_FORMAT_UINT32, SetVencChannelDescPicFormat},
        {ACL_VENC_ENCODE_TYPE_UINT32, SetVencChannelDescEnType},
        {ACL_VENC_PIC_WIDTH_UINT32, SetVencChannelDescPicWidth},
        {ACL_VENC_PIC_HEIGHT_UINT32, SetVencChannelDescPicHeight},
        {ACL_VENC_KEY_FRAME_INTERVAL_UINT32, SetVencChannelDescKeyFrameInterval},
        {ACL_VENC_BUF_ADDR_PTR, SetVencChannelDescBufAddr},
        {ACL_VENC_BUF_SIZE_UINT32, SetVencChannelDescBufSize},
        {ACL_VENC_RC_MODE_UINT32, SetVencChannelDescRcMode},
        {ACL_VENC_SRC_RATE_UINT32, SetVencChannelDescSrcRate},
        {ACL_VENC_MAX_BITRATE_UINT32, SetVencChannelDescMaxBitRate},
        {ACL_VENC_MAX_IP_PROP_UINT32, SetVencChannelDescIpProp}
    };

    typedef aclError (*GetVencParamFunc)(const aclvencChannelDesc *, size_t, size_t *, void *);
    std::map<aclvencChannelDescParamType, GetVencParamFunc> g_vencGetParamFuncMap = {
        {ACL_VENC_THREAD_ID_UINT64, GetVencChannelDescThreadId},
        {ACL_VENC_CALLBACK_PTR, GetVencChannelDescCallback},
        {ACL_VENC_PIXEL_FORMAT_UINT32, GetVencChannelDescPicFormat},
        {ACL_VENC_ENCODE_TYPE_UINT32, GetVencChannelDescEnType},
        {ACL_VENC_PIC_WIDTH_UINT32, GetVencChannelDescPicWidth},
        {ACL_VENC_PIC_HEIGHT_UINT32, GetVencChannelDescPicHeight},
        {ACL_VENC_KEY_FRAME_INTERVAL_UINT32, GetVencChannelDescKeyFrameInterval},
        {ACL_VENC_BUF_ADDR_PTR, GetVencChannelDescBufAddr},
        {ACL_VENC_BUF_SIZE_UINT32, GetVencChannelDescBufSize},
        {ACL_VENC_RC_MODE_UINT32, GetVencChannelDescRcMode},
        {ACL_VENC_SRC_RATE_UINT32, GetVencChannelDescSrcRate},
        {ACL_VENC_MAX_BITRATE_UINT32, GetVencChannelDescMaxBitRate},
        {ACL_VENC_MAX_IP_PROP_UINT32, GetVencChannelDescIpProp}
    };
}

static aclError GetAlignedSize(size_t size, size_t &alignedSize)
{
    // check overflow, the max value of size must be less than 0xFFFFFFFFFFFFFFFF-32*2
    if (size + DATA_MEMORY_ALIGN_SIZE * 2 < size) {
        ACL_LOG_ERROR("size too large: %zu", size);
        return ACL_ERROR_INVALID_PARAM;
    }

    // // align size to multiple of 32 and puls 32
    alignedSize = (size + DATA_MEMORY_ALIGN_SIZE * 2 - 1) / DATA_MEMORY_ALIGN_SIZE * DATA_MEMORY_ALIGN_SIZE;
    return ACL_SUCCESS;
}

aclError acldvppMalloc(void **devPtr, size_t size)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_DVPP_MALLOC_FREE);
    ACL_LOG_DEBUG("start to execute acldvppMalloc, size = %zu", size);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(devPtr);
    size_t alignedSize;
    ACL_REQUIRES_OK(GetAlignedSize(size, alignedSize));

    rtError_t rtErr = rtDvppMalloc(devPtr, alignedSize);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_ERROR("alloc device memory for dvpp failed, result = %d", rtErr);
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_DVPP_MALLOC_FREE);
    return ACL_SUCCESS;
}

aclError acldvppFree(void *devPtr)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_DVPP_MALLOC_FREE);
    ACL_LOG_DEBUG("start to execute acldvppFree");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(devPtr);

    rtError_t rtErr = rtDvppFree(devPtr);
    if (rtErr != RT_ERROR_NONE) {
        ACL_LOG_ERROR("free device memory for dvpp failed, result = %d", rtErr);
        return ACL_GET_ERRCODE_RTS(rtErr);
    }
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_DVPP_MALLOC_FREE);
    return ACL_SUCCESS;
}

acldvppPicDesc *acldvppCreatePicDesc()
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_PIC_DESC);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return nullptr;
    }
    auto aclPicDesc = imageProcessor->acldvppCreatePicDesc();
    if (aclPicDesc != nullptr) {
        ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_PIC_DESC);
    }
    return aclPicDesc;
}

aclError acldvppDestroyPicDesc(acldvppPicDesc *picDesc)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_PIC_DESC);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    aclError aclRet = imageProcessor->acldvppDestroyPicDesc(picDesc);
    if (aclRet == ACL_SUCCESS) {
        ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_PIC_DESC);
    }
    return aclRet;
}

aclError acldvppSetPicDescData(acldvppPicDesc *picDesc, void *dataDev)
{
    ACL_LOG_DEBUG("start to execute acldvppSetPicDescData");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(picDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dataDev);
    picDesc->dvppPicDesc.data = reinterpret_cast<uintptr_t>(dataDev);
    return ACL_SUCCESS;
}

aclError acldvppSetPicDescSize(acldvppPicDesc *picDesc, uint32_t size)
{
    ACL_LOG_DEBUG("start to execute acldvppSetPicDescSize");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(picDesc);
    picDesc->dvppPicDesc.size = size;
    return ACL_SUCCESS;
}

aclError acldvppSetPicDescFormat(acldvppPicDesc *picDesc, acldvppPixelFormat format)
{
    ACL_LOG_DEBUG("start to execute acldvppSetPicDescFormat");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(picDesc);
    picDesc->dvppPicDesc.format = format;
    // 0x0001 represents that picture format is used first.
    picDesc->dvppPicDesc.bitMap |= 0x0001;
    return ACL_SUCCESS;
}

aclError acldvppSetPicDescWidth(acldvppPicDesc *picDesc, uint32_t width)
{
    ACL_LOG_DEBUG("start to execute acldvppSetPicDescWidth");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(picDesc);
    picDesc->dvppPicDesc.width = width;
    return ACL_SUCCESS;
}

aclError acldvppSetPicDescHeight(acldvppPicDesc *picDesc, uint32_t height)
{
    ACL_LOG_DEBUG("start to execute acldvppSetPicDescHeight");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(picDesc);
    picDesc->dvppPicDesc.height = height;
    return ACL_SUCCESS;
}

aclError acldvppSetPicDescWidthStride(acldvppPicDesc *picDesc, uint32_t widthStride)
{
    ACL_LOG_DEBUG("start to execute acldvppSetPicDescWidthStride");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(picDesc);
    picDesc->dvppPicDesc.widthStride = widthStride;
    return ACL_SUCCESS;
}

aclError acldvppSetPicDescHeightStride(acldvppPicDesc *picDesc, uint32_t heightStride)
{
    ACL_LOG_DEBUG("start to execute acldvppSetPicDescHeightStride");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(picDesc);
    picDesc->dvppPicDesc.heightStride = heightStride;
    return ACL_SUCCESS;
}

aclError acldvppSetPicDescRetCode(acldvppPicDesc *picDesc, uint32_t retCode)
{
    ACL_LOG_DEBUG("start to execute acldvppSetPicDescRetCode");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(picDesc);
    picDesc->dvppPicDesc.retCode = retCode;
    return ACL_SUCCESS;
}

void *acldvppGetPicDescData(const acldvppPicDesc *picDesc)
{
    ACL_LOG_DEBUG("start to execute acldvppGetPicDescData");
    if (picDesc == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<void *>(static_cast<uintptr_t>(picDesc->dvppPicDesc.data));
}

uint32_t acldvppGetPicDescSize(const acldvppPicDesc *picDesc)
{
    ACL_LOG_DEBUG("start to execute acldvppGetPicDescSize");
    if (picDesc == nullptr) {
        return 0; // default 0
    }
    return picDesc->dvppPicDesc.size;
}

acldvppPixelFormat acldvppGetPicDescFormat(const acldvppPicDesc *picDesc)
{
    ACL_LOG_DEBUG("start to execute acldvppGetPicDescFormat");
    if (picDesc == nullptr) {
        return PIXEL_FORMAT_YUV_400;
    }
    return static_cast<acldvppPixelFormat>(picDesc->dvppPicDesc.format);
}

uint32_t acldvppGetPicDescWidth(const acldvppPicDesc *picDesc)
{
    ACL_LOG_DEBUG("start to execute acldvppGetPicDescWidth");
    if (picDesc == nullptr) {
        return 0; // default 0
    }
    return picDesc->dvppPicDesc.width;
}

uint32_t acldvppGetPicDescHeight(const acldvppPicDesc *picDesc)
{
    ACL_LOG_DEBUG("start to execute acldvppGetPicDescHeight");
    if (picDesc == nullptr) {
        return 0; // default 0
    }
    return picDesc->dvppPicDesc.height;
}

uint32_t acldvppGetPicDescWidthStride(const acldvppPicDesc *picDesc)
{
    ACL_LOG_DEBUG("start to execute acldvppGetPicDescWidthStride");
    if (picDesc == nullptr) {
        return 0; // default 0
    }
    return picDesc->dvppPicDesc.widthStride;
}

uint32_t acldvppGetPicDescHeightStride(const acldvppPicDesc *picDesc)
{
    ACL_LOG_DEBUG("start to execute acldvppGetPicDescHeightStride");
    if (picDesc == nullptr) {
        return 0; // default 0
    }
    return picDesc->dvppPicDesc.heightStride;
}

uint32_t acldvppGetPicDescRetCode(const acldvppPicDesc *picDesc)
{
    ACL_LOG_DEBUG("start to execute acldvppGetPicDescRetCode");
    if (picDesc == nullptr) {
        return 0; // default 0
    }
    return picDesc->dvppPicDesc.retCode;
}

acldvppRoiConfig *acldvppCreateRoiConfig(uint32_t left,
                                         uint32_t right,
                                         uint32_t top,
                                         uint32_t bottom)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_ROI_CONFIG);
    if (left > UINT16_MAX || right > UINT16_MAX || top > UINT16_MAX || bottom > UINT16_MAX) {
        ACL_LOG_ERROR("param great than UINT16_MAX, left=%u, right=%u, top=%u, bottom=%u.",
            left, right, top, bottom);
        return nullptr;
    }
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return nullptr;
    }
    auto aclRoiConfig = imageProcessor->acldvppCreateRoiConfig(left, right, top, bottom);
    if (aclRoiConfig != nullptr) {
        ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_ROI_CONFIG);
    }
    return aclRoiConfig;
}

aclError acldvppDestroyRoiConfig(acldvppRoiConfig *roiConfig)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_ROI_CONFIG);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    aclError aclRet = imageProcessor->acldvppDestroyRoiConfig(roiConfig);
    if (aclRet == ACL_SUCCESS) {
        ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_ROI_CONFIG);
    }
    return aclRet;
}

aclError acldvppSetRoiConfigLeft(acldvppRoiConfig *config, uint32_t left)
{
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(config);
    if (left > UINT16_MAX) {
        ACL_LOG_ERROR("param great than UINT16_MAX.");
        return ACL_ERROR_INVALID_PARAM;
    }
    config->dvppRoiConfig.leftOffset = static_cast<uint16_t>(left);
    return ACL_SUCCESS;
}

aclError acldvppSetRoiConfigRight(acldvppRoiConfig *config, uint32_t right)
{
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(config);
    if (right > UINT16_MAX) {
        ACL_LOG_ERROR("param great than UINT16_MAX.");
        return ACL_ERROR_INVALID_PARAM;
    }
    config->dvppRoiConfig.rightOffset = static_cast<uint16_t>(right);
    return ACL_SUCCESS;
}

aclError acldvppSetRoiConfigTop(acldvppRoiConfig *config, uint32_t top)
{
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(config);
    if (top > UINT16_MAX) {
        ACL_LOG_ERROR("param great than UINT16_MAX.");
        return ACL_ERROR_INVALID_PARAM;
    }
    config->dvppRoiConfig.upOffset = static_cast<uint16_t>(top);
    return ACL_SUCCESS;
}

aclError acldvppSetRoiConfigBottom(acldvppRoiConfig *config, uint32_t bottom)
{
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(config);
    if (bottom > UINT16_MAX) {
        ACL_LOG_ERROR("param great than UINT16_MAX.");
        return ACL_ERROR_INVALID_PARAM;
    }
    config->dvppRoiConfig.downOffset = static_cast<uint16_t>(bottom);
    return ACL_SUCCESS;
}

aclError acldvppSetRoiConfig(acldvppRoiConfig *config,
                             uint32_t left,
                             uint32_t right,
                             uint32_t top,
                             uint32_t bottom)
{
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(config);
    if (left > UINT16_MAX || right > UINT16_MAX || top > UINT16_MAX || bottom > UINT16_MAX) {
        ACL_LOG_ERROR("param great than UINT16_MAX, left=%u, right=%u, top=%u, bottom=%u.",
            left, right, top, bottom);
        return ACL_ERROR_INVALID_PARAM;
    }
    config->dvppRoiConfig.leftOffset = static_cast<uint16_t>(left);
    config->dvppRoiConfig.rightOffset = static_cast<uint16_t>(right);
    config->dvppRoiConfig.upOffset = static_cast<uint16_t>(top);
    config->dvppRoiConfig.downOffset = static_cast<uint16_t>(bottom);
    return ACL_SUCCESS;
}

acldvppJpegeConfig *acldvppCreateJpegeConfig()
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_JPEGE_CONFIG);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return nullptr;
    }
    auto aclJpegeConfig = imageProcessor->acldvppCreateJpegeConfig();
    if (aclJpegeConfig != nullptr) {
        ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_JPEGE_CONFIG);
    }
    return aclJpegeConfig;
}

aclError acldvppDestroyJpegeConfig(acldvppJpegeConfig *jpegeConfig)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_JPEGE_CONFIG);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    aclError aclRet = imageProcessor->acldvppDestroyJpegeConfig(jpegeConfig);
    if (aclRet == ACL_SUCCESS) {
        ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_JPEGE_CONFIG);
    }
    return aclRet;
}

aclError acldvppSetJpegeConfigLevel(acldvppJpegeConfig *jpegeConfig, uint32_t level)
{
    ACL_LOG_DEBUG("start to execute acldvppSetJpegeConfigLevel");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(jpegeConfig);
    jpegeConfig->dvppJpegeConfig.level = level;
    return ACL_SUCCESS;
}

uint32_t acldvppGetJpegeConfigLevel(const acldvppJpegeConfig *jpegeConfig)
{
    ACL_LOG_DEBUG("start to execute acldvppGetJpegeConfigLevel");
    if (jpegeConfig == nullptr) {
        ACL_LOG_ERROR("jpegeConfig is null");
        return 0;  // default 0
    }
    return jpegeConfig->dvppJpegeConfig.level;
}

acldvppResizeConfig *acldvppCreateResizeConfig()
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_RESIZE_CONFIG);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return nullptr;
    }
    auto aclResizeConfig = imageProcessor->acldvppCreateResizeConfig();
    if (aclResizeConfig != nullptr) {
        ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_RESIZE_CONFIG);
    }
    return aclResizeConfig;
}

aclError acldvppDestroyResizeConfig(acldvppResizeConfig *resizeConfig)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_RESIZE_CONFIG);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    aclError aclRet = imageProcessor->acldvppDestroyResizeConfig(resizeConfig);
    if (aclRet == ACL_SUCCESS) {
        ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_RESIZE_CONFIG);
    }
    return aclRet;
}

aclError acldvppSetResizeConfigInterpolation(acldvppResizeConfig *resizeConfig, uint32_t interpolation)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppSetResizeConfigInterpolation(resizeConfig, interpolation);
}

uint32_t acldvppGetResizeConfigInterpolation(const acldvppResizeConfig *resizeConfig)
{
    ACL_LOG_DEBUG("start to execute acldvppGetResizeConfigInterpolation");
    if (resizeConfig == nullptr) {
        ACL_LOG_ERROR("resizeConfig was nullptr.");
        return 0;
    }

    return resizeConfig->dvppResizeConfig.interpolation;
}

aclError acldvppSetChannelDescMode(acldvppChannelDesc *channelDesc, uint32_t mode)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppSetChannelDescMode(channelDesc, mode);
}

acldvppChannelDesc *acldvppCreateChannelDesc()
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_CHANNEL_DESC);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return nullptr;
    }
    auto aclChannelDesc = imageProcessor->acldvppCreateChannelDesc();
    if (aclChannelDesc != nullptr) {
        ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_CHANNEL_DESC);
    }
    return aclChannelDesc;
}

aclError acldvppDestroyChannelDesc(acldvppChannelDesc *channelDesc)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_CHANNEL_DESC);
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    aclError aclRet = imageProcessor->acldvppDestroyChannelDesc(channelDesc);
    if (aclRet == ACL_SUCCESS) {
        ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_CHANNEL_DESC);
    }
    return aclRet;
}

uint64_t acldvppGetChannelDescChannelId(const acldvppChannelDesc *channelDesc)
{
    ACL_LOG_DEBUG("start to execute acldvppGetChannelDescChannelId");
    if (channelDesc == nullptr) {
        return 0;
    }
    return channelDesc->channelIndex;
}

aclvdecChannelDesc *aclvdecCreateChannelDesc()
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_VDEC_CHANNEL_DESC);
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return nullptr;
    }
    auto aclChannelDesc = videoProcessor->aclvdecCreateChannelDesc();
    if (aclChannelDesc != nullptr) {
        ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_VDEC_CHANNEL_DESC);
    }
    return aclChannelDesc;
}

aclError aclvdecDestroyChannelDesc(aclvdecChannelDesc *channelDesc)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_VDEC_CHANNEL_DESC);
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    aclError aclRet = videoProcessor->aclvdecDestroyChannelDesc(channelDesc);
    if (aclRet == ACL_SUCCESS) {
        ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_VDEC_CHANNEL_DESC);
    }
    return aclRet;
}

aclError aclvdecSetChannelDescOutMode(aclvdecChannelDesc *channelDesc, uint32_t outMode)
{
    ACL_LOG_DEBUG("start to execute aclvdecSetChannelDescOutMode");
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null.");
        return ACL_ERROR_INVALID_PARAM;
    }
    // outMode support 0 : default mode, 1 : fast mode
    if (outMode > 1) {
        ACL_LOG_ERROR("unsupported outMode, outMode = %u", outMode);
        return ACL_ERROR_INVALID_PARAM;
    }

    channelDesc->vdecDesc.outMode = outMode;
    return ACL_SUCCESS;
}

aclError aclvdecSetChannelDescThreadId(aclvdecChannelDesc *channelDesc, uint64_t threadId)
{
    ACL_LOG_DEBUG("start to execute aclvdecSetChannelDescThreadId");
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null.");
        return ACL_ERROR_INVALID_PARAM;
    }

    channelDesc->threadId = threadId;
    return ACL_SUCCESS;
}

aclError aclvdecSetChannelDescCallback(aclvdecChannelDesc *channelDesc, aclvdecCallback callback)
{
    ACL_LOG_DEBUG("start to execute aclvdecSetChannelDescCallback");
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null.");
        return ACL_ERROR_INVALID_PARAM;
    }

    if (callback == nullptr) {
        ACL_LOG_ERROR("aclvdecCallback is null.");
        return ACL_ERROR_INVALID_PARAM;
    }

    channelDesc->callback = callback;
    return ACL_SUCCESS;
}

uint32_t aclvdecGetChannelDescChannelId(const aclvdecChannelDesc *channelDesc)
{
    ACL_LOG_DEBUG("start to execute aclvdecGetChannelDescChannelId");
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null");
        return 0; // default 0
    }
    return channelDesc->vdecDesc.channelId;
}

uint64_t aclvdecGetChannelDescThreadId(const aclvdecChannelDesc *channelDesc)
{
    ACL_LOG_DEBUG("start to execute aclvdecGetChannelDescThreadId");
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null");
        return 0; // default 0
    }
    return channelDesc->threadId;
}

aclvdecCallback aclvdecGetChannelDescCallback(const aclvdecChannelDesc *channelDesc)
{
    ACL_LOG_DEBUG("start to execute aclvdecGetChannelDescCallback");
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null");
        return nullptr; // default null
    }
    return channelDesc->callback;
}

acldvppStreamFormat aclvdecGetChannelDescEnType(const aclvdecChannelDesc *channelDesc)
{
    ACL_LOG_DEBUG("start to execute aclvdecGetChannelDescEnType");
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null");
        return H265_MAIN_LEVEL;
    }
    return static_cast<acldvppStreamFormat>(channelDesc->vdecDesc.enType);
}

acldvppPixelFormat aclvdecGetChannelDescOutPicFormat(const aclvdecChannelDesc *channelDesc)
{
    ACL_LOG_DEBUG("start to execute aclvdecGetChannelDescOutPicFormat");
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null");
        return PIXEL_FORMAT_YUV_400;
    }
    return static_cast<acldvppPixelFormat>(channelDesc->vdecDesc.outPicFormat);
}

uint32_t aclvdecGetChannelDescOutPicWidth(const aclvdecChannelDesc *channelDesc)
{
    ACL_LOG_DEBUG("start to execute aclvdecGetChannelDescOutPicWidth");
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null");
        return 0; // default 0
    }
    return channelDesc->vdecDesc.outPicWidth;
}

uint32_t aclvdecGetChannelDescOutPicHeight(const aclvdecChannelDesc *channelDesc)
{
    ACL_LOG_DEBUG("start to execute aclvdecGetChannelDescOutPicHeight");
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null");
        return 0; // default 0
    }
    return channelDesc->vdecDesc.outPicHeight;
}

uint32_t aclvdecGetChannelDescRefFrameNum(const aclvdecChannelDesc *channelDesc)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return 0;
    }
    return videoProcessor->aclvdecGetChannelDescRefFrameNum(channelDesc);
}

acldvppStreamDesc *acldvppCreateStreamDesc()
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_STREAM_DESC);
    ACL_LOG_INFO("start to execute acldvppCreateStreamDesc");
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return nullptr;
    }
    auto aclDvppStreamDesc = videoProcessor->acldvppCreateStreamDesc();
    if (aclDvppStreamDesc != nullptr) {
        ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_STREAM_DESC);
    }
    return aclDvppStreamDesc;
}

aclError acldvppDestroyStreamDesc(acldvppStreamDesc *streamDesc)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_STREAM_DESC);
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    auto aclRet = videoProcessor->acldvppDestroyStreamDesc(streamDesc);
    if (aclRet == ACL_SUCCESS) {
        ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_STREAM_DESC);
    }
    return ACL_SUCCESS;
}

aclError acldvppSetStreamDescData(acldvppStreamDesc *streamDesc, void *dataDev)
{
    ACL_LOG_DEBUG("start to execute acldvppSetStreamDescData");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(streamDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dataDev);
    streamDesc->dvppStreamDesc.data = reinterpret_cast<uintptr_t>(dataDev);
    return ACL_SUCCESS;
}

aclError acldvppSetStreamDescSize(acldvppStreamDesc *streamDesc, uint32_t size)
{
    ACL_LOG_DEBUG("start to execute acldvppSetStreamDescSize");
    if (streamDesc == nullptr) {
        ACL_LOG_ERROR("streamDesc is null.");
        return ACL_ERROR_INVALID_PARAM;
    }
    streamDesc->dvppStreamDesc.size = size;
    return ACL_SUCCESS;
}

aclError acldvppSetStreamDescFormat(acldvppStreamDesc *streamDesc, acldvppStreamFormat format)
{
    ACL_LOG_DEBUG("start to execute acldvppSetStreamDescFormat");
    if (streamDesc == nullptr) {
        ACL_LOG_ERROR("streamDesc is null.");
        return ACL_ERROR_INVALID_PARAM;
    }
    streamDesc->dvppStreamDesc.format = static_cast<uint32_t>(format);
    return ACL_SUCCESS;
}

aclError acldvppSetStreamDescTimestamp(acldvppStreamDesc *streamDesc, uint64_t timestamp)
{
    ACL_LOG_DEBUG("start to execute acldvppSetStreamDescTimestamp");
    if (streamDesc == nullptr) {
        ACL_LOG_ERROR("streamDesc is null.");
        return ACL_ERROR_INVALID_PARAM;
    }
    streamDesc->dvppStreamDesc.timestamp = timestamp;
    return ACL_SUCCESS;
}

aclError acldvppSetStreamDescRetCode(acldvppStreamDesc *streamDesc, uint32_t retCode)
{
    ACL_LOG_DEBUG("start to execute acldvppSetStreamDescRetCode");
    if (streamDesc == nullptr) {
        ACL_LOG_ERROR("streamDesc is null.");
        return ACL_ERROR_INVALID_PARAM;
    }
    streamDesc->dvppStreamDesc.retCode = retCode;
    return ACL_SUCCESS;
}

aclError acldvppSetStreamDescEos(acldvppStreamDesc *streamDesc, uint8_t eos)
{
    ACL_LOG_DEBUG("start to execute acldvppSetStreamDescEos");
    if (streamDesc == nullptr) {
        ACL_LOG_ERROR("streamDesc is null.");
        return ACL_ERROR_INVALID_PARAM;
    }
    // 0: false, 1: true
    streamDesc->dvppStreamDesc.eos = (eos != 0);
    return ACL_SUCCESS;
}

void *acldvppGetStreamDescData(const acldvppStreamDesc *streamDesc)
{
    ACL_LOG_DEBUG("start to execute acldvppGetStreamDescData");
    if (streamDesc == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<void *>(static_cast<uintptr_t>(streamDesc->dvppStreamDesc.data));
}

uint32_t acldvppGetStreamDescSize(const acldvppStreamDesc *streamDesc)
{
    ACL_LOG_DEBUG("start to execute acldvppGetStreamDescSize");
    if (streamDesc == nullptr) {
        ACL_LOG_ERROR("streamDesc is null.");
        return 0; // default 0
    }
    return streamDesc->dvppStreamDesc.size;
}

acldvppStreamFormat acldvppGetStreamDescFormat(const acldvppStreamDesc *streamDesc)
{
    ACL_LOG_DEBUG("start to execute acldvppGetStreamDescFormat");
    if (streamDesc == nullptr) {
        ACL_LOG_ERROR("streamDesc is null.");
        return H265_MAIN_LEVEL;
    }
    return static_cast<acldvppStreamFormat>(streamDesc->dvppStreamDesc.format);
}

uint64_t acldvppGetStreamDescTimestamp(const acldvppStreamDesc *streamDesc)
{
    ACL_LOG_DEBUG("start to execute acldvppGetStreamDescTimestamp");
    if (streamDesc == nullptr) {
        ACL_LOG_ERROR("streamDesc is null.");
        return 0; // default 0
    }
    return streamDesc->dvppStreamDesc.timestamp;
}

uint32_t acldvppGetStreamDescRetCode(const acldvppStreamDesc *streamDesc)
{
    ACL_LOG_DEBUG("start to execute acldvppGetStreamDescRetCode");
    if (streamDesc == nullptr) {
        ACL_LOG_ERROR("streamDesc is null.");
        return 0; // default 0
    }
    return streamDesc->dvppStreamDesc.retCode;
}

uint8_t acldvppGetStreamDescEos(const acldvppStreamDesc *streamDesc)
{
    ACL_LOG_DEBUG("start to execute acldvppGetStreamDescEos");
    if (streamDesc == nullptr) {
        ACL_LOG_ERROR("streamDesc is null.");
        return 0; // default 0
    }
    return (streamDesc->dvppStreamDesc.eos) ? 1 : 0;
}

aclvdecFrameConfig *aclvdecCreateFrameConfig()
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_VDEC_FRAME_CONFIG);
    ACL_LOG_INFO("start to execute aclvdecCreateFrameConfig");
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return nullptr;
    }
    auto aclVdecFrameConfig = videoProcessor->aclvdecCreateFrameConfig();
    if (aclVdecFrameConfig != nullptr) {
        ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_VDEC_FRAME_CONFIG);
    }
    return aclVdecFrameConfig;
}

aclError aclvdecDestroyFrameConfig(aclvdecFrameConfig *vdecFrameConfig)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_VDEC_FRAME_CONFIG);
    ACL_LOG_DEBUG("start to execute acldvppSetJpegeConfigLevel");
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    auto aclRet = videoProcessor->aclvdecDestroyFrameConfig(vdecFrameConfig);
    if (aclRet == ACL_SUCCESS) {
        ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_VDEC_FRAME_CONFIG);
    }
    return aclRet;
}

aclvencChannelDesc *aclvencCreateChannelDesc()
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_VENC_CHANNEL_DESC);
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return nullptr;
    }
    auto aclChannelDesc = videoProcessor->aclvencCreateChannelDesc();
    if (aclChannelDesc != nullptr) {
        ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_VENC_CHANNEL_DESC);
    }
    return aclChannelDesc;
}

aclError aclvencDestroyChannelDesc(aclvencChannelDesc *channelDesc)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_VENC_CHANNEL_DESC);
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    aclError aclRet = videoProcessor->aclvencDestroyChannelDesc(channelDesc);
    if (aclRet == ACL_SUCCESS) {
        ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_VENC_CHANNEL_DESC);
    }
    return aclRet;
}

aclvencFrameConfig *aclvencCreateFrameConfig()
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_VENC_FRAME_CONFIG);
    ACL_LOG_INFO("start to execute aclvencCreateFrameConfig");
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return nullptr;
    }
    auto aclvencFrameConfig = videoProcessor->aclvencCreateFrameConfig();
    if (aclvencFrameConfig != nullptr) {
        ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_VENC_FRAME_CONFIG);
    }
    return aclvencFrameConfig;
}

aclError aclvencDestroyFrameConfig(aclvencFrameConfig *vencFrameConfig)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_VENC_FRAME_CONFIG);
    ACL_LOG_DEBUG("start to execute aclvencDestroyFrameConfig");
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    auto aclRet = videoProcessor->aclvencDestroyFrameConfig(vencFrameConfig);
    if (aclRet == ACL_SUCCESS) {
        ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_VENC_FRAME_CONFIG);
    }
    return aclRet;
}

aclError aclvencSetChannelDescThreadId(aclvencChannelDesc *channelDesc, uint64_t threadId)
{
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null.");
        return ACL_ERROR_INVALID_PARAM;
    }

    channelDesc->threadId = threadId;
    return ACL_SUCCESS;
}

aclError aclvencSetChannelDescCallback(aclvencChannelDesc *channelDesc, aclvencCallback callback)
{
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null.");
        return ACL_ERROR_INVALID_PARAM;
    }

    if (callback == nullptr) {
        ACL_LOG_ERROR("aclvencCallback is null.");
        return ACL_ERROR_INVALID_PARAM;
    }

    channelDesc->callback = callback;
    return ACL_SUCCESS;
}

aclError aclvencSetChannelDescEnType(aclvencChannelDesc *channelDesc, acldvppStreamFormat enType)
{
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null.");
        return ACL_ERROR_INVALID_PARAM;
    }

    channelDesc->vencDesc.enType = static_cast<uint32_t>(enType);
    return ACL_SUCCESS;
}

aclError aclvencSetChannelDescPicFormat(aclvencChannelDesc *channelDesc, acldvppPixelFormat picFormat)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return videoProcessor->aclvencSetChannelDescPicFormat(channelDesc, picFormat);
}

aclError aclvencSetChannelDescPicWidth(aclvencChannelDesc *channelDesc, uint32_t picWidth)
{
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null.");
        return ACL_ERROR_INVALID_PARAM;
    }

    channelDesc->vencDesc.picWidth = picWidth;
    return ACL_SUCCESS;
}

aclError aclvencSetChannelDescPicHeight(aclvencChannelDesc *channelDesc, uint32_t picHeight)
{
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null.");
        return ACL_ERROR_INVALID_PARAM;
    }

    channelDesc->vencDesc.picHeight = picHeight;
    return ACL_SUCCESS;
}

aclError aclvencSetChannelDescKeyFrameInterval(aclvencChannelDesc *channelDesc, uint32_t keyFrameInterval)
{
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null.");
        return ACL_ERROR_INVALID_PARAM;
    }

    channelDesc->vencDesc.keyFrameInterval = keyFrameInterval;
    return ACL_SUCCESS;
}

aclError aclvencSetChannelDescBufAddr(aclvencChannelDesc *channelDesc, void *bufAddr)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return videoProcessor->aclvencSetChannelDescBufAddr(channelDesc, bufAddr);
}

aclError aclvencSetChannelDescRcMode(aclvencChannelDesc *channelDesc, uint32_t rcMode)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return videoProcessor->aclvencSetChannelDescRcMode(channelDesc, rcMode);
}

aclError aclvencSetChannelDescSrcRate(aclvencChannelDesc *channelDesc, uint32_t srcRate)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return videoProcessor->aclvencSetChannelDescSrcRate(channelDesc, srcRate);
}

aclError aclvencSetChannelDescMaxBitRate(aclvencChannelDesc *channelDesc, uint32_t maxBitRate)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return videoProcessor->aclvencSetChannelDescMaxBitRate(channelDesc, maxBitRate);
}

aclError aclvencSetChannelDescBufSize(aclvencChannelDesc *channelDesc, uint32_t bufSize)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return videoProcessor->aclvencSetChannelDescBufSize(channelDesc, bufSize);
}

aclError aclvencSetChannelDescParam(aclvencChannelDesc *channelDesc,
    aclvencChannelDescParamType paramType, size_t length, const void *param)
{
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(param);
    if (g_vencSetParamFuncMap.find(paramType) == g_vencSetParamFuncMap.end()) {
        ACL_LOG_ERROR("invalid venc channelDesc parameter type %d.", static_cast<int32_t>(paramType));
        return ACL_ERROR_INVALID_PARAM;
    }
    return g_vencSetParamFuncMap[paramType](channelDesc, length, param);
}

uint32_t aclvencGetChannelDescBufSize(const aclvencChannelDesc *channelDesc)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return 0;
    }
    bool isSupport = false;
    return videoProcessor->aclvencGetChannelDescBufSize(channelDesc, isSupport);
}

void *aclvencGetChannelDescBufAddr(const aclvencChannelDesc *channelDesc)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return nullptr;
    }
    bool isSupport = false;
    return videoProcessor->aclvencGetChannelDescBufAddr(channelDesc, isSupport);
}

uint32_t aclvencGetChannelDescChannelId(const aclvencChannelDesc *channelDesc)
{
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null");
        return 0; // default 0
    }
    return channelDesc->vencDesc.channelId;
}

uint64_t aclvencGetChannelDescThreadId(const aclvencChannelDesc *channelDesc)
{
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null");
        return 0; // default 0
    }
    return channelDesc->threadId;
}

aclvencCallback aclvencGetChannelDescCallback(const aclvencChannelDesc *channelDesc)
{
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc or callback is null");
        return nullptr; // default null
    }
    return channelDesc->callback;
}

acldvppStreamFormat aclvencGetChannelDescEnType(const aclvencChannelDesc *channelDesc)
{
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null");
        return H265_MAIN_LEVEL; // default H265_MAIN_LEVEL
    }
    return static_cast<acldvppStreamFormat>(channelDesc->vencDesc.enType);
}

acldvppPixelFormat aclvencGetChannelDescPicFormat(const aclvencChannelDesc *channelDesc)
{
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null");
        return PIXEL_FORMAT_YUV_SEMIPLANAR_420; // default PIXEL_FORMAT_YUV_SEMIPLANAR_420
    }
    return static_cast<acldvppPixelFormat>(channelDesc->vencDesc.picFormat);
}

uint32_t aclvencGetChannelDescPicWidth(const aclvencChannelDesc *channelDesc)
{
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null");
        return 0; // default 0
    }
    return channelDesc->vencDesc.picWidth;
}

uint32_t aclvencGetChannelDescPicHeight(const aclvencChannelDesc *channelDesc)
{
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null");
        return 0; // default 0
    }
    return channelDesc->vencDesc.picHeight;
}

uint32_t aclvencGetChannelDescKeyFrameInterval(const aclvencChannelDesc *channelDesc)
{
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null");
        return 0; // default 0
    }
    return channelDesc->vencDesc.keyFrameInterval;
}

uint32_t aclvencGetChannelDescRcMode(const aclvencChannelDesc *channelDesc)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return 0;
    }
    bool isSupport = false;
    return videoProcessor->aclvencGetChannelDescRcMode(channelDesc, isSupport);
}

uint32_t aclvencGetChannelDescSrcRate(const aclvencChannelDesc *channelDesc)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return 0;
    }
    bool isSupport = false;
    return videoProcessor->aclvencGetChannelDescSrcRate(channelDesc, isSupport);
}

uint32_t aclvencGetChannelDescMaxBitRate(const aclvencChannelDesc *channelDesc)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return 0;
    }
    bool isSupport = false;
    return videoProcessor->aclvencGetChannelDescMaxBitRate(channelDesc, isSupport);
}

aclError aclvencGetChannelDescParam(const aclvencChannelDesc *channelDesc,
    aclvencChannelDescParamType paramType, size_t length, size_t *paramRetSize, void *param)
{
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(channelDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(paramRetSize);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(param);
    if (g_vencGetParamFuncMap.find(paramType) == g_vencGetParamFuncMap.end()) {
        ACL_LOG_ERROR("invalid venc channelDesc parameter type %d.", static_cast<int32_t>(paramType));
        return ACL_ERROR_INVALID_PARAM;
    }
    return g_vencGetParamFuncMap[paramType](channelDesc, length, paramRetSize, param);
}

aclError aclvencSetFrameConfigForceIFrame(aclvencFrameConfig *config, uint8_t forceIFrame)
{
    ACL_LOG_DEBUG("start to execute aclvencSetFrameConfigForceIFrame");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(config);
    config->vencFrameConfig.forceIFrame = (forceIFrame != 0);
    return ACL_SUCCESS;
}

uint8_t aclvencGetFrameConfigForceIFrame(const aclvencFrameConfig *config)
{
    ACL_LOG_DEBUG("start to execute aclvencGetFrameConfigForceIFrame");
    if (config == nullptr) {
        ACL_LOG_ERROR("config is null");
        return 0; // default 0
    }
    return config->vencFrameConfig.forceIFrame;
}

aclError aclvencSetFrameConfigEos(aclvencFrameConfig *config, uint8_t eos)
{
    ACL_LOG_DEBUG("start to execute aclvencSetFrameConfigEos");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(config);
    config->vencFrameConfig.eos = (eos == 0) ? false : true;
    return ACL_SUCCESS;
}

uint8_t aclvencGetFrameConfigEos(const aclvencFrameConfig *config)
{
    ACL_LOG_DEBUG("start to execute aclvencGetFrameConfigEos");
    if (config == nullptr) {
        ACL_LOG_ERROR("config is null");
        return 0; // default 0
    }
    return config->vencFrameConfig.eos;
}

acldvppBatchPicDesc *acldvppCreateBatchPicDesc(uint32_t batchSize)
{
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_BATCH_PIC_DESC);
    // dvpp kernel process max pictures number is 128
    if (batchSize == 0 || batchSize > BATCH_MAX_SIZE) {
        ACL_LOG_ERROR("param batchSize is 0 or greater than %u, batch size = %u.", BATCH_MAX_SIZE, batchSize);
        return nullptr;
    }

    // DvppPicDesc dvppPicDescs[1] in struct acldvppBatchPicDesc, so batchSize - 1
    uint32_t hostSize = sizeof(acldvppBatchPicDesc) + sizeof(aicpu::dvpp::DvppPicDesc) * (batchSize - 1);
    size_t pageSize = mmGetPageSize();
    void *aclBatchPicAddr = mmAlignMalloc(hostSize, pageSize);
    if (aclBatchPicAddr == nullptr) {
        ACL_LOG_ERROR("malloc acldvppBatchPicDesc failed.");
        return nullptr;
    }
    acldvppBatchPicDesc *batchPicDesc = new (aclBatchPicAddr)acldvppBatchPicDesc;
    batchPicDesc->dvppBatchPicDescs.batchSize = batchSize;
    batchPicDesc->aclDvppPicDescs = new (std::nothrow)acldvppPicDesc[batchSize];
    if (batchPicDesc->aclDvppPicDescs == nullptr) {
        ACL_ALIGN_FREE(aclBatchPicAddr);
        ACL_LOG_ERROR("create acldvppPicDesc failed. batch size=%u.", batchSize);
        return nullptr;
    }

    // alloc device memory
    void *devPtr = nullptr;
    size_t devSize = sizeof(aicpu::dvpp::DvppPicDesc) * batchSize + sizeof(uint32_t);
    uint32_t flags = RT_MEMORY_DEFAULT | RT_MEMORY_POLICY_DEFAULT_PAGE_ONLY;
    aclError ret = rtMalloc(&devPtr, devSize, flags);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_ERROR("malloc device memory for acl dvpp batch pic desc failed, result = %d. batch size=%u.",
            ret, batchSize);
        ACL_DELETE_ARRAY_AND_SET_NULL(batchPicDesc->aclDvppPicDescs);
        ACL_ALIGN_FREE(aclBatchPicAddr);
        return nullptr;
    }

    batchPicDesc->dataBuffer.data = devPtr;
    batchPicDesc->dataBuffer.length = devSize;
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_BATCH_PIC_DESC);
    return batchPicDesc;
}

acldvppPicDesc *acldvppGetPicDesc(acldvppBatchPicDesc *batchPicDesc, uint32_t index)
{
    if (batchPicDesc == nullptr) {
        ACL_LOG_ERROR("param batchPicDesc is nullptr.");
        return nullptr;
    }

    if (index >= batchPicDesc->dvppBatchPicDescs.batchSize) {
        ACL_LOG_ERROR("index Out of bounds. index=%u, but batchsize=%u.", index,
                      batchPicDesc->dvppBatchPicDescs.batchSize);
        return nullptr;
    }

    return &(batchPicDesc->aclDvppPicDescs[index]);
}

aclError acldvppDestroyBatchPicDesc(acldvppBatchPicDesc *batchPicDesc)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_BATCH_PIC_DESC);
    if (batchPicDesc != nullptr) {
        acl::dvpp::FreeDeviceBuffer(batchPicDesc->dataBuffer);
        ACL_DELETE_ARRAY_AND_SET_NULL(batchPicDesc->aclDvppPicDescs);
        ACL_ALIGN_FREE(batchPicDesc);
    }

    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DVPP_BATCH_PIC_DESC);
    return ACL_SUCCESS;
}

acldvppLutMap *acldvppCreateLutMap()
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return nullptr;
    }
    return imageProcessor->acldvppCreateLutMap();
}

aclError acldvppDestroyLutMap(acldvppLutMap *lutMap)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppDestroyLutMap(lutMap);
}

uint32_t acldvppGetLutMapDims(const acldvppLutMap *lutMap)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return 0;
    }
    return imageProcessor->acldvppGetLutMapDims(lutMap);
}

aclError acldvppGetLutMapData(const acldvppLutMap *lutMap, uint32_t dim, uint8_t **data, uint32_t *len)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppGetLutMapData(lutMap, dim, data, len);
}

uint32_t aclvdecGetChannelDescOutMode(const aclvdecChannelDesc *channelDesc)
{
    ACL_LOG_DEBUG("start to execute aclvdecGetChannelDescOutMode");
    if (channelDesc == nullptr) {
        ACL_LOG_ERROR("channelDesc is null.");
        return 0; // default 0
    }

    return channelDesc->vdecDesc.outMode;
}

acldvppBorderConfig *acldvppCreateBorderConfig()
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return nullptr;
    }
    return imageProcessor->acldvppCreateBorderConfig();
}

aclError acldvppSetBorderConfigValue(acldvppBorderConfig *borderConfig, uint32_t index, double value)
{
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(borderConfig);
    if (index >= DVPP_MAKE_BORDER_MAX_COMPONENT) {
        ACL_LOG_ERROR("index[%u] should be smaller than array[%u]", index, DVPP_MAKE_BORDER_MAX_COMPONENT);
        return ACL_ERROR_INVALID_PARAM;
    }

    borderConfig->dvppBorderConfig.value[index] = value;
    return ACL_SUCCESS;
}

aclError acldvppSetBorderConfigBorderType(acldvppBorderConfig *borderConfig, acldvppBorderType borderType)
{
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(borderConfig);
    borderConfig->dvppBorderConfig.borderType = static_cast<uint32_t>(borderType);;
    return ACL_SUCCESS;
}

aclError acldvppSetBorderConfigTop(acldvppBorderConfig *borderConfig, uint32_t top)
{
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(borderConfig);
    if (top > UINT16_MAX) {
        ACL_LOG_ERROR("the param of top[%u] can't be larger than UINT16_MAX[%u]", top, UINT16_MAX);
        return ACL_ERROR_INVALID_PARAM;
    }
    borderConfig->dvppBorderConfig.top = static_cast<uint16_t>(top);
    return ACL_SUCCESS;
}

aclError acldvppSetBorderConfigBottom(acldvppBorderConfig *borderConfig, uint32_t bottom)
{
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(borderConfig);
    if (bottom > UINT16_MAX) {
        ACL_LOG_ERROR("the param of bottom[%u] can't be larger than UINT16_MAX[%u]", bottom, UINT16_MAX);
        return ACL_ERROR_INVALID_PARAM;
    }
    borderConfig->dvppBorderConfig.bottom = static_cast<uint16_t>(bottom);
    return ACL_SUCCESS;
}

aclError acldvppSetBorderConfigLeft(acldvppBorderConfig *borderConfig, uint32_t left)
{
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(borderConfig);
    if (left > UINT16_MAX) {
        ACL_LOG_ERROR("the param of left[%u] can't be larger than UINT16_MAX[%u]", left, UINT16_MAX);
        return ACL_ERROR_INVALID_PARAM;
    }
    borderConfig->dvppBorderConfig.left = static_cast<uint16_t>(left);
    return ACL_SUCCESS;
}

aclError acldvppSetBorderConfigRight(acldvppBorderConfig *borderConfig, uint32_t right)
{
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(borderConfig);
    if (right > UINT16_MAX) {
        ACL_LOG_ERROR("the param of right[%u] can't be larger than UINT16_MAX[%u]", right, UINT16_MAX);
        return ACL_ERROR_INVALID_PARAM;
    }
    borderConfig->dvppBorderConfig.right = static_cast<uint16_t>(right);
    return ACL_SUCCESS;
}

double acldvppGetBorderConfigValue(const acldvppBorderConfig *borderConfig, uint32_t index)
{
    if (borderConfig == nullptr) {
        ACL_LOG_ERROR("border config is null.");
        return -1;
    }
    if (index >= DVPP_MAKE_BORDER_MAX_COMPONENT) {
        ACL_LOG_ERROR("index[%u] should be smaller than array[%u]", index, DVPP_MAKE_BORDER_MAX_COMPONENT);
        // less than 0 is invalid value
        return -1;
    }
    return borderConfig->dvppBorderConfig.value[index];
}

acldvppBorderType acldvppGetBorderConfigBorderType(const acldvppBorderConfig *borderConfig)
{
    if (borderConfig == nullptr) {
        ACL_LOG_ERROR("border config is null.");
        return BORDER_CONSTANT;
    }

    return static_cast<acldvppBorderType>(borderConfig->dvppBorderConfig.borderType);
}

uint32_t acldvppGetBorderConfigTop(const acldvppBorderConfig *borderConfig)
{
    if (borderConfig == nullptr) {
        ACL_LOG_ERROR("border config is null.");
        return DVPP_BORDER_CONFIG_INVALID_VALUE;
    }

    return static_cast<uint32_t>(borderConfig->dvppBorderConfig.top);
}

uint32_t acldvppGetBorderConfigBottom(const acldvppBorderConfig *borderConfig)
{
    if (borderConfig == nullptr) {
        ACL_LOG_ERROR("border config is null.");
        return DVPP_BORDER_CONFIG_INVALID_VALUE;
    }

    return static_cast<uint32_t>(borderConfig->dvppBorderConfig.bottom);
}

uint32_t acldvppGetBorderConfigLeft(const acldvppBorderConfig *borderConfig)
{
    if (borderConfig == nullptr) {
        ACL_LOG_ERROR("border config is null.");
        return DVPP_BORDER_CONFIG_INVALID_VALUE;
    }

    return static_cast<uint32_t>(borderConfig->dvppBorderConfig.left);
}

uint32_t acldvppGetBorderConfigRight(const acldvppBorderConfig *borderConfig)
{
    if (borderConfig == nullptr) {
        ACL_LOG_ERROR("border config is null.");
        return DVPP_BORDER_CONFIG_INVALID_VALUE;
    }

    return static_cast<uint32_t>(borderConfig->dvppBorderConfig.right);
}

aclError acldvppDestroyBorderConfig(acldvppBorderConfig *borderConfig)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppDestroyBorderConfig(borderConfig);
}

aclError aclvdecSetChannelDescEnType(aclvdecChannelDesc *channelDesc, acldvppStreamFormat enType)
{
    ACL_LOG_DEBUG("start to execute aclvdecSetChannelDescEnType");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(channelDesc);

    if (enType < H265_MAIN_LEVEL) {
        ACL_LOG_ERROR("unsupported enType, enType = %u", enType);
        return ACL_ERROR_INVALID_PARAM;
    }

    channelDesc->vdecDesc.enType = enType;
    return ACL_SUCCESS;
}

aclError aclvdecSetChannelDescOutPicWidth(aclvdecChannelDesc *channelDesc, uint32_t outPicWdith)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return videoProcessor->aclvdecSetChannelDescOutPicWidth(channelDesc, outPicWdith);
}

aclError aclvdecSetChannelDescOutPicHeight(aclvdecChannelDesc *channelDesc, uint32_t outPicHeight)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return videoProcessor->aclvdecSetChannelDescOutPicHeight(channelDesc, outPicHeight);
}

aclError aclvdecSetChannelDescRefFrameNum(aclvdecChannelDesc *channelDesc, uint32_t refFrameNum)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return videoProcessor->aclvdecSetChannelDescRefFrameNum(channelDesc, refFrameNum);
}

aclError aclvdecSetChannelDescOutPicFormat(aclvdecChannelDesc *channelDesc, acldvppPixelFormat outPicFormat)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return videoProcessor->aclvdecSetChannelDescOutPicFormat(channelDesc, outPicFormat);
}

aclError aclvdecSetChannelDescChannelId(aclvdecChannelDesc *channelDesc, uint32_t channelId)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return videoProcessor->aclvdecSetChannelDescChannelId(channelDesc, channelId);
}

aclError aclvdecSetChannelDescBitDepth(aclvdecChannelDesc *channelDesc, uint32_t bitDepth)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return videoProcessor->aclvdecSetChannelDescBitDepth(channelDesc, bitDepth);
}

uint32_t aclvdecGetChannelDescBitDepth(const aclvdecChannelDesc *channelDesc)
{
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return 0;
    }
    return videoProcessor->aclvdecGetChannelDescBitDepth(channelDesc);
}

acldvppHist* acldvppCreateHist()
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return nullptr;
    }
    return imageProcessor->acldvppCreateHist();
}

aclError acldvppDestroyHist(acldvppHist *hist)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppDestroyHist(hist);
}

uint32_t acldvppGetHistDims(acldvppHist *hist)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppGetHistDims(hist);
}

aclError acldvppGetHistData(acldvppHist *hist, uint32_t dim, uint32_t **data, uint16_t *len)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppGetHistData(hist, dim, data, len);
}

uint32_t acldvppGetHistRetCode(acldvppHist* hist)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppGetHistRetCode(hist);
}

aclError acldvppClearHist(acldvppHist *hist)
{
    auto imageProcessor = acl::dvpp::DvppManager::GetInstance().GetImageProcessor();
    if (imageProcessor == nullptr) {
        ACL_LOG_ERROR("image processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return imageProcessor->acldvppClearHist(hist);
}
