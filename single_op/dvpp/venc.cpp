/**
* @file venc.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "acl/ops/acl_dvpp.h"
#include "common/log_inner.h"
#include "toolchain/profiling_manager.h"
#include "toolchain/resource_statistics.h"
#include "single_op/dvpp/mgr/dvpp_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

aclError aclvencCreateChannel(aclvencChannelDesc *channelDesc)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_VENC_CHANNEL);
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    aclError aclRet = videoProcessor->aclvencCreateChannel(channelDesc);
    if (aclRet == ACL_SUCCESS) {
        ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_VENC_CHANNEL);
    }
    return aclRet;
}

aclError aclvencDestroyChannel(aclvencChannelDesc *channelDesc)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_VENC_CHANNEL);
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    aclError aclRet = videoProcessor->aclvencDestroyChannel(channelDesc);
    if (aclRet == ACL_SUCCESS) {
        ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_VENC_CHANNEL);
    }
    return aclRet;
}

aclError aclvencSendFrame(aclvencChannelDesc *channelDesc, acldvppPicDesc *input, void *reserve,
    aclvencFrameConfig *config, void *userdata)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OTHERS);
    auto videoProcessor = acl::dvpp::DvppManager::GetInstance().GetVideoProcessor();
    if (videoProcessor == nullptr) {
        ACL_LOG_ERROR("video processor is null.");
        return ACL_ERROR_INTERNAL_ERROR;
    }
    return videoProcessor->aclvencSendFrame(channelDesc, input, reserve, config, userdata);
}

#ifdef __cplusplus
}
#endif
