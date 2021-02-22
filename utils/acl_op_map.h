/**
* @file acl_op_map.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ACL_UTILS_ACL_OP_MAP_H
#define ACL_UTILS_ACL_OP_MAP_H

#include <map>
#include <mutex>
#include <vector>
#include <climits>
#include "types/acl_op.h"
#include "utils/attr_utils.h"
#include <chrono>
#include "hash_utils.h"
#include <unordered_map>

namespace acl {
template<typename T>
class AclOpMap {
public:
    bool IsEmpty() const
    {
        return entries_.empty();
    }

    aclError Insert(const AclOp &aclOp, const T &entry, T &agingT);

    aclError Get(const AclOp &aclOp, T &entry, bool needUpdateTimestamp = false);

    void SetMaxOpNum(uint64_t max) { maxOpNum = max; }

private:
    static std::string TensorDescArr2Str(int num, const aclTensorDesc *const descArr[]);
    aclError Aging(T &agingT);
    void Updatetimestamp(T &entry);
    aclError AddMemAndAging(std::vector<std::pair<aclopAttr, T>> &modelVec,
                        const aclopAttr &attr, const T &entry, T &agingT, size_t &seed);

    using HashMap = std::unordered_map<size_t, std::vector<T>>;

    using AttrMap = std::map<size_t, std::vector<std::pair<aclopAttr, T>>>;
    using OutputMap = std::map<std::string, AttrMap>;
    using InputMap = std::map<std::string, OutputMap>;
    using ModelMap = std::map<std::string, InputMap>;

    HashMap hashMap_;
    ModelMap entries_;
    mutable std::mutex mutex_;
    uint64_t count{0};
    uint64_t maxOpNum{DEFAULT_MAX_OPQUEUE_NUM};
};

template<typename T>
std::string AclOpMap<T>::TensorDescArr2Str(int num, const aclTensorDesc *const descArr[])
{
    if ((num > 0) && (descArr == nullptr)) {
        ACL_LOG_ERROR("[Check][Params]param descArr must not be null");
        return "";
    }
    string descStr;
    descStr.append(std::to_string(num));
    descStr.push_back('~');
    for (int i = 0; i < num; ++i) {
        ACL_REQUIRES_NOT_NULL_RET_STR(descArr[i]);
        descStr.append(descArr[i]->GetKey());
        descStr.push_back('|');
    }
    return descStr;
}

template<typename T>
aclError AclOpMap<T>::Aging(T &agingT)
{
    typename ModelMap::iterator itTypeMin;
    typename InputMap::iterator itInputMin;
    typename OutputMap::iterator itOutputMin;
    typename AttrMap::iterator itAttrMin;
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
                    auto &opVec = itAttr->second;
                    for (size_t i = 0; i < opVec.size(); ++i) {
                        if (opVec[i].second->timestamp < timestampMin) {
                            timestampMin = opVec[i].second->timestamp;
                            itTypeMin = itType;
                            itInputMin = itInput;
                            itOutputMin = itOutput;
                            itAttrMin = itAttr;
                            index = i;
                            found = true;
                        }
                    }
                }
            }
        }
    }
    if (!found) {
        ACL_LOG_WARN("AclOpMap::Aging IN, can not find minimum value");
        return ACL_SUCCESS;
    }
    ACL_LOG_INFO("AclOpMap::Aging IN, type = %s, input = %s, output = %s, digest = %zu",
        itTypeMin->first.c_str(), itInputMin->first.c_str(), itOutputMin->first.c_str(), itAttrMin->first);
    auto &modelVec = itAttrMin->second;
    if (index >= modelVec.size()) {
        ACL_LOG_WARN("AclOpMap::Aging IN, index %zu is larger than vec size %zu", index, modelVec.size());
        return ACL_SUCCESS;
    }
    agingT = modelVec[index].second;
    ACL_LOG_INFO("AclOpMap::Aging model in model map success, time stamp is %lu", agingT->timestamp);
    modelVec.erase(modelVec.begin() + index);
    --count;
    if (modelVec.empty()) {
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

    // remove model in hash map while time stamp in hash map is equal to model map 
    bool foundHash = false;
    for (auto hashMapIter = hashMap_.begin(); hashMapIter != hashMap_.end(); ++hashMapIter) {
        for (auto vecIter = hashMapIter->second.begin(); vecIter != hashMapIter->second.end(); ++vecIter) {
            ACL_LOG_INFO("AclOpMap::seed is %zu, time stamp is %lu", hashMapIter->first, (*vecIter)->timestamp);
            if ((*vecIter).get() == agingT.get()) {
                ACL_LOG_INFO("AclOpMap::Aging model in hash map success, hash seed is %zu, hash time "
                             "stamp is %zu, aging time stamp is %zu",
                             hashMapIter->first, (*vecIter)->timestamp, agingT->timestamp);
                hashMapIter->second.erase(vecIter);
                foundHash = true;
                break;
            }
        }
        if (foundHash && hashMapIter->second.empty()) {
            ACL_LOG_INFO("AclOpMap::After delete model, hash map empty while seed is %zu, delete seed in HashMap", 
                hashMapIter->first);
            hashMap_.erase(hashMapIter);
            break;
        }
    }
    if (!foundHash) {
        ACL_LOG_ERROR("[Check][foundHash]AclOpMap::Aging model with time stamp is %lu, ModelMap has aging model "
            "while HashMap has node aging", agingT->timestamp);
        return ACL_ERROR_FAILURE;
    }
    return ACL_SUCCESS;
}

template<typename T>
void AclOpMap<T>::Updatetimestamp(T &entry)
{
    if (entry->timestamp == ULLONG_MAX) {
        ACL_LOG_INFO("Updatetimestamp IN, no need to update timestamp");
        return;
    }
    entry->timestamp = attr_utils::GetCurrentTimestamp();
    ACL_LOG_INFO("Updatetimestamp IN, timestamp = %lu", entry->timestamp);
}

template<typename T>
aclError AclOpMap<T>::AddMemAndAging(std::vector<std::pair<aclopAttr, T>> &modelVec,
                                 const aclopAttr &attr, const T &entry, T &agingT, size_t &seed)
{
    // Check if it needs to be overwritten
    bool found = false;
    T foundEntry;
    for (std::pair<aclopAttr, T> &attr_and_entry : modelVec) {
        if (attr_utils::OpAttrEquals(&attr_and_entry.first, &attr)) {
            ACL_LOG_DEBUG("Override entry with same op_desc in ModelMap");
            foundEntry = attr_and_entry.second;
            attr_and_entry.second = entry;
            found = true;
            break;
        }
    }
    if (found) {
        bool foundHash = false;
        for (auto hashMapIter = hashMap_.begin(); hashMapIter != hashMap_.end(); ++hashMapIter) {
            for (auto vecIter = hashMapIter->second.begin(); vecIter != hashMapIter->second.end(); ++vecIter) {
                ACL_LOG_INFO("AclOpMap::seed is %zu, time stamp is %lu", hashMapIter->first, (*vecIter)->timestamp);
                if ((*vecIter).get() == foundEntry.get()) {
                    foundHash = true;
                    *vecIter = entry;
                    ACL_LOG_DEBUG("Override entry with same op_desc in HashMap");
                    return ACL_SUCCESS;
                }
            }
        }
        if (!foundHash) {
            ACL_LOG_ERROR("[Check][foundHash]AclOpMap::Override entry with same op_desc in ModelMap while not found in "
                          "HashMap");
            return ACL_ERROR_FAILURE;
        }
    }

    // Insert directly
    modelVec.emplace_back(attr, entry);
    hashMap_[seed].emplace_back(entry);
    ACL_LOG_INFO("AclOpMap::Insert aclOp into HashMap success, seed = %zu", seed);

    ++count;
    if ((entry->timestamp == ULLONG_MAX) || (count <= maxOpNum)) {
        ACL_LOG_INFO("AclOpMap::AddMemAndAging in, count is %llu, maxOpNum is %llu, no need aging", count, maxOpNum);
        return ACL_SUCCESS;
    }
    ACL_LOG_INFO("AclOpMap::time stamp is %lu, count is %lu, maxOpNum is %lu, start aging", 
        entry->timestamp, count, maxOpNum);
    return Aging(agingT);
}

template<typename T>
aclError AclOpMap<T>::Insert(const AclOp &aclOp, const T &entry, T &agingT)
{
    ACL_LOG_DEBUG("AclOpMap::Insert IN, aclOp = %s", aclOp.DebugString().c_str());
    string inputDescStr = TensorDescArr2Str(aclOp.numInputs, aclOp.inputDesc);
    string outputDescStr = TensorDescArr2Str(aclOp.numOutputs, aclOp.outputDesc);
    
    size_t digest = 0;
    auto opAttr = aclOp.opAttr;
    aclopAttr emptyAttr; 
    if (aclOp.opAttr != nullptr) {
        if (!attr_utils::SaveConstToAttr(aclOp, const_cast<aclopAttr *>(opAttr))) {
            ACL_LOG_ERROR("[Save][ConstData]save const data buffer to attr fail");
            return ACL_ERROR_FAILURE;
        }
        digest = attr_utils::AttrMapToDigest(opAttr->Attrs());
    } else {
        if (!attr_utils::SaveConstToAttr(aclOp, &emptyAttr)) {
            ACL_LOG_ERROR("[Save][ConstData]save const data buffer to attr fail");
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
        std::lock_guard<std::mutex> lk(mutex_);
        auto &modelVec = entries_[aclOp.opType][inputDescStr][outputDescStr][digest];
        AddMemAndAging(modelVec, *opAttr, entry, agingT, seed);
        ACL_LOG_INFO("AclShapeRangeMap::Insert success, seed = %zu, aclOp = %s", seed, aclOp.DebugString().c_str());
    }
    return ACL_SUCCESS;
}

template<typename T>
aclError AclOpMap<T>::Get(const AclOp &aclOp, T &entry, bool needUpdateTimestamp)
{
    auto opAttr = aclOp.opAttr;
    size_t digest = 0;
    aclopAttr emptyAttr;
    if (opAttr != nullptr) {
        digest = aclOp.opAttr->GetDigest();
        if (!attr_utils::SaveConstToAttr(aclOp, const_cast<aclopAttr *>(opAttr))) {
            ACL_LOG_ERROR("[Save][ConstData]save const data buffer to attr fail");
            return ACL_ERROR_INVALID_PARAM;
        }
    } else {
        if (!attr_utils::SaveConstToAttr(aclOp, &emptyAttr)) {
            ACL_LOG_ERROR("[Save][ConstData]save const data buffer to attr fail");
            return ACL_ERROR_INVALID_PARAM;
        }
        opAttr = &emptyAttr;
    }
    size_t seed = 0;
    ACL_REQUIRES_OK(hash_utils::GetAclOpHash(aclOp, digest, seed));
    std::lock_guard<std::mutex> lk(mutex_);
    auto iter = hashMap_.find(seed);
    if (iter == hashMap_.end()) {
        ACL_LOG_WARN("Get aclOp from aclOpMap failed due to hashMap_ is empty when seed = %zu, aclOp = %s", 
            seed, aclOp.DebugString().c_str());
        return ACL_ERROR_OP_NOT_FOUND;
    } else if (iter->second.size() == 1) {
        if (needUpdateTimestamp) {
            Updatetimestamp(iter->second.back());
        }
        // should use local variable opAttr due to we create emptyAttr object when aclOp.opAttr is nullptr
        if (hash_utils::CheckModelAndAttrMatch(aclOp, opAttr, iter->second.back())) {
            ACL_LOG_INFO("Get aclOp from aclOpMap success! seed = %zu, aclOp = %s", seed, aclOp.DebugString().c_str());
            entry = iter->second.back();
            return ACL_SUCCESS;
        } else {
            ACL_LOG_WARN("Get aclOp from aclOpMap failed due to CheckModelMatch failed! seed = %zu, aclOp = %s",
                 seed, aclOp.DebugString().c_str());
            return ACL_ERROR_OP_NOT_FOUND;
        }
    } else {
        ACL_LOG_INFO("Match aclOp by string from aclOpMap due to seed has conflict! seed = %zu, aclOp = %s", 
            seed, aclOp.DebugString().c_str());
        const string &opType = aclOp.opType;
        string inputDescStr = TensorDescArr2Str(aclOp.numInputs, aclOp.inputDesc);
        string outputDescStr = TensorDescArr2Str(aclOp.numOutputs, aclOp.outputDesc);
        
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

        // matches by attr
        auto &filteredByOutputs = iter2->second;
        auto iter3 = filteredByOutputs.find(digest);
        if (iter3 == filteredByOutputs.end()) {
            ACL_LOG_WARN("Match attr by digest failed. user input attr = %s",
                opAttr == nullptr ? "None" : opAttr->DebugString().c_str());
            return ACL_ERROR_OP_ATTR_NOT_MATCH;
        }

        auto &entries = iter3->second;
        T *matchedByAttr = nullptr;
        for (auto &attrAndValue : entries) {
            if (attr_utils::OpAttrEquals(&attrAndValue.first, opAttr)) {
                matchedByAttr = &attrAndValue.second;
                break;
            }
        }

        if (matchedByAttr == nullptr) {
            ACL_LOG_WARN("Match op attr or constData failed. opType = %s, attr = %s",
                opType.c_str(), opAttr == nullptr ? "None" : opAttr->DebugString().c_str());
            return ACL_ERROR_OP_ATTR_NOT_MATCH;
        }
        if (needUpdateTimestamp) {
            Updatetimestamp(*matchedByAttr);
        }
        entry = *matchedByAttr;
        return ACL_SUCCESS;
    }
    
}
} // namespace acl

#endif // ACL_UTILS_ACL_OP_MAP_H
