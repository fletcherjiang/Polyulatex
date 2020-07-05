/**
* @file model_desc_internal.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ACL_MODEL_DESC_INTERNAL_H
#define ACL_MODEL_DESC_INTERNAL_H

#include <vector>
#include <string>
#include <set>
#include "acl/acl_base.h"
#include "types/tensor_desc_internal.h"

struct aclmdlTensorDesc {
    aclmdlTensorDesc() : name(""), size(0), format(ACL_FORMAT_UNDEFINED), dataType(ACL_DT_UNDEFINED) {}
    ~aclmdlTensorDesc() = default;
    std::string name;
    size_t size;
    aclFormat format;
    aclDataType dataType;
    std::vector<int64_t> dims;
    std::vector<int64_t> dimsV2; // supported for static aipp scene
    std::vector<std::pair<int64_t, int64_t>> shapeRanges;
};

struct aclmdlDesc {
    void Clear()
    {
        inputDesc.clear();
        outputDesc.clear();
        dynamicBatch.clear();
        dynamicHW.clear();
        dynamicDims.clear();
        dynamicOutputShape.clear();
        dataNameOrder.clear();
        modelId = 0;
    }
    uint32_t modelId = 0;
    std::vector<aclmdlTensorDesc> inputDesc;
    std::vector<aclmdlTensorDesc> outputDesc;
    std::vector<uint64_t> dynamicBatch;
    std::vector<std::vector<uint64_t>> dynamicHW;
    std::vector<std::vector<uint64_t>> dynamicDims;
    std::vector<std::vector<int64_t>> dynamicOutputShape;
    std::vector<std::string> dataNameOrder;
};

struct AclModelTensor {
    AclModelTensor(aclDataBuffer *dataBufIn, aclTensorDesc *tensorDescIn) : dataBuf(dataBufIn), tensorDesc(tensorDescIn)
    {
    }

    ~AclModelTensor() = default;
    aclDataBuffer *dataBuf;
    aclTensorDesc *tensorDesc;
};

struct aclmdlDataset {
    aclmdlDataset()
        : seq(0),
          modelId(0),
          timestamp(0),
          timeout(0),
          requestId(0),
          dynamicBatchSize(0),
          dynamicResolutionHeight(0),
          dynamicResolutionWidth(0) {}
    ~aclmdlDataset() = default;
    uint32_t seq;
    uint32_t modelId;
    std::vector<AclModelTensor> blobs;
    uint32_t timestamp;
    uint32_t timeout;
    uint64_t requestId;
    uint64_t dynamicBatchSize;
    uint64_t dynamicResolutionHeight;
    uint64_t dynamicResolutionWidth;
    std::vector<uint64_t> dynamicDims;
};

enum CceAippInputFormat {
    CCE_YUV420SP_U8 = 1,
    CCE_XRGB8888_U8,
    CCE_NC1HWC0DI_FP16,
    CCE_NC1HWC0DI_S8,
    CCE_RGB888_U8,
    CCE_ARGB8888_U8,
    CCE_YUYV_U8,
    CCE_YUV422SP_U8,
    CCE_AYUV444_U8,
    CCE_YUV400_U8,
    CCE_RAW10,
    CCE_RAW12,
    CCE_RAW16,
    // Hardware needs 15 and aipp component needs to reduce 1, so here it needs to be configured as 16.
    CCE_RAW24 = 16,
    CCE_RESERVED
};

enum AippMode {
    UNDEFINED = 0,
    STATIC_AIPP = 1,
    DYNAMIC_AIPP = 2
};

struct aclmdlAIPP {
    aclmdlAIPP()
        : batchSize(0) {}
    ~aclmdlAIPP() = default;
    uint64_t batchSize;
    std::vector<kAippDynamicBatchPara> aippBatchPara;
    kAippDynamicPara aippParms;
};

struct aclAippExtendInfo {
    bool isAippExtend = false;
};

struct aclmdlConfigHandle {
    aclmdlConfigHandle()
        : priority(0),
          mdlLoadType(0),
          mdlAddr(nullptr),
          mdlSize(0),
          workPtr(nullptr),
          workSize(0),
          weightPtr(nullptr),
          weightSize(0),
          inputQ(nullptr),
          inputQNum(0),
          outputQ(nullptr),
          outputQNum(0) {}
    int32_t priority;
    size_t mdlLoadType;
    std::string loadPath;
    void *mdlAddr;
    size_t mdlSize;
    void *workPtr;
    size_t workSize;
    void *weightPtr;
    size_t weightSize;
    const uint32_t *inputQ;
    size_t inputQNum;
    const uint32_t *outputQ;
    size_t outputQNum;
    std::set<aclmdlConfigAttr> attrState;
};

#endif // ACL_MODEL_DESC_INTERNAL_H
