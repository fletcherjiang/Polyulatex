/**
* @file hash_utils.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "hash_utils.h"

namespace acl {
namespace hash_utils {
// use aclTensorDesc to calculate a hash seed according to HashCombine function
aclError GetTensorDescHash(const int32_t num, const aclTensorDesc *const descArr[], size_t &seed)
{
    ACL_LOG_DEBUG("GetTensorDescHash in, num = %d, seed = %zu", num, seed);
    if ((num > 0) && (descArr == nullptr)) {
        ACL_LOG_ERROR("[Check][Params] param descArr must not be null");
        return ACL_ERROR_FAILURE;
    }
    HashCombine(seed, num);
    for (int32_t i = 0; i < num; ++i) {
        ACL_REQUIRES_NOT_NULL(descArr[i]);
        HashCombine(seed, static_cast<int32_t>(descArr[i]->dataType));
        HashCombine(seed, static_cast<int32_t>(descArr[i]->format));
        if (descArr[i]->storageFormat != ACL_FORMAT_UNDEFINED) {
            HashCombine(seed, static_cast<int32_t>(descArr[i]->storageFormat));
        }
        ACL_LOG_DEBUG("i = %d, dims size is %zu", i, descArr[i]->dims.size());
        for (auto &dim : descArr[i]->dims) {
            HashCombine(seed, dim);
        }
        ACL_LOG_DEBUG("i = %d, shapeRange size is %zu", i, descArr[i]->shapeRange.size());
        for (auto &shapeRange : descArr[i]->shapeRange) {
            HashCombine(seed, shapeRange.first);
            HashCombine(seed, shapeRange.second);
        }
        ACL_LOG_DEBUG("i = %d, isConst is %d", i, static_cast<int32_t>(descArr[i]->isConst));
        HashCombine(seed, descArr[i]->isConst); 
        ACL_LOG_DEBUG("i = %d, memtype is %d", i, static_cast<int32_t>(descArr[i]->memtype));
        HashCombine(seed, static_cast<int32_t>(descArr[i]->memtype));
    }
    ACL_LOG_DEBUG("GetTensorDescHash out, seed = %zu", seed);
    return ACL_SUCCESS;
}

aclError GetAclOpHash(const AclOp &aclOp, const size_t &attrDigest, size_t &seed)
{
    // Init seed to maintain consistency between Insert and Get
    seed = 0;

    ACL_LOG_DEBUG("GetAclOpHash start, aclOp is %s", aclOp.DebugString().c_str());
    HashCombine(seed, aclOp.opType);
    
    ACL_REQUIRES_OK(GetTensorDescHash(aclOp.numInputs, aclOp.inputDesc, seed));
    ACL_LOG_DEBUG("After Input GetTensorDescHash, seed = %zu", seed);

    ACL_REQUIRES_OK(GetTensorDescHash(aclOp.numOutputs, aclOp.outputDesc, seed));
    ACL_LOG_DEBUG("After Output GetTensorDescHash, seed = %zu", seed);

    HashCombine(seed, attrDigest);
    if (aclOp.opAttr != nullptr) {
        for (auto &constBuf : aclOp.opAttr->GetConstBuf()) {
            HashCombine(seed, constBuf);
        }
    }
    ACL_LOG_DEBUG("After get attr combine hash, seed = %zu", seed);

    return ACL_SUCCESS;
}
} // namespace hash_utils
} // namespace acl