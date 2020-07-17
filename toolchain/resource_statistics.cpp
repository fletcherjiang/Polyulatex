/**
* @file resource_statistics.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "resource_statistics.h"

#include <map>
#include <string>

#include "log_inner.h"

namespace {
    const std::map<ResourceType, std::string> RESOURCE_TPYR_TO_STRING = {
        {ACL_STATISTICS_MALLOC_FREE, "ACL_STATISTICS_MALLOC_FREE"},
        {ACL_STATISTICS_MALLOC_FREE_HOST, "ACL_STATISTICS_MALLOC_FREE_HOST"},
        {ACL_STATISTICS_CREATE_DESTROY_CONTEXT, "ACL_STATISTICS_CREATE_DESTROY_CONTEXT"},
        {ACL_STATISTICS_SET_RESET_DEVICE, "ACL_STATISTICS_SET_RESET_DEVICE"},
        {ACL_STATISTICS_CREATE_DESTROY_EVENT, "ACL_STATISTICS_CREATE_DESTROY_EVENT"},
        {ACL_STATISTICS_CREATE_DESTROY_STREAM, "ACL_STATISTICS_CREATE_DESTROY_STREAM"},
        {ACL_STATISTICS_DVPP_MALLOC_FREE, "ACL_STATISTICS_DVPP_MALLOC_FREE"},
        {ACL_STATISTICS_RECORD_RESET_EVENT, "ACL_STATISTICS_RECORD_RESET_EVENT"},
        {ACL_STATISTICS_CREATE_DESTROY_DATA_BUFFER, "ACL_STATISTICS_CREATE_DESTROY_DATA_BUFFER"},
        {ACL_STATISTICS_CREATE_DESTROY_TENSOR_DESC, "ACL_STATISTICS_CREATE_DESTROY_TENSOR_DESC"},
        {ACL_STATISTICS_CREATE_DESTROY_DESC, "ACL_STATISTICS_CREATE_DESTROY_DESC"},
        {ACL_STATISTICS_CREATE_DESTROY_DATASET, "ACL_STATISTICS_CREATE_DESTROY_DATASET"},
        {ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL, "ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL"},
        {ACL_STATISTICS_CREATE_DESTROY_AIPP, "ACL_STATISTICS_CREATE_DESTROY_AIPP"},
        {ACL_STATISTICS_CREATE_DESTROY_ATTR, "ACL_STATISTICS_CREATE_DESTROY_ATTR"},
        {ACL_STATISTICS_CREATE_DESTROY_HANDLE, "ACL_STATISTICS_CREATE_DESTROY_HANDLE"},
        {ACL_STATISTICS_CREATE_DESTROY_DVPP_CHANNEL_DESC, "ACL_STATISTICS_CREATE_DESTROY_DVPP_CHANNEL_DESC"},
        {ACL_STATISTICS_CREATE_DESTROY_DVPP_PIC_DESC, "ACL_STATISTICS_CREATE_DESTROY_DVPP_PIC_DESC"},
        {ACL_STATISTICS_CREATE_DESTROY_DVPP_ROI_CONFIG, "ACL_STATISTICS_CREATE_DESTROY_DVPP_ROI_CONFIG"},
        {ACL_STATISTICS_CREATE_DESTROY_DVPP_RESIZE_CONFIG, "ACL_STATISTICS_CREATE_DESTROY_DVPP_RESIZE_CONFIG"},
        {ACL_STATISTICS_CREATE_DESTROY_DVPP_JPEGE_CONFIG, "ACL_STATISTICS_CREATE_DESTROY_DVPP_JPEGE_CONFIG"},
        {ACL_STATISTICS_CREATE_DESTROY_VDEC_CHANNEL_DESC, "ACL_STATISTICS_CREATE_DESTROY_VDEC_CHANNEL_DESC"},
        {ACL_STATISTICS_CREATE_DESTROY_VENC_CHANNEL_DESC, "ACL_STATISTICS_CREATE_DESTROY_VENC_CHANNEL_DESC"},
        {ACL_STATISTICS_CREATE_DESTROY_DVPP_STREAM_DESC, "ACL_STATISTICS_CREATE_DESTROY_DVPP_STREAM_DESC"},
        {ACL_STATISTICS_CREATE_DESTROY_VDEC_FRAME_CONFIG, "ACL_STATISTICS_CREATE_DESTROY_VDEC_FRAME_CONFIG"},
        {ACL_STATISTICS_CREATE_DESTROY_VENC_FRAME_CONFIG, "ACL_STATISTICS_CREATE_DESTROY_VENC_FRAME_CONFIG"},
        {ACL_STATISTICS_CREATE_DESTROY_DVPP_CHANNEL, "ACL_STATISTICS_CREATE_DESTROY_DVPP_CHANNEL"},
        {ACL_STATISTICS_CREATE_DESTROY_VDEC_CHANNEL, "ACL_STATISTICS_CREATE_DESTROY_VDEC_CHANNEL"},
        {ACL_STATISTICS_CREATE_DESTROY_VENC_CHANNEL, "ACL_STATISTICS_CREATE_DESTROY_VENC_CHANNEL"},
        {ACL_STATISTICS_CREATE_DESTROY_DVPP_BATCH_PIC_DESC, "ACL_STATISTICS_CREATE_DESTROY_DVPP_BATCH_PIC_DESC"},
        {ACL_STATISTICS_CREATE_DESTROY_GROUP_INFO, "ACL_STATISTICS_CREATE_DESTROY_GROUP_INFO"},
        {ACL_STATISTICS_CREATE_DESTROY_PROF_CONFIG, "ACL_STATISTICS_CREATE_DESTROY_PROF_CONFIG"},
        {ACL_STATISTICS_CREATE_DESTROY_PROF_SUB_CONFIG, "ACL_STATISTICS_CREATE_DESTROY_PROF_SUB_CONFIG"},
        {ACL_STATISTICS_CREATE_DESTROY_MODEL_CONFIG, "ACL_STATISTICS_CREATE_DESTROY_MODEL_CONFIG"}
    };
}

namespace acl {
ResourceStatistics::ResourceStatistics() {}

ResourceStatistics::~ResourceStatistics() {}

ResourceStatistics &ResourceStatistics::GetInstance()
{
    static ResourceStatistics resourceStatistics;
    return resourceStatistics;
}

void ResourceStatistics::AddApplyTotalCount(ResourceType resourceType)
{
    ++counter_[resourceType].appplyReleaseValue[APPLY_TOTAL];
}

void ResourceStatistics::AddApplySuccCount(ResourceType resourceType)
{
    ++counter_[resourceType].appplyReleaseValue[APPLY_SUCCESS];
}

void ResourceStatistics::AddReleaseTotalCount(ResourceType resourceType)
{
    ++counter_[resourceType].appplyReleaseValue[RELEASE_TOTAL];
}

void ResourceStatistics::AddReleaseSuccCount(ResourceType resourceType)
{
    ++counter_[resourceType].appplyReleaseValue[RELEASE_SUCCESS];
}

void ResourceStatistics::TraverseStatistics()
{
    for (uint32_t i = 0; i < ACL_STATISTICS_RESOURCE_TPYE_SIZE; ++i) {
        std::string resourceTypeToString;
        auto iter = RESOURCE_TPYR_TO_STRING.find(static_cast<ResourceType>(i));
        if (iter == RESOURCE_TPYR_TO_STRING.end()) {
            ACL_LOG_EVENT("The ResourceType:%u is not exist!", i);
            continue;
        } else {
            resourceTypeToString = iter->second;
        }

        ACL_LOG_EVENT("The ResourceType:%s, applyTotal = %lu, applySucc = %lu, releaseTotal = %lu, releaseSucc = %lu",
            resourceTypeToString.c_str(),
            counter_[i].appplyReleaseValue[APPLY_TOTAL].load(std::memory_order_relaxed),
            counter_[i].appplyReleaseValue[APPLY_SUCCESS].load(std::memory_order_relaxed),
            counter_[i].appplyReleaseValue[RELEASE_TOTAL].load(std::memory_order_relaxed),
            counter_[i].appplyReleaseValue[RELEASE_SUCCESS].load(std::memory_order_relaxed));
    }
}
}
