/**
* @file tensor_data_transfer.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "tensor_data_transfer.h"
#include <map>
#include <mutex>

#include "tdt_host_interface.h"

#include "log_inner.h"
#include "acl/acl_mdl.h"
#include "acl/acl_tdt_queue.h"
#include "queue.h"
#include "runtime/rt_mem_queue.h"

namespace {
    std::mutex aclChannleMutex;
    std::map<std::string, acltdtChannelHandle *> aclChannleMap;
    std::map<std::string, aclDataType> aclDataTypeStrMap =
    {
        {"bool",     ACL_BOOL},
        {"int8",     ACL_INT8},
        {"uint8",    ACL_UINT8},
        {"half",     ACL_FLOAT16},
        {"int16",    ACL_INT16},
        {"uint16",   ACL_UINT16},
        {"float",    ACL_FLOAT},
        {"int32",    ACL_INT32},
        {"uint32",   ACL_UINT32},
        {"int64",    ACL_INT64},
        {"uint64",   ACL_UINT64},
        {"double",   ACL_DOUBLE},
        {"string",   ACL_STRING}
    };
}

namespace acl {
    bool GetTensorShape(const std::string &dimsStr, std::vector<int64_t> &dims)
    {
        // change "[32,224,224,3]" => "32,224,224,3"
        // tensor_shape.size() - 2 is the second to last
        if (dimsStr.size() < 2) {
            ACL_LOG_INNER_ERROR("[Check][dimsStr]Invalid shape string: %s", dimsStr.c_str());
            return false;
        }

        std::string str = dimsStr.substr(1, dimsStr.size() - 2);
        std::string::size_type index = 0;
        if (!str.empty()) {
            while ((index = str.find(' ', index)) != std::string::npos) {
                str.erase(index, 1);
            }
        }
        std::string split = ",";
        std::string::size_type pos2 = str.find(split);
        std::string::size_type pos1 = 0;
        while (pos2 != std::string::npos) {
            try {
                dims.push_back(std::stoll(str.substr(pos1, pos2 - pos1)));
            } catch (...) {
                ACL_LOG_INNER_ERROR("[Check][Shape]Invalid shape string: %s", dimsStr.c_str());
                return false;
            }
            // string::size_type can store the length of any string object
            pos1 = pos2 + split.size();
            pos2 = str.find(split, pos1);
        }
        if (pos1 != str.length()) {
            try {
                dims.push_back(std::stoll(str.substr(pos1)));
            } catch (...) {
                ACL_LOG_INNER_ERROR("[Check][Shape]Invalid shape string: %s", dimsStr.c_str());
                return false;
            }
        }
        return true;
    }

    aclError GetTdtDataTypeByAclDataType(acltdtTensorType aclType, tdt::TdtDataType &tdtDataType)
    {
        switch (aclType) {
            case ACL_TENSOR_DATA_END_OF_SEQUENCE: {
                tdtDataType = tdt::TDT_END_OF_SEQUENCE;
                break;
            }
            case ACL_TENSOR_DATA_TENSOR: {
                tdtDataType = tdt::TDT_TENSOR;
                break;
            }
            case ACL_TENSOR_DATA_ABNORMAL: {
                tdtDataType = tdt::TDT_ABNORMAL;
                break;
            }
            default: {
                ACL_LOG_INNER_ERROR("[Check][Type]unkown acltdtTensorType %d.", aclType);
                return ACL_ERROR_INVALID_PARAM;
            }
        }
        return ACL_SUCCESS;
    }

    aclError GetTdtDataTypeByAclDataTypeV2(acltdtTensorType aclType, int32_t &tdtDataType)
    {
        switch (aclType) {
            case ACL_TENSOR_DATA_END_OF_SEQUENCE: {
                tdtDataType = 1;
                break;
            }
            case ACL_TENSOR_DATA_TENSOR: {
                tdtDataType = 0;
                break;
            }
            case ACL_TENSOR_DATA_ABNORMAL: {
                tdtDataType = 2;
                break;
            }
            default: {
                ACL_LOG_INNER_ERROR("[Check][Type]unkown acltdtTensorType %d.", aclType);
                return ACL_ERROR_INVALID_PARAM;
            }
        }
        return ACL_SUCCESS;
    }

    aclError GetAclTypeByTdtDataType(tdt::TdtDataType tdtDataType, acltdtTensorType &aclType)
    {
        switch (tdtDataType) {
            case tdt::TDT_END_OF_SEQUENCE: {
                aclType = ACL_TENSOR_DATA_END_OF_SEQUENCE;
                break;
            }
            case tdt::TDT_TENSOR: {
                aclType = ACL_TENSOR_DATA_TENSOR;
                break;
            }
            case tdt::TDT_ABNORMAL: {
                aclType = ACL_TENSOR_DATA_ABNORMAL;
                break;
            }
            default: {
                ACL_LOG_INNER_ERROR("[Check][Datatype]unkown TdtDataType %d.", tdtDataType);
                return ACL_ERROR_UNSUPPORTED_DATA_TYPE;
            }
        }
        return ACL_SUCCESS;
    }

    aclError GetAclTypeByTdtDataTypeV2(int32_t tdtDataType, acltdtTensorType &aclType)
    {
        switch (tdtDataType) {
            case 1: {
                aclType = ACL_TENSOR_DATA_END_OF_SEQUENCE;
                break;
            }
            case 0: {
                aclType = ACL_TENSOR_DATA_TENSOR;
                break;
            }
            case 2: {
                aclType = ACL_TENSOR_DATA_ABNORMAL;
                break;
            }
            default: {
                ACL_LOG_INNER_ERROR("[Check][Datatype]unkown TdtDataType %d.", tdtDataType);
                return ACL_ERROR_UNSUPPORTED_DATA_TYPE;
            }
        }
        return ACL_SUCCESS;
    }

    aclError TensorDatasetSerializes(const acltdtDataset *dataset, std::vector<tdt::DataItem> &itemVec)
    {
        ACL_REQUIRES_NOT_NULL(dataset);

        for (size_t i = 0; i < dataset->blobs.size(); ++i) {
            tdt::DataItem item;
            tdt::TdtDataType tdtDataType;
            auto ret = GetTdtDataTypeByAclDataType(dataset->blobs[i]->tdtType, tdtDataType);
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Check][Dataset]TensorDatasetSerializes failed, "
                    "invalid tdt type %d", dataset->blobs[i]->tdtType);
                itemVec.clear();
                return ret;
            }

            item.dataType_ = tdtDataType;
            item.tensorShape_ = dataset->blobs[i]->dimsStr;
            item.tensorType_ = dataset->blobs[i]->dataTypeStr;
            item.dataLen_ = dataset->blobs[i]->dataLen;
            item.dataPtr_ = dataset->blobs[i]->dataPtr;
            itemVec.emplace_back(item);
        }
        return ACL_SUCCESS;
    }

    aclError TensorDatasetSerializesV2(const acltdtDataset *dataset, std::vector<aclTdtDataItemInfo> &itemVec)
    {
        ACL_REQUIRES_NOT_NULL(dataset);

        for (size_t i = 0; i < dataset->blobs.size(); ++i) {
            aclTdtDataItemInfo item;
            int32_t tdtDataType;
            auto ret = GetTdtDataTypeByAclDataTypeV2(dataset->blobs[i]->tdtType, tdtDataType);
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Check][Dataset]TensorDatasetSerializes failed, "
                    "invalid tdt type %d", dataset->blobs[i]->tdtType);
                itemVec.clear();
                return ret;
            }

            item.ctrlInfo.dataType = tdtDataType;
            item.ctrlInfo.tensorType = dataset->blobs[i]->dataType;
            item.ctrlInfo.dimNum = dataset->blobs[i]->dims.size();
            item.dims = dataset->blobs[i]->dims;
            item.ctrlInfo.dataLen = dataset->blobs[i]->dataLen;
            item.dataPtr = dataset->blobs[i]->dataPtr;
            itemVec.emplace_back(item);
            ACL_LOG_INFO("TensorDatasetSerializesWithQueue, dataType %d, tensorType %d, dimNum %u, dataLen %lu",
                item.ctrlInfo.dataType, item.ctrlInfo.tensorType, item.ctrlInfo.dimNum, item.ctrlInfo.dataLen);
        }
        return ACL_SUCCESS;
    }

    aclError TensorDatasetDeserializes(const std::vector<tdt::DataItem> &itemVec, acltdtDataset *dataset)
    {
        ACL_REQUIRES_NOT_NULL(dataset);
        if (dataset->blobs.size() != 0) {
            ACL_LOG_INNER_ERROR("[Check][Dataset]Dataset size[%zu] is not empty", dataset->blobs.size());
            return ACL_ERROR_INVALID_PARAM;
        }
        aclError ret = ACL_SUCCESS;
        for (size_t i = 0; i < itemVec.size(); ++i) {
            acltdtTensorType aclType;
            ret = GetAclTypeByTdtDataType(itemVec[i].dataType_, aclType);
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Check][Dataset]TensorDatasetDeserializes failed, invalid data type %d",
                    itemVec[i].dataType_);
                break;
            }

            if (aclType == ACL_TENSOR_DATA_TENSOR) {
                std::vector<int64_t> dims;
                if (!GetTensorShape(itemVec[i].tensorShape_, dims)) {
                    ACL_LOG_INNER_ERROR("[Check][TensorDataset]TensorDatasetDeserializes failed, "
                        "invalid tensor shape[%s]", itemVec[i].tensorShape_.c_str());
                    ret = ACL_ERROR_INTERNAL_ERROR;
                    break;
                }

                auto iter = aclDataTypeStrMap.find(itemVec[i].tensorType_);
                if (iter == aclDataTypeStrMap.end()) {
                    ACL_LOG_INNER_ERROR("[Deserialize][TensorDataset]TensorDatasetDeserializes failed, "
                        "unkown data type[%s]", itemVec[i].tensorType_.c_str());
                    ret = ACL_ERROR_INTERNAL_ERROR;
                    break;
                }
                aclDataType dataType = iter->second;
                acltdtDataItem *item = new(std::nothrow) acltdtDataItem(aclType,
                    &dims[0], dims.size(), itemVec[i].tensorShape_,
                    dataType, itemVec[i].tensorType_,
                    itemVec[i].dataPtr_, itemVec[i].dataLen_);
                if (item == nullptr) {
                    ACL_LOG_INNER_ERROR("[Check][Item]TensorDatasetDeserializes alloc failed");
                    ret = ACL_ERROR_BAD_ALLOC;
                    break;
                }
                dataset->blobs.push_back(item);
            } else {
                acltdtDataItem *item = new(std::nothrow) acltdtDataItem(aclType,
                    nullptr, 0, itemVec[i].tensorShape_, ACL_DT_UNDEFINED,
                    itemVec[i].tensorType_, itemVec[i].dataPtr_, itemVec[i].dataLen_);
                if (item == nullptr) {
                    ACL_LOG_INNER_ERROR("[Check][Item]TensorDatasetDeserializes alloc failed");
                    ret = ACL_ERROR_BAD_ALLOC;
                    break;
                }
                dataset->blobs.push_back(item);
            }
        }

        if (ret != ACL_SUCCESS) {
            for (size_t i = 0; i < dataset->blobs.size(); ++i) {
                ACL_DELETE_AND_SET_NULL(dataset->blobs[i]);
            }
            dataset->blobs.clear();
        }
        dataset->freeSelf = true;
        return ret;
    }

    aclError TensorDatasetDeserializesV2(const std::vector<aclTdtDataItemInfo> &itemVec, acltdtDataset *dataset)
    {
        ACL_REQUIRES_NOT_NULL(dataset);
        if (dataset->blobs.size() != 0) {
            ACL_LOG_INNER_ERROR("[Check][Dataset]Dataset size[%zu] is not empty", dataset->blobs.size());
            return ACL_ERROR_INVALID_PARAM;
        }
        aclError ret = ACL_SUCCESS;
        for (size_t i = 0; i < itemVec.size(); ++i) {
            acltdtTensorType aclType;
            ret = GetAclTypeByTdtDataTypeV2(itemVec[i].ctrlInfo.dataType, aclType);
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Check][Dataset]TensorDatasetDeserializes failed, invalid data type %d",
                    itemVec[i].ctrlInfo.dataType);
                break;
            }

            if (aclType == ACL_TENSOR_DATA_TENSOR) {
                std::vector<int64_t> dims = itemVec[i].dims;
                aclDataType dataType = static_cast<aclDataType>(itemVec[i].ctrlInfo.tensorType);
                
                acltdtDataItem *item = new(std::nothrow) acltdtDataItem(aclType,
                    &dims[0], dims.size(), "",
                    dataType, "",
                    itemVec[i].dataPtr, itemVec[i].ctrlInfo.dataLen);
                if (item == nullptr) {
                    ACL_LOG_INNER_ERROR("[Check][Item]TensorDatasetDeserializes alloc failed");
                    ret = ACL_ERROR_BAD_ALLOC;
                    break;
                }
                dataset->blobs.push_back(item);
            } else {
                acltdtDataItem *item = new(std::nothrow) acltdtDataItem(aclType,
                    nullptr, 0, "", ACL_DT_UNDEFINED,
                    "", itemVec[i].dataPtr, itemVec[i].ctrlInfo.dataLen);
                if (item == nullptr) {
                    ACL_LOG_INNER_ERROR("[Check][Item]TensorDatasetDeserializes alloc failed");
                    ret = ACL_ERROR_BAD_ALLOC;
                    break;
                }
                dataset->blobs.push_back(item);
            }
        }

        if (ret != ACL_SUCCESS) {
            for (size_t i = 0; i < dataset->blobs.size(); ++i) {
                ACL_DELETE_AND_SET_NULL(dataset->blobs[i]);
            }
            dataset->blobs.clear();
        }
        dataset->freeSelf = true;
        return ret;
    }

    void GetTensorDimsString(const int64_t *dims, size_t dimNum, std::string &dimsStr)
    {
        for (size_t i = 0; i < dimNum; ++i) {
            dimsStr += std::to_string(dims[i]);
            if (i + 1 == dimNum) {
                break;
            }
            dimsStr.push_back(',');
        }
        dimsStr += "]";
    }
} // namespace acl

acltdtTensorType acltdtGetTensorTypeFromItem(const acltdtDataItem *dataItem)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (dataItem == nullptr) {
        ACL_LOG_ERROR("[Check][Dataitem]param [dataItem] must not be null.");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"dataItem"}));
        return ACL_TENSOR_DATA_UNDEFINED;
    }
    return dataItem->tdtType;
}

aclDataType acltdtGetDataTypeFromItem(const acltdtDataItem *dataItem)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (dataItem == nullptr) {
        ACL_LOG_ERROR("[Check][Dataitem]param [dataItem] must not be null.");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"dataItem"}));
        return ACL_DT_UNDEFINED;
    }
    return dataItem->dataType;
}

void *acltdtGetDataAddrFromItem(const acltdtDataItem *dataItem)
{
    ACL_REQUIRES_NOT_NULL_RET_NULL(dataItem);
    return dataItem->dataPtr.get();
}

size_t acltdtGetDataSizeFromItem(const acltdtDataItem *dataItem)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (dataItem == nullptr) {
        ACL_LOG_ERROR("[Check][Dataitem]param [dataItem] must not be null.");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"dataItem"}));
        return 0;
    }
    return dataItem->dataLen;
}

size_t acltdtGetDimNumFromItem(const acltdtDataItem *dataItem)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (dataItem == nullptr) {
        ACL_LOG_ERROR("[Check][Dataitem]param [dataItem] must not be null.");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"dataItem"}));
        return 0;
    }
    return dataItem->dims.size();
}

aclError acltdtGetDimsFromItem(const acltdtDataItem *dataItem, int64_t *dims, size_t dimNum)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL(dataItem);
    // check dims and dimNum
    if ((dims == nullptr && dimNum != 0) || (dims != nullptr && dimNum == 0)) {
        ACL_LOG_INNER_ERROR("[Check][Params]acltdtGetDimsFromItem failed, invalid dims and dimNum[%zu]", dimNum);
        return ACL_ERROR_INVALID_PARAM;
    }

    if (dimNum < dataItem->dims.size()) {
        ACL_LOG_INNER_ERROR("[Check][dimNum]output dimNum[%zu] cannot be less than dims number[%zu]",
            dimNum, dataItem->dims.size());
        return ACL_ERROR_INVALID_PARAM;
    }

    for (size_t i = 0; i < dataItem->dims.size(); ++i) {
        dims[i] = dataItem->dims[i];
    }

    return ACL_SUCCESS;
}

acltdtDataItem *acltdtCreateDataItem(acltdtTensorType tdtType,
    const int64_t *dims, size_t dimNum, aclDataType dataType, void *data, size_t size)
{
    ACL_STAGES_REG(acl::ACL_STAGE_CREATE, acl::ACL_STAGE_DEFAULT);
    if ((dims == nullptr && dimNum != 0) || (dims != nullptr && dimNum == 0)) {
        ACL_LOG_INNER_ERROR("[Check][Params]acltdtCreateDataItem failed, invalid dims and dimNum[%zu]", dimNum);
        return nullptr;
    }
    if (dimNum > ACL_MAX_DIM_CNT) {
        ACL_LOG_INNER_ERROR("[Check][Dimnum]acltdtCreateDataItem failed, dimNum[%zu] can't be larger than "
            "ACL_MAX_DIM_CNT[%d]", dimNum, ACL_MAX_DIM_CNT);
        return nullptr;
    }

    if (tdtType != ACL_TENSOR_DATA_TENSOR) {
        if (dims != nullptr) {
            ACL_LOG_INNER_ERROR("[Check][Dims]acltdtCreateDataItem failed, "
                "dims must be nullptr. tdtType is %d", tdtType);
            return nullptr;
        }
        return new(std::nothrow) acltdtDataItem(tdtType, dims, dimNum, "[]", ACL_DT_UNDEFINED, "", nullptr, 0);
    }

    // tdtType: ACL_TENSOR_DATA_TENSOR
    std::string dimsStr = "[";
    acl::GetTensorDimsString(dims, dimNum, dimsStr);

    std::string typeStr;
    for (const auto &item: aclDataTypeStrMap) {
        if (item.second == dataType) {
            typeStr = item.first;
            break;
        }
    }
    if (typeStr.empty()) {
        ACL_LOG_INNER_ERROR("[Check][Typestr]acltdtCreateDataItem failed, unspported datatype: %d", dataType);
        return nullptr;
    }
    std::shared_ptr<void> dataPtr;
    dataPtr.reset(data, [](const void *p) {});
    return new(std::nothrow) acltdtDataItem(tdtType, dims, dimNum, dimsStr, dataType, typeStr, dataPtr, size);
}

aclError acltdtDestroyDataItem(acltdtDataItem *dataItem)
{
    ACL_STAGES_REG(acl::ACL_STAGE_DESTROY, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL(dataItem);
    ACL_DELETE_AND_SET_NULL(dataItem);
    return ACL_SUCCESS;
}

acltdtDataset *acltdtCreateDataset()
{
    ACL_STAGES_REG(acl::ACL_STAGE_CREATE, acl::ACL_STAGE_DEFAULT);
    return new(std::nothrow) acltdtDataset();
}

aclError acltdtDestroyDataset(acltdtDataset *dataset)
{
    ACL_STAGES_REG(acl::ACL_STAGE_DESTROY, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL(dataset);
    ACL_DELETE_AND_SET_NULL(dataset);
    return ACL_SUCCESS;
}

aclError acltdtAddDataItem(acltdtDataset *dataset, acltdtDataItem *dataItem)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL(dataset);
    ACL_REQUIRES_NOT_NULL(dataItem);
    if (dataset->freeSelf) {
        ACL_LOG_INNER_ERROR("[Check][Freeself]item cannot be added, because internal item already exists");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    dataset->blobs.push_back(dataItem);
    return ACL_SUCCESS;
}

acltdtDataItem *acltdtGetDataItem(const acltdtDataset *dataset, size_t index)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if ((dataset == nullptr) || (index >= dataset->blobs.size())) {
        ACL_LOG_INNER_ERROR("[Check][Dataset]input param is invalid, index[%zu]", index);
        return nullptr;
    }

    return dataset->blobs[index];
}

size_t acltdtGetDatasetSize(const acltdtDataset *dataset)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (dataset == nullptr) {
        ACL_LOG_ERROR("[Check][Dataset]dataset is null.");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}), std::vector<std::string>({"dataset"}));
        return 0;
    }
    return dataset->blobs.size();
}

acltdtChannelHandle *acltdtCreateChannel(uint32_t deviceId, const char *name)
{
    ACL_STAGES_REG(acl::ACL_STAGE_CREATE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_RET_NULL(name);
    auto ret = tdt::TdtHostInit(deviceId);
    if (ret != 0) {
        ACL_LOG_INNER_ERROR("[Init][Tdt]tdt host init failed, tdt result = %d", ret);
        return nullptr;
    }
    acltdtChannelHandle *handle = new(std::nothrow) acltdtChannelHandle(deviceId, name);
    if (handle == nullptr) {
        ACL_LOG_INNER_ERROR("acltdtChannelHandle is nullptr");
        return nullptr;
    }
    if (handle != nullptr) {
        if (!handle->recvName.empty()) {
            (void)tdt::TdtHostPreparePopData();
        }
        {
            std::unique_lock<std::mutex> lk(aclChannleMutex);
            aclChannleMap[name] = handle;
        }
    }
    return handle;
}

acltdtChannelHandle *acltdtCreateChannelWithDepth(uint32_t deviceId, const char *name, uint32_t maxDepth)
{
    ACL_STAGES_REG(acl::ACL_STAGE_CREATE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_RET_NULL(name);
    ACL_LOG_INFO("acltdtCreateChannelWithDepth devId is %u, name is %s, maxDepth is %u", deviceId, name, maxDepth);
    if (strlen(name) + 1 > RT_MQ_MAX_NAME_LEN) {
        ACL_LOG_ERROR("name [%s] length %d can not be larger than %d", name, strlen(name) + 1, RT_MQ_MAX_NAME_LEN);
        return nullptr;
    }
    acltdtChannelHandle *handle = new(std::nothrow) acltdtChannelHandle(deviceId, name);
    if (handle == nullptr) {
        ACL_LOG_INNER_ERROR("acltdtChannelHandle is nullptr");
        return nullptr;
    }
    handle->qid = maxDepth;
    handle->isTdtProcess = false;
    std::string realName = handle->recvName.empty() ? handle->name :handle->recvName;
    if (!handle->recvName.empty() && (realName.length() + 1 > RT_MQ_MAX_NAME_LEN)) {
        ACL_LOG_ERROR("name [%s] length %d can not be larger than %d", name, realName.c_str(), RT_MQ_MAX_NAME_LEN);
        return nullptr;
    }
    acltdtQueueAttr attr;
    auto ret = memcpy_s(attr.name, RT_MQ_MAX_NAME_LEN, realName.c_str(), strlen(name) + 1);
    if (ret != EN_OK) {
        ACL_LOG_INNER_ERROR("[Call][MemCpy]call memcpy failed, result=%d, srcLen=%zu, dstLen=%zu",
                ret, realName.length() + 1, RT_MQ_MAX_NAME_LEN);
        ACL_DELETE_AND_SET_NULL(handle);
        return nullptr;
    }
    attr.depth = maxDepth;
    attr.workMode = RT_MQ_MODE_DEFAULT;
    attr.flowCtrlFlag = false;
    attr.flowCtrlDropTime = 0;
    attr.overWriteFlag = false;
    if (acltdtCreateQueue(&attr, &handle->qid) != ACL_SUCCESS) {
        return nullptr;
    }
    ACL_LOG_INFO("acltdtCreateChannelWithDepth devId is %u, name is %s, real name is %s, qid is %u",
                 deviceId, handle->name.c_str(), realName.c_str(), handle->qid);
    return handle;
}

aclError acltdtStopChannel(acltdtChannelHandle *handle)
{
    ACL_STAGES_REG(acl::ACL_STAGE_TDT, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL(handle);
    ACL_LOG_INFO("start to acltdtStopChannel, device is %u, name is %s",
        handle->devId, handle->name.c_str());
    if (!handle->isTdtProcess) {
        ACL_LOG_INFO("new process , stop channel is no use");
        return ACL_SUCCESS;
    }
    if (!handle->recvName.empty()) {
        auto ret = tdt::TdtHostStop(handle->recvName);
        if (ret != 0) {
            ACL_LOG_INNER_ERROR("[Init][Tdt]tdt host stop failed for channel %s, tdt result = %d",
                handle->name.c_str(), ret);
            return ACL_ERROR_FAILURE;
        }
    }
    ACL_LOG_INFO("acltdtStopChannel success, device is %u, name is %s",
        handle->devId, handle->name.c_str());
    return ACL_SUCCESS;
}


aclError acltdtDestroyChannel(acltdtChannelHandle *handle)
{
    ACL_STAGES_REG(acl::ACL_STAGE_DESTROY, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL(handle);
    ACL_LOG_INFO("start to acltdtDestroyChannel, device is %u, name is %s",
        handle->devId, handle->name.c_str());
    if (!handle->isTdtProcess) {
        ACL_REQUIRES_OK(acltdtDestroyQueue(handle->qid));
        ACL_LOG_INFO("acltdtDestroyChannel success, device is %u, name is %s",
            handle->devId, handle->name.c_str());
        ACL_DELETE_AND_SET_NULL(handle);
        return ACL_SUCCESS;
    }
    std::unique_lock<std::mutex> lk(aclChannleMutex);
    aclChannleMap.erase(handle->name);
    if (aclChannleMap.size() == 0) {
        auto ret = tdt::TdtHostDestroy();
        if (ret != 0) {
            ACL_LOG_INNER_ERROR("[Destroy][Tdt]TdtHostDestroy failed, tdt result = %d", ret);
        }
    }

    ACL_DELETE_AND_SET_NULL(handle);
    return ACL_SUCCESS;
}

static aclError TensorDataitemSerialize(std::vector<aclTdtDataItemInfo> &itemVec,
                                        std::vector<rtMemQueueBuffInfo> &qBufVec)
{
    uint32_t currentCnt = 0;
    for (size_t i = 0; i < itemVec.size(); ++i) {
        itemVec[i].ctrlInfo.curCnt = currentCnt;
        itemVec[i].ctrlInfo.cnt = itemVec.size();
        size_t ctrlSize = sizeof(ItemInfo) + itemVec[i].dims.size() * sizeof(int64_t);
        
        std::shared_ptr<uint8_t> ctrlSharedPtr(new uint8_t[ctrlSize], [] (uint8_t *p) {delete p;});
        ACL_CHECK_MALLOC_RESULT(ctrlSharedPtr);
        void *ctrlPtr = ctrlSharedPtr.get();
        ACL_LOG_INFO("TensorDataitemSerialize ctrlSize is %zu, i is %zu, shape size is %zu",
                     ctrlSize, i, itemVec[i].dims.size());
        auto ret = memcpy_s(ctrlPtr, ctrlSize, &itemVec[i].ctrlInfo, sizeof(ItemInfo));
        if (ret != EN_OK) {
            ACL_LOG_INNER_ERROR("[Call][MemCpy]call memcpy failed, result=%d, srcLen=%zu, dstLen=%zu",
                ret, sizeof(ItemInfo), ctrlSize);
        }
        size_t offset = sizeof(ItemInfo);
        for (size_t j = 0; j < itemVec[i].dims.size(); ++j) {
            ACL_LOG_INFO("before memcpy offset is %zu, remain size is %zu, j is %zu", offset, ctrlSize - offset, j);
            ret = memcpy_s(reinterpret_cast<uint8_t *>(ctrlPtr) + offset,
                           ctrlSize - offset, &itemVec[i].dims[j], sizeof(int64_t));
            if (ret != EN_OK) {
                ACL_LOG_INNER_ERROR("[Call][MemCpy]call memcpy failed, result=%d, srcLen=%zu, dstLen=%zu",
                                    ret, sizeof(int64_t), ctrlSize - offset);
            }
            offset += sizeof(int64_t);
            ACL_LOG_INFO("after memcpy offset is %zu, remain size is %zu, j is %zu", offset, ctrlSize - offset, j);
        }
        rtMemQueueBuffInfo qItem = {ctrlPtr, ctrlSize};
        qBufVec.push_back(qItem);
        if (itemVec[i].ctrlInfo.dataLen != 0) {
            rtMemQueueBuffInfo tmpQItem = {itemVec[i].dataPtr.get(), itemVec[i].ctrlInfo.dataLen};
            qBufVec.push_back(tmpQItem);
        } else {
            ACL_LOG_INFO("no need to insert data buf");
        }
        ++currentCnt;
    }
    return ACL_SUCCESS;
}

aclError acltdtSendTensorV2(const acltdtChannelHandle *handle, const acltdtDataset *dataset, int32_t timeout)
{
    ACL_STAGES_REG(acl::ACL_STAGE_TDT, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute acltdtSendTensor, device is %u, name is %s",
        handle->devId, handle->name.c_str());
    ACL_REQUIRES_NOT_NULL(handle);

    std::vector<aclTdtDataItemInfo> itemVec;
    auto ret = acl::TensorDatasetSerializesV2(dataset, itemVec);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Serialize][Dataset]failed to TensorDatasetSerializesV2, device is %u, name is %s",
            handle->devId, handle->name.c_str());
        itemVec.clear();
        return ret;
    }

    std::vector<rtMemQueueBuffInfo> queueBufInfoVec;
    ret = TensorDataitemSerialize(itemVec, queueBufInfoVec);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Serialize][Dataset]failed to TensorDataitemSerialize, device is %u, name is %s",
            handle->devId, handle->name.c_str());
        return ret;
    }

    rtMemQueueBuff_t queueBuf = {0};
    queueBuf.buffCount = queueBufInfoVec.size();
    queueBuf.buffInfo = queueBufInfoVec.data();
    ret = rtMemQueueEnQueueBuf(handle->devId, handle->qid, &queueBuf, timeout);
    if (ret != RT_ERROR_NONE) {
        ACL_LOG_INNER_ERROR("Faile to execute acltdtSendTensor, device is %u, name is %s",
            handle->devId, handle->name.c_str());
    }
    ACL_LOG_INFO("success to execute acltdtSendTensor, device is %u, name is %s",
        handle->devId, handle->name.c_str());
    return ACL_SUCCESS;
}

aclError acltdtSendTensor(const acltdtChannelHandle *handle, const acltdtDataset *dataset, int32_t timeout)
{
    ACL_STAGES_REG(acl::ACL_STAGE_TDT, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute acltdtSendTensor, device is %u, name is %s",
        handle->devId, handle->name.c_str());
    ACL_REQUIRES_NOT_NULL(handle);

    // -1 represents infinite wait, it is must be -1 now
    if (timeout != -1) {
        ACL_LOG_ERROR("[Check][Timeout]only infinite wait is supported, "
            "it can only be set to -1, timeout[%d].", timeout);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("it can only be set to -1, timeout[%d].", timeout);
        acl::AclErrorLogManager::ReportInputError(acl::UNSUPPORTED_FEATURE_MSG,
            std::vector<std::string>({"feature", "reason"}), std::vector<std::string>({"timeout",
            errMsg}));
        return ACL_ERROR_INVALID_PARAM;
    }

    std::vector<tdt::DataItem> itemVec;
    auto ret = acl::TensorDatasetSerializes(dataset, itemVec);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Serialize][Dataset]failed to TensorDatasetSerializes, device is %u, name is %s",
            handle->devId, handle->name.c_str());
        itemVec.clear();
        return ret;
    }

    int32_t sendRet = tdt::TdtHostPushData(handle->name, itemVec);
    if (sendRet != 0) {
        ACL_LOG_INNER_ERROR("[Push][Data]failed to send, tdt result = %d, device is %u, name is %s",
            sendRet, handle->devId, handle->name.c_str());
        return ACL_ERROR_FAILURE;
    }

    ACL_LOG_INFO("success to execute acltdtSendTensor, device is %u, name is %s",
        handle->devId, handle->name.c_str());
    return ACL_SUCCESS;
}

static aclError UnpackageRecvDataInfo(uint8_t *outputHostAddr, size_t size, std::vector<aclTdtDataItemInfo> &itemVec)
{
    ItemInfo *head = reinterpret_cast<ItemInfo *>(outputHostAddr);
    uint32_t cnt = head->cnt;
    ACL_LOG_INFO("get tensor cnt is %u", cnt);
    size_t offset =0;
    for (uint32_t i = 0; i < cnt; ++i) {
        if (offset + sizeof(ItemInfo) > size) {
            ACL_LOG_ERROR("offset is %zu, size is %zu", offset , size);
            return ACL_ERROR_FAILURE;
        }
        aclTdtDataItemInfo item;
        ItemInfo *tmp = reinterpret_cast<ItemInfo *>(outputHostAddr + offset);
        item.ctrlInfo = *tmp;
        ACL_LOG_INFO("Unpack data, dataType %d, curCnt %u, cnt %u, tensorType %d, dimNum %u, dataLen %lu",
                     tmp->dataType, tmp->curCnt, tmp->cnt, tmp->tensorType, tmp->dimNum, tmp->dataLen);
        offset += sizeof(ItemInfo);

        for (uint32_t j = 0; j < tmp->dimNum; ++j) {
            if (offset + sizeof(int64_t) > size) {
                ACL_LOG_ERROR("offset is %zu, size is %zu", offset , size);
                return ACL_ERROR_FAILURE;
            }
            int64_t dimTmp = *(reinterpret_cast<int64_t *>(outputHostAddr + offset));
            item.dims.push_back(dimTmp);
            ACL_LOG_INFO("current dims[%u] is %ld", j, dimTmp);
            offset += sizeof(int64_t);
        }
        if (offset + tmp->dataLen > size) {
            ACL_LOG_ERROR("offset is %zu, data len is %lu, size is %zu", offset, tmp->dataLen, size);
            return ACL_ERROR_FAILURE;
        }
        if (tmp->dataLen > 0) {
            std::shared_ptr<uint8_t> data(new uint8_t[tmp->dataLen], [] (uint8_t *p) {delete p;});
            auto ret = memcpy_s(data.get(), tmp->dataLen, outputHostAddr + offset, tmp->dataLen);
            if (ret != EN_OK) {
                ACL_LOG_INNER_ERROR("[Call][MemCpy]call memcpy failed, result=%d, srcLen=%lu, dstLen=%lu",
                    ret, tmp->dataLen, tmp->dataLen);
                return ACL_ERROR_FAILURE;
            }
            offset += tmp->dataLen;
            item.dataPtr = data;
        } else {
            ACL_LOG_INFO("data length is 0");
        }
        ACL_LOG_INFO("after %u tensor, offset is %zu", i + 1, offset);
        itemVec.push_back(item);
    }
    return ACL_SUCCESS;
}

aclError acltdtReceiveTensorV2(const acltdtChannelHandle *handle, acltdtDataset *dataset, int32_t timeout)
{
    ACL_STAGES_REG(acl::ACL_STAGE_TDT, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute acltdtReceiveTensor, device is %u, name is %s",
        handle->devId, handle->name.c_str());
    ACL_REQUIRES_NOT_NULL(handle);

    if (handle->recvName.empty()) {
        ACL_LOG_ERROR("[Check][Recvname]it is not a receive channel, failed to receive, device is %u, name is %s",
            handle->devId, handle->name.c_str());
        std::string errMsg = acl::AclErrorLogManager::FormatStr("failed to receive, device is %u, name is %s",
            handle->devId, handle->name.c_str());
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"receive channel", "", errMsg}));
        return ACL_ERROR_INVALID_PARAM;
    }

    size_t bufLen = 0;
    auto ret = rtMemQueuePeek(handle->devId, handle->qid, &bufLen, timeout);
    if (ret != RT_ERROR_NONE) {
        ACL_LOG_ERROR("peek queue [%u] failed", handle->qid);
        return ret;
    }
    ACL_LOG_INFO("peek queue [%u] success, bufLen is %zu", handle->qid, bufLen);
    std::shared_ptr<uint8_t> outHostSharedAddr(new uint8_t[bufLen], [] (uint8_t *p) {delete p;});
    ACL_CHECK_MALLOC_RESULT(outHostSharedAddr);
    uint8_t *outHostAddr = outHostSharedAddr.get();

    rtMemQueueBuff_t queueBuf = {0};
    rtMemQueueBuffInfo queueBufInfo = {outHostAddr, bufLen};
    queueBuf.buffCount = 1;
    queueBuf.buffInfo = &queueBufInfo;
    ret = rtMemQueueDeQueueBuf(handle->devId, handle->qid, &queueBuf, 0);
    if (ret != RT_ERROR_NONE) {
        ACL_LOG_ERROR("failed to rtMemQueueDeQueueBuf, device is %u, name is %s", handle->devId, handle->name.c_str());
        return ret;
    }

    std::vector<aclTdtDataItemInfo> itemVec;
    ret = UnpackageRecvDataInfo(outHostAddr, bufLen, itemVec);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_ERROR("failed to UnpackageRecvDataInfo, device is %u, name is %s",
                      handle->devId, handle->name.c_str());
        return ret;
    }
    ACL_LOG_INFO("success to execute acltdtReceiveTensor, device is %u, name is %s",
        handle->devId, handle->name.c_str());
    return ACL_SUCCESS;
}

aclError acltdtReceiveTensor(const acltdtChannelHandle *handle, acltdtDataset *dataset, int32_t timeout)
{
    ACL_STAGES_REG(acl::ACL_STAGE_TDT, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute acltdtReceiveTensor, device is %u, name is %s",
        handle->devId, handle->name.c_str());
    ACL_REQUIRES_NOT_NULL(handle);

    // -1 represents infinite wait, it is must be -1 now
    if (timeout != -1) {
        ACL_LOG_ERROR("[Check][Timeout]only infinite wait is supported, "
            "it can only be set to -1, timeout[%d]", timeout);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("it can only be set to -1, timeout[%d].", timeout);
        acl::AclErrorLogManager::ReportInputError(acl::UNSUPPORTED_FEATURE_MSG,
            std::vector<std::string>({"feature", "reason"}), std::vector<std::string>({"timeout", errMsg}));
        return ACL_ERROR_INVALID_PARAM;
    }

    if (handle->recvName.empty()) {
        ACL_LOG_ERROR("[Check][Recvname]it is not a receive channel, failed to receive, device is %u, name is %s",
            handle->devId, handle->name.c_str());
        std::string errMsg = acl::AclErrorLogManager::FormatStr("failed to receive, device is %u, name is %s",
            handle->devId, handle->name.c_str());
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"receive channel", "", errMsg}));
        return ACL_ERROR_INVALID_PARAM;
    }

    std::vector<tdt::DataItem> itemVec;
    int32_t recvRet = tdt::TdtHostPopData(handle->recvName, itemVec);
    if (recvRet != 0) {
        ACL_LOG_INNER_ERROR("[Pop][Data]failed to receive, tdt result = %d, device is %u, name is %s",
            recvRet, handle->devId, handle->name.c_str());
        return ACL_ERROR_FAILURE;
    }

    auto ret = acl::TensorDatasetDeserializes(itemVec, dataset);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Deserialize][Dataset]failed to TensorDatasetDeserializes, device is %u, name is %s",
            handle->devId, handle->name.c_str());
        return ret;
    }

    ACL_LOG_INFO("success to execute acltdtReceiveTensor, device is %u, name is %s",
        handle->devId, handle->name.c_str());
    return ACL_SUCCESS;
}


