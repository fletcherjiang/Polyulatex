/**
* @file resource_manager.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef TRUNK_PROJ_RESOURCE_MANAGER_H
#define TRUNK_PROJ_RESOURCE_MANAGER_H

#include <vector>
#include <map>
#include <mutex>
#include "acl/acl.h"

namespace acl {
class ResourceManager {
public:
    explicit ResourceManager(aclrtContext context);
    ~ResourceManager();

    ResourceManager(const ResourceManager &) = delete;
    ResourceManager &operator=(const ResourceManager &) = delete;
    ResourceManager &operator=(ResourceManager &&) = delete;
    ResourceManager(ResourceManager &&) = delete;

    aclError GetMemory(void **address, size_t size);
    aclError ReleasePendingMemory();

private:
    std::vector<void *> pending_mem_;
    void *current_mem_ = nullptr;
    size_t current_mem_size_ = 0;

    // used by release proc
    aclrtContext context_;
};
} // namespace acl

#endif //TRUNK_PROJ_RESOURCE_MANAGER_H
