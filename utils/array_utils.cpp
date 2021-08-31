/**
* @file array_utils.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "array_utils.h"
#include <map>
#include <vector>
#include <set>
#include "types/tensor_desc_internal.h"

namespace acl {
namespace array_utils {
bool IsAllTensorEmpty(int32_t size, const aclTensorDesc *const *arr)
{
    if (size == 0) {
        return false;
    }

    for (int32_t idx = 0; idx < size; ++idx) {
        bool flag = false;
        for (size_t idy = 0U; idy < arr[idx]->dims.size(); ++idy) {
            if (arr[idx]->dims[idy] == 0) {
                flag = true;
                break;
            }
        }

        if (!flag) {
            return false;
        }
    }

    return true;
}

bool IsAllTensorEmpty(int32_t size, const aclDataBuffer *const *arr)
{
    if (size == 0) {
        return false;
    }

    for (int32_t idx = 0; idx < size; ++idx) {
        if (arr[idx]->length > 0U) {
            return false;
        }
    }

    return true;
}

bool GetDynamicInputIndex(int32_t size, const aclTensorDesc *const *arr, DynamicInputIndexPair &indexPair)
{
    indexPair.first.clear();
    indexPair.second.clear();
    auto &startVec = indexPair.first;
    auto &endVec = indexPair.second;
    if (size == 0) {
        return true;
    }

    std::set<std::string> attrNameSet;
    int32_t start = -1;
    int32_t ended = -1;
    std::string lastName;
    for (int32_t i = 0; i < size; ++i) {
        std::string curName = arr[i]->dynamicInputName;
        if ((curName.size() == 0U) && (lastName.size() == 0U)) {
            continue;
        }

        if (curName != lastName) {
            // current name exist before
            if (attrNameSet.find(curName) != attrNameSet.end()) {
                return false;
            }

            // valid attr name
            if (lastName.size() > 0U) {
                startVec.emplace_back(start);
                endVec.emplace_back(ended);
                (void)attrNameSet.insert(lastName);
            }

            if (curName.size() > 0U) {
                start = i;
                ended = i;
            }
        }
        ended = i; // refresh ended index
        lastName = curName; // refresh lastName
    }

    if (lastName.size() > 0U) {
        startVec.emplace_back(start);
        endVec.emplace_back(ended);
    }

    return true;
}

aclError IsHostMemTensorDesc(int32_t size, const aclTensorDesc *const *arr)
{
    if (size == 0) {
        return ACL_SUCCESS;
    }
    ACL_REQUIRES_NOT_NULL(arr);
    for (int32_t idx = 0; idx < size; ++idx) {
        if ((!arr[idx]->IsConstTensor()) && (arr[idx]->IsHostMemTensor())) {
            ACL_LOG_INNER_ERROR("[Check][HostMemTensorDesc]PlaceMent of element at index %d is hostMem", idx);;
            return ACL_ERROR_INVALID_PARAM;
        }
    }

    return ACL_SUCCESS;
}
} // namespace array_utils
} // acl
