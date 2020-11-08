/**
* @file op_attr.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "op_attr.h"
#include <sstream>
#include "graph/ge_attr_value.h"
#include "utils/attr_utils.h"
#include "toolchain/resource_statistics.h"

using namespace ge;

aclopAttr *aclopCreateAttr()
{
    ACL_STAGES_REG(acl::ACL_STAGE_CREATE, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_ATTR);
    auto *attr = new(std::nothrow) aclopAttr();
    if (attr == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][Attr]opAttr memory apply failed");
        return nullptr;
    }
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_ATTR);
    return attr;
}

void aclopDestroyAttr(const aclopAttr *attr)
{
    ACL_STAGES_REG(acl::ACL_STAGE_DESTROY, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_ATTR);
    ACL_DELETE_AND_SET_NULL(attr);
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_ATTR);
}

aclError aclopSetAttrBool(aclopAttr *attr, const char *attrName, uint8_t attrValue)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attr);
    return attr->SetAttr(attrName, static_cast<bool>(attrValue));
}

aclError aclopSetAttrInt(aclopAttr *attr, const char *attrName, int64_t attrValue)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attr);
    return attr->SetAttr(attrName, attrValue);
}

aclError aclopSetAttrFloat(aclopAttr *attr, const char *attrName, float attrValue)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attr);
    return attr->SetAttr(attrName, attrValue);
}

aclError aclopSetAttrString(aclopAttr *attr, const char *attrName, const char *attrValue)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attr);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attrValue);
    return attr->SetAttr(attrName, std::string(attrValue));
}

aclError aclopSetAttrDataType(aclopAttr *attr, const char *attrName, aclDataType attrValue)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attr);
    ge::DataType dt = ge::DT_UNDEFINED;
    if (attrValue != ACL_DT_UNDEFINED) {
        dt = static_cast<ge::DataType>(attrValue);
    }
    return attr->SetAttr(attrName, dt);
}

aclError aclopSetAttrListDataType(aclopAttr *attr, const char *attrName, int numValues, const aclDataType values[])
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attr);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attrName);
    if (numValues > 0) {
        ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(values);
    }
    std::vector<ge::DataType> dtVec;
    for (int i = 0; i < numValues; ++i) {
        ge::DataType dt = ge::DT_UNDEFINED;
        if (values[i] != ACL_DT_UNDEFINED) {
            dt = static_cast<ge::DataType>(values[i]);
        }
        dtVec.push_back(dt);
    }
    return attr->SetAttr(attrName, numValues, dtVec.data());
}
aclError aclopSetAttrListBool(aclopAttr *attr, const char *attrName, int numValues, const uint8_t *values)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attr);
    auto *boolValues = reinterpret_cast<const bool *>(values);
    return attr->SetAttr(attrName, numValues, boolValues);
}

aclError aclopSetAttrListInt(aclopAttr *attr, const char *attrName, int numValues, const int64_t *values)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attr);
    return attr->SetAttr(attrName, numValues, values);
}

aclError aclopSetAttrListFloat(aclopAttr *attr, const char *attrName, int numValues, const float *values)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attr);
    return attr->SetAttr(attrName, numValues, values);
}

aclError aclopSetAttrListString(aclopAttr *attr, const char *attrName, int numValues, const char **values)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attr);
    ACL_REQUIRES_OK(acl::array_utils::CheckPtrArray(numValues, values));
    std::vector<std::string> strValues;
    for (int i = 0; i < numValues; ++i) {
        strValues.emplace_back(std::string(values[i]));
    }
    return attr->SetAttr(attrName, strValues);
}

aclError aclopSetAttrListListInt(aclopAttr *attr,
                                 const char *attrName,
                                 int numLists,
                                 const int *numListValues,
                                 const int64_t *const values[])
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attr);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attrName);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(numListValues);
    ACL_REQUIRES_OK(acl::array_utils::CheckPtrArray(numLists, values));
    std::vector<std::vector<int64_t>> valueVec;
    for (int i = 0; i < numLists; ++i) {
        std::vector<int64_t> vec;
        for (int j = 0; j < numListValues[i]; ++j) {
            vec.emplace_back(values[i][j]);
        }
        valueVec.emplace_back(vec);
    }

    return attr->SetAttr(attrName, valueVec);
}

aclopAttr::aclopAttr(const aclopAttr &opAttr)
{
    this->attrs_ = opAttr.attrs_;
    this->digest_ = opAttr.digest_;
    this->constDataBuf_ = opAttr.constDataBuf_;
}

std::string aclopAttr::DebugString() const
{
    return acl::attr_utils::AttrMapToString(attrs_);
}


aclError aclopAttr::SetAttrByType(const char *attrName, aclDataType type, const void *value)
{
    ACL_REQUIRES_NOT_NULL(attrName);
    ACL_REQUIRES_NOT_NULL(value);
    switch (type) {
        case ACL_FLOAT:
            return SetAttr<float>(attrName, *reinterpret_cast<const float *>(value));
        case ACL_FLOAT16:
            return SetAttr(attrName, aclFloat16ToFloat(*reinterpret_cast<const aclFloat16 *>(value)));
        case ACL_INT8:
            return SetAttr(attrName, static_cast<int64_t >(*reinterpret_cast<const int8_t *>(value)));
        case ACL_INT16:
            return SetAttr(attrName, static_cast<int64_t >(*reinterpret_cast<const int16_t *>(value)));
        case ACL_INT32:
            return SetAttr(attrName, static_cast<int64_t >(*reinterpret_cast<const int32_t *>(value)));
        default:
            ACL_LOG_INNER_ERROR("[Check][Type]unsupported type: %d", static_cast<int32_t>(type));
            return ACL_ERROR_UNSUPPORTED_DATA_TYPE;
    }
}

void aclopAttr::UpdateDigest()
{
    digest_ = acl::attr_utils::AttrMapToDigest(attrs_);
}

size_t aclopAttr::GetDigest() const
{
    if (digest_ != 0) {
        return digest_;
    }

    return acl::attr_utils::AttrMapToDigest(attrs_);
}

bool aclopAttr::HasAttr(const char *attrName) const
{
    if (attrName != nullptr) {
        auto it = attrs_.find(string(attrName));
        if (it != attrs_.end()) {
            ACL_LOG_INFO("Find attr [%s] from attrs", attrName);
            return true;
        }
    }
    return false;
}
