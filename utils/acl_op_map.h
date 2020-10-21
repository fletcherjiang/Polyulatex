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

namespace acl {
template<typename T>
class AclOpMap {
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
    void Aging(T &agingT);
    void Updatetimestamp(T &entry);
    void AddMemAndAging(std::vector<std::pair<aclopAttr, T>> &modelVec,
                        const aclopAttr &attr, const T &entry, T &agingT);

    using AttrMap = std::map<size_t, std::vector<std::pair<aclopAttr, T>>>;
    using OutputMap = std::map<std::string, AttrMap>;
    using InputMap = std::map<std::string, OutputMap>;
    using ModelMap = std::map<std::string, InputMap>;

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
void AclOpMap<T>::Aging(T &agingT)
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
        return;
    }
    ACL_LOG_INFO("AclOpMap::Aging IN, type = %s, input = %s, output = %s, digest = %zu",
                 itTypeMin->first.c_str(), itInputMin->first.c_str(), itOutputMin->first.c_str(), itAttrMin->first);
    auto &modelVec = itAttrMin->second;
    if (index >= modelVec.size()) {
        ACL_LOG_WARN("AclOpMap::Aging IN, index %zu is larger than vec size %zu", index, modelVec.size());
        return;
    }
    agingT = modelVec[index].second;
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
    return;
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
void AclOpMap<T>::AddMemAndAging(std::vector<std::pair<aclopAttr, T>> &modelVec,
                                 const aclopAttr &attr, const T &entry, T &agingT)
{
    modelVec.emplace_back(attr, entry);
    ++count;
    if ((entry->timestamp == ULLONG_MAX) || (count <= maxOpNum)) {
        ACL_LOG_INFO("AclOpMap::AddMemAndAging in, count is %llu, maxOpNum is %llu, no need aging", count, maxOpNum);
        return;
    }
    Aging(agingT);
}

template<typename T>
void AclOpMap<T>::Insert(const AclOp &aclOp, const T &entry, T &agingT)
{
    ACL_LOG_DEBUG("AclOpMap::Insert IN, aclOp = %s", aclOp.DebugString().c_str());
    string inputDescStr = TensorDescArr2Str(aclOp.numInputs, aclOp.inputDesc);
    string outputDescStr = TensorDescArr2Str(aclOp.numOutputs, aclOp.outputDesc);
    if (aclOp.opAttr != nullptr) {
        if (!attr_utils::SaveConstToAttr(aclOp, const_cast<aclopAttr *>(aclOp.opAttr))) {
            ACL_LOG_ERROR("[Save][ConstData]save const data buffer to attr fail");
            return;
        }
        size_t digest = attr_utils::AttrMapToDigest(aclOp.opAttr->Attrs());
        std::lock_guard<std::mutex> lk(mutex_);
        auto &modelVec = entries_[aclOp.opType][inputDescStr][outputDescStr][digest];
        bool found = false;
        for (std::pair<aclopAttr, T> &attr_and_entry : modelVec) {
            if (attr_utils::OpAttrEquals(&attr_and_entry.first, aclOp.opAttr)) {
                ACL_LOG_DEBUG("Override entry with same op_desc, aclOp = %s", aclOp.DebugString().c_str());
                attr_and_entry.second = entry;
                found = true;
                break;
            }
        }
        if (!found) {
            ACL_LOG_DEBUG("Adding new entry, aclOp = %s", aclOp.DebugString().c_str());
            AddMemAndAging(modelVec, *aclOp.opAttr, entry, agingT);
        }
    } else {
        aclopAttr emptyAttr;
        if (!attr_utils::SaveConstToAttr(aclOp, &emptyAttr)) {
            ACL_LOG_ERROR("[Save][ConstData]save const data buffer to attr fail");
            return;
        }
        size_t digest = attr_utils::AttrMapToDigest(emptyAttr.Attrs());
        std::lock_guard<std::mutex> lk(mutex_);
        auto &modelVec = entries_[aclOp.opType][inputDescStr][outputDescStr][digest];
        if (modelVec.empty()) {
            ACL_LOG_DEBUG("Adding new entry, aclOp = %s", aclOp.DebugString().c_str());
            AddMemAndAging(modelVec, emptyAttr, entry, agingT);
        } else {
            ACL_LOG_DEBUG("Override entry with same op_desc, aclOp = %s", aclOp.DebugString().c_str());
            modelVec[0].second = entry;
        }
    }
}

template<typename T>
aclError AclOpMap<T>::Get(const AclOp &aclOp, T &entry, bool needUpdateTimestamp)
{
    const string &opType = aclOp.opType;
    auto *opAttr = aclOp.opAttr;
    string inputDescStr = TensorDescArr2Str(aclOp.numInputs, aclOp.inputDesc);
    string outputDescStr = TensorDescArr2Str(aclOp.numOutputs, aclOp.outputDesc);
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
} // namespace acl

#endif // ACL_UTILS_ACL_OP_MAP_H
