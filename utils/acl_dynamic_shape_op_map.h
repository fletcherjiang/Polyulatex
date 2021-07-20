/**
* @file acl_dynamic_shape_op_map.h
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
#include "utils/hash_utils.h"
#include <chrono>
#include <unordered_map>

namespace acl {
template<typename T>
class AclShapeRangeMap {
public:
    bool IsEmpty() const
    {
        return entries_.empty();
    }

    aclError Insert(const AclOp &op, const T &entry, T &agingT);

    aclError Get(const AclOp &op, T &entry, bool needUpdateTimestamp = false);

    void SetMaxOpNum(uint64_t maxNum) { maxOpNum = maxNum; }

private:
    static std::string TensorDescArr2Str(int32_t num, const aclTensorDesc *const descArr[]);
    static std::string ShapeRangeArr2Str(int32_t num, const aclTensorDesc *const descArr[]);
    aclError Aging(T &agingT);
    void Updatetimestamp(T &entry);
    aclError AddMemAndAging(std::vector<std::pair<aclopAttr, T>> &modelVec,
                        const aclopAttr &attr, const T &entry, T &agingT, size_t &seed);
    bool CheckValueRange(const AclOp &op, T &entry);

    using HashMap = std::unordered_map<size_t, std::vector<T>>;

    using RangeMap = std::map<std::string, std::vector<std::pair<aclopAttr, T>>>;
    using AttrMap = std::map<size_t, RangeMap>;
    using OutputMap = std::map<std::string, AttrMap>;
    using InputMap = std::map<std::string, OutputMap>;
    using ModelMap = std::map<std::string, InputMap>;

    HashMap hashMap_;
    ModelMap entries_;
    mutable std::mutex mutex_;
    uint64_t cnt{0};
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
    (void)descStr.append(std::to_string(num));
    descStr.push_back('~');
    for (int32_t i = 0; i < num; ++i) {
        ACL_REQUIRES_NOT_NULL_RET_STR(descArr[i]);
        ACL_LOG_DEBUG("TensorDescArr2Str::descArr[%d] addr = %p", i, descArr[i]);
        (void)descStr.append(descArr[i]->GetKey());
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
    (void)descStr.append(std::to_string(num));
    descStr.push_back('~');
    for (int32_t i = 0; i < num; ++i) {
        ACL_REQUIRES_NOT_NULL_RET_STR(descArr[i]);
        (void)descStr.append(descArr[i]->GetShapeKey());
        descStr.push_back('|');
    }
    return descStr;
}

template<typename T>
aclError AclShapeRangeMap<T>::Aging(T &agingT)
{
    typename ModelMap::iterator itTypeMin;
    typename InputMap::iterator itInputMin;
    typename OutputMap::iterator itOutputMin;
    typename AttrMap::iterator itAttrMin;
    typename RangeMap::iterator itRangeMin;
    size_t idx = 0;
    uint64_t timestampMin = static_cast<uint64_t>(ULLONG_MAX);
    bool found = false;
    for (auto itType = entries_.begin(); itType != entries_.end(); ++itType) {
        const string typeStr = itType->first;
        auto &inputMap = itType->second;
        for (auto itInput = inputMap.begin(); itInput != inputMap.end(); ++itInput) {
            const string inputStr = itInput->first;
            auto &outputMap = itInput->second;
            for (auto itOutput = outputMap.begin(); itOutput != outputMap.end(); ++itOutput) {
                const string outputStr = itOutput->first;
                auto &attrMap = itOutput->second;
                for (auto itAttr = attrMap.begin(); itAttr != attrMap.end(); ++itAttr) {
                    auto &rangeMap = itAttr->second;
                    for (auto itRange = rangeMap.begin(); itRange != rangeMap.end(); ++itRange) {
                        const string rangeStr = itRange->first;
                        auto &opVec = itRange->second;
                        for (int32_t i = 0; i < opVec.size(); ++i) {
                            if (opVec[i].second->timestamp < timestampMin) {
                                timestampMin = opVec[i].second->timestamp;
                                itTypeMin = itType;
                                itInputMin = itInput;
                                itOutputMin = itOutput;
                                itAttrMin = itAttr;
                                itRangeMin = itRange;
                                idx = i;
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
        return ACL_SUCCESS;
    }
    ACL_LOG_INFO("AclShapeRangeMap::Aging IN, type = %s, input = %s, output = %s, digest = %zu, range = %s",
                 itTypeMin->first.c_str(), itInputMin->first.c_str(),
                 itOutputMin->first.c_str(), itAttrMin->first, itRangeMin->first.c_str());
    auto &model_vec = itRangeMin->second;
    if (idx >= model_vec.size()) {
        ACL_LOG_WARN("AclShapeRangeMap::Aging IN, idx %zu is larger than vec size %zu", idx, model_vec.size());
        return ACL_SUCCESS;
    }
    agingT = model_vec[idx].second;
    ACL_LOG_INFO("AclShapeRangeMap::Aging model in model map success, time stamp is %lu", agingT->timestamp);
    (void)model_vec.erase(model_vec.begin() + idx);
    --cnt;
    if (model_vec.empty()) {
        (void)itAttrMin->second.erase(itRangeMin);
        if (itAttrMin->second.empty()) {
            (void)itOutputMin->second.erase(itAttrMin);
            if (itOutputMin->second.empty()) {
                (void)itInputMin->second.erase(itOutputMin);
                if (itInputMin->second.empty()) {
                    (void)itTypeMin->second.erase(itInputMin);
                    if (entries_[itTypeMin->first].empty()) {
                        (void)entries_.erase(itTypeMin);
                    }
                }
            }
        }
    }

    // remove model in hash map while time stamp in hash map is equal to model map
    bool foundHash = false;
    for (auto hashMapIter = hashMap_.begin(); hashMapIter != hashMap_.end(); ++hashMapIter) {
        for (auto vecIter = hashMapIter->second.begin(); vecIter != hashMapIter->second.end(); ++vecIter) {
            ACL_LOG_INFO("AclShapeRangeMap::seed is %zu, time stamp is %lu", hashMapIter->first, (*vecIter)->timestamp);
            if ((*vecIter).get() == agingT.get())  {
                ACL_LOG_INFO("AclShapeRangeMap::Aging model in hash map success, hash seed is %zu, hash time stamp is %zu, "
                    "aging time stamp is %zu", hashMapIter->first, (*vecIter)->timestamp, agingT->timestamp);
                (void)hashMapIter->second.erase(vecIter);
                foundHash = true;
                break;
            }
        }
        if (foundHash && hashMapIter->second.empty()) {
            ACL_LOG_INFO("AclShapeRangeMap::After delete model, hash map empty while seed is %zu, delete seed in HashMap", 
                hashMapIter->first);
            (void)hashMap_.erase(hashMapIter);
            break;
        }
    }
    if (!foundHash) {
        ACL_LOG_ERROR("[Check][foundHash]AclShapeRangeMap::Aging model with time stamp is %lu, ModelMap has aging model"
            "while HashMap has node aging", agingT->timestamp);
        return ACL_ERROR_FAILURE;
    }
    return ACL_SUCCESS;
}

template<typename T>
void AclShapeRangeMap<T>::Updatetimestamp(T &entry)
{
    if (entry->timestamp == static_cast<uint64_t>(ULLONG_MAX)) {
        ACL_LOG_INFO("Updatetimestamp IN, no need to update timestamp");
        return;
    }
    entry->timestamp = attr_utils::GetCurrentTimestamp();
    ACL_LOG_INFO("Updatetimestamp IN, timestamp = %lu", entry->timestamp);
}

template<typename T>
aclError AclShapeRangeMap<T>::AddMemAndAging(std::vector<std::pair<aclopAttr, T>> &modelVec,
                                         const aclopAttr &attr, const T &entry, T &agingT, size_t &seed)
{
    modelVec.emplace_back(attr, entry);
    hashMap_[seed].emplace_back(entry);
    ACL_LOG_INFO("AclShapeRangeMap::Insert aclOp into HashMap success, seed = %zu", seed);
    ++cnt;
    if ((entry->timestamp == static_cast<uint64_t>(ULLONG_MAX)) || (cnt <= maxOpNum)) {
        ACL_LOG_INFO("AclShapeRangeMap::AddMemAndAging in, cnt is %llu, maxOpNum is %llu, no need aging",
            cnt, maxOpNum);
        return ACL_SUCCESS;
    }
    ACL_LOG_INFO("AclShapeRangeMap::time stamp is %lu, cnt is %lu, maxOpNum is %lu, start aging", 
        entry->timestamp, cnt, maxOpNum);
    return Aging(agingT);
}

template<typename T>
bool AclShapeRangeMap<T>::CheckValueRange(const AclOp &aclOp, T &entry)
{
    for (int32_t i = 0; i < entry->inputDescArr.size(); ++i) {
        if ((entry->inputDescArr[i].IsHostMemTensor()) && (!entry->inputDescArr[i].valueRange.empty())) {
            ACL_LOG_INFO("the input [%d] needs to check value range", i);
            if (!attr_utils::ValueRangeCheck(entry->inputDescArr[i].valueRange,
                                             aclOp.inputs[i], entry->inputDescArr[i].dataType)) {
                ACL_LOG_WARN("ValueRangeCheck input is not match");
                return false;
            }
        }
    }
    for (int32_t i = 0; i < entry->outputDescArr.size(); ++i) {
        if ((entry->outputDescArr[i].IsHostMemTensor()) && (!entry->outputDescArr[i].valueRange.empty())) {
            ACL_LOG_INFO("the output [%d] needs to check value range", i);
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
aclError AclShapeRangeMap<T>::Insert(const AclOp &aclOp, const T &entry, T &agingT)
{
    ACL_LOG_INFO("AclShapeRangeMap::Insert IN, aclOp = %s", aclOp.DebugString().c_str());
    const string inputDescStr = TensorDescArr2Str(aclOp.numInputs, aclOp.inputDesc);
    const string outputDescStr = TensorDescArr2Str(aclOp.numOutputs, aclOp.outputDesc);
    const string inputRangeStr = ShapeRangeArr2Str(aclOp.numInputs, aclOp.inputDesc);
    const string outputRangeStr = ShapeRangeArr2Str(aclOp.numOutputs, aclOp.outputDesc);
    const string rangeKeyStr = inputRangeStr + outputRangeStr;
    ACL_LOG_INFO("inputDescStr = %s, outputDescStr = %s, inputRangeStr = %s, outputRangeStr = %s, rangeKeyStr = %s",
        inputDescStr.c_str(), outputDescStr.c_str(), inputRangeStr.c_str(),
        outputRangeStr.c_str(), rangeKeyStr.c_str());
    
    size_t digest = 0;
    auto opAttr = aclOp.opAttr;
    aclopAttr emptyAttr;
    if (opAttr != nullptr) {
        if (!attr_utils::SaveConstToAttr(aclOp, const_cast<aclopAttr *>(opAttr))) {
            ACL_LOG_ERROR("[Check][ConstData]save const data buffer to attr fail");
            return ACL_ERROR_FAILURE;
        }
        digest = attr_utils::AttrMapToDigest(opAttr->Attrs());
    } else {
        if (!attr_utils::SaveConstToAttr(aclOp, &emptyAttr)) {
            ACL_LOG_ERROR("[Check][ConstData]save const data buffer to attr fail");
            return ACL_ERROR_FAILURE;
        }
        digest = attr_utils::AttrMapToDigest(emptyAttr.Attrs());
        opAttr = &emptyAttr;
    }
    size_t seed = 0;
    if (hash_utils::GetAclOpHash(aclOp, digest, seed) != ACL_SUCCESS) {
        ACL_LOG_ERROR("[Check][GetAclOpHash]GetAclOpHash failed, seed = %zu, aclOp = %s",
            seed, aclOp.DebugString().c_str());
        return ACL_ERROR_FAILURE;
    }
    // Lock
    {
        const std::lock_guard<std::mutex> lk(mutex_);
        auto &modelVec = entries_[aclOp.opType][inputDescStr][outputDescStr][digest][rangeKeyStr];
        ACL_REQUIRES_OK(AddMemAndAging(modelVec, *opAttr, entry, agingT, seed));
        ACL_LOG_INFO("AclShapeRangeMap::Insert success, seed = %zu, aclOp = %s", seed, aclOp.DebugString().c_str());
    }

    return ACL_SUCCESS;
}

template<typename T>
aclError AclShapeRangeMap<T>::Get(const AclOp &aclOp, T &entry, bool needUpdateTimestamp)
{
    auto opAttr = aclOp.opAttr;
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

    size_t seed = 0;
    ACL_REQUIRES_OK(hash_utils::GetAclOpHash(aclOp, digest, seed));
    const std::lock_guard<std::mutex> lk(mutex_);
    const auto iter = hashMap_.find(seed);
    if (iter == hashMap_.end()) {
        ACL_LOG_WARN("Get aclOp from AclShapeRangeMap failed due to hashMap_ is empty when seed = %zu, aclOp = %s", 
            seed, aclOp.DebugString().c_str());
        return ACL_ERROR_OP_NOT_FOUND;
    } else if (iter->second.size() == 1) {
        if (needUpdateTimestamp) {
            Updatetimestamp(iter->second.back());
        }
        if (CheckValueRange(aclOp, iter->second.back())) {
            // should use local variable opAttr due to we create emptyAttr object when aclOp.opAttr is nullptr
            if (hash_utils::CheckModelAndAttrMatch(aclOp, opAttr, iter->second.back())) {
                ACL_LOG_INFO("Get aclOp from aclOpMap success! seed = %zu, aclOp = %s", seed,
                    aclOp.DebugString().c_str());
                entry = iter->second.back();
                return ACL_SUCCESS;
            } else {
                ACL_LOG_WARN("Get aclOp from aclOpMap failed due to CheckModelMatch failed! seed = %zu, aclOp = %s",
                    seed, aclOp.DebugString().c_str());
                return ACL_ERROR_OP_NOT_FOUND;
            }
        }
        ACL_LOG_WARN("Get aclOp from aclOpMap failed due to CheckValueRange failed! seed = %zu, aclOp = %s",
            seed, aclOp.DebugString().c_str());
        return ACL_ERROR_OP_NOT_FOUND;
    } else {
        ACL_LOG_INFO("Match aclOp by string from AclShapeRangeMap due to seed has conflict! seed = %zu, aclOp = %s", 
            seed, aclOp.DebugString().c_str());
        const string &opType = aclOp.opType;
        const string inputDescStr = TensorDescArr2Str(aclOp.numInputs, aclOp.inputDesc);
        const string outputDescStr = TensorDescArr2Str(aclOp.numOutputs, aclOp.outputDesc);
        const string inputRangeStr = ShapeRangeArr2Str(aclOp.numInputs, aclOp.inputDesc);
        const string outputRangeStr = ShapeRangeArr2Str(aclOp.numOutputs, aclOp.outputDesc);
        const string rangeKeyStr = inputRangeStr + outputRangeStr;

        const auto it = entries_.find(opType);
        if (it == entries_.end()) {
            ACL_LOG_WARN("Match op type failed. opType = %s", opType.c_str());
            return ACL_ERROR_OP_TYPE_NOT_MATCH;
        }

        ACL_LOG_INFO("inputDescStr = %s, outputDescStr = %s, inputRangeStr = %s, outputRangeStr = %s, rangeKeyStr = %s",
            inputDescStr.c_str(), outputDescStr.c_str(), inputRangeStr.c_str(),
            outputRangeStr.c_str(), rangeKeyStr.c_str());

        auto &filteredByType = it->second;
        const auto iter = filteredByType.find(inputDescStr);
        if (iter == filteredByType.end()) {
            ACL_LOG_WARN("Match op inputs failed. opType = %s, inputDesc = %s",
                opType.c_str(), inputDescStr.c_str());
            return ACL_ERROR_OP_INPUT_NOT_MATCH;
        }

        auto &filteredByInputs = iter->second;
        const auto iter2 = filteredByInputs.find(outputDescStr);
        if (iter2 == filteredByInputs.end()) {
            ACL_LOG_WARN("Match op outputs failed. opType = %s, outputDesc = %s",
                opType.c_str(), outputDescStr.c_str());
            return ACL_ERROR_OP_OUTPUT_NOT_MATCH;
        }

        auto &filteredByOutputs = iter2->second;
        const auto iter3 = filteredByOutputs.find(digest);
        if (iter3 == filteredByOutputs.end()) {
            ACL_LOG_WARN("Match attr by digest failed. user input attr = %s",
                opAttr == nullptr ? "None" : (opAttr->DebugString().c_str()));
            return ACL_ERROR_OP_ATTR_NOT_MATCH;
        }

        // matches by shapeRange
        auto &filteredByAttr = iter3->second;
        const auto iter4 = filteredByAttr.find(rangeKeyStr);
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
                opType.c_str(), opAttr == nullptr ? "None" : (opAttr->DebugString().c_str()));
            return ACL_ERROR_OP_ATTR_NOT_MATCH;
        }
        if (needUpdateTimestamp) {
            Updatetimestamp(*matchedByRange);
        }
        entry = *matchedByRange;
        return ACL_SUCCESS;
    }
}
} // namespace acl

#endif // ACL_UTILS_ACL_DYNAMIC_SHAPE_OP_MAP_H
