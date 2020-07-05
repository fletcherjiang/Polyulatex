/**
* @file op_model_cache.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_OP_EXEC_OP_MODEL_CACHE_H_
#define ACL_OP_EXEC_OP_MODEL_CACHE_H_

#include <map>
#include <mutex>

#include "types/op_model.h"

namespace acl {
class OpModelCache {
public:
    OpModelCache() = default;

    ~OpModelCache() = default;

    aclError GetOpModel(const OpModelDef &modelDef, OpModel &opModel);

    aclError Add(const OpModelDef &modelDef, OpModel &opModel);

    aclError Delete(const OpModelDef &modelDef);

private:
    std::map<std::string, OpModel> cachedModels_;
    std::map<std::string, OpModel> cachedCompiledModels_;
    std::mutex mutex_;
};
} // namespace acl

#endif // ACL_OP_EXEC_OP_MODEL_CACHE_H_
