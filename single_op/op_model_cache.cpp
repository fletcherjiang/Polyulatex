/**
* @file op_model_cache.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "op_model_cache.h"

#include "framework/common/util.h"

namespace {
    std::atomic<std::uint64_t> atomicId(0U);
}

namespace acl {
aclError OpModelCache::GetOpModel(const OpModelDef &modelDef, OpModel &model)
{
    auto &key = modelDef.modelPath;
    ACL_LOG_INFO("start to execute GetOpModel, modelPath = %s, key = %p", modelDef.modelPath.c_str(), &modelDef);
    {
        std::lock_guard<std::mutex> locker(mutex_);
        auto iter = cachedModels_.find(key);
        if (iter != cachedModels_.end()) {
            model = iter->second;
            ACL_LOG_INFO("GetOpModel success, modelPath = %s", modelDef.modelPath.c_str());
            return ACL_SUCCESS;
        }
    }
    ACL_LOG_INNER_ERROR("[Get][OpModel]GetOpModel fail, modelPath = %s", modelDef.modelPath.c_str());
    return ACL_ERROR_FAILURE;
}

aclError OpModelCache::Add(const OpModelDef &modelDef, OpModel &model)
{
    ACL_LOG_INFO("start to execute OpModelCache::Add, modelPath = %s, key = %p", modelDef.modelPath.c_str(), &modelDef);
    auto key = modelDef.modelPath;
    std::lock_guard<std::mutex> locker(mutex_);
    model.opModelId = atomicId++;
    cachedModels_[key] = model;
    return ACL_SUCCESS;
}

aclError OpModelCache::Delete(const OpModelDef &modelDef)
{
    ACL_LOG_INFO("start to execute OpModelCache::Delete, modelPath = %s", modelDef.modelPath.c_str());
    auto key = modelDef.modelPath;
    std::lock_guard<std::mutex> locker(mutex_);
    (void)cachedModels_.erase(key);
    return ACL_SUCCESS;
}
} // namespace acl
