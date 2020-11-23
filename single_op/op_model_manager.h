/**
* @file op_model_manager.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_OP_EXEC_OP_MODEL_MANAGER_H_
#define ACL_OP_EXEC_OP_MODEL_MANAGER_H_

#include "acl/acl.h"

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>

#include "graph/ge_attr_value.h"
#include "mmpa/mmpa_api.h"
#include "types/op_model.h"
#include "types/acl_op.h"
#include "op_model_cache.h"
#include "compile/op_compiler.h"
#include "utils/acl_op_map.h"
#include "utils/acl_dynamic_shape_op_map.h"

namespace acl {
struct aclTensorShapeStatus {
    bool isUnkownRank = false;
    std::vector<bool> shapeStatus;
};

class ACL_FUNC_VISIBILITY OpModelManager {
public:
    ~OpModelManager() = default;

    static OpModelManager &GetInstance()
    {
        static OpModelManager instance;
        return instance;
    }

    aclError LoadAllModels(const std::string &modelDir);

    aclError LoadModelFromMem(const void *model, size_t modelSize, bool isStatic = false);

    aclError LoadModelFromSharedMem(std::shared_ptr<void> &model, size_t modelSize, bool isStatic = false);

    aclError GetOpModel(AclOp &aclOp);

    aclError MatchOpModel(const AclOp &aclOp, OpModel &opModel, bool &isDynamic);

    aclError HandleMaxOpQueueConfig(const char *configPath);

    aclError SetHostMemToConst(AclOp &aclopHostMemToConst, bool &isExistConst);

    aclError SetTensorConst(aclTensorDesc *desc, const aclDataBuffer *hostMem);

    void SetCompileFlag(int32_t flag);

private:
    OpModelManager() = default;

    using ModelMap = AclOpMap<std::shared_ptr<OpModelDef>>;

    using DynamicModelMap = AclShapeRangeMap<std::shared_ptr<OpModelDef>>;

    aclError RegisterModel(OpModelDef &&modelConfig,
                           ModelMap &opModelDefs,
                           DynamicModelMap &opDynamicModelDefs,
                           bool isDynamic,
                           bool isStaticRegister = true);

    aclError ReadModelDefs(const std::string &configPath,
                            std::vector<OpModelDef> &configList);

    aclError BuildOpModel(const AclOp &aclOp);

    static bool MatchModelDefByAttr(const OpModelDef &modelDef, const aclopAttr *attr);

    static bool OmFileFilterFn(const std::string &fileName);

    static bool IsDynamicOpModel(const OpModelDef &modelDef);

    static bool IsDynamicOpModel(const AclOp &aclOp);

    void SetTensorShapeStatus(const AclOp &aclOp, std::vector<aclTensorShapeStatus> &shapeStatus);
    void SetTensorShapeRange(const AclOp &aclOp, const std::vector<aclTensorShapeStatus> &tensorShapeStatus);
    void GetTensorShapeStatus(const AclOp &aclOp, std::vector<std::vector<aclTensorShapeStatus>> &shapeStatus);
    void GetTensorShapeRange(const std::vector<aclTensorShapeStatus> &tensorShapeStatus,
                             std::vector<std::vector<std::pair<int64_t, int64_t>>> &shapeRanges);

    static std::string TensorStatusToStr(const std::vector<aclTensorShapeStatus> &tensorShapeStatus);
    static bool CheckShapeRange(const AclOp &aclOp,
                                const std::vector<aclTensorShapeStatus> &tensorShapeStatus,
                                const std::vector<std::pair<int64_t, int64_t>> &shapeRange);
    static void FixedAclopMatch(AclOp &aclOpMatch,
                                const std::vector<aclTensorShapeStatus> &tensorShapeStatus,
                                const std::vector<std::pair<int64_t, int64_t>> &shapeRange,
                                std::vector<std::vector<int64_t>> &tensorDims,
                                std::vector<int64_t> &storageTensorDims);
    static bool BackAclopMatch(AclOp &aclOpMatch,
                               const std::vector<aclTensorShapeStatus> &tensorShapeStatus,
                               const std::vector<std::vector<int64_t>> &tensorDims,
                               const std::vector<int64_t> &storageTensorDims);
    aclError MatchStaticOpModel(const AclOp &aclOp, OpModel &opModel, bool &isDynamic, bool &isNeedMatchDymaic);
    aclError MatchDynamicOpModel(const AclOp &aclOp, OpModel &opModel, bool &isDynamic);

private:
    ModelMap opModels_;
    DynamicModelMap dynamicOpModels_;
    ModelMap onlineCompiledModels_;
    OpModelCache modelCache_;
    OpModelCache dynamicModelCache_;
    std::uint32_t counter_ = 0;

    std::mutex shapeStatusMutex_;
    std::unordered_map<std::string, std::vector<std::vector<aclTensorShapeStatus>>> tensorShapeStatus_;
    std::unordered_map<std::string, std::vector<std::string>> tensorShapeStatusDesc_;
    std::mutex tensorShapeMutex_;
    std::unordered_map<std::string, std::vector<std::vector<std::pair<int64_t, int64_t>>>> tensorShapeRange_;
};
} // namespace acl

#endif // ACL_OP_EXEC_OP_MODEL_MANAGER_H_
