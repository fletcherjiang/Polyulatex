/**
* @file op_attr.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_TYPES_OP_ATTR_H_
#define ACL_TYPES_OP_ATTR_H_

#include "graph/op_desc.h"
#include "graph/utils/attr_utils.h"

#include "acl/acl.h"
#include "common/log_inner.h"
#include "utils/array_utils.h"

struct ACL_FUNC_VISIBILITY aclopAttr {
    aclopAttr() = default;
    aclopAttr(const aclopAttr &opAttr);

    ~aclopAttr() = default;

    inline const std::map<string, ge::GeAttrValue> &Attrs() const
    {
        return attrs_;
    }

    inline const std::map<string, ge::GeAttrValue> &EmplaceAttr(string str, ge::GeAttrValue val)
    {
        attrs_.emplace(str, val);
        return attrs_;
    }

    inline void ClearConstBuf()
    {
        ACL_LOG_INFO("clear constBuf");
        constDataBuf_.clear();
    }

    inline void EmplaceConstBuf(std::string &str)
    {
        ACL_LOG_INFO("insert const:[%s]", str.c_str());
        constDataBuf_.emplace_back(str);
    }

    inline const std::vector<std::string> &GetConstBuf() const
    {
        return constDataBuf_;
    }

    aclError SetAttrByType(const char *attrName, aclDataType type, const void *value);

    template<typename T>
    aclError SetAttr(const char *attrName, T value)
    {
        ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attrName);
        auto attrVal = ge::GeAttrValue::CreateFrom<T>(value);
        attrs_[std::string(attrName)] = attrVal;
        return ACL_SUCCESS;
    }

    template<typename T>
    aclError SetAttr(const char *attrName, int numValues, const T *values)
    {
        ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(attrName);
        if (numValues > 0) {
            ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(values);
        }
        std::vector<T> valueVec;
        for (int32_t i = 0; i < numValues; ++i) {
            valueVec.push_back(values[i]);
        }

        auto attrValues = ge::GeAttrValue::CreateFrom<std::vector<T>>(valueVec);
        attrs_[std::string(attrName)] = attrValues;
        return ACL_SUCCESS;
    }

    void UpdateDigest();

    size_t GetDigest() const;

    std::string DebugString() const;

    bool HasAttr(const char *attrName) const;

private:
    std::map<string, ge::GeAttrValue> attrs_;
    std::vector<std::string> constDataBuf_;
    mutable size_t digest_ = 0;
};
#endif // ACL_TYPES_OP_ATTR_H_
