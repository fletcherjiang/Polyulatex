#include "ge_tensor_cache.h"

namespace acl {
const size_t MAX_SIZE = 10000U;

GeTensorDescCache& GeTensorDescCache::GetInstance()
{
    static GeTensorDescCache inst;
    return inst;
}

GeTensorDescVecPtr GeTensorDescCache::GetDescVecPtr(size_t size)
{
    GeTensorDescVecPtr ptr = nullptr;
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
    ptr = new(std::nothrow) std::vector<ge::GeTensorDesc>(size);
    return ptr;
}

void GeTensorDescCache::ReleaseDescVecPtr(const GeTensorDescVecPtr ptr)
{
    std::lock_guard<std::mutex> lk(cacheMutex_);
    for (size_t i = 0U; i < descCache_.size(); ++i) {
        GeTensorDescVecPtr &it = descCache_[i];
        if (it == nullptr) {
            it = ptr;
            return;
        }
    }
    descCache_.emplace_back(ptr);
    if (descCache_.size() >= MAX_SIZE) {
        std::vector<GeTensorDescVecPtr>().swap(descCache_);
    }
}
} //namespace acl