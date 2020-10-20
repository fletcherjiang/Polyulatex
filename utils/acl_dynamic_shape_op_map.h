/**
* @file acl_op_map.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ACL_UTILS_ACL_DYNAMIC_SHAPE_OP_MAP_H
#define ACL_UTILS_ACL_DYNAMIC_SHAPE_OP_MAP_H

#include <map>
#include <mutex>
#include <vector>
#include <climits>
#include "types/acl_op.h"
#include "utils/attr_utils.h"
#include <chrono>

namespace acl {
template<typename T>
class AclShapeRangeMap {
public:
    bool IsEmpty() const
    {
        return entries_.empty();
    }

    void Insert(const AclOp &aclOp, const T &entry, T &agingT);

    aclError Get(const AclOp &aclOp, T &entry, bool needUpdateTimestamp = false);

    void SetMaxOpNum(uint64_t max) { maxOpNum = max; }

private:
    static std::string TensorDescArr2Str(int num, const aclTensorDesc *const descArr[]);
    static std::string ShapeRangeArr2Str(int num, const aclTensorDesc *const descArr[]);
    void Aging(T &agingT);
    void Updatetimestamp(T &entry);
    void AddMemAndAging(std::vector<std::pair<aclopAttr, T>> &modelVec,
                        const aclopAttr &attr, const T &entry, T &agingT);
    bool CheckValueRange(const AclOp &aclOp, T &entry);

    using RangeMap = std::map<std::string, std::vector<std::pair<aclopAttr, T>>>;
    using AttrMap = std::map<size_t, RangeMap>;
    using OutputMap = std::map<std::string, AttrMap>;
    using InputMap = std::map<std::string, OutputMap>;
    using ModelMap = std::map<std::string, InputMap>;

    ModelMap entries_;
    mutable std::mutex mutex_;
    uint64_t count{0};
    uint64_t maxOpNum{DEFAULT_MAX_OPQUEUE_NUM};
};

template<typename T>
std::string AclShapeRangeMap<T>::TensorDescArr2Str(int num, const aclTensorDesc *const descArr[])
{
    if ((num > 0) && (descArr == nullptr)) {
        ACL_LOG_ERROR("[Check][Param]param descArr must not be null");
        return "";
    }
    string descStr;
    descStr.append(std::to_string(num));
    descStr.push_back('~');
    for (int i = 0; i < num; ++i) {
        ACL_REQUIRES_NOT_NULL_RET_STR(descArr[i]);
        ACL_LOG_DEBUG("TensorDescArr2Str::descArr[%d] addr = %p", i, descArr[i]);
        descStr.append(descArr[i]->GetKey());
        descStr.push_back('|');
    }
    return descStr;
}

template<typename T>
std::string AclShapeRangeMap<T>::ShapeRangeArr2Str(int num, const aclTensorDesc *const descArr[])
{
    if ((num > 0) && (descArr == nullptr)) {
        ACL_LOG_ERROR("[Check][Param]param descArr must not be null");
        return "";
    }
    string descStr;
    descStr.append(std::to_string(num));
    descStr.push_back('~');
    for (int i = 0; i < num; ++i) {
        ACL_REQUIRES_NOT_NULL_RET_STR(descArr[i]);
        descStr.append(descArr[i]->GetShapeKey());
        descStr.push_back('|');
    }
    return descStr;
}

template<typename T>
void AclShapeRangeMap<T>::Aging(T &agingT)
{
    typename ModelMap::iterator itTypeMin;
    typename InputMap::iterator itInputMin;
    typename OutputMap::iterator itOutputMin;
    typename AttrMap::iterator itAttrMin;
    typename RangeMap::iterator itRangeMin;
    size_t index = 0;
    uint64_t timestampMin = ULLONG_MAX;
    bool found = false;
    for (auto itType = entries_.begin(); itType != entries_.end(); ++itType) {
        string typeStr = itType->first;
        auto &inputMap = itType->second;
        for (auto itInput = inputMap.begin(); itInput != inputMap.end(); ++itInput) {
            string inputStr = itInput->first;
            auto &outputMap = itInput->second;
            for (auto itOutput = outputMap.begin(); itOutput != outputMap.end(); ++itOutput) {
                string outputStr = itOutput->first;
                auto &attrMap = itOutput->second;
                for (auto itAttr = attrMap.begin(); itAttr != attrMap.end(); ++itAttr) {
                    auto &rangeMap = itAttr->second;
                    for (auto itRange = rangeMap.begin(); itRange != rangeMap.end(); ++itRange) {
                        string rangeStr = itRange->first;
                        auto &opVec = itRange->second;
                        for (size_t i = 0; i < opVec.size(); ++i) {
                            if (opVec[i].second->timestamp < timestampMin) {
                                timestampMin = opVec[i].second->timestamp;
                                itTypeMin = itType;
                                itInputMin = itInput;
                                itOutputMin = itOutput;
                                itAttrMin = itAttr;
                                itRangeMin = itRange;
                                index = i;
                                found = true;
                            }
                        }
                    }
                }
            }
        }
    }
    if (!found) {
        ACL_LOG_WARN("AclShapeRangeMap::Aging IN, can not find minimum value");
        return;
    }
    ACL_LOG_INFO("AclShapeRangeMap::Aging IN, type = %s, input = %s, output = %s, digest = %zu, range = %s",
                 itTypeMin->first.c_str(), itInputMin->first.c_str(),
                 itOutputMin->first.c_str(), itAttrMin->first, itRangeMin->first.c_str());
    auto &model_vec = itRangeMin->second;
    if (index >= model_vec.size()) {
        ACL_LOG_WARN("AclShapeRangeMap::Aging IN, index %zu is larger than vec size %zu", index, model_vec.size());
        return;
    }
    agingT = model_vec[index].second;
    model_vec.erase(model_vec.begin() + index);
    --count;
    if (model_vec.empty()) {
        itAttrMin->second.erase(itRangeMin);
        if (itAttrMin->second.empty()) {
            itOutputMin->second.erase(itAttrMin);
            if (itOutputMin->second.empty()) {
                itInputMin->second.erase(itOutputMin);
                if (itInputMin->second.empty()) {
                    itTypeMin->second.erase(itInputMin);
                    if (entries_[itTypeMin->first].empty()) {
                        entries_.erase(itTypeMin);
                    }
                }
            }
        }
    }
    return;
}

template<typename T>
void AclShapeRangeMap<T>::Updatetimestamp(T &entry)
{
    if (entry->timestamp == ULLONG_MAX) {
        ACL_LOG_INFO("Updatetimestamp IN, no need to update timestamp");
        return;
    }
    entry->timestamp = attr_utils::GetCurrentTimestamp();
    ACL_LOG_INFO("Updatetimestamp IN, timestamp = %lu", entry->timestamp);
}

template<typename T>
void AclShapeRangeMap<T>::AddMemAndAging(std::vector<std::pair<aclopAttr, T>> &modelVec,
                                         const aclopAttr &attr, const T &entry, T &agingT)
{
    modelVec.emplace_back(attr, entry);
    ++count;
    if ((entry->timestamp == ULLONG_MAX) || (count <= maxOpNum)) {
        ACL_LOG_INFO("AclShapeRangeMap::AddMemAndAging in, count is %llu, maxOpNum is %llu, no need aging",
            count, maxOpNum);
        return;
    }
    Aging(agingT);
}

template<typename T>
bool AclShapeRangeMap<T>::CheckValueRange(const AclOp &aclOp, T &entry)
{
    for (size_t i = 0; i < entry->inputDescArr.size(); ++i) {
        if ((entry->inputDescArr[i].IsHostMemTensor()) && (!entry->inputDescArr[i].valueRange.empty())) {
            ACL_LOG_INFO("the input [%zu] needs to check value range", i);
            if (!attr_utils::ValueRangeCheck(entry->inputDescArr[i].valueRange,
                                             aclOp.inputs[i], entry->inputDescArr[i].dataType)) {
                ACL_LOG_WARN("ValueRangeCheck input is not match");
                return false;
            }
        }
    }
    for (size_t i = 0; i < entry->outputDescArr.size(); ++i) {
        if ((entry->outputDescArr[i].IsHostMemTensor()) && (!entry->outputDescArr[i].valueRange.empty())) {
            ACL_LOG_INFO("the output [%zu] needs to check value range", i);
            if (!attr_utils::ValueRangeCheck(entry->outputDescArr[i].valueRange,
                                             aclOp.outputs[i], entry->outputDescArr[i].dataType)) {
                ACL_LOG_WARN("ValueRangeCheck output is not match");
                return false;
            }
        }
    }
    return true;
}

template<typename T>
void AclShapeRangeMap<T>::Insert(const AclOp &aclOp, const T &entry, T &agingT)
{
    ACL_LOG_INFO("AclShapeRangeMap::Insert IN, aclOp = %s", aclOp.DebugString().c_str());
    string inputDescStr = TensorDescArr2Str(aclOp.numInputs, aclOp.inputDesc);
    string outputDescStr = TensorDescArr2Str(aclOp.numOutputs, aclOp.outputDesc);
    string inputRangeStr = ShapeRangeArr2Str(aclOp.numInputs, aclOp.inputDesc);
    string outputRangeStr = ShapeRangeArr2Str(aclOp.numOutputs, aclOp.outputDesc);
    string rangeKeyStr = inputRangeStr + outputRangeStr;
    ACL_LOG_INFO("inputDescStr = %s, outputDescStr = %s, inputRangeStr = %s, outputRangeStr = %s, rangeKeyStr = %s",
        inputDescStr.c_str(), outputDescStr.c_str(), inputRangeStr.c_str(),
        outputRangeStr.c_str(), rangeKeyStr.c_str());
    if (aclOp.opAttr != nullptr) {
        if (!attr_utils::SaveConstToAttr(aclOp, const_cast<aclopAttr *>(aclOp.opAttr))) {
            ACL_LOG_ERROR("[Check][ConstData]save const data buffer to attr fail");
            return;
        }
        size_t digest = attr_utils::AttrMapToDigest(aclOp.opAttr->Attrs());
        std::lock_guard<std::mutex> lk(mutex_);
        entries_[aclOp.opType][inputDescStr][outputDescStr][digest][rangeKeyStr].emplace_back(*aclOp.opAttr, entry);
        AddMemAndAging(entries_[aclOp.opType][inputDescStr][outputDescStr][digest][rangeKeyStr],
                       *aclOp.opAttr, entry, agingT);
    } else {
        aclopAttr emptyAttr;
        if (!attr_utils::SaveConstToAttr(aclOp, &emptyAttr)) {
            ACL_LOG_ERROR("[Check][ConstData]save const data buffer to attr fail");
            return;
        }
        size_t digest = attr_utils::AttrMapToDigest(emptyAttr.Attrs());
        std::lock_guard<std::mutex> lk(mutex_);
        entries_[aclOp.opType][inputDescStr][outputDescStr][digest][rangeKeyStr].emplace_back(emptyAttr, entry);
        AddMemAndAging(entries_[aclOp.opType][inputDescStr][outputDescStr][digest][rangeKeyStr],
                       emptyAttr, entry, agingT);
    }
}

template<typename T>
aclError AclShapeRangeMap<T>::Get(const AclOp &aclOp, T &entry, bool needUpdateTimestamp)
{
    ACL_LOG_INFO("AclShapeRangeMap::Get Out, aclOp = %s", aclOp.DebugString().c_str());
    const string &opType = aclOp.opType;
    auto *opAttr = aclOp.opAttr;
    string inputDescStr = TensorDescArr2Str(aclOp.numInputs, aclOp.inputDesc);
    string outputDescStr = TensorDescArr2Str(aclOp.numOutputs, aclOp.outputDesc);
    string inputRangeStr = ShapeRangeArr2Str(aclOp.numInputs, aclOp.inputDesc);
    string outputRangeStr = ShapeRangeArr2Str(aclOp.numOutputs, aclOp.outputDesc);
    string rangeKeyStr = inputRangeStr + outputRangeStr;
    ACL_LOG_INFO("inputDescStr = %s, outputDescStr = %s, inputRangeStr = %s, outputRangeStr = %s, rangeKeyStr = %s",
        inputDescStr.c_str(), outputDescStr.c_str(), inputRangeStr.c_str(),
        outputRangeStr.c_str(), rangeKeyStr.c_str());
    size_t digest = 0;
    aclopAttr emptyAttr;
    if (opAttr != nullptr) {
        digest = aclOp.opAttr->GetDigest();
        ACL_CHECK_WITH_MESSAGE_AND_RETURN(attr_utils::SaveConstToAttr(aclOp, const_cast<aclopAttr *>(opAttr)),
            ACL_ERROR_INVALID_PARAM, "save const data buffer to attr fail");
    } else {
        ACL_CHECK_WITH_MESSAGE_AND_RETURN(attr_utils::SaveConstToAttr(aclOp, &emptyAttr),
            ACL_ERROR_INVALID_PARAM, "save const data buffer to attr fail");
        opAttr = &emptyAttr;
    }

    std::lock_guard<std::mutex> lk(mutex_);

    auto it = entries_.find(opType);
    if (it == entries_.end()) {
        ACL_LOG_WARN("Match op type failed. opType = %s", opType.c_str());
        return ACL_ERROR_OP_TYPE_NOT_MATCH;
    }

    auto &filteredByType = it->second;
    auto iter = filteredByType.find(inputDescStr);
    if (iter == filteredByType.end()) {
        ACL_LOG_WARN("Match op inputs failed. opType = %s, inputDesc = %s",
            opType.c_str(), inputDescStr.c_str());
        return ACL_ERROR_OP_INPUT_NOT_MATCH;
    }

    auto &filteredByInputs = iter->second;
    auto iter2 = filteredByInputs.find(outputDescStr);
    if (iter2 == filteredByInputs.end()) {
        ACL_LOG_WARN("Match op outputs failed. opType = %s, outputDesc = %s",
            opType.c_str(), outputDescStr.c_str());
        return ACL_ERROR_OP_OUTPUT_NOT_MATCH;
    }

    auto &filteredByOutputs = iter2->second;
    auto iter3 = filteredByOutputs.find(digest);
    if (iter3 == filteredByOutputs.end()) {
        ACL_LOG_WARN("Match attr by digest failed. user input attr = %s",
            opAttr == nullptr ? "None" : opAttr->DebugString().c_str());
        return ACL_ERROR_OP_ATTR_NOT_MATCH;
    }

    // matches by shapeRange
    auto &filteredByAttr = iter3->second;
    auto iter4 = filteredByAttr.find(rangeKeyStr);
    if (iter4 == filteredByAttr.end()) {
        ACL_LOG_WARN("Match op rangeKey failed. opType = %s, rangeKey = %s", opType.c_str(), rangeKeyStr.c_str());
        return ACL_ERROR_OP_ATTR_NOT_MATCH;
    }

    auto &entries = iter4->second;
    T *matchedByRange = nullptr;
    for (auto &attrAndValue : entries) {
        if (attr_utils::OpAttrEquals(&attrAndValue.first, opAttr)) {
            if (CheckValueRange(aclOp, attrAndValue.second)) {
                ACL_LOG_INFO("check value range success");
                matchedByRange = &attrAndValue.second;
                break;
            }
        }
    }

    if (matchedByRange == nullptr) {
        ACL_LOG_WARN("Match op attr or constData failed. opType = %s, attr = %s",
            opType.c_str(), opAttr == nullptr ? "None" : opAttr->DebugString().c_str());
        return ACL_ERROR_OP_ATTR_NOT_MATCH;
    }
    if (needUpdateTimestamp) {
        Updatetimestamp(*matchedByRange);
    }
    entry = *matchedByRange;
    return ACL_SUCCESS;
}
} // namespace acl

#endif // ACL_UTILS_ACL_DYNAMIC_SHAPE_OP_MAP_H
