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
bool IsAllTensorEmpty(int size, const aclTensorDesc *const *arr)
{
    if (size == 0) {
        return false;
    }

    for (int32_t idx = 0; idx < size; ++idx) {
        bool flag = false;
        for (size_t idy = 0; idy < arr[idx]->dims.size(); ++idy) {
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

bool IsAllTensorEmpty(int size, const aclDataBuffer *const *arr)
{
    if (size == 0) {
        return false;
    }

    for (int32_t idx = 0; idx < size; ++idx) {
        if (arr[idx]->length > 0) {
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
    int32_t end = -1;
    std::string lastName;
    for (int32_t i = 0; i < size; ++i) {
        std::string curName = arr[i]->dynamicInputName;
        if (curName.size() == 0 && lastName.size() == 0) {
            continue;
        }

        if (curName != lastName) {
            // current name exist before
            if (attrNameSet.find(curName) != attrNameSet.end()) {
                return false;
            }

            // valid attr name
            if (lastName.size() > 0) {
                startVec.emplace_back(start);
                endVec.emplace_back(end);
                attrNameSet.insert(lastName);
            }

            if (curName.size() > 0) {
                start = i;
                end = i;
            }
        }
        end = i; // refresh end index
        lastName = curName; // refresh lastName
    }

    if (lastName.size() > 0) {
        startVec.emplace_back(start);
        endVec.emplace_back(end);
    }

    return true;
}

aclError IsHostMemTensorDesc(int size, const aclTensorDesc *const *arr)
{
    if (size == 0) {
        return ACL_SUCCESS;
    }
    ACL_REQUIRES_NOT_NULL(arr);
    for (int idx = 0; idx < size; ++idx) {
        if ((!arr[idx]->IsConstTensor()) && (arr[idx]->IsHostMemTensor())) {
            ACL_LOG_INNER_ERROR("[Check][HostMemTensorDesc]PlaceMent of element at index %d is hostMem", idx);;
            return ACL_ERROR_INVALID_PARAM;
        }
    }

    return ACL_SUCCESS;
}
} // namespace array_utils
} // acl
