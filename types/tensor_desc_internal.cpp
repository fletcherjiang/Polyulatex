/**
* @file tensor_desc_internal.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "tensor_desc_internal.h"
#include <sstream>
#include "utils/string_utils.h"
#include "utils/math_utils.h"
#include "common/log_inner.h"
#include "toolchain/resource_statistics.h"

using namespace std;

namespace {
    const int DEFAULT_BUFFER_SIZE = 32;
}

aclTensorDesc::aclTensorDesc(aclDataType dataType, std::initializer_list<int64_t> shape, aclFormat format): dims(shape)
{
    this->dataType = dataType;
    this->format = format;
}

aclTensorDesc::aclTensorDesc(aclDataType dataType, size_t numDims, const int64_t *dims, aclFormat format)
{
    this->dataType = dataType;
    this->format = format;
    for (size_t i = 0; i < numDims; ++i) {
        this->dims.push_back(dims[i]);
    }
}

void aclTensorDesc::Init(const aclTensorDesc &tensorDesc)
{
    this->dataType = tensorDesc.dataType;
    this->storageFormat = tensorDesc.storageFormat;
    this->format = tensorDesc.format;
    this->dims = tensorDesc.dims;
    this->storageDims = tensorDesc.storageDims;
    this->name = tensorDesc.name;
    this->shapeRange = tensorDesc.shapeRange;
    this->address = tensorDesc.address;
    this->isConst = tensorDesc.isConst;
    this->constDataLen = tensorDesc.constDataLen;
    this->constDataBuf = tensorDesc.constDataBuf;
    this->cachedKey = tensorDesc.cachedKey;
    this->cachedShapeKey = tensorDesc.cachedShapeKey;
    this->memtype = tensorDesc.memtype;
    for (auto it = tensorDesc.valueRange.begin(); it != tensorDesc.valueRange.end(); ++it) {
        this->valueRange[it->first] = it->second.Copy();
    }
}

aclTensorDesc::aclTensorDesc(const aclTensorDesc &tensorDesc)
{
    Init(tensorDesc);
}

const string &aclTensorDesc::GetKey() const
{
    if (!cachedKey.empty()) {
        ACL_LOG_INFO("cachedKey is not empty %s", cachedKey.c_str());
        cachedKey.clear();
    }
    ACL_LOG_INFO("begin to build cachedKey");
    cachedKey.reserve(DEFAULT_BUFFER_SIZE);
    cachedKey += std::to_string(dataType);
    cachedKey.push_back('_');
    cachedKey += std::to_string(format);
    cachedKey.push_back('_');

    if (storageFormat != ACL_FORMAT_UNDEFINED) {
        cachedKey += std::to_string(storageFormat);
        cachedKey.push_back('_');
    }
    for (auto dim : dims) {
        cachedKey += std::to_string(dim);
        cachedKey.push_back('_');
    }
    if (isConst) {
        cachedKey += "true";
    } else {
        cachedKey += "false";
    }
    cachedKey.push_back('_');
    cachedKey += std::to_string(memtype);
    ACL_LOG_INFO("cachedKey is %s", cachedKey.c_str());
    return cachedKey;
}

const string &aclTensorDesc::GetShapeKey() const
{
    if (!cachedShapeKey.empty()) {
        ACL_LOG_DEBUG("cachedShapeKey is not empty %s", cachedShapeKey.c_str());
        return cachedShapeKey;
    }
    ACL_LOG_INFO("begin to build cachedShapeKey");
    for (auto range : shapeRange) {
        cachedShapeKey += std::to_string(range.first);
        cachedShapeKey.push_back('_');
        cachedShapeKey += std::to_string(range.second);
        cachedShapeKey.push_back('_');
    }
    return cachedShapeKey;
}

static std::string DebugConstData(bool isConst, const void *constDataBuf, size_t constDataLen)
{
    stringstream ss;
    if (isConst) {
        ss << " , isConst = true, Const Len = "<< (constDataLen / sizeof(int)) << " ,Const data = ";
        for (size_t i = 0; i < (constDataLen / sizeof(int32_t)); ++i) {
            ss <<  *((int32_t *)constDataBuf + i);
            ss << ",";
        }
    }
    return ss.str();
}

std::string aclTensorDesc::DebugString() const
{
    stringstream ss;
    ss << "[TensorDesc] ";
    ss << "DataType = " << dataType;
    ss << ", Format = " << format;
    ss << ", StorageFormat = " << storageFormat;
    ss << ", Shape = " << acl::string_utils::VectorToString(dims);
    ss << ", StorageShape = " << acl::string_utils::VectorToString(storageDims);
    ss << ", shapeRange = " << acl::string_utils::VectorToString(shapeRange);
    ss << ", memtype = " << memtype;
    ss << ", isConst = " << isConst;
    ss << DebugConstData(isConst, constDataBuf.get(), constDataLen);
    return ss.str();
}

bool aclTensorDesc::IsDynamicTensor() const
{
    for (size_t i = 0; i < dims.size(); ++i) {
        if ((dims[i] == UNKNOW_DIM) || (dims[i] == UNKNOW_RANK)) {
            return true;
        }
    }
    if (!valueRange.empty()) {
        return true;
    }
    return false;
}

bool aclTensorDesc::CheckConstTensor(bool needCheckHostMem) const
{
    if (isConst) {
        return true;
    } else if (needCheckHostMem && (memtype == ACL_MEMTYPE_HOST)) {
        return true;
    } else {
        return false;
    }
}

bool aclTensorDesc::IsOptinalTensor() const
{
    if ((dataType == ACL_DT_UNDEFINED) && (format == ACL_FORMAT_UNDEFINED) && (dims.empty())) {
        return true;
    }
    return false;
}

void aclTensorDesc::UpdateTensorShape(const std::vector<int64_t> &shape)
{
    dims.clear();
    for (size_t i = 0; i < shape.size(); ++i) {
        dims.emplace_back(shape[i]);
    }
}

void aclTensorDesc::UpdateTensorShapeRange(const std::vector<std::pair<int64_t, int64_t>> &ranges)
{
    shapeRange.clear();
    for (size_t i = 0; i < ranges.size(); ++i) {
        shapeRange.emplace_back(ranges[i]);
    }
}

bool aclTensorDesc::CheckShapeRange() const
{
    if ((dims.size() > 0) && (dims[0] == UNKNOW_RANK)) {
        return shapeRange.empty();
    }
    bool isUnkownDim = false;
    for (size_t i = 0; i < dims.size(); ++i) {
        if (dims[i] == UNKNOW_DIM) {
            isUnkownDim = true;
            break;
        }
    }
    if (isUnkownDim) {
        if (dims.size() != shapeRange.size()) {
            return false;
        }
    }
    return true;
}

bool aclTensorDesc::operator==(const aclTensorDesc* other)
{
    ACL_LOG_DEBUG("Check aclTensorDesc is equal start!");
    ACL_REQUIRES_NOT_NULL(other);

    ACL_LOG_DEBUG("Check dataType is equal");
    ACL_CHECK_EQUAL(this->dataType, other->dataType);

    ACL_LOG_DEBUG("Check format is equal");
    ACL_CHECK_EQUAL(this->format, other->format);

    ACL_LOG_DEBUG("Check storageFormat is equal");
    ACL_CHECK_EQUAL(this->storageFormat, other->storageFormat);

    if (this->dims != other->dims) {
        ACL_LOG_INFO("leftDim [%s] is not equal to otherDim [%s]",
            acl::string_utils::VectorToString(this->dims).c_str(),
            acl::string_utils::VectorToString(other->dims).c_str());
        return false;
    }
    if (this->shapeRange != other->shapeRange) {
        ACL_LOG_INFO("thisShapeRange [%s] is not equal to otherShapeRange [%s]",
            acl::string_utils::VectorToString(this->shapeRange).c_str(),
            acl::string_utils::VectorToString(other->shapeRange).c_str());
        return false;
    }

    ACL_LOG_DEBUG("Check isConst is equal");
    ACL_CHECK_EQUAL(this->isConst, other->isConst);

    ACL_LOG_DEBUG("Check memtype is equal");
    ACL_CHECK_EQUAL(this->memtype, other->memtype);

    ACL_LOG_INFO("aclTensorDesc is equal!");
    return true;
}

size_t aclDataTypeSize(aclDataType dataType)
{
    switch (dataType) {
        case ACL_STRING:
        case ACL_DT_UNDEFINED:
            return 0;
        case ACL_BOOL:
        case ACL_INT8:
        case ACL_UINT8:
            return sizeof(int8_t);
        case ACL_FLOAT16:
        case ACL_INT16:
        case ACL_UINT16:
            return sizeof(int16_t);
        case ACL_FLOAT:
        case ACL_INT32:
        case ACL_UINT32:
            return sizeof(int32_t);
        case ACL_COMPLEX128:
            return 2 * sizeof(int64_t);
        case ACL_INT64:
        case ACL_UINT64:
        case ACL_DOUBLE:
        case ACL_COMPLEX64:
        default:
            return sizeof(int64_t);
    }
}

aclTensorDesc *aclCreateTensorDesc(aclDataType dataType,
                                   int numDims,
                                   const int64_t *dims,
                                   aclFormat format)
{
    ACL_STAGES_REG(acl::ACL_STAGE_CREATE, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_TENSOR_DESC);
    if (numDims < 0) {
        ACL_LOG_ERROR("[Check][NumDims]numDims[%d] is smaller than 0", numDims);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"numDims", std::to_string(numDims),
            "can't smaller than 0"}));
        return nullptr;
    }
    if ((numDims > 0) && (dims == nullptr)) {
        ACL_LOG_ERROR("[Check][Dims]dims is null while numDims[%d] > 0", numDims);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"numDims", std::to_string(numDims),
            "dims is null while numDims > 0"}));
        return nullptr;
    }

    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_TENSOR_DESC);
    // 1 respresents that creates one tensor
    return new(std::nothrow) aclTensorDesc[1]{{dataType, static_cast<size_t>(numDims), dims, format}};
}

void aclDestroyTensorDesc(const aclTensorDesc *desc)
{
    ACL_STAGES_REG(acl::ACL_STAGE_DESTROY, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_TENSOR_DESC);
    ACL_DELETE_ARRAY_AND_SET_NULL(desc);
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_TENSOR_DESC);
}

aclError aclSetTensorShapeRange(aclTensorDesc* desc, size_t dimsCout, int64_t dimsRange[][ACL_TENSOR_SHAPE_RANGE_NUM])
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(desc);
    // dimsCout should be equal to length of array dimsRange
    desc->shapeRange.clear();
    for (size_t i = 0; i < dimsCout; ++i) {
        desc->shapeRange.emplace_back(make_pair(dimsRange[i][0], dimsRange[i][1]));
    }
    return ACL_SUCCESS;
}

aclDataType aclGetTensorDescType(const aclTensorDesc *desc)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (desc == nullptr) {
        ACL_LOG_ERROR("[Check][Desc]desc is null");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}), std::vector<std::string>({"desc"}));
        return ACL_DT_UNDEFINED;
    }

    return desc->dataType;
}

aclFormat aclGetTensorDescFormat(const aclTensorDesc *desc)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (desc == nullptr) {
        ACL_LOG_ERROR("[Check][Desc]desc is null");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}), std::vector<std::string>({"desc"}));
        return ACL_FORMAT_UNDEFINED;
    }

    return desc->format;
}

size_t aclGetTensorDescSize(const aclTensorDesc *desc)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (desc == nullptr) {
        ACL_LOG_ERROR("[Check][Desc]desc is null");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}), std::vector<std::string>({"desc"}));
        return 0;
    }
    size_t size = 0;
    size_t count = aclGetTensorDescElementCount(desc);
    size_t typeSize = aclDataTypeSize(desc->dataType);
    (void)acl::CheckSizeTMultiOverflow(count, typeSize, size);
    return size;
}

size_t aclGetTensorDescElementCount(const aclTensorDesc *desc)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (desc == nullptr) {
        ACL_LOG_ERROR("[Check][Desc]desc is null");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}), std::vector<std::string>({"desc"}));
        return 0;
    }

    if (desc->dims.empty()) {
        return 1;
    }

    size_t count = 1;
    for (int64_t dim : desc->dims) {
        if (dim < 0) { // dim cannot be less than 0
            ACL_LOG_INNER_ERROR("[Check][Dim]invalid dim value %ld", dim);
            return 0;
        }
        aclError ret = acl::CheckSizeTMultiOverflow(count, static_cast<size_t>(dim), count);
        if (ret != ACL_SUCCESS) {
            return 0;
        }
    }

    return count;
}

size_t aclGetTensorDescNumDims(const aclTensorDesc *desc)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (desc == nullptr) {
        ACL_LOG_ERROR("[Check][Desc]desc is null");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}), std::vector<std::string>({"desc"}));
        return 0;
    }
    if ((desc->dims.size() > 0) && (desc->dims[0] == UNKNOW_RANK)) {
        return ACL_UNKNOWN_RANK;
    }
    return desc->dims.size();
}

aclError aclGetTensorDescDimRange(const aclTensorDesc* desc,
                                  size_t index,
                                  size_t dimRangeNum,
                                  int64_t *dimRange)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(desc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dimRange);
    if (index >= desc->shapeRange.size()) {
        ACL_LOG_ERROR("[Check][Index]index out of range. index = %zu, numDims = %zu",
            index, desc->shapeRange.size());
        std::string errMsg = acl::AclErrorLogManager::FormatStr("index out of range. numDims = %zu",
            desc->shapeRange.size());
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"index", std::to_string(index), errMsg}));
        return ACL_ERROR_INVALID_PARAM;
    }
    if (dimRangeNum < ACL_TENSOR_SHAPE_RANGE_NUM) {
        ACL_LOG_ERROR("[Check][DimRangeNum]dimRangeNum cannot be less than 2. dimRangeNum = %zu",
            dimRangeNum);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"dimRangeNum", std::to_string(dimRangeNum),
            "cannot be less than 2"}));
        return ACL_ERROR_INVALID_PARAM;
    }
    dimRange[0] = desc->shapeRange[index].first;
    dimRange[1] = desc->shapeRange[index].second;

    return ACL_SUCCESS;
}

int64_t aclGetTensorDescDim(const aclTensorDesc *desc, size_t index)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (desc == nullptr) {
        ACL_LOG_ERROR("[Check][Desc]desc is null");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}), std::vector<std::string>({"desc"}));
        return -1;
    }

    if (index >= desc->dims.size()) {
        ACL_LOG_ERROR("[Check][Index]index out of range. index = %zu, numDims = %zu",
            index, desc->dims.size());
        std::string errMsg = acl::AclErrorLogManager::FormatStr("index out of range. "
            "numDims = %zu", desc->dims.size());
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"index", std::to_string(index), errMsg}));
        return -1;
    }

    return desc->dims[index];
}

aclError aclGetTensorDescDimV2(const aclTensorDesc *desc, size_t index, int64_t *dimSize)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (desc == nullptr) {
        ACL_LOG_ERROR("[Check][Desc]desc is null");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"desc"}));
        return ACL_ERROR_INVALID_PARAM;
    }
    if (index >= desc->dims.size()) {
        ACL_LOG_ERROR("[Check][Index]index out of range. index = %zu, numDims = %zu",
            index, desc->dims.size());
        std::string errMsg = acl::AclErrorLogManager::FormatStr("index out of range. numDims = %zu",
            desc->dims.size());
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"index", std::to_string(index), errMsg}));
        return ACL_ERROR_INVALID_PARAM;
    }
    *dimSize = desc->dims[index];

    return ACL_SUCCESS;
}

aclDataBuffer *aclCreateDataBuffer(void *data, size_t size)
{
    ACL_STAGES_REG(acl::ACL_STAGE_CREATE, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DATA_BUFFER);
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DATA_BUFFER);
    return new(std::nothrow) aclDataBuffer(data, size);
}

aclError aclDestroyDataBuffer(const aclDataBuffer *dataBuffer)
{
    ACL_STAGES_REG(acl::ACL_STAGE_DESTROY, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DATA_BUFFER);
    if (dataBuffer == nullptr) {
        return ACL_ERROR_INVALID_PARAM;
    }

    ACL_DELETE_AND_SET_NULL(dataBuffer);
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DATA_BUFFER);
    return ACL_SUCCESS;
}

aclError aclUpdateDataBuffer(aclDataBuffer *dataBuffer, void *data, size_t size)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    if (dataBuffer == nullptr) {
        ACL_LOG_ERROR("[Check][DataBuffer]invalid input pointer of dataBuffer, please use aclCreateDataBuffer "
            "interface to create.");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"dataBuffer"}));
        return ACL_ERROR_INVALID_PARAM;
    }
    dataBuffer->data = data;
    dataBuffer->length = size;
    return ACL_SUCCESS;
}

void *aclGetDataBufferAddr(const aclDataBuffer *dataBuffer)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (dataBuffer == nullptr) {
        return nullptr;
    }

    return dataBuffer->data;
}

uint32_t aclGetDataBufferSize(const aclDataBuffer *dataBuffer)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (dataBuffer == nullptr) {
        return 0;
    }

    return dataBuffer->length;
}

size_t aclGetDataBufferSizeV2(const aclDataBuffer *dataBuffer)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (dataBuffer == nullptr) {
        return 0;
    }

    return dataBuffer->length;
}

void aclSetTensorDescName(aclTensorDesc *desc, const char *name)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    if (desc == nullptr) {
        ACL_LOG_ERROR("[Check][Desc]desc is null");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}), std::vector<std::string>({"desc"}));
        return;
    }

    if (name == nullptr) {
        desc->name = "";
        return;
    }

    desc->name = std::string(name);
}

const char *aclGetTensorDescName(aclTensorDesc *desc)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (desc == nullptr) {
        return "";
    }

    return desc->name.c_str();
}

aclError aclSetTensorStorageFormat(aclTensorDesc *desc, aclFormat format)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    if (desc != nullptr) {
        desc->storageFormat = format;
    }

    return ACL_SUCCESS;
}

aclError aclSetTensorStorageShape(aclTensorDesc *desc, int numDims, const int64_t *dims)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(desc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dims);

    desc->storageDims.clear();
    for (int i = 0; i < numDims; ++i) {
        desc->storageDims.push_back(dims[i]);
    }

    return ACL_SUCCESS;
}

aclError aclSetTensorFormat(aclTensorDesc *desc, aclFormat format)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    if (desc != nullptr) {
        desc->storageFormat = format;
    }

    return ACL_SUCCESS;
}

aclError aclSetTensorShape(aclTensorDesc *desc, int numDims, const int64_t *dims)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(desc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dims);

    desc->storageDims.clear();
    for (int i = 0; i < numDims; ++i) {
        desc->storageDims.push_back(dims[i]);
    }

    return ACL_SUCCESS;
}

aclError aclSetTensorOriginFormat(aclTensorDesc *desc, aclFormat format)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    if (desc != nullptr) {
        desc->format = format;
    }

    return ACL_SUCCESS;
}

aclError aclSetTensorOriginShape(aclTensorDesc *desc, int numDims, const int64_t *dims)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(desc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dims);

    desc->dims.clear();
    for (int i = 0; i < numDims; ++i) {
        desc->dims.push_back(dims[i]);
    }

    return ACL_SUCCESS;
}

aclTensorDesc *aclGetTensorDescByIndex(aclTensorDesc *desc, size_t index)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_RET_NULL_INPUT_REPORT(desc);

    return (desc + index);
}

void *aclGetTensorDescAddress(const aclTensorDesc *desc)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_RET_NULL_INPUT_REPORT(desc);
    return desc->address;
}

aclError aclSetTensorDynamicInput(aclTensorDesc *desc, const char *dynamicInputName)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(desc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dynamicInputName);

    desc->dynamicInputName = std::string(dynamicInputName);
    return ACL_SUCCESS;
}

aclError aclSetTensorConst(aclTensorDesc *desc, void *dataBuffer, size_t length)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclSetTensorConst");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(desc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dataBuffer);
    if (length <= 0) {
        ACL_LOG_ERROR("[Check][Length]The length of const dataBuffer is invalid. size = %zu", length);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"desc"}));
        return ACL_ERROR_INVALID_PARAM;
    }
    desc->isConst = true;

    auto *constData = new (std::nothrow) char[length];
    ACL_REQUIRES_NOT_NULL(constData);
    if (memcpy_s(constData, length, dataBuffer, length) != EOK) {
        ACL_LOG_INNER_ERROR("[Copy][Data]Copy const data failed. size = %zu", length);
        ACL_DELETE_ARRAY_AND_SET_NULL(constData);
        return ACL_ERROR_FAILURE;
    }

    desc->constDataBuf.reset(constData, [](const char *p)
        { delete[]p; ACL_LOG_DEBUG("delete const data in aclSetTensorConst"); });
    desc->constDataLen = length;
    return ACL_SUCCESS;
}

aclError aclSetTensorPlaceMent(aclTensorDesc *desc, aclMemType memType)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclSetTensorPlaceMent, memType is %d", static_cast<int32_t>(memType));
    ACL_REQUIRES_NOT_NULL(desc);
    desc->memtype = memType;
    return ACL_SUCCESS;
}

