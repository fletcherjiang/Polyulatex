/**
* @file hash_utils.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_UTILS_HASH_UTILS_H
#define ACL_UTILS_HASH_UTILS_H
#include "acl/acl_base.h"
#include "utils/string_utils.h"
#include "utils/attr_utils.h"
#include "types/op_attr.h"
#include "types/acl_op.h"
#include "types/tensor_desc_internal.h"
#include "common/log_inner.h"

namespace acl {
namespace hash_utils {

template <class T>
inline void HashCombine(size_t &seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

aclError GetTensorDescHash(const int32_t num, const aclTensorDesc *const descArr[], size_t &seed);

aclError GetAclOpHash(const AclOp &aclOp, const size_t &attrDigest, size_t &seed);

template<typename T>
bool CheckModelMatch(const AclOp &aclOp, const T &entry)
{
    ACL_REQUIRES_NOT_NULL(entry);
    if(aclOp.opType != entry->opType) {
        return false;
    }

    ACL_CHECK_EQUAL(aclOp.numInputs, entry->inputDescArr.size());
    for (int32_t i = 0; i < aclOp.numInputs; ++i) {
        if (!(entry->inputDescArr[i] == aclOp.inputDesc[i])) {
            ACL_LOG_INFO("Check inputDescArr failed!");
            return false;
        }
    }

    ACL_CHECK_EQUAL(aclOp.numOutputs, entry->outputDescArr.size());
    for (int32_t i = 0; i < aclOp.numOutputs; ++i) {
        if (!(entry->outputDescArr[i] == aclOp.outputDesc[i])) {
            ACL_LOG_INFO("Check outputDescArr failed!");
            return false;
        }
    }
    if (!attr_utils::OpAttrEquals(aclOp.opAttr, &(entry->opAttr))) {
        ACL_LOG_INFO("Check attr failed!");
        return false;
    }
    return true;
}

} // namespace hash_utils
} // namespace acl

#endif // ACL_UTILS_HASH_UTILS_H