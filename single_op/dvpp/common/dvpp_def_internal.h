/**
* @file dvpp_def_internal.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_TYPES_DVPP_DEF_INTERNAL_H
#define ACL_TYPES_DVPP_DEF_INTERNAL_H

#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <unordered_map>
#include "acl/ops/acl_dvpp.h"
#include "aicpu/dvpp/dvpp_def.h"
#include "types/tensor_desc_internal.h"
#include "runtime/base.h"
#include "runtime/context.h"
#include "single_op/dvpp/common/callback_info_manager.h"

namespace acl {
    namespace dvpp {
        constexpr const char *DVPP_KERNELS_SONAME = "libdvpp_kernels.so";
        constexpr const char *DVPP_KERNELNAME_CREATE_CHANNEL = "DvppCreateChannel";
        constexpr const char *DVPP_KERNELNAME_DESTROY_CHANNEL = "DvppDestroyChannel";
        constexpr const char *DVPP_KERNELNAME_RESIZE = "DvppResize";
        constexpr const char *DVPP_KERNELNAME_CROP = "DvppCrop";
        constexpr const char *DVPP_KERNELNAME_CROP_RESIZE = "DvppCropResize";
        constexpr const char *DVPP_KERNELNAME_CROP_RESIZE_PASTE = "DvppCropResizeAndPaste";
        constexpr const char *DVPP_KERNELNAME_CROP_AND_PASTE = "DvppCropAndPaste";
        constexpr const char *DVPP_KERNELNAME_CONVERT_COLOR = "DvppConvertColor";
        constexpr const char *DVPP_KERNELNAME_PYR_DOWN = "DvppPyrDown";
        constexpr const char *DVPP_KERNELNAME_CALC_HIST = "DvppCalcHist";
        constexpr const char *DVPP_KERNELNAME_DECODE_JPEG = "DvppDecodeJpeg";
        constexpr const char *DVPP_KERNELNAME_ENCODE_JPEG = "DvppEncodeJpeg";
        constexpr const char *DVPP_KERNELNAME_DECODE_PNG = "DvppDecodePng";
        constexpr const char *DVPP_KERNELNAME_GET_VERSION = "DvppGetVersion";
        constexpr const char *DVPP_KERNELNAME_EQUALIZE_HIST = "DvppEqualizeHist";
        constexpr const char *DVPP_KERNELNAME_MAKE_BORDER = "DvppMakeBorder";
        constexpr const char *DVPP_KERNELNAME_BATCH_CROP = "DvppBatchCrop";
        constexpr const char *DVPP_KERNELNAME_BATCH_CROP_RESIZE = "DvppBatchCropResize";
        constexpr const char *DVPP_KERNELNAME_BATCH_CROP_AND_PASTE = "DvppBatchCropAndPaste";
        constexpr const char *DVPP_KERNELNAME_BATCH_CROP_RESIZE_PASTE = "DvppBatchCropResizeAndPaste";
        constexpr const char *DVPP_KERNELNAME_BATCH_CROP_RESIZE_MAKEBORDER = "DvppBatchCropResizeAndMakeBorder";
        constexpr const char *DVPP_KERNELNAME_CREATE_VDEC_CHANNEL = "DvppCreateVdecChannel";
        constexpr const char *DVPP_KERNELNAME_DESTROY_VDEC_CHANNEL = "DvppDestroyVdecChannel";
        constexpr const char *DVPP_KERNELNAME_CREATE_VENC_CHANNEL = "DvppCreateVencChannel";
        constexpr const char *DVPP_KERNELNAME_DESTROY_VENC_CHANNEL = "DvppDestroyVencChannel";
        constexpr const char *DVPP_KERNELNAME_SEND_FRAME = "DvppSendVdecFrame";
        constexpr const char *DVPP_KERNELNAME_GET_FRAME = "DvppGetVdecFrame";
        constexpr const char *DVPP_KERNELNAME_VDEC_RELEASE_FRAME = "DvppReleaseVdecFrame";
        constexpr const char *DVPP_KERNELNAME_VENC_SEND_FRAME = "DvppSendVencFrame";
        constexpr const char *DVPP_KERNELNAME_VENC_GET_FRAME = "DvppGetVencFrame";
        constexpr const char *DVPP_KERNELNAME_VENC_RELEASE_FRAME = "DvppReleaseVencFrame";

        constexpr const char *DVPP_KERNELNAME_CREATE_CHANNEL_V2 = "DvppCreateChannelV2";
        constexpr const char *DVPP_KERNELNAME_DESTROY_CHANNEL_V2 = "DvppDestroyChannelV2";

        constexpr const char *DVPP_KERNELNAME_CREATE_VDEC_CHANNEL_V2 = "DvppCreateVdecChannelV2";
        constexpr const char *DVPP_KERNELNAME_DESTROY_VDEC_CHANNEL_V2 = "DvppDestroyVdecChannelV2";
        constexpr const char *DVPP_KERNELNAME_SEND_FRAME_V2 = "DvppSendVdecFrameV2";
        constexpr const char *DVPP_KERNELNAME_GET_FRAME_V2 = "DvppGetVdecFrameV2";

        // dvpp cmdlist buffer size, default 2M in v100
        constexpr size_t DVPP_CMDLIST_BUFFER_SIZE_V100 = 2 * 1024 * 1024;
        // dvpp cmdlist buffer size, default 4M in v200
        constexpr size_t DVPP_CMDLIST_BUFFER_SIZE_V200 = 4 * 1024 * 1024;
        // dvpp channel share buffer size, default 1k
        constexpr size_t DVPP_CHANNEL_SHARE_BUFFER_SIZE = 1 * 1024;
        // vdec channel share buffer size, default 2k
        constexpr size_t VDEC_CHANNEL_SHARE_BUFFER_SIZE = 2 * 1024;

        // define flexible array length
        constexpr uint32_t DVPP_RESIZE_CONFIG_TLV_LEN = 0;
        constexpr uint32_t DVPP_JPEGE_CONFIG_TLV_LEN = 0;
        constexpr uint32_t DVPP_CHANNEL_DESC_TLV_LEN = 1024;
        constexpr uint32_t DVPP_BORDER_CONFIG_TLV_LEN = 0;
        constexpr uint32_t DVPP_LUT_MAP_TLV_LEN = 0;
        constexpr uint32_t DVPP_HIST_DESC_LEN = 0;
        constexpr uint32_t VDEC_FRAME_CONFIG_TLV_LEN = 0;
        constexpr uint32_t VENC_FRAME_CONFIG_TLV_LEN = 0;
        // length of vdec TLV
        constexpr uint32_t VDEC_CHANNEL_DESC_TLV_LEN = 1024;
        // length of venc TLV
        constexpr uint32_t VENC_CHANNEL_DESC_TLV_LEN = 1024;
        // max resolution of input picture, width and height are from 128 to 1920
        constexpr size_t MAX_VENC_OUTPUT_BUFFER_SIZE = 3686400;
        // aicpu version supporting no notify task
        constexpr uint32_t AICPU_VERSION_NO_NOTIFY = 3;
        // max number of rois
        constexpr uint32_t BATCH_ROI_MAX_SIZE = 256;
    }
}

enum DvppVersion {
    DVPP_KERNELS_UNKOWN = -1,
    DVPP_KERNELS_V100 = 100,
    DVPP_KERNELS_V200 = 200,
};

enum WaitTaskType {
    NOTIFY_TASK = 0,
    EVENT_TASK
};

enum VencTLVType {
    VENC_RATE_CONTROL = 1,
    VENC_IP_PROP
};

enum VdecTLVType {
    VDEC_BIT_DEPTH = 1,
    VDEC_CSC_MATRIX
};

enum DvppTLVType {
    DVPP_CSC_MATRIX = 1
};

struct acldvppPicDesc {
    aclDataBuffer dataBuffer = {nullptr, 0};
    aicpu::dvpp::DvppPicDesc dvppPicDesc;
};

struct acldvppBatchPicDesc {
    aclDataBuffer dataBuffer = {nullptr, 0};
    acldvppPicDesc *aclDvppPicDescs = nullptr;
    aicpu::dvpp::DvppBatchPicDesc dvppBatchPicDescs;
};

struct acldvppRoiConfig {
    aicpu::dvpp::DvppRoiConfig dvppRoiConfig;
};

struct acldvppResizeConfig {
    aicpu::dvpp::DvppResizeConfig dvppResizeConfig;
};

struct DvppChannelDescTLVParam {
    size_t valueLen = 0;
    std::shared_ptr<void> value = nullptr;
};

struct acldvppChannelDesc {
    WaitTaskType dvppWaitTaskType = NOTIFY_TASK;
    void *notify = nullptr;
    bool isNeedNotify = true;
    uint64_t channelIndex = 0;
    aclDataBuffer dataBuffer = {nullptr, 0};
    aclDataBuffer cmdListBuffer = {nullptr, 0};
    aclDataBuffer shareBuffer = {nullptr, 0};
    std::mutex mutexForTLVMap;
    std::map<DvppTLVType, DvppChannelDescTLVParam> tlvParamMap;
    aicpu::dvpp::DvppChannelDesc dvppDesc;
};

struct acldvppJpegeConfig {
    aicpu::dvpp::DvppJpegeConfig dvppJpegeConfig;
};

struct acldvppStreamDesc {
    aclDataBuffer dataBuffer = {nullptr, 0};
    aicpu::dvpp::DvppStreamDesc dvppStreamDesc;
};

struct aclvdecFrameConfig {
    aicpu::dvpp::DvppVdecFrameConfig vdecFrameConfig;
};

struct aclvencFrameConfig {
    aicpu::dvpp::DvppVencFrameConfig vencFrameConfig;
};

struct VdecCallbackResultInfo {
    uint64_t frameId;
    aicpu::dvpp::DvppPicDesc dvppPicDesc;
};

struct VdecChannelDescTLVParam {
    size_t valueLen = 0;
    std::shared_ptr<void> value = nullptr;
};

struct aclvdecChannelDesc {
    aclvdecCallback callback = nullptr;
    uint64_t threadId = 0;
    WaitTaskType vdecWaitTaskType = NOTIFY_TASK;
    void *sendFrameNotify = nullptr;
    void *getFrameNotify = nullptr;
    rtStream_t sendFrameStream = nullptr;
    rtStream_t getFrameStream = nullptr;
    bool isNeedNotify = true;
    int32_t getStreamId = 0;
    int32_t sendStreamId = 0;
    std::atomic<bool> queueEmptyFlag = {true};  // task queue empty flag
    std::mutex mutexForQueue;
    std::condition_variable condVarForEos;      // condition variable for eos
    std::atomic<bool> eosBackFlag = {false};    // eos back flag
    std::queue<aicpu::dvpp::VdecCallbackInfoPtr> taskQueue;
    rtContext_t vdecMainContext;                // record context
    VdecCallbackResultInfo callbackResult;
    std::mutex mutexForCallbackMap;
    uint64_t frameId = 0; // 0 represents eos frame id
    std::unordered_map<uint64_t, aicpu::dvpp::VdecCallbackInfoPtr> callbackMap;
    std::mutex mutexForTLVMap;
    std::map<VdecTLVType, VdecChannelDescTLVParam> tlvParamMap;
    aclDataBuffer dataBuffer = {nullptr, 0};
    aclDataBuffer shareBuffer = {nullptr, 0};
    aicpu::dvpp::DvppVdecDesc vdecDesc;
};

struct VencChannelDescTLVParam {
    size_t valueLen = 0;
    std::shared_ptr<void> value = nullptr;
};

struct aclvencChannelDesc {
    aclvencCallback callback = nullptr;
    acldvppStreamDesc *outStreamDesc = nullptr;
    uint64_t threadId = 0;
    WaitTaskType vencWaitTaskType = NOTIFY_TASK;
    void *sendFrameNotify = nullptr;
    void *getFrameNotify = nullptr;
    rtStream_t sendFrameStream = nullptr;
    rtStream_t getFrameStream = nullptr;
    uint8_t outputMemMode = 0;
    uint64_t bufAddr = 0;
    uint32_t bufSize = 0;
    std::mutex mutexForTLVMap;
    std::map<VencTLVType, VencChannelDescTLVParam> tlvParamMap;
    aclDataBuffer dataBuffer = {nullptr, 0};
    aicpu::dvpp::DvppVencDesc vencDesc;
};

struct acldvppLutMap {
    aicpu::dvpp::DvppLutMap dvppLutMap;
};

struct acldvppBorderConfig {
    aicpu::dvpp::DvppBorderConfig dvppBorderConfig;
};

struct acldvppHist {
    aclDataBuffer dataBuffer = {nullptr, 0};
    aclDataBuffer shareBuffer = {nullptr, 0};       // save hist data device address
    aicpu::dvpp::DvppHistDesc dvppHistDesc;
};

#endif //ACL_TYPES_DVPP_DEF_INTERNAL_H
