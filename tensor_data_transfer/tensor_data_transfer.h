/**
* @file tensor_data_transfer.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ACL_TENSOR_DATA_TRANSFER_H
#define ACL_TENSOR_DATA_TRANSFER_H
#include <string.h>
#include <string>
#include <vector>
#include<memory>

#include "acl/acl_base.h"
#include "acl/acl_tdt.h"

struct acltdtDataItem {
    acltdtDataItem(acltdtTensorType tdtType,
        const int64_t *dims, size_t dimNum, const std::string &dimsStr,
        aclDataType type, const std::string &typeStr,
        std::shared_ptr<void> tensorData, size_t size)
    {
        this->tdtType = tdtType;
        for (size_t i = 0; i < dimNum; ++i) {
            this->dims.push_back(dims[i]);
        }
        this->dimsStr = dimsStr;
        this->dataType = type;
        this->dataTypeStr = typeStr;
        this->dataLen = size;
        this->dataPtr = tensorData;
    }
    acltdtDataItem() = default;
    ~acltdtDataItem() = default;
    acltdtTensorType tdtType;
    std::vector<int64_t> dims;
    std::string dimsStr;
    aclDataType dataType;
    std::string dataTypeStr;
    size_t dataLen;
    std::shared_ptr<void> dataPtr;
};

struct acltdtDataset {
    acltdtDataset()  : freeSelf(false) {};
    ~acltdtDataset()
    {
        if (freeSelf) {
            for (auto iter = blobs.begin(); iter != blobs.end(); ++iter) {
                (void)acltdtDestroyDataItem(*iter);
            }
        }
    }
    std::vector<acltdtDataItem *> blobs;
    bool freeSelf;
};

struct acltdtChannelHandle {
    acltdtChannelHandle(uint32_t deviceId, const char *channelName)
    {
        devId = deviceId;
        isTdtProcess = true;
        qid = 0;
        if (channelName != nullptr) {
            name = channelName;
            size_t prefixLen = sizeof("TF_RECEIVE_") - 1;
            if (0 == strncmp(channelName, "TF_RECEIVE_", prefixLen)) {
                recvName = channelName + prefixLen;
            }
        }
    }
    acltdtChannelHandle() = default;
    ~acltdtChannelHandle() = default;
    std::string name;
    std::string recvName;
    uint32_t devId;
    uint32_t qid;
    bool isTdtProcess;
};

aclError acltdtSendTensorV2(const acltdtChannelHandle *handle, const acltdtDataset *dataset, int32_t timeout);

aclError acltdtReceiveTensorV2(const acltdtChannelHandle *handle, acltdtDataset *dataset, int32_t timeout);

struct ItemInfo {
    int32_t version;
    int32_t dataType;
    uint32_t curCnt;
    uint32_t cnt;
    int32_t tensorType;
    uint32_t dimNum;
    char reserved[32];
    uint64_t dataLen;
};

struct aclTdtDataItemInfo {
    ItemInfo ctrlInfo;
    std::vector<int64_t> dims;
    std::shared_ptr<void> dataPtr;
};


#endif //ACL_TENSOR_DATA_TRANSFER_H

