/**
* @file vdec.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/ops/acl_dvpp.h"

#include "common/log_inner.h"
#include "single_op/dvpp/mgr/dvpp_manager.h"
#include "single_op/dvpp/common/dvpp_util.h"
#include "toolchain/profiling_manager.h"
#include "toolchain/resource_statistics.h"

#ifdef __cplusplus
extern "C" {
#endif

aclError aclvdecCreateChannel(aclvdecChannelDesc *channelDesc)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_VDEC_CHANNEL);
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    ACL_REQUIRES_NOT_NULL(channelDesc);
    DvppVersion dvppVersion = acl::dvpp::DvppManager::GetInstance().GetDvppVersion();
    uint32_t aiCpuVersion = acl::dvpp::DvppManager::GetInstance().GetAicpuVersion();
    if (acl::dvpp::IsSupportSuperTask(dvppVersion, aiCpuVersion)) {
        channelDesc->isNeedNotify = false;
    }
    aclError aclRet = videoProcessor->aclvdecCreateChannel(channelDesc);
    if (aclRet == ACL_SUCCESS) {
        ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_VDEC_CHANNEL);
    }
    return aclRet;
}

aclError aclvdecDestroyChannel(aclvdecChannelDesc *channelDesc)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_VDEC_CHANNEL);
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    aclError aclRet = videoProcessor->aclvdecDestroyChannel(channelDesc);
    if (aclRet == ACL_SUCCESS) {
        ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_VDEC_CHANNEL);
    }
    return aclRet;
}

aclError aclvdecSendFrame(aclvdecChannelDesc *channelDesc,
                          acldvppStreamDesc *input,
                          acldvppPicDesc *output,
                          aclvdecFrameConfig *config,
                          void *userData)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return videoProcessor->aclvdecSendFrame(channelDesc, input, output, config, userData, false);
}

aclError aclvdecSendSkippedFrame(aclvdecChannelDesc *channelDesc,
                                 acldvppStreamDesc *input,
                                 aclvdecFrameConfig *config,
                                 void *userData)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return videoProcessor->aclvdecSendFrame(channelDesc, input, nullptr, config, userData, true);
}

#ifdef __cplusplus
}
#endif
