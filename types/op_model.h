/**
* @file op_model.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ACL_TYPES_OP_MODEL_H_
#define ACL_TYPES_OP_MODEL_H_

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <climits>

#include "graph/ge_attr_value.h"
#include "acl/acl_base.h"
#include "types/tensor_desc_internal.h"
#include "types/op_attr.h"
#include "types/acl_op.h"


namespace acl {
struct OpModel {
    OpModel() = default;

    ~OpModel() = default;

    std::shared_ptr<void> data;
    uint32_t size = 0;
    std::string name;
    uint64_t opModelId = 0;
    size_t isStaticModelWithFuzzCompile = 0;
};

struct OpModelDef {
    std::string opType;
    std::vector<aclTensorDesc> inputDescArr;
    std::vector<aclTensorDesc> outputDescArr;
    aclopAttr opAttr;

    std::string modelPath;
    // 0: ACL_OP_COMPILE_DEFAULT mode
    // 1：ACL_OP_COMPILE_FUZZ mode but model is static
    // 2：ACL_OP_COMPILE_FUZZ mode and model is dynamic
    size_t isStaticModelWithFuzzCompile = 0;

    std::string DebugString() const;

    uint64_t timestamp = ULLONG_MAX;
};

ACL_FUNC_VISIBILITY aclError ReadOpModelFromFile(const std::string &path, OpModel &opModel);
} // namespace acl

#endif // ACL_TYPES_OP_MODEL_H_
