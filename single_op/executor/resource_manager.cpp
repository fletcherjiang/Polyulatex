/**
* @file resource_manager.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "resource_manager.h"
#include "runtime/rt.h"
#include "common/log_inner.h"


namespace acl {
ResourceManager::ResourceManager(aclrtContext theContext) : context_(theContext) {}

ResourceManager::~ResourceManager()
{
    ACL_LOG_INFO("ResourceManager::~ResourceManager IN");
    if (current_mem_ == nullptr) {
        return;
    }

    aclrtContext oldCtx = nullptr;
    (void) aclrtGetCurrentContext(&oldCtx);
    (void) aclrtSetCurrentContext(context_);
    if (current_mem_ != nullptr) {
        (void) aclrtFree(current_mem_);
    }
    for (auto addr : pending_mem_) {
        (void) aclrtFree(addr);
    }
    (void) aclrtSetCurrentContext(oldCtx);
}

aclError ResourceManager::GetMemory(void **address, size_t size)
{
    ACL_REQUIRES_NOT_NULL(address);
    if (current_mem_size_ >= size) {
        ACL_LOG_DEBUG("current memory size[%zu] should be smaller than required memory size[%zu]",
            current_mem_size_, size);
        *address = current_mem_;
        return ACL_SUCCESS;
    }

    ACL_REQUIRES_OK(aclrtMalloc(address, size, ACL_MEM_MALLOC_NORMAL_ONLY));
    pending_mem_.push_back(current_mem_);
    current_mem_ = *address;
    current_mem_size_ = size;

    return ACL_SUCCESS;
}

aclError ResourceManager::ReleasePendingMemory()
{
    for (auto addr : pending_mem_) {
        (void) aclrtFree(addr);
    }

    pending_mem_.clear();
    return ACL_SUCCESS;
}
} // namespace acl