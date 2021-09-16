/**
* @file ge_tensor_cache.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2021-2021. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ACL_OP_GE_TENSOR_CACHE_H
#define ACL_OP_GE_TENSOR_CACHE_H

#include <vector>
#include <mutex>
#include "graph/tensor.h"
#include "graph/ge_tensor.h"
#include "graph/utils/attr_utils.h"

namespace acl {

inline void WrapGeShape(ge::GeShape &geShape, const std::vector<int64_t>&dims)
{
    geShape.SetDimNum(dims.size());
    for (size_t i = 0U; i < dims.size(); ++i) {
        geShape.SetDim(i, dims[i]);
    }
}

using GeTensorDescVecPtr = std::vector<ge::GeTensorDesc>*;

class GeTensorDescCache {
public:
    GeTensorDescCache() = default;
    ~GeTensorDescCache();
    // must use GetInstance when we need use this calss, otherwise memory error may happen
    static GeTensorDescCache& GetInstance();
    GeTensorDescVecPtr GetDescVecPtr(size_t size);
    void ReleaseDescVecPtr(const GeTensorDescVecPtr ptr);
private:
    std::vector<GeTensorDescVecPtr> descCache_;
    std::mutex cacheMutex_;
};
} // namespace acl

#endif // ACL_OP_GE_TENSOR_CACHE_H