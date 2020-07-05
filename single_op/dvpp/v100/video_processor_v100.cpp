/**
* @file image_processor_v100.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "video_processor_v100.h"
#include "common/log_inner.h"

namespace acl {
    namespace dvpp {
    namespace {
        constexpr int V100_CHANNEL_ID_CEILING = 31;
        constexpr int V100_CHANNEL_ID_FLOOR = 0;
    }

        /**
          * VDEC set channel id for channel desc.
          * @param channelDesc[in|out] channel desc
          * @param channelId[in] channel id
          * @return ACL_SUCCESS for ok, others for fail
          */
        aclError VideoProcessorV100::aclvdecSetChannelDescChannelId(aclvdecChannelDesc *channelDesc,
                                                                    uint32_t channelId)
        {
            ACL_LOG_DEBUG("start to execute aclvdecSetChannelDescChannelId");
            if (channelDesc == nullptr) {
                ACL_LOG_ERROR("channelDesc is null.");
                return ACL_ERROR_INVALID_PARAM;
            }

            if ((channelId < V100_CHANNEL_ID_FLOOR) || (channelId > V100_CHANNEL_ID_CEILING)) {
                ACL_LOG_ERROR("the value of channelId[%u] is invalid, it should be between in [%d, %d]",
                    channelId, V100_CHANNEL_ID_FLOOR, V100_CHANNEL_ID_CEILING);
                return ACL_ERROR_INVALID_PARAM;
            }

            channelDesc->vdecDesc.channelId = channelId;
            return ACL_SUCCESS;
        }

        /**
          * VDEC set outPicFormat for channel desc.
          * @param channelDesc[in|out] channel desc
          * @param outPicFormat[in] outPicFormat
          * @return ACL_SUCCESS for ok, others for fail
          */
        aclError VideoProcessorV100::aclvdecSetChannelDescOutPicFormat(aclvdecChannelDesc *channelDesc,
                                                                       acldvppPixelFormat outPicFormat)
        {
            ACL_LOG_DEBUG("start to execute aclvdecSetChannelDescOutPicFormat");
            if (channelDesc == nullptr) {
                ACL_LOG_ERROR("channelDesc is null.");
                return ACL_ERROR_INVALID_PARAM;
            }
            // v100 only support 2 format
            if ((outPicFormat != PIXEL_FORMAT_YUV_SEMIPLANAR_420) &&
                (outPicFormat != PIXEL_FORMAT_YVU_SEMIPLANAR_420)) {
                ACL_LOG_ERROR("the current outPicFormat[%d] is not support in this version, only support "
                    "PIXEL_FORMAT_YUV_SEMIPLANAR_420 and PIXEL_FORMAT_YVU_SEMIPLANAR_420",
                    static_cast<int32_t>(outPicFormat));
                return ACL_ERROR_INVALID_PARAM;
            }

            channelDesc->vdecDesc.outPicFormat = static_cast<uint32_t>(outPicFormat);
            return ACL_SUCCESS;
        }

        /**
         * Set ip proportion for venc channel desc.
         * @param channelDesc[OUT] venc channel desc
         * @param ipProp[IN] I frame and P frame proportion
         * @return ACL_SUCCESS for success, other for failure
         */
        aclError VideoProcessorV100::aclvencSetChannelDescIPProp(aclvencChannelDesc *channelDesc, uint32_t ipProp)
        {
            ACL_LOG_DEBUG("start to execute aclvencSetChannelDescIpProp, ipProp %u", ipProp);
            ACL_REQUIRES_NOT_NULL(channelDesc);

            {
                std::unique_lock<std::mutex> lock{channelDesc->mutexForTLVMap};
                auto it = channelDesc->tlvParamMap.find(VENC_IP_PROP);
                if (it == channelDesc->tlvParamMap.end()) {
                    VencChannelDescTLVParam vencTLVParam;
                    std::shared_ptr<aicpu::dvpp::VencIpProportion> ipProportion =
                        std::make_shared<aicpu::dvpp::VencIpProportion>();
                    ACL_REQUIRES_NOT_NULL(ipProportion);
                    ipProportion->maxIpProp = ipProp;
                    vencTLVParam.value = static_cast<std::shared_ptr<void>>(ipProportion);
                    vencTLVParam.valueLen = sizeof(aicpu::dvpp::VencIpProportion);
                    channelDesc->tlvParamMap[VENC_IP_PROP] = vencTLVParam;
                } else {
                    aicpu::dvpp::VencIpProportion *ipProportion =
                        static_cast<aicpu::dvpp::VencIpProportion *>(it->second.value.get());
                    ACL_REQUIRES_NOT_NULL(ipProportion);
                    ipProportion->maxIpProp = ipProp;
                }
            }

            return ACL_SUCCESS;
        }

        /**
         * Get ip proportion for venc channel desc.
         * @param channelDesc[IN] venc channel desc
         * @param isSupport[OUT] support flag
         * @return I frame and P frame proportion
         */
        uint32_t VideoProcessorV100::aclvencGetChannelDescIPProp(const aclvencChannelDesc *channelDesc,
            bool &isSupport)
        {
            ACL_LOG_DEBUG("start to execute aclvencSetChannelDescIpProp.");
            isSupport = true;
            if (channelDesc == nullptr) {
                ACL_LOG_ERROR("venc channelDesc is null.");
                return 0;
            }

            {
                std::mutex &mutexMap = const_cast<std::mutex &>(channelDesc->mutexForTLVMap);
                std::unique_lock<std::mutex> lock{mutexMap};
                const auto &it = channelDesc->tlvParamMap.find(VENC_IP_PROP);
                // no set venc rate control para, return 0
                if (it == channelDesc->tlvParamMap.end()) {
                    return 0;
                }

                aicpu::dvpp::VencIpProportion *ipProportion =
                    static_cast<aicpu::dvpp::VencIpProportion *>(it->second.value.get());
                if (ipProportion == nullptr) {
                    ACL_LOG_ERROR("ipProportion ptr is null.");
                    return 0;
                }

                return ipProportion->maxIpProp;
            }
        }
    }
}
