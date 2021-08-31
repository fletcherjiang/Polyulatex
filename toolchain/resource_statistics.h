/**
* @file resource_statistics.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_RESOURCE_STATISTICS_H_
#define ACL_RESOURCE_STATISTICS_H_

#include <atomic>
#include <cstdint>

#include "acl/acl_base.h"

#define ACL_ADD_APPLY_TOTAL_COUNT(resourceType) \
    acl::ResourceStatistics::GetInstance().AddApplyTotalCount(resourceType)
#define ACL_ADD_APPLY_SUCCESS_COUNT(resourceType) \
    acl::ResourceStatistics::GetInstance().AddApplySuccCount(resourceType)
#define ACL_ADD_RELEASE_TOTAL_COUNT(resourceType) \
    acl::ResourceStatistics::GetInstance().AddReleaseTotalCount(resourceType)
#define ACL_ADD_RELEASE_SUCCESS_COUNT(resourceType) \
    acl::ResourceStatistics::GetInstance().AddReleaseSuccCount(resourceType)

enum ResourceType {
    ACL_STATISTICS_MALLOC_FREE = 0,
    ACL_STATISTICS_MALLOC_FREE_HOST,
    ACL_STATISTICS_CREATE_DESTROY_CONTEXT,
    ACL_STATISTICS_SET_RESET_DEVICE,
    ACL_STATISTICS_CREATE_DESTROY_EVENT,
    ACL_STATISTICS_CREATE_DESTROY_STREAM,
    ACL_STATISTICS_DVPP_MALLOC_FREE,
    ACL_STATISTICS_RECORD_RESET_EVENT,
    ACL_STATISTICS_CREATE_DESTROY_DATA_BUFFER,
    ACL_STATISTICS_CREATE_DESTROY_TENSOR_DESC,
    ACL_STATISTICS_CREATE_DESTROY_DESC,
    ACL_STATISTICS_CREATE_DESTROY_DATASET,
    ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL,
    ACL_STATISTICS_CREATE_DESTROY_AIPP,
    ACL_STATISTICS_CREATE_DESTROY_ATTR,
    ACL_STATISTICS_CREATE_DESTROY_HANDLE,
    ACL_STATISTICS_CREATE_DESTROY_DVPP_CHANNEL_DESC,
    ACL_STATISTICS_CREATE_DESTROY_DVPP_PIC_DESC,
    ACL_STATISTICS_CREATE_DESTROY_DVPP_ROI_CONFIG,
    ACL_STATISTICS_CREATE_DESTROY_DVPP_RESIZE_CONFIG,
    ACL_STATISTICS_CREATE_DESTROY_DVPP_JPEGE_CONFIG,
    ACL_STATISTICS_CREATE_DESTROY_VDEC_CHANNEL_DESC,
    ACL_STATISTICS_CREATE_DESTROY_VENC_CHANNEL_DESC,
    ACL_STATISTICS_CREATE_DESTROY_DVPP_STREAM_DESC,
    ACL_STATISTICS_CREATE_DESTROY_VDEC_FRAME_CONFIG,
    ACL_STATISTICS_CREATE_DESTROY_VENC_FRAME_CONFIG,
    ACL_STATISTICS_CREATE_DESTROY_DVPP_CHANNEL,
    ACL_STATISTICS_CREATE_DESTROY_VDEC_CHANNEL,
    ACL_STATISTICS_CREATE_DESTROY_VENC_CHANNEL,
    ACL_STATISTICS_CREATE_DESTROY_DVPP_BATCH_PIC_DESC,
    ACL_STATISTICS_CREATE_DESTROY_GROUP_INFO,
    ACL_STATISTICS_CREATE_DESTROY_PROF_CONFIG,
    ACL_STATISTICS_CREATE_DESTROY_PROF_SUB_CONFIG,
    ACL_STATISTICS_CREATE_DESTROY_MODEL_CONFIG,
    ACL_STATISTICS_CREATE_DESTROY_QUEUE_ID,
    ACL_STATISTICS_CREATE_DESTROY_QUEUE_ATTR,
    ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE,
    ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE_LIST,
    ACL_STATISTICS_CREATE_DESTROY_QUEUE_ROUTE_QUERY,
    ACL_STATISTICS_CREATE_DESTROY_MBUF,
    ACL_STATISTICS_RESOURCE_TPYE_SIZE
};

namespace acl {
    enum ApplyReleaseType {
        APPLY_TOTAL = 0,
        APPLY_SUCCESS,
        RELEASE_TOTAL,
        RELEASE_SUCCESS,
        APPLY_RELEASE_SIZE
    };

    struct ResourceStatisticsValue {
        ResourceStatisticsValue()
        {
            Init();
        }

        void Init()
        {
            for (uint32_t i = 0U; i < static_cast<uint32_t>(APPLY_RELEASE_SIZE); ++i) {
                (void)appplyReleaseValue[i].exchange(0U);
            }
        }

        ~ResourceStatisticsValue() {}

        std::atomic<uint64_t> appplyReleaseValue[APPLY_RELEASE_SIZE];
    };

    class ACL_FUNC_VISIBILITY ResourceStatistics {
    public:
        static ResourceStatistics &GetInstance();
        void AddApplyTotalCount(ResourceType resourceType);
        void AddApplySuccCount(ResourceType resourceType);
        void AddReleaseTotalCount(ResourceType resourceType);
        void AddReleaseSuccCount(ResourceType resourceType);
        const void TraverseStatistics();
    private:
        ResourceStatistics();
        ~ResourceStatistics();
        ResourceStatisticsValue counter_[ACL_STATISTICS_RESOURCE_TPYE_SIZE];
    };
}

#endif // ACL_RESOURCE_STATISTICS_H_