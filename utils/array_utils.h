/**
* @file array_utils.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_UTILS_ARRAY_UTILS_H
#define ACL_UTILS_ARRAY_UTILS_H

#include <utility>
#include <vector>
#include <map>
#include "common/log_inner.h"

namespace acl {
namespace array_utils {
using DynamicInputIndexPair = std::pair<std::vector<int32_t>, std::vector<int32_t>>;
template<typename T>
aclError CheckPtrArray(int size, const T *const *arr)
{
    if (size == 0) {
        return ACL_SUCCESS;
    }

    ACL_REQUIRES_NOT_NULL(arr);
    for (int32_t i = 0; i < size; ++i) {
        if (arr[i] == nullptr) {
            ACL_LOG_ERROR("element at index %d is NULL", i);
            return ACL_ERROR_INVALID_PARAM;
        }
    }

    return ACL_SUCCESS;
}

template<typename T>
aclError CheckPtrArray(int size, T *const *arr)
{
    if (size == 0) {
        return ACL_SUCCESS;
    }

    ACL_REQUIRES_NOT_NULL(arr);
    for (int32_t idx = 0; idx < size; ++idx) {
        if (arr[idx] == nullptr) {
            ACL_LOG_ERROR("element at index %d is NULL", idx);
            return ACL_ERROR_INVALID_PARAM;
        }
    }

    return ACL_SUCCESS;
}

ACL_FUNC_VISIBILITY bool IsAllTensorEmpty(int size, const aclTensorDesc *const *arr);

bool IsAllTensorEmpty(int size, const aclDataBuffer *const *arr);

ACL_FUNC_VISIBILITY aclError IsHostMemTensorDesc(int size, const aclTensorDesc *const *arr);

bool GetDynamicInputIndex(int32_t size, const aclTensorDesc *const *arr, DynamicInputIndexPair &indexPair);

void GetOptionalInputMap(int32_t size, const aclTensorDesc *const *inputDesc,
    std::map<int32_t, bool> &optionalInputMap);

} // namespace array_utils
} // acl
#endif // ACL_UTILS_ARRAY_UTILS_H
