/**
* @file ge_tensor_cache.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "ge_tensor_cache.h"

namespace acl {
const size_t GE_TENSOR_CACHE_MAX_SIZE = 10000U;

GeTensorDescCache& GeTensorDescCache::GetInstance()
{
    static GeTensorDescCache inst;
    return inst;
}

GeTensorDescCache::~GeTensorDescCache()
{
    for (size_t i = 0U; i < descCache_.size(); ++i) {
        if (descCache_[i] != nullptr) {
            delete descCache_[i];
        }
    }
    descCache_.clear();
}

GeTensorDescVecPtr GeTensorDescCache::GetDescVecPtr(size_t size)
{
    GeTensorDescVecPtr ptr = nullptr;
    {
        const std::lock_guard<std::mutex> lk(cacheMutex_);
        for (size_t i = 0U; i < descCache_.size(); ++i) {
            GeTensorDescVecPtr &it = descCache_[i];
            if (it == nullptr) {
                continue;
            }
            if (it->size() != size) {
                continue;
            }
            ptr = it;
            it = nullptr;
            return ptr;
        }
    }
    ptr = new(std::nothrow) std::vector<ge::GeTensorDesc>(size);
    return ptr;
}

void GeTensorDescCache::ReleaseDescVecPtr(const GeTensorDescVecPtr ptr)
{
    const std::lock_guard<std::mutex> lk(cacheMutex_);
    for (size_t i = 0U; i < descCache_.size(); ++i) {
        GeTensorDescVecPtr &it = descCache_[i];
        if (it == nullptr) {
            it = ptr;
            return;
        }
    }
    if (descCache_.size() >= GE_TENSOR_CACHE_MAX_SIZE) {
        delete ptr;
    } else {
        descCache_.emplace_back(ptr);
    }
}
} // namespace acl
