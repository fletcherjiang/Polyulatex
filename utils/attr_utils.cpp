/**
* @file attr_utils.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "attr_utils.h"
#include <cmath>
#include "securec.h"
#include "common/log_inner.h"
#include "mmpa/mmpa_api.h"

namespace acl {
namespace attr_utils {
namespace {
constexpr int DEFAULT_BUFFER_SIZE = 32;
constexpr float FLOAT_DELTA = 1e-6;
}

template<typename T>
std::string ScalarAttrToString(const ge::GeAttrValue &value)
{
    std::stringstream ss;
    T val {};
    value.GetValue<T>(val);
    ss << val;
    return ss.str();
}

template<>
std::string ScalarAttrToString<string>(const ge::GeAttrValue &val)
{
    std::string value;
    val.GetValue<string>(value);
    return value;
}

template<>
std::string ScalarAttrToString<bool>(const ge::GeAttrValue &val)
{
    bool value = false;
    val.GetValue<bool>(value);
    return value ? "True" : "False";
}

template<typename T>
std::string ListAttrToString(const ge::GeAttrValue &value)
{
    std::stringstream ss;
    std::vector<T> values;
    value.GetValue<std::vector<T>>(values);
    return string_utils::VectorToString(values);
}

template<>
std::string ListAttrToString<bool>(const ge::GeAttrValue &value)
{
    std::stringstream ss;
    std::vector<bool> values;
    value.GetValue<std::vector<bool>>(values);
    ss << '[';
    auto size = values.size();
    for (size_t i = 0; i < size; ++i) {
        if (values[i]) {
            ss << "True";
        } else {
            ss << "False";
        }
        if (i != size - 1) {
            ss << ", ";
        }
    }
    ss << ']';
    return ss.str();
}

template<typename T>
std::string ListListAttrToString(const ge::GeAttrValue &value)
{
    std::stringstream ss;
    ss << '[';
    std::vector<std::vector<T>> values;
    value.GetValue<std::vector<std::vector<T>>>(values);
    return string_utils::VectorToString(values);
}

string GeAttrValueToString(const ge::GeAttrValue &val)
{
    switch (val.GetValueType()) {
        case ge::GeAttrValue::VT_STRING:
            return ScalarAttrToString<string>(val);
        case ge::GeAttrValue::VT_BOOL:
            return ScalarAttrToString<bool>(val);
        case ge::GeAttrValue::VT_INT:
            return ScalarAttrToString<int64_t>(val);
        case ge::GeAttrValue::VT_FLOAT:
            return ScalarAttrToString<float>(val);
        case ge::GeAttrValue::VT_LIST_STRING:
            return ListAttrToString<string>(val);
        case ge::GeAttrValue::VT_LIST_BOOL:
            return ListAttrToString<bool>(val);
        case ge::GeAttrValue::VT_LIST_INT:
            return ListAttrToString<int64_t>(val);
        case ge::GeAttrValue::VT_LIST_FLOAT:
            return ListAttrToString<float>(val);
        case ge::GeAttrValue::VT_LIST_LIST_INT:
            return ListListAttrToString<int64_t>(val);
        default:
            std::stringstream ss;
            ss << "Unknown type ";
            ss << val.GetValueType();
            return ss.str();
    }
}

template<typename T>
void ScalarAttrToString(std::string &buffer, const ge::GeAttrValue &value)
{
    T val{};
    value.GetValue<T>(val);
    buffer += std::to_string(val);
}

template<>
void ScalarAttrToString<string>(std::string &buffer, const ge::GeAttrValue &value)
{
    std::string val;
    value.GetValue<string>(val);
    buffer += val;
}

template<>
void ScalarAttrToString<float>(std::string &buffer, const ge::GeAttrValue &value)
{
    float val = 0.0;
    value.GetValue<float>(val);
    buffer += std::to_string(val);
}

template<typename T>
void ListAttrToStringForDigest(std::string &buffer, const ge::GeAttrValue &value)
{
    std::vector<T> values;
    value.GetValue<std::vector<T>>(values);
    for (const T &str : values) {
        buffer += std::to_string(str);
        buffer.push_back(',');
    }
}

template<typename T>
void ListListAttrToStringForDigest(std::string &buffer, const ge::GeAttrValue &value)
{
    std::vector<std::vector<T>> values;
    value.GetValue<std::vector<std::vector<T>>>(values);
    for (auto &subVec : values) {
        for (auto &val : subVec) {
            buffer += std::to_string(val);
            buffer.push_back(',');
        }
        buffer.push_back('|');
    }
}

template<>
void ListAttrToStringForDigest<bool>(std::string &buffer, const ge::GeAttrValue &value)
{
    std::vector<bool> values;
    value.GetValue<std::vector<bool>>(values);
    for (bool val : values) {
        buffer.push_back(val ? 'T' : 'F');
    }
}

template<>
void ListAttrToStringForDigest<float>(std::string &buffer, const ge::GeAttrValue &value)
{
    std::vector<float> values;
    value.GetValue<std::vector<float>>(values);
    for (float val : values) {
        buffer += std::to_string(val);
        buffer.push_back(',');
    }
}

template<>
void ListAttrToStringForDigest<string>(std::string &buffer, const ge::GeAttrValue &value)
{
    std::vector<string> values;
    value.GetValue<vector<string>>(values);
    for (const string &str : values) {
        buffer.append(str);
        buffer.push_back(',');
    }
}

void GeAttrValueToStringForDigest(std::string &buffer, const ge::GeAttrValue &val)
{
    switch (val.GetValueType()) {
        case ge::GeAttrValue::VT_BOOL: {
            bool value = false;
            val.GetValue<bool>(value);
            buffer.push_back(value ? 'T' : 'F');
            break;
        }
        case ge::GeAttrValue::VT_STRING:
            ScalarAttrToString<string>(buffer, val);
            break;
        case ge::GeAttrValue::VT_INT:
            ScalarAttrToString<int64_t>(buffer, val);
            break;
        case ge::GeAttrValue::VT_FLOAT:
            ScalarAttrToString<float>(buffer, val);
            break;
        case ge::GeAttrValue::VT_LIST_STRING:
            ListAttrToStringForDigest<string>(buffer, val);
            break;
        case ge::GeAttrValue::VT_LIST_BOOL:
            ListAttrToStringForDigest<bool>(buffer, val);
            break;
        case ge::GeAttrValue::VT_LIST_INT:
            ListAttrToStringForDigest<int64_t>(buffer, val);
            break;
        case ge::GeAttrValue::VT_LIST_FLOAT:
            ListAttrToStringForDigest<float>(buffer, val);
            break;
        case ge::GeAttrValue::VT_LIST_LIST_INT:
            ListListAttrToStringForDigest<int64_t>(buffer, val);
            break;
        default:
            break;
    }
}

std::string AttrMapToString(const std::map<std::string, ge::GeAttrValue> &attrMap)
{
    std::stringstream ss;
    ss << "{";
    size_t size = attrMap.size();
    size_t count = 0;
    for (auto &attr : attrMap) {
        ss << attr.first << " = " << GeAttrValueToString(attr.second);
        if (++count != size) {
            ss << ", ";
        }
    }
    ss << "}";
    return ss.str();
}

size_t AttrMapToDigest(const std::map<std::string, ge::GeAttrValue> &attrMap)
{
    if (attrMap.empty()) {
        return 0;
    }

    std::string digest;
    digest.reserve(DEFAULT_BUFFER_SIZE);
    for (auto &attr : attrMap) {
        GeAttrValueToStringForDigest(digest, attr.second);
    }

    std::hash<std::string> hashFunc;
    return hashFunc(digest);
}

template<typename T>
bool AttrScalarValueEquals(const ge::GeAttrValue &lhs, const ge::GeAttrValue &rhs)
{
    T lhsValue {};
    T rhsValue {};
    lhs.GetValue<T>(lhsValue);
    rhs.GetValue<T>(rhsValue);
    return lhsValue == rhsValue;
}

template<>
bool AttrScalarValueEquals<float>(const ge::GeAttrValue &lhs, const ge::GeAttrValue &rhs)
{
    float lhsValue = 0.0;
    float rhsValue = 0.0;
    lhs.GetValue<float>(lhsValue);
    rhs.GetValue<float>(rhsValue);

    return fabsf(lhsValue - rhsValue) <= FLOAT_DELTA;
}

template<typename T>
bool AttrListValueEquals(const ge::GeAttrValue &lhs, const ge::GeAttrValue &rhs)
{
    vector<T> lhsValue;
    vector<T> rhsValue;
    lhs.GetValue<vector<T>>(lhsValue);
    rhs.GetValue<vector<T>>(rhsValue);
    return lhsValue == rhsValue;
}

bool IsListFloatEquals(const vector<float> &lhsValue, const vector<float> &rhsValue)
{
    if (lhsValue.size() != rhsValue.size()) {
        return false;
    }

    for (size_t i = 0; i < lhsValue.size(); ++i) {
        float val1 = lhsValue[i];
        float val2 = rhsValue[i];
        if (fabsf(val1 - val2) > FLOAT_DELTA) {
            return false;
        }
    }

    return true;
}

template<>
bool AttrListValueEquals<float>(const ge::GeAttrValue &lhs, const ge::GeAttrValue &rhs)
{
    std::vector<float> lhsValue;
    std::vector<float> rhsValue;
    lhs.GetValue<vector<float>>(lhsValue);
    rhs.GetValue<vector<float>>(rhsValue);
    return IsListFloatEquals(lhsValue, rhsValue);
}

template<typename T>
bool AttrListListValueEquals(const ge::GeAttrValue &lhs, const ge::GeAttrValue &rhs)
{
    vector<vector<T>> lhsValue;
    vector<vector<T>> rhsValue;
    lhs.GetValue<vector<vector<T>>>(lhsValue);
    rhs.GetValue<vector<vector<T>>>(rhsValue);
    return lhsValue == rhsValue;
}

bool AttrValueEquals(const ge::GeAttrValue &lhs, const ge::GeAttrValue &rhs)
{
    auto type = lhs.GetValueType();
    if (rhs.GetValueType() != type) {
        ACL_LOG_INFO("type mismatch, lhs type = %d, rhs type = %d",
            static_cast<int32_t>(type), static_cast<int32_t>(rhs.GetValueType()));
        return false;
    }

    switch (type) {
        case ge::GeAttrValue::VT_STRING:
            return AttrScalarValueEquals<string>(lhs, rhs);
        case ge::GeAttrValue::VT_BOOL:
            return AttrScalarValueEquals<bool>(lhs, rhs);
        case ge::GeAttrValue::VT_INT:
            return AttrScalarValueEquals<int64_t>(lhs, rhs);
        case ge::GeAttrValue::VT_FLOAT:
            return AttrScalarValueEquals<float>(lhs, rhs);
        case ge::GeAttrValue::VT_LIST_STRING:
            return AttrListValueEquals<string>(lhs, rhs);
        case ge::GeAttrValue::VT_LIST_BOOL:
            return AttrListValueEquals<bool>(lhs, rhs);
        case ge::GeAttrValue::VT_LIST_INT:
            return AttrListValueEquals<int64_t>(lhs, rhs);
        case ge::GeAttrValue::VT_LIST_FLOAT:
            return AttrListValueEquals<float>(lhs, rhs);
        case ge::GeAttrValue::VT_LIST_LIST_INT:
            return AttrListListValueEquals<int64_t>(lhs, rhs);
        default:
            ACL_LOG_INNER_ERROR("[Check][Type]Unknown type %d", static_cast<int32_t>(type));
            return false;
    }
}

template<typename T>
bool CheckValueRange(const std::vector<std::vector<T>> &valueRange, const std::vector<T> &data)
{
    if (valueRange.size() != data.size()) {
        ACL_LOG_WARN("input data size [%zu] must be equal to value range size [%zu]", data.size(), valueRange.size());
        return false;
    }
    for (size_t i = 0; i < valueRange.size(); ++i) {
        if (valueRange.[i].size() != 2) {
            ACL_LOG_WARN("range size must be 2");
            return false;
        }
        ACL_LOG_DEBUG("min is %s, max is %s, value is %s", std::to_string(valueRange[i][0]).c_str(),
                      std::to_string(valueRange[i][1]).c_str(), std::to_string(data[i]).c_str());
        if ((valueRange[i][0] > data[i]) || (valueRange[i][1] < data[i])) {
            return false;
        }
    }
    return true;
}

template<typename T>
bool CheckValueExact(const std::vector<T> &valueRange, const std::vector<T> &data)
{
    if (valueRange.size() != data.size()) {
        ACL_LOG_WARN("input data size [%zu] must be equal to value range size [%zu]", data.size(), valueRange.size());
        return false;
    }
    for (size_t i = 0; i < valueRange.size(); ++i) {
        ACL_LOG_DEBUG("valeRange is %s, value is %s", std::to_string(valueRange[i]).c_str(),
                      std::to_string(data[i]).c_str());
        if (valueRange[i] != data[i]) {
            return false;
        }
    }
    return true;
}

bool GetInputData(const aclDataBuffer *value, aclDataType dataType,
                  std::vector<int64_t> &inputIntData, std::vector<float> &inputFloatData)
{
    switch (dataType) {
        case ACL_FLOAT:
            for (size_t i = 0; i < (value->length / sizeof(float)); ++i) {
                inputFloatData.push_back(*(reinterpret_cast<const float *>(value->data) + i));
            }
            break;
        case ACL_FLOAT16:
            for (size_t i = 0; i < (value->length / sizeof(float) * 2); ++i) {
                inputFloatData.push_back(aclFloat16ToFloat(*(reinterpret_cast<const aclFloat16 *>(value->data) + i)));
            }
            break;
        case ACL_INT8:
            for (size_t i = 0; i < (value->length / sizeof(int8_t)); ++i) {
                inputIntData.push_back(*(reinterpret_cast<const int8_t *>(value->data) + i));
            }
            break;
        case ACL_UINT8:
            for (size_t i = 0; i < (value->length / sizeof(uint8_t)); ++i) {
                inputIntData.push_back(*(reinterpret_cast<const uint8_t *>(value->data) + i));
            }
            break;
        case ACL_INT16:
            for (size_t i = 0; i < (value->length / sizeof(int16_t)); ++i) {
                inputIntData.push_back(*(reinterpret_cast<const int16_t *>(value->data) + i));
            }
            break;
        case ACL_UINT16:
            for (size_t i = 0; i < (value->length / sizeof(uint16_t)); ++i) {
                inputIntData.push_back(*(reinterpret_cast<const uint16_t *>(value->data) + i));
            }
            break;
        case ACL_INT32:
            for (size_t i = 0; i < (value->length / sizeof(int32_t)); ++i) {
                inputIntData.push_back(*(reinterpret_cast<const int32_t *>(value->data) + i));
            }
            break;
        case ACL_UINT32:
            for (size_t i = 0; i < (value->length / sizeof(uint32_t)); ++i) {
                inputIntData.push_back(*(reinterpret_cast<const uint32_t *>(value->data) + i));
            }
            break;
        case ACL_INT64:
            for (size_t i = 0; i < (value->length / sizeof(int64_t)); ++i) {
                inputIntData.push_back(*(reinterpret_cast<const int64_t *>(value->data) + i));
            }
            break;
        case ACL_UINT64:
            for (size_t i = 0; i < (value->length / sizeof(uint64_t)); ++i) {
                inputIntData.push_back(*(reinterpret_cast<const uint64_t *>(value->data) + i));
            }
            break;
        default:
            ACL_LOG_WARN("unsupported type: %d", dataType);
            return false;
    }
    ACL_LOG_INFO("Get input data success, dt is %d, int type size is %zu, float type size is %zu",
                 dataType, inputIntData.size(), inputFloatData.size());
    return true;
}

bool CheckIntValueRange(const std::map<AttrRangeType, ge::GeAttrValue> &valueRange,
                        const std::vector<int64_t> &inputIntData)
{
    // Check range
    auto it = valueRange.find(RANGE_TYPE);
    if (it != valueRange.end()) {
        std::vector<std::vector<int64_t>> valRangeInt;
        if (it->second.GetValue<vector<vector<int64_t>>>(valRangeInt) == ge::GRAPH_SUCCESS) {
            ACL_LOG_INFO("Get listlistInt value");
            return CheckValueRange(valRangeInt, inputIntData);
        } else {
            vector<int64_t> tmpInt;
            if (it->second.GetValue<vector<int64_t>>(tmpInt) == ge::GRAPH_SUCCESS) {
                valRangeInt.push_back(tmpInt);
                ACL_LOG_INFO("Get listInt value");
                return CheckValueRange(valRangeInt, inputIntData);
            }
        }
    }
    // Check value
    it = valueRange.find(VALUE_TYPE);
    if (it != valueRange.end()) {
        std::vector<int64_t> valExactInt;
        if (it->second.GetValue<vector<int64_t>>(valExactInt) == ge::GRAPH_SUCCESS) {
            ACL_LOG_INFO("Get listInt value");
            return CheckValueExact(valExactInt, inputIntData);
        } else {
            int64_t tmpInt = 0;
            if (it->second.GetValue<int64_t>(tmpInt) == ge::GRAPH_SUCCESS) {
                valExactInt.push_back(tmpInt);
                ACL_LOG_INFO("Get int value");
                return CheckValueExact(valExactInt, inputIntData);
            }
        }
    }
    return true;
}

bool CheckFloatValueRange(const std::map<AttrRangeType, ge::GeAttrValue> &valueRange,
                          const std::vector<float> &inputFloatData)
{
    // Check range
    auto it = valueRange.find(RANGE_TYPE);
    if (it != valueRange.end()) {
        std::vector<std::vector<float>> valRangeFloat;
        if (it->second.GetValue<vector<vector<float>>>(valRangeFloat) == ge::GRAPH_SUCCESS) {
            ACL_LOG_INFO("Get listlistfloat value");
            return CheckValueRange(valRangeFloat, inputFloatData);
        } else {
            vector<float> tmpFloat;
            if (it->second.GetValue<vector<float>>(tmpFloat) == ge::GRAPH_SUCCESS) {
                valRangeFloat.push_back(tmpFloat);
                ACL_LOG_INFO("Get listfloat value");
                return CheckValueRange(valRangeFloat, inputFloatData);
            }
        }
    }
    // Check value
    it = valueRange.find(VALUE_TYPE);
    if (it != valueRange.end()) {
        std::vector<float> valExactFloat;
        if (it->second.GetValue<vector<float>>(valExactFloat) == ge::GRAPH_SUCCESS) {
            ACL_LOG_INFO("Get listfloat value");
            return IsListFloatEquals(valExactFloat, inputFloatData);
        } else {
            float tmpFloat = 0.0;
            if (it->second.GetValue<float>(tmpFloat) == ge::GRAPH_SUCCESS) {
                valExactFloat.push_back(tmpFloat);
                ACL_LOG_INFO("Get float value");
                return IsListFloatEquals(valExactFloat, inputFloatData);
            }
        }
    }
    return true;
}

bool ValueRangeCheck(const std::map<AttrRangeType, ge::GeAttrValue> &valueRange,
                     const aclDataBuffer *value, aclDataType dataType)
{
    // value is not nullptr
    if (value->data == nullptr) {
        return true;
    }
    std::vector<int64_t> inputIntData;
    std::vector<float> inputFloatData;
    if (!GetInputData(value, dataType, inputIntData, inputFloatData)) {
        return false;
    }
    // check value range
    if (!inputIntData.empty()) {
        return CheckIntValueRange(valueRange, inputIntData);
    }
    if (!inputFloatData.empty()) {
        return CheckFloatValueRange(valueRange, inputFloatData);
    }
    return true;
}

bool OpAttrEquals(const aclopAttr *lhs, const aclopAttr *rhs)
{
    if (lhs == rhs) {
        return true;
    }
    // inner logic, must not be nullptr
    if ((lhs == nullptr) || (rhs == nullptr)) {
        return false;
    }

    size_t lhsAttrNum = lhs->Attrs().size();
    size_t rhsAttrNum = rhs->Attrs().size();
    if (lhsAttrNum != rhsAttrNum) {
        ACL_LOG_INFO("Attr num mismatches, lhs attr num = %zu, rhs attr num = %zu", lhsAttrNum, rhsAttrNum);
        return false;
    }
    // attr num of lhs and rhs is not 0, so attr is not nullptr
    const auto &lhsAttrs = lhs->Attrs();
    for (const auto &iter : rhs->Attrs()) {
        const string &attrName = iter.first;
        const ge::GeAttrValue &attrValue = iter.second;
        ACL_LOG_DEBUG("Matching attr %s", attrName.c_str());

        auto it = lhsAttrs.find(attrName);
        if (it == lhsAttrs.end()) {
            ACL_LOG_DEBUG("Attr[%s] NOT in lhs attr", attrName.c_str());
            return false;
        }

        // no overloaded operator != for GeAttrValue
        if (!attr_utils::AttrValueEquals(it->second, attrValue)) {
            ACL_LOG_INFO("Attr[%s] value mismatches. lhs's value = %s, rhs's value = %s", attrName.c_str(),
                         attr_utils::GeAttrValueToString(it->second).c_str(),
                         attr_utils::GeAttrValueToString(attrValue).c_str());
            return false;
        }
    }

    ACL_LOG_INFO("begin to compare constBuf");
    auto constStrLeft = lhs->GetConstBuf();
    for (size_t i = 0; i < constStrLeft.size(); ++i) {
        ACL_LOG_INFO("the %zu constBuf is %s in constStrLeft", i, constStrLeft[i].c_str());
    }
    auto constStrRight = rhs->GetConstBuf();
    for (size_t i = 0; i < constStrRight.size(); ++i) {
        ACL_LOG_INFO("the %zu constBuf is %s in constStrRight", i, constStrRight[i].c_str());
    }
    if (constStrLeft != constStrRight) {
        ACL_LOG_INFO("the constBuf mismatches.");
        return false;
    }

    ACL_LOG_INFO("Match attr success.");
    return true;
}

uint64_t GetCurrentTimestamp()
{
    mmTimeval tv{};
    auto ret = mmGetTimeOfDay(&tv, nullptr);
    if (ret != EN_OK) {
        ACL_LOG_WARN("Func mmGetTimeOfDay failed, ret = %d", ret);
    }
    // 1000000: seconds to microseconds
    uint64_t total_use_time = tv.tv_usec + static_cast<uint64_t>(tv.tv_sec) * 1000000;
    return total_use_time;
}

static bool ConstToAttr(int tensorNum,
                        const aclTensorDesc *const tensorDesc[],
                        std::vector<std::string> &constStr)
{
    for (int i = 0; i < tensorNum; ++i) {
        const aclTensorDesc *desc = tensorDesc[i];
        if (desc->isConst) {
            if ((desc->constDataBuf != nullptr) && (desc->constDataLen <= 0)) {
                ACL_LOG_INNER_ERROR("[Check][constDataBuf]constDataBuf is not nullptr and dataLen is <= 0");
                return false;
            }
            if ((desc->constDataBuf == nullptr) && (desc->constDataLen > 0)) {
                ACL_LOG_INNER_ERROR("[Check][constDataBuf]constDataBuf is nullptr and dataLen is > 0");
                return false;
            }
            std::string constBufStr =
                std::string(reinterpret_cast<const char *>(desc->constDataBuf.get()), desc->constDataLen);
            constStr.emplace_back(constBufStr);
            ACL_LOG_INFO("aclopAttr insert constDataBuf:[%s] when execute", constBufStr.c_str());
        }
    }
    return true;
}

bool SaveConstToAttr(const AclOp &aclOp, aclopAttr *opAttr)
{
    std::vector<std::string> constStr;
    ACL_LOG_INFO("begin to inset constDataBuf in aclopAttr");
    bool ret = ConstToAttr(aclOp.numInputs, aclOp.inputDesc, constStr);
    if (!ret) {
        ACL_LOG_INNER_ERROR("[Check][InputTenspr]inputTenspr get const dataLen failed");
        return false;
    }
    ret = ConstToAttr(aclOp.numOutputs, aclOp.outputDesc, constStr);
    if (!ret) {
        ACL_LOG_INNER_ERROR("[Check][OutputTensor]outputTensor get const dataLen failed");
        return false;
    }
    // opAttr is not nullptr
    opAttr->ClearConstBuf();
    for (size_t i = 0; i < constStr.size(); ++i) {
        ACL_LOG_INFO("Get the [%zu] constBuf is [%s] to emplace emptyAttr", i, constStr[i].c_str());
        opAttr->EmplaceConstBuf(constStr[i]);
    }
    return true;
}
} // namespace attr_utils
} // acl
