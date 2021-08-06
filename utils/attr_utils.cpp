/**
* @file attr_utils.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
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
constexpr size_t DEFAULT_STRING_LEN = 32U;
constexpr float FLOAT_DELTA = 1e-6;
}

template<typename T>
std::string ScalarAttrToString(const ge::GeAttrValue &attrVal)
{
    std::stringstream ss;
    T val {};
    (void)attrVal.GetValue<T>(val);
    ss << val;
    return ss.str();
}

template<>
std::string ScalarAttrToString<string>(const ge::GeAttrValue &attrVal)
{
    std::string val;
    (void)attrVal.GetValue<string>(val);
    return val;
}

template<>
std::string ScalarAttrToString<bool>(const ge::GeAttrValue &attrVal)
{
    bool val = false;
    (void)attrVal.GetValue<bool>(val);
    return val ? "True" : "False";
}

template<typename T>
std::string ListAttrToString(const ge::GeAttrValue &attrVal)
{
    std::vector<T> values;
    (void)attrVal.GetValue<std::vector<T>>(values);
    return string_utils::VectorToString(values);
}

template<>
std::string ListAttrToString<bool>(const ge::GeAttrValue &attrVal)
{
    std::stringstream ss;
    std::vector<bool> values;
    (void)attrVal.GetValue<std::vector<bool>>(values);
    ss << '[';
    const auto size = values.size();
    for (size_t i = 0U; i < size; ++i) {
        if (values[i]) {
            ss << "True";
        } else {
            ss << "False";
        }
        if (i != (size - 1U)) {
            ss << ", ";
        }
    }
    ss << ']';
    return ss.str();
}

template<typename T>
std::string ListListAttrToString(const ge::GeAttrValue &attrVal)
{
    std::stringstream ss;
    ss << '[';
    std::vector<std::vector<T>> values;
    (void)attrVal.GetValue<std::vector<std::vector<T>>>(values);
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
        case ge::GeAttrValue::VT_DATA_TYPE:
            return ScalarAttrToString<ge::GeAttrValue::DATA_TYPE>(val);
        case ge::GeAttrValue::VT_LIST_STRING:
            return ListAttrToString<string>(val);
        case ge::GeAttrValue::VT_LIST_BOOL:
            return ListAttrToString<bool>(val);
        case ge::GeAttrValue::VT_LIST_INT:
            return ListAttrToString<int64_t>(val);
        case ge::GeAttrValue::VT_LIST_FLOAT:
            return ListAttrToString<float>(val);
        case ge::GeAttrValue::VT_LIST_DATA_TYPE:
            return ListAttrToString<ge::GeAttrValue::DATA_TYPE>(val);
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
void ScalarAttrToString(std::string &buffer, const ge::GeAttrValue &attrVal)
{
    T val{};
    (void)attrVal.GetValue<T>(val);
    buffer += std::to_string(val);
}

template<>
void ScalarAttrToString<string>(std::string &buffer, const ge::GeAttrValue &attrVal)
{
    std::string val;
    (void)attrVal.GetValue<string>(val);
    buffer += val;
}

template<>
void ScalarAttrToString<float>(std::string &buffer, const ge::GeAttrValue &attrVal)
{
    float val = 0.0;
    (void)attrVal.GetValue<float>(val);
    buffer += std::to_string(val);
}

template<typename T>
void ListAttrToStringForDigest(std::string &buffer, const ge::GeAttrValue &attrVal)
{
    std::vector<T> values;
    (void)attrVal.GetValue<std::vector<T>>(values);
    for (const T &str : values) {
        buffer += std::to_string(str);
        buffer.push_back(',');
    }
}

template<typename T>
void ListListAttrToStringForDigest(std::string &buffer, const ge::GeAttrValue &attrVal)
{
    std::vector<std::vector<T>> values;
    (void)attrVal.GetValue<std::vector<std::vector<T>>>(values);
    for (auto &subVec : values) {
        for (auto &val : subVec) {
            buffer += std::to_string(val);
            buffer.push_back(',');
        }
        buffer.push_back('|');
    }
}

template<>
void ListAttrToStringForDigest<bool>(std::string &buffer, const ge::GeAttrValue &attrVal)
{
    std::vector<bool> values;
    (void)attrVal.GetValue<std::vector<bool>>(values);
    for (const bool val : values) {
        buffer.push_back(val ? 'T' : 'F');
    }
}

template<>
void ListAttrToStringForDigest<float>(std::string &buffer, const ge::GeAttrValue &attrVal)
{
    std::vector<float> values;
    (void)attrVal.GetValue<std::vector<float>>(values);
    for (const float val : values) {
        buffer += std::to_string(val);
        buffer.push_back(',');
    }
}

template<>
void ListAttrToStringForDigest<string>(std::string &buffer, const ge::GeAttrValue &attrVal)
{
    std::vector<string> values;
    (void)attrVal.GetValue<vector<string>>(values);
    for (const string &str : values) {
        (void)buffer.append(str);
        buffer.push_back(',');
    }
}

void GeAttrValueToStringForDigest(std::string &buffer, const ge::GeAttrValue &val)
{
    switch (val.GetValueType()) {
        case ge::GeAttrValue::VT_BOOL: {
            bool boolVal = false;
            (void)val.GetValue<bool>(boolVal);
            buffer.push_back(boolVal ? 'T' : 'F');
            break;
        }
        case ge::GeAttrValue::VT_STRING:
            ScalarAttrToString<string>(buffer, val);
            break;
        case ge::GeAttrValue::VT_INT:
            ScalarAttrToString<int64_t>(buffer, val);
            break;
        case ge::GeAttrValue::VT_DATA_TYPE:
            ScalarAttrToString<ge::GeAttrValue::DATA_TYPE>(buffer, val);
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
        case ge::GeAttrValue::VT_LIST_DATA_TYPE:
            ListAttrToStringForDigest<ge::GeAttrValue::DATA_TYPE>(buffer, val);
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
    const size_t size = attrMap.size();
    size_t cnt = 0U;
    for (auto &attr : attrMap) {
        ss << attr.first << " = " << GeAttrValueToString(attr.second);
        cnt++;
        if (cnt != size) {
            ss << ", ";
        }
    }
    ss << "}";
    return ss.str();
}

size_t AttrMapToDigest(const std::map<std::string, ge::GeAttrValue> &attrMap)
{
    if (attrMap.empty()) {
        return 0U;
    }

    std::string digest;
    digest.reserve(DEFAULT_STRING_LEN);
    for (auto &attr : attrMap) {
        GeAttrValueToStringForDigest(digest, attr.second);
    }

    std::hash<std::string> hashFunc;
    auto seed = hashFunc(digest);
    return seed;
}

template<typename T>
bool AttrScalarValueEquals(const ge::GeAttrValue &lhs, const ge::GeAttrValue &rhs)
{
    T lhsValue {};
    T rhsValue {};
    (void)lhs.GetValue<T>(lhsValue);
    (void)rhs.GetValue<T>(rhsValue);
    return lhsValue == rhsValue;
}

template<>
bool AttrScalarValueEquals<float>(const ge::GeAttrValue &lhs, const ge::GeAttrValue &rhs)
{
    float lhsValue = 0.0;
    float rhsValue = 0.0;
    (void)lhs.GetValue<float>(lhsValue);
    (void)rhs.GetValue<float>(rhsValue);

    return fabsf(lhsValue - rhsValue) <= FLOAT_DELTA;
}

template<typename T>
bool AttrListValueEquals(const ge::GeAttrValue &lhs, const ge::GeAttrValue &rhs)
{
    vector<T> lhsValue;
    vector<T> rhsValue;
    (void)lhs.GetValue<vector<T>>(lhsValue);
    (void)rhs.GetValue<vector<T>>(rhsValue);
    return lhsValue == rhsValue;
}

bool IsListFloatEquals(const vector<float> &lhsValue, const vector<float> &rhsValue)
{
    if (lhsValue.size() != rhsValue.size()) {
        return false;
    }

    for (size_t i = 0U; i < lhsValue.size(); ++i) {
        const float val1 = lhsValue[i];
        const float val2 = rhsValue[i];
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
    (void)lhs.GetValue<vector<float>>(lhsValue);
    (void)rhs.GetValue<vector<float>>(rhsValue);
    return IsListFloatEquals(lhsValue, rhsValue);
}

template<typename T>
bool AttrListListValueEquals(const ge::GeAttrValue &lhs, const ge::GeAttrValue &rhs)
{
    vector<vector<T>> lhsValue;
    vector<vector<T>> rhsValue;
    (void)lhs.GetValue<vector<vector<T>>>(lhsValue);
    (void)rhs.GetValue<vector<vector<T>>>(rhsValue);
    return lhsValue == rhsValue;
}

bool AttrValueEquals(const ge::GeAttrValue &lhs, const ge::GeAttrValue &rhs)
{
    const auto type = lhs.GetValueType();
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
        case ge::GeAttrValue::VT_DATA_TYPE:
            return AttrScalarValueEquals<ge::GeAttrValue::DATA_TYPE>(lhs, rhs);
        case ge::GeAttrValue::VT_LIST_STRING:
            return AttrListValueEquals<string>(lhs, rhs);
        case ge::GeAttrValue::VT_LIST_BOOL:
            return AttrListValueEquals<bool>(lhs, rhs);
        case ge::GeAttrValue::VT_LIST_INT:
            return AttrListValueEquals<int64_t>(lhs, rhs);
        case ge::GeAttrValue::VT_LIST_FLOAT:
            return AttrListValueEquals<float>(lhs, rhs);
        case ge::GeAttrValue::VT_LIST_DATA_TYPE:
            return AttrListValueEquals<ge::GeAttrValue::DATA_TYPE>(lhs, rhs);
        case ge::GeAttrValue::VT_LIST_LIST_INT:
            return AttrListListValueEquals<int64_t>(lhs, rhs);
        default:
            ACL_LOG_INNER_ERROR("[Check][Type]Unknown type %d", static_cast<int32_t>(type));
            return false;
    }
}

template<typename T>
bool CheckValRange(const std::vector<std::vector<T>> &valueRange, const std::vector<T> &data)
{
    if (valueRange.size() != data.size()) {
        ACL_LOG_WARN("input data size [%zu] must be equal to value range size [%zu]", data.size(), valueRange.size());
        return false;
    }
    for (size_t i = 0U; i < valueRange.size(); ++i) {
        // 2 is range size
        if (valueRange[i].size() != 2U) {
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
bool CheckValExact(const std::vector<T> &valueRange, const std::vector<T> &data)
{
    if (valueRange.size() != data.size()) {
        ACL_LOG_WARN("input data size [%zu] must be equal to value range size [%zu]", data.size(), valueRange.size());
        return false;
    }
    for (size_t i = 0U; i < valueRange.size(); ++i) {
        ACL_LOG_DEBUG("valeRange is %s, value is %s", std::to_string(valueRange[i]).c_str(),
                      std::to_string(data[i]).c_str());
        if (valueRange[i] != data[i]) {
            return false;
        }
    }
    return true;
}

template<typename T1, typename T2>
void SetInputValue(const aclDataBuffer *const dataBuffer, std::vector<T1> &inputData)
{
    for (size_t i = 0U; i < (dataBuffer->length / sizeof(T2)); ++i) {
        inputData.push_back(static_cast<T1>(*(reinterpret_cast<const T2 *>(dataBuffer->data) + i)));
    }
    return;
}

bool GetInputData(const aclDataBuffer *const dataBuffer, const aclDataType dataType,
                  std::vector<int64_t> &inputIntData, std::vector<float> &inputFloatData)
{
    switch (dataType) {
        case ACL_FLOAT:
            SetInputValue<float, float>(dataBuffer, inputFloatData);
            break;
        case ACL_FLOAT16:
            for (size_t i = 0U; i < (dataBuffer->length / sizeof(aclFloat16)); ++i) {
                inputFloatData.push_back(
                    aclFloat16ToFloat(*(reinterpret_cast<const aclFloat16 *>(dataBuffer->data) + i)));
            }
            break;
        case ACL_INT8:
            SetInputValue<int64_t, int8_t>(dataBuffer, inputIntData);
            break;
        case ACL_UINT8:
            SetInputValue<int64_t, uint8_t>(dataBuffer, inputIntData);
            break;
        case ACL_INT16:
            SetInputValue<int64_t, int16_t>(dataBuffer, inputIntData);
            break;
        case ACL_UINT16:
            SetInputValue<int64_t, uint16_t>(dataBuffer, inputIntData);
            break;
        case ACL_INT32:
            SetInputValue<int64_t, int32_t>(dataBuffer, inputIntData);
            break;
        case ACL_UINT32:
            SetInputValue<int64_t, uint32_t>(dataBuffer, inputIntData);
            break;
        case ACL_INT64:
            SetInputValue<int64_t, int64_t>(dataBuffer, inputIntData);
            break;
        case ACL_UINT64:
            SetInputValue<int64_t, uint64_t>(dataBuffer, inputIntData);
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
            return CheckValRange(valRangeInt, inputIntData);
        } else {
            vector<int64_t> tmpInt;
            if (it->second.GetValue<vector<int64_t>>(tmpInt) == ge::GRAPH_SUCCESS) {
                valRangeInt.push_back(tmpInt);
                ACL_LOG_INFO("Get listInt value");
                return CheckValRange(valRangeInt, inputIntData);
            } else {
                ACL_LOG_WARN("can not find listlist or list data struct");
                return false;
            }
        }
    }
    // Check value
    it = valueRange.find(VALUE_TYPE);
    if (it != valueRange.end()) {
        std::vector<int64_t> valExactInt;
        if (it->second.GetValue<vector<int64_t>>(valExactInt) == ge::GRAPH_SUCCESS) {
            ACL_LOG_INFO("Get listInt value");
            return CheckValExact(valExactInt, inputIntData);
        } else {
            int64_t tmpInt = 0;
            if (it->second.GetValue<int64_t>(tmpInt) == ge::GRAPH_SUCCESS) {
                valExactInt.push_back(tmpInt);
                ACL_LOG_INFO("Get int value");
                return CheckValExact(valExactInt, inputIntData);
            } else {
                ACL_LOG_WARN("can not find list or single data struct");
                return false;
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
            return CheckValRange(valRangeFloat, inputFloatData);
        } else {
            vector<float> tmpFloat;
            if (it->second.GetValue<vector<float>>(tmpFloat) == ge::GRAPH_SUCCESS) {
                valRangeFloat.push_back(tmpFloat);
                ACL_LOG_INFO("Get listfloat value");
                return CheckValRange(valRangeFloat, inputFloatData);
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
                     const aclDataBuffer *const dataBuffer, aclDataType dataType)
{
    // value is not nullptr
    if (dataBuffer->data == nullptr) {
        return true;
    }
    std::vector<int64_t> inputIntData;
    std::vector<float> inputFloatData;
    if (!GetInputData(dataBuffer, dataType, inputIntData, inputFloatData)) {
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

bool OpAttrEquals(const aclopAttr *const lhs, const aclopAttr *const rhs)
{
    if (lhs == rhs) {
        return true;
    }
    // inner logic, must not be nullptr
    if ((lhs == nullptr) || (rhs == nullptr)) {
        return false;
    }

    const size_t lhsAttrNum = lhs->Attrs().size();
    const size_t rhsAttrNum = rhs->Attrs().size();
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

        const auto it = lhsAttrs.find(attrName);
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
    const auto constStrLeft = lhs->GetConstBuf();
    for (size_t i = 0U; i < constStrLeft.size(); ++i) {
        ACL_LOG_INFO("the %zu constBuf is %s in constStrLeft", i, constStrLeft[i].c_str());
    }
    const auto constStrRight = rhs->GetConstBuf();
    for (size_t i = 0U; i < constStrRight.size(); ++i) {
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
    static uint64_t timeStamp = 0UL;
    ++timeStamp;
    return timeStamp;
}

static bool ConstToAttr(const vector<aclTensorDesc> &tensorDesc,
                        std::vector<std::string> &constStr)
{
    for (auto &desc : tensorDesc) {
        if (desc.isConst) {
            if ((desc.constDataBuf != nullptr) && (desc.constDataLen <= 0U)) {
                ACL_LOG_INNER_ERROR("[Check][constDataBuf]constDataBuf is not nullptr and dataLen is <= 0");
                return false;
            }
            if ((desc.constDataBuf == nullptr) && (desc.constDataLen > 0U)) {
                ACL_LOG_INNER_ERROR("[Check][constDataBuf]constDataBuf is nullptr and dataLen is > 0");
                return false;
            }
            std::string constBufStr =
                    std::string(reinterpret_cast<const char *>(desc.constDataBuf.get()), desc.constDataLen);
            constStr.emplace_back(constBufStr);
            ACL_LOG_INFO("aclopAttr insert constDataBuf:[%s] when execute", constBufStr.c_str());
        }
    }
    return true;
}

bool SaveConstToAttr(OpModelDef &modelDef)
{
    std::vector<std::string> constStr;
    ACL_LOG_INFO("begin to insert constDataBuf in aclopAttr");
    bool ret = ConstToAttr(modelDef.inputDescArr, constStr);
    if (!ret) {
        ACL_LOG_INNER_ERROR("[Check][InputTenspr]inputTenspr get const dataLen failed");
        return false;
    }
    ret = ConstToAttr(modelDef.outputDescArr, constStr);
    if (!ret) {
        ACL_LOG_INNER_ERROR("[Check][OutputTensor]outputTensor get const dataLen failed");
        return false;
    }
    // opAttr is not nullptr
    for (size_t i = 0U; i < constStr.size(); ++i) {
        ACL_LOG_INFO("Get the [%zu] constBuf is [%s] to emplace emptyAttr", i, constStr[i].c_str());
        modelDef.opAttr.EmplaceConstBuf(constStr[i]);
    }
    return true;
}

static bool ConstToAttr(const int32_t tensorNum,
                        const aclTensorDesc *const tensorDesc[],
                        std::vector<std::string> &constStr)
{
    for (int32_t i = 0; i < tensorNum; ++i) {
        const aclTensorDesc *const desc = tensorDesc[i];
        if (desc->isConst) {
            if ((desc->constDataBuf != nullptr) && (desc->constDataLen <= 0U)) {
                ACL_LOG_INNER_ERROR("[Check][constDataBuf]constDataBuf is not nullptr and dataLen is <= 0");
                return false;
            }
            if ((desc->constDataBuf == nullptr) && (desc->constDataLen > 0U)) {
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

bool SaveConstToAttr(const AclOp &opDesc, aclopAttr *opAttr)
{
    std::vector<std::string> constStr;
    ACL_LOG_INFO("begin to insert constDataBuf in aclopAttr");
    bool ret = ConstToAttr(opDesc.numInputs, opDesc.inputDesc, constStr);
    if (!ret) {
        ACL_LOG_INNER_ERROR("[Check][InputTenspr]inputTenspr get const dataLen failed");
        return false;
    }
    ret = ConstToAttr(opDesc.numOutputs, opDesc.outputDesc, constStr);
    if (!ret) {
        ACL_LOG_INNER_ERROR("[Check][OutputTensor]outputTensor get const dataLen failed");
        return false;
    }
    // opAttr is not nullptr
    opAttr->ClearConstBuf();
    for (size_t i = 0U; i < constStr.size(); ++i) {
        ACL_LOG_INFO("Get the [%zu] constBuf is [%s] to emplace emptyAttr", i, constStr[i].c_str());
        opAttr->EmplaceConstBuf(constStr[i]);
    }
    return true;
}
} // namespace attr_utils
} // acl
