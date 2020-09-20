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
            ACL_LOG_INNER_ERROR("Invalid shape string: %s", dimsStr.c_str());
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
                ACL_LOG_INNER_ERROR("Invalid shape string: %s", dimsStr.c_str());
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
                ACL_LOG_INNER_ERROR("Invalid shape string: %s", dimsStr.c_str());
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
                ACL_LOG_INNER_ERROR("unkown acltdtTensorType %d.", aclType);
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
                ACL_LOG_INNER_ERROR("unkown TdtDataType %d.", tdtDataType);
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
                ACL_LOG_INNER_ERROR("TensorDatasetSerializes failed, invalid tdt type %d", dataset->blobs[i]->tdtType);
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

    aclError TensorDatasetDeserializes(const std::vector<tdt::DataItem> &itemVec, acltdtDataset *dataset)
    {
        ACL_REQUIRES_NOT_NULL(dataset);
        if (dataset->blobs.size() != 0) {
            ACL_LOG_INNER_ERROR("Dataset size[%zu] is not empty", dataset->blobs.size());
            return ACL_ERROR_INVALID_PARAM;
        }
        aclError ret = ACL_SUCCESS;
        for (size_t i = 0; i < itemVec.size(); ++i) {
            acltdtTensorType aclType;
            ret = GetAclTypeByTdtDataType(itemVec[i].dataType_, aclType);
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("TensorDatasetDeserializes failed, invalid data type %d",
                    itemVec[i].dataType_);
                break;
            }

            if (aclType == ACL_TENSOR_DATA_TENSOR) {
                std::vector<int64_t> dims;
                if (!GetTensorShape(itemVec[i].tensorShape_, dims)) {
                    ACL_LOG_INNER_ERROR("TensorDatasetDeserializes failed, invalid tensor shape[%s]",
                        itemVec[i].tensorShape_.c_str());
                    ret = ACL_ERROR_INTERNAL_ERROR;
                    break;
                }

                auto iter = aclDataTypeStrMap.find(itemVec[i].tensorType_);
                if (iter == aclDataTypeStrMap.end()) {
                    ACL_LOG_INNER_ERROR("TensorDatasetDeserializes failed, unkown data type[%s]",
                        itemVec[i].tensorType_.c_str());
                    ret = ACL_ERROR_INTERNAL_ERROR;
                    break;
                }
                aclDataType dataType = iter->second;
                acltdtDataItem *item = new(std::nothrow) acltdtDataItem(aclType,
                    &dims[0], dims.size(), itemVec[i].tensorShape_,
                    dataType, itemVec[i].tensorType_,
                    itemVec[i].dataPtr_, itemVec[i].dataLen_);
                if (item == nullptr) {
                    ACL_LOG_INNER_ERROR("TensorDatasetDeserializes alloc failed");
                    ret = ACL_ERROR_BAD_ALLOC;
                    break;
                }
                dataset->blobs.push_back(item);
            } else {
                acltdtDataItem *item = new(std::nothrow) acltdtDataItem(aclType,
                    nullptr, 0, itemVec[i].tensorShape_, ACL_DT_UNDEFINED,
                    itemVec[i].tensorType_, itemVec[i].dataPtr_, itemVec[i].dataLen_);
                if (item == nullptr) {
                    ACL_LOG_INNER_ERROR("TensorDatasetDeserializes alloc failed");
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
        ACL_LOG_INNER_ERROR("param [dataItem] must not be null.");
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
        ACL_LOG_INNER_ERROR("param [dataItem] must not be null.");
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
        ACL_LOG_INNER_ERROR("param [dataItem] must not be null.");
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
        ACL_LOG_INNER_ERROR("param [dataItem] must not be null.");
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
        ACL_LOG_INNER_ERROR("acltdtGetDimsFromItem failed, invalid dims and dimNum[%zu]", dimNum);
        return ACL_ERROR_INVALID_PARAM;
    }

    if (dimNum < dataItem->dims.size()) {
        ACL_LOG_INNER_ERROR("output dimNum[%zu] cannot be less than dims number[%zu]", dimNum, dataItem->dims.size());
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
        ACL_LOG_INNER_ERROR("acltdtCreateDataItem failed, invalid dims and dimNum[%zu]", dimNum);
        return nullptr;
    }
    if (dimNum > ACL_MAX_DIM_CNT) {
        ACL_LOG_INNER_ERROR("acltdtCreateDataItem failed, dimNum[%zu] can't be larger than ACL_MAX_DIM_CNT[%d]",
            dimNum, ACL_MAX_DIM_CNT);
        return nullptr;
    }

    if (tdtType != ACL_TENSOR_DATA_TENSOR) {
        if (dims != nullptr) {
            ACL_LOG_INNER_ERROR("acltdtCreateDataItem failed, dims must be nullptr. tdtType is %d", tdtType);
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
        ACL_LOG_INNER_ERROR("acltdtCreateDataItem failed, unspported datatype: %d", dataType);
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
        ACL_LOG_INNER_ERROR("item cannot be added, because internal item already exists");
        return ACL_ERROR_FEATURE_UNSUPPORTED;
    }
    dataset->blobs.push_back(dataItem);
    return ACL_SUCCESS;
}

acltdtDataItem *acltdtGetDataItem(const acltdtDataset *dataset, size_t index)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if ((dataset == nullptr) || (index >= dataset->blobs.size())) {
        ACL_LOG_INNER_ERROR("input param is invalid, index[%zu]", index);
        return nullptr;
    }

    return dataset->blobs[index];
}

size_t acltdtGetDatasetSize(const acltdtDataset *dataset)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (dataset == nullptr) {
        ACL_LOG_INNER_ERROR("dataset is null.");
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
        ACL_LOG_INNER_ERROR("tdt host init failed, tdt result = %d", ret);
        return nullptr;
    }
    acltdtChannelHandle *handle = new(std::nothrow) acltdtChannelHandle(deviceId, name);
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

aclError acltdtStopChannel(acltdtChannelHandle *handle)
{
    ACL_STAGES_REG(acl::ACL_STAGE_TDT, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL(handle);
    ACL_LOG_INFO("start to acltdtStopChannel, device is %u, name is %s",
        handle->devId, handle->name.c_str());
    if (!handle->recvName.empty()) {
        auto ret = tdt::TdtHostStop(handle->recvName);
        if (ret != 0) {
            ACL_LOG_INNER_ERROR("tdt host stop failed for channel %s, tdt result = %d",
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

    std::unique_lock<std::mutex> lk(aclChannleMutex);
    aclChannleMap.erase(handle->name);
    if (aclChannleMap.size() == 0) {
        auto ret = tdt::TdtHostDestroy();
        if (ret != 0) {
            ACL_LOG_INNER_ERROR("TdtHostDestroy failed, tdt result = %d", ret);
        }
    }

    ACL_DELETE_AND_SET_NULL(handle);
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
        ACL_LOG_INNER_ERROR("only infinite wait is supported, it can only be set to -1, timeout[%d].", timeout);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("it can only be set to -1, timeout[%d].", timeout);
        acl::AclErrorLogManager::ReportInputError(acl::UNSUPPORTED_FEATURE_MSG,
            std::vector<std::string>({"feature", "reason"}), std::vector<std::string>({"timeout",
            errMsg}));
        return ACL_ERROR_INVALID_PARAM;
    }

    std::vector<tdt::DataItem> itemVec;
    auto ret = acl::TensorDatasetSerializes(dataset, itemVec);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("failed to TensorDatasetSerializes, device is %u, name is %s",
            handle->devId, handle->name.c_str());
        itemVec.clear();
        return ret;
    }

    int32_t sendRet = tdt::TdtHostPushData(handle->name, itemVec);
    if (sendRet != 0) {
        ACL_LOG_INNER_ERROR("failed to send, tdt result = %d, device is %u, name is %s",
            sendRet, handle->devId, handle->name.c_str());
        return ACL_ERROR_FAILURE;
    }

    ACL_LOG_INFO("success to execute acltdtSendTensor, device is %u, name is %s",
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
        ACL_LOG_INNER_ERROR("only infinite wait is supported, it can only be set to -1, timeout[%d]", timeout);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("it can only be set to -1, timeout[%d].", timeout);
        acl::AclErrorLogManager::ReportInputError(acl::UNSUPPORTED_FEATURE_MSG,
            std::vector<std::string>({"feature", "reason"}), std::vector<std::string>({"timeout", errMsg}));
        return ACL_ERROR_INVALID_PARAM;
    }

    if (handle->recvName.empty()) {
        ACL_LOG_INNER_ERROR("it is not a receive channel, failed to receive, device is %u, name is %s",
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
        ACL_LOG_INNER_ERROR("failed to receive, tdt result = %d, device is %u, name is %s",
            recvRet, handle->devId, handle->name.c_str());
        return ACL_ERROR_FAILURE;
    }

    auto ret = acl::TensorDatasetDeserializes(itemVec, dataset);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("failed to TensorDatasetDeserializes, device is %u, name is %s",
            handle->devId, handle->name.c_str());
        return ret;
    }

    ACL_LOG_INFO("success to execute acltdtReceiveTensor, device is %u, name is %s",
        handle->devId, handle->name.c_str());
    return ACL_SUCCESS;
}


