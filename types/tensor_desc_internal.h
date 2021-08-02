/**
* @file tensor_desc_internal.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_TYPES_TENSOR_DESC_INTERNAL_H
#define ACL_TYPES_TENSOR_DESC_INTERNAL_H

#include <vector>
#include <string>
#include <memory>

#include "graph/ge_attr_value.h"
#include "acl/acl_base.h"

namespace acl{
    constexpr int64_t UNKNOW_DIM = -1;
    constexpr int64_t UNKNOW_RANK = -2;
    enum AttrRangeType {
        RANGE_TYPE,
        VALUE_TYPE
    };
}

struct aclDataBuffer {
    aclDataBuffer(void* const dataIn, uint64_t len) : data(dataIn), length(len)
    {
    }

    ~aclDataBuffer() = default;
    void *data;
    uint64_t length;
};

struct ACL_FUNC_VISIBILITY aclTensorDesc {
    aclTensorDesc(aclDataType dataType, std::initializer_list<int64_t> shape, aclFormat format);
    aclTensorDesc(aclDataType dataType, size_t numDims, const int64_t* const dims, aclFormat format);
    aclTensorDesc(const aclTensorDesc &tensorDesc);
    aclTensorDesc() = default;
    ~aclTensorDesc() = default;
    aclDataType dataType;
    aclFormat storageFormat = ACL_FORMAT_UNDEFINED;
    aclFormat format;
    std::vector<int64_t> dims;
    std::vector<int64_t> dimsBack;    
    std::vector<int64_t> storageDims;
    std::vector<int64_t> storageDimsBack;
    std::string name;
    std::vector<std::pair<int64_t, int64_t>> shapeRange;
    std::vector<std::pair<int64_t, int64_t>> shapeRangeBack;
    void *address = nullptr;
    std::string dynamicInputName;
    bool isConst = false;
    std::shared_ptr<void> constDataBuf;
    size_t constDataLen = 0U;
    bool isConstBack = false;
    std::shared_ptr<void> constDataBufBack;
    size_t constDataLenBack = 0U;
    aclMemType memtype = ACL_MEMTYPE_DEVICE;
    // for windows compile,use map ignore dvpp.so find the implementation GeAttrValue
    std::map<acl::AttrRangeType, ge::GeAttrValue> valueRange;
    const std::string& GetKey() const;
    const std::string &GetShapeKey() const;
    std::string DebugString() const;
    bool IsDynamicTensor() const;
    bool CheckShapeRange() const;
    bool IsConstTensor() const
    {
        return isConst;
    }
    bool IsHostMemTensor() const
    {
        return memtype == ACL_MEMTYPE_HOST;
    }
    bool IsOptinalTensor() const;
    void Init(const aclTensorDesc &tensorDesc);
    void UpdateTensorShape(const std::vector<int64_t> &shape);
    void UpdateTensorShapeRange(const std::vector<std::pair<int64_t, int64_t>> &ranges);
    bool CheckConstTensor(bool needCheckHostMem) const;

    bool operator==(const aclTensorDesc* const other);
    void BackDimsAndShapeRanges();
    void RecoverDimsAndShapeRanges();
    void BackConst();
    void RecoverConst();

private:
    mutable std::string cachedKey;
    mutable std::string cachedShapeKey;
};

#endif // ACL_TYPES_TENSOR_DESC_INTERNAL_H
