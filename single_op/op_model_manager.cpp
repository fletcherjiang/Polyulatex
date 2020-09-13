/**
* @file op_model_manager.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "op_model_manager.h"

#include "types/tensor_desc_internal.h"
#include "common/log_inner.h"
#include "common/common_inner.h"
#include "types/op_attr.h"
#include "json_parser.h"

#include "op_model_parser.h"
#include "utils/attr_utils.h"
#include "utils/file_utils.h"
#include "compile/op_compile_service.h"

using namespace std;

namespace acl {
namespace {
constexpr int OM_FILE_SUFFIX_LEN = 3;
constexpr int OM_DIR_MAX_DEPTH = 3;
constexpr int DECIMAL = 10;
const std::string ACL_MAX_OPQUEUE_NUM = "max_opqueue_num";
}

void OpModelManager::SetCompileFlag(int32_t flag)
{
    SetGlobalCompileFlag(flag);
}

aclError OpModelManager::HandleMaxOpQueueConfig(const char *configPath)
{
    ACL_LOG_INFO("start to execute HandleMaxOpQueueConfig");
    nlohmann::json js;
    acl::JsonParser jsonParser;
    aclError ret = jsonParser.ParseJsonFromFile(configPath, js, nullptr, nullptr);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Parse][Config]parse max_opqueue_num config from file[%s] failed, result = %d",
            configPath, ret);
        return ret;
    }
    try {
        if (js.find(ACL_MAX_OPQUEUE_NUM) != js.end()) {
            std::string maxOpNumStr = js.at(ACL_MAX_OPQUEUE_NUM).get<std::string>();
            long maxOpNum = strtol(maxOpNumStr.c_str(), nullptr, DECIMAL);
            ACL_LOG_EVENT("max_opqueue_num is set [%ld].", maxOpNum);
            if (maxOpNum <= 0) {
                ACL_LOG_INNER_ERROR("[Check][MaxOpNum]max_opqueue_num [%s] is invalid from file[%s], "
                    "it should be larger than 0.", maxOpNumStr.c_str(), configPath);
                return ACL_ERROR_INVALID_PARAM;
            }
            if (static_cast<uint64_t>(maxOpNum) < DEFAULT_MAX_OPQUEUE_NUM) {
                ACL_LOG_WARN("max_opqueue_num [%lu] is less than default value DEFAULT_MAX_OPQUEUE_NUM[%lu], "
                             "it may be low performance.", static_cast<uint64_t>(maxOpNum), DEFAULT_MAX_OPQUEUE_NUM);
            }
            opModels_.SetMaxOpNum(static_cast<uint64_t>(maxOpNum));
            dynamicOpModels_.SetMaxOpNum(static_cast<uint64_t>(maxOpNum));
        } else {
            ACL_LOG_INFO("no max_opqueue_num found, set default DEFAULT_MAX_OPQUEUE_NUM[%lu].",
                DEFAULT_MAX_OPQUEUE_NUM);
        }
    } catch (const nlohmann::json::exception &e) {
        ACL_LOG_INNER_ERROR("[Parse][Json]parse json for max_opqueue_num config failed, exception:%s.", e.what());
        return ACL_ERROR_INVALID_MAX_OPQUEUE_NUM_CONFIG;
    }
    ACL_LOG_INFO("HandleMaxOpQueueConfig end in HandleMaxOpQueueConfig");
    return ACL_SUCCESS;
}

bool OpModelManager::OmFileFilterFn(const std::string &fileName)
{
    auto pos = fileName.rfind(".om");
    if (pos == string::npos) {
        return false;
    }

    return pos == (fileName.size() - OM_FILE_SUFFIX_LEN);
}

bool OpModelManager::IsDynamicOpModel(const OpModelDef &modelDef)
{
    if (modelDef.isStaticModelWithFuzzCompile == 0) {
        // 0: ACL_OP_COMPILE_DEFAULT mode
        ACL_LOG_INFO("the model is compiled by exact compile");
        for (size_t i = 0; i < modelDef.inputDescArr.size(); ++i) {
            if (modelDef.inputDescArr[i].IsDynamicTensor()) {
                return true;
            }
        }
        for (size_t i = 0; i < modelDef.outputDescArr.size(); ++i) {
            if (modelDef.outputDescArr[i].IsDynamicTensor()) {
                return true;
            }
        }
        return false;
    } else if (modelDef.isStaticModelWithFuzzCompile == 1) {
        // 1:ACL_OP_COMPILE_FUZZ mode but model is static
        ACL_LOG_INFO("the model is static model with fuzz compile");
        return false;
    } else {
        // 2:ACL_OP_COMPILE_FUZZ mode and model is dynamic
        ACL_LOG_INFO("the model is dynamic model with fuzz compile");
        return true;
    }
}

bool OpModelManager::IsDynamicOpModel(const AclOp &aclOp)
{
    for (int i = 0; i < aclOp.numInputs; ++i) {
        if (aclOp.inputDesc[i]->IsDynamicTensor()) {
            return true;
        }
    }
    for (int i = 0; i < aclOp.numOutputs; ++i) {
        if (aclOp.outputDesc[i]->IsDynamicTensor()) {
            return true;
        }
    }
    return false;
}

aclError OpModelManager::LoadAllModels(const std::string &modelDir)
{
    ACL_LOG_INFO("LoadAllModels begin. config path = %s", modelDir.c_str());
    std::vector<OpModelDef> defList;
    ACL_REQUIRES_OK(ReadModelDefs(modelDir, defList));

    if (defList.empty()) {
        ACL_LOG_WARN("No model is loaded.");
        return ACL_SUCCESS;
    }

    ACL_LOG_INFO("Found %zu model config", defList.size());
    for (auto &modelDef : defList) {
        bool isDynamic = IsDynamicOpModel(modelDef);
        // it is static load
        ACL_REQUIRES_OK(RegisterModel(std::move(modelDef), opModels_, dynamicOpModels_, isDynamic, true));
    }
    return ACL_SUCCESS;
}

aclError OpModelManager::LoadModelFromMem(const void *model, size_t modelSize, bool isStatic)
{
    ACL_LOG_INFO("Load op model begin. modelSize = %zu", modelSize);
    ACL_REQUIRES_NOT_NULL(model);
    ACL_REQUIRES_POSITIVE(modelSize);
    auto *aclModelData = new (std::nothrow) char[modelSize];
    ACL_REQUIRES_NOT_NULL(aclModelData);
    std::shared_ptr<void> modelData;
    modelData.reset(aclModelData, [](const char *p) { delete[]p; });
    if (memcpy_s(aclModelData, modelSize, model, modelSize) != EOK) {
        ACL_LOG_INNER_ERROR("[Copy][Data]Copy model data failed. size = %zu", modelSize);
        return ACL_ERROR_FAILURE;
    }
    return LoadModelFromSharedMem(modelData, modelSize, isStatic);
}

aclError OpModelManager::LoadModelFromSharedMem(std::shared_ptr<void> &model, size_t modelSize, bool isStatic)
{
    ACL_LOG_INFO("Load inner op model begin. modelSize = %zu", modelSize);
    ACL_REQUIRES_NOT_NULL(model);
    ACL_REQUIRES_POSITIVE(modelSize);
    OpModel opModel;
    opModel.data = model;
    opModel.size = static_cast<uint32_t>(modelSize);
    opModel.name = std::to_string(reinterpret_cast<uintptr_t>(model.get()));
    ACL_LOG_INFO("opModel.name = %s", opModel.name.c_str());

    OpModelDef modelDef;
    modelDef.modelPath = opModel.name;

    auto ret = OpModelParser::ParseOpModel(opModel, modelDef);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_WARN("parse model failed. result = %d. skip it", ret);
        return ret;
    }
    bool isDynamic = IsDynamicOpModel(modelDef);
    if (isDynamic) {
        ACL_LOG_INFO("The model is dynamic shape, the name of opModel is %s", opModel.name.c_str());
        ACL_REQUIRES_OK(dynamicModelCache_.Add(modelDef, opModel));
    } else {
        ACL_LOG_INFO("The model is static shape, the name of opModel is %s", opModel.name.c_str());
        ACL_REQUIRES_OK(modelCache_.Add(modelDef, opModel));
    }
    ACL_REQUIRES_OK(RegisterModel(std::move(modelDef), opModels_, dynamicOpModels_, isDynamic, isStatic));

    return ACL_SUCCESS;
}

aclError OpModelManager::LoadBasicModels(const std::string &modelDir)
{
    ACL_LOG_DEBUG("LoadBasicModels begin.");
    vector<string> modelFilePaths;
    ACL_REQUIRES_OK(file_utils::ListFiles(modelDir, OmFileFilterFn, modelFilePaths, OM_DIR_MAX_DEPTH));
    ACL_LOG_INFO("Found %zu model config", modelFilePaths.size());
    for (auto &path : modelFilePaths) {
        OpModel opModel;
        ACL_REQUIRES_OK(ReadOpModelFromFile(path, opModel));

        OpModelDef modelDef;
        modelDef.modelPath = path;
        auto ret = OpModelParser::ParseOpModel(opModel, modelDef);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_WARN("Parse model failed. result = %d, model = %s. skip it", ret, modelDef.modelPath.c_str());
            continue;
        }

        ACL_LOG_DEBUG("Add op model: %s, model path = %s", modelDef.opType.c_str(), modelDef.modelPath.c_str());
        basicModelMap_.emplace(modelDef.opType, opModel);
    }

    return ACL_SUCCESS;
}

static void SetShapeStatus(int tensorNum,
                           const aclTensorDesc *const tensorDesc[],
                           std::vector<aclTensorShapeStatus> &shapeStatus)
{
    for (int tensorIndex = 0; tensorIndex < tensorNum; ++tensorIndex) {
        aclTensorShapeStatus tensorShapeStatus;
        if ((tensorDesc[tensorIndex]->dims.size() > 0) && (tensorDesc[tensorIndex]->dims[0] == UNKNOW_RANK)) {
            tensorShapeStatus.isUnkownRank = true;
            shapeStatus.push_back(tensorShapeStatus);
            continue;
        }
        tensorShapeStatus.isUnkownRank = false;
        for (size_t dimIndex = 0; dimIndex < tensorDesc[tensorIndex]->dims.size(); ++dimIndex) {
            if (tensorDesc[tensorIndex]->dims[dimIndex] == -1) {
                tensorShapeStatus.shapeStatus.push_back(false);
            } else {
                tensorShapeStatus.shapeStatus.push_back(true);
            }
        }
        for (size_t dimIndex = 0; dimIndex < tensorDesc[tensorIndex]->storageDims.size(); ++dimIndex) {
            if (tensorDesc[tensorIndex]->storageDims[dimIndex] == -1) {
                tensorShapeStatus.shapeStatus.push_back(false);
            } else {
                tensorShapeStatus.shapeStatus.push_back(true);
            }
        }
        shapeStatus.emplace_back(tensorShapeStatus);
    }
}

void OpModelManager::SetTensorShapeStatus(const AclOp &aclOp, std::vector<aclTensorShapeStatus> &shapeStatus)
{
    // Save shape status for tensors
    SetShapeStatus(aclOp.numInputs, aclOp.inputDesc, shapeStatus);
    SetShapeStatus(aclOp.numOutputs, aclOp.outputDesc, shapeStatus);
    const string &shapeStatusDesc = TensorStatusToStr(shapeStatus);
    std::lock_guard<std::mutex> lk(shapeStatusMutex_);
    auto it = tensorShapeStatusDesc_.find(aclOp.opType);
    if (it != tensorShapeStatusDesc_.end()) {
        auto shapeStatusIt = find(it->second.begin(), it->second.end(), shapeStatusDesc);
        if (shapeStatusIt == it->second.end()) {
            it->second.emplace_back(shapeStatusDesc);
            tensorShapeStatus_[aclOp.opType].emplace_back(shapeStatus);
        }
    } else {
        tensorShapeStatus_[aclOp.opType].emplace_back(shapeStatus);
        tensorShapeStatusDesc_[aclOp.opType].emplace_back(shapeStatusDesc);
    }
}

void OpModelManager::GetTensorShapeStatus(const AclOp &aclOp,
                                          std::vector<std::vector<aclTensorShapeStatus>> &shapeStatus)
{
    std::lock_guard<std::mutex> lk(shapeStatusMutex_);
    auto it = tensorShapeStatus_.find(aclOp.opType);
    if (it != tensorShapeStatus_.end()) {
        shapeStatus.assign(it->second.begin(), it->second.end());
    }

    ACL_LOG_INFO("GetTensorShapeStatus opType is %s, size of shapeStatus is %zu",
        aclOp.opType.c_str(), shapeStatus.size());
}

std::string OpModelManager::TensorStatusToStr(const std::vector<aclTensorShapeStatus> &tensorShapeStatus)
{
    std::string tensorShapeStatusDesc;
    for (size_t i = 0; i < tensorShapeStatus.size(); ++i) {
        tensorShapeStatusDesc.push_back('|');
        if (tensorShapeStatus[i].isUnkownRank) {
            tensorShapeStatusDesc += std::to_string(-2); // -2 means unkwon rank dim
            tensorShapeStatusDesc.push_back('_');
        }
        for (size_t j = 0; j < tensorShapeStatus[i].shapeStatus.size(); ++j) {
            if (tensorShapeStatus[i].shapeStatus[j]) {
                tensorShapeStatusDesc += std::to_string(1); // 1 means dynamic shape dim
                tensorShapeStatusDesc.push_back('_');
            } else {
                tensorShapeStatusDesc += std::to_string(0); // 0 means static shape dim
                tensorShapeStatusDesc.push_back('_');
            }
        }
    }
    return tensorShapeStatusDesc;
}

void OpModelManager::SetTensorShapeRange(const AclOp &aclOp, const std::vector<aclTensorShapeStatus> &tensorShapeStatus)
{
    std::vector<std::pair<int64_t, int64_t>> shapeRanges;
    for (int tensorIndex = 0; tensorIndex < aclOp.numInputs; ++tensorIndex) {
        // Complete the shape range for static tensor
        if (aclOp.inputDesc[tensorIndex]->shapeRange.empty()) {
            if ((aclOp.inputDesc[tensorIndex]->dims.size() > 0) &&
                (aclOp.inputDesc[tensorIndex]->dims[0] == UNKNOW_RANK)) {
                ACL_LOG_INFO("the %d inputTensor dim is unknownrank", tensorIndex);
            } else {
                std::vector<std::pair<int64_t, int64_t>> range;
                for (size_t dimIndex = 0; dimIndex < aclOp.inputDesc[tensorIndex]->dims.size(); ++dimIndex) {
                    range.emplace_back(make_pair(aclOp.inputDesc[tensorIndex]->dims[dimIndex],
                        aclOp.inputDesc[tensorIndex]->dims[dimIndex]));
                }
                const_cast<aclTensorDesc *>(aclOp.inputDesc[tensorIndex])->shapeRange = range;
            }
        }
        // Save shape range for input tensors
        for (size_t rangeIndex = 0; rangeIndex < aclOp.inputDesc[tensorIndex]->shapeRange.size(); ++rangeIndex) {
            shapeRanges.emplace_back(aclOp.inputDesc[tensorIndex]->shapeRange[rangeIndex]);
        }
    }

    for (int tensorIndex = 0; tensorIndex < aclOp.numOutputs; ++tensorIndex) {
        // Complete the shape range for static tensor
        if (aclOp.outputDesc[tensorIndex]->shapeRange.empty()) {
            if ((aclOp.outputDesc[tensorIndex]->dims.size() > 0) &&
                (aclOp.outputDesc[tensorIndex]->dims[0] == UNKNOW_RANK)) {
                ACL_LOG_INFO("the %d outputTensor dim is unknownrank", tensorIndex);
            } else {
                std::vector<std::pair<int64_t, int64_t>> range;
                for (size_t dimIndex = 0; dimIndex < aclOp.outputDesc[tensorIndex]->dims.size(); ++dimIndex) {
                    range.emplace_back(make_pair(aclOp.outputDesc[tensorIndex]->dims[dimIndex],
                        aclOp.outputDesc[tensorIndex]->dims[dimIndex]));
                }
                const_cast<aclTensorDesc *>(aclOp.outputDesc[tensorIndex])->shapeRange = range;
            }
        }
        // Save shape range for output tensors
        for (size_t rangeIndex = 0; rangeIndex < aclOp.outputDesc[tensorIndex]->shapeRange.size(); ++rangeIndex) {
            shapeRanges.emplace_back(aclOp.outputDesc[tensorIndex]->shapeRange[rangeIndex]);
        }
    }
    std::string tensorShapeStatusDesc = TensorStatusToStr(tensorShapeStatus);
    ACL_LOG_INFO("SetTensorShapeRange tensorShapeStatusDesc is %s, shapeRanges size is %zu",
        tensorShapeStatusDesc.c_str(), shapeRanges.size());
    std::lock_guard<std::mutex> lk(tensorShapeMutex_);
    auto it = tensorShapeRange_.find(tensorShapeStatusDesc);
    if (it != tensorShapeRange_.end()) {
        auto shapeRangeIt = find(it->second.begin(), it->second.end(), shapeRanges);
        if (shapeRangeIt == it->second.end()) {
            it->second.emplace_back(shapeRanges);
        }
    } else {
        tensorShapeRange_[tensorShapeStatusDesc].emplace_back(shapeRanges);
    }
}

void OpModelManager::GetTensorShapeRange(const std::vector<aclTensorShapeStatus> &tensorShapeStatus,
                                         std::vector<std::vector<std::pair<int64_t, int64_t>>> &shapeRanges)
{
    const std::string &tensorShapeStatusDesc = TensorStatusToStr(tensorShapeStatus);
    ACL_LOG_INFO("GetTensorShapeRange tensorShapeStatusDesc is %s", tensorShapeStatusDesc.c_str());
    std::lock_guard<std::mutex> lk(tensorShapeMutex_);
    auto it = tensorShapeRange_.find(tensorShapeStatusDesc);
    if (it == tensorShapeRange_.end()) {
        ACL_LOG_WARN("Match ShapeStatusDesc failed. ShapeStatusDesc = %s", tensorShapeStatusDesc.c_str());
        return;
    }

    shapeRanges = it->second;
    ACL_LOG_INFO("GetTensorShapeRange size of shapeRanges is %zu", shapeRanges.size());
}

aclError OpModelManager::RegisterModel(OpModelDef &&modelConfig,
                                       ModelMap &opModelDefs,
                                       DynamicModelMap &opDynamicModelDefs,
                                       bool isDynamic,
                                       bool isStaticRegister)
{
    auto numInputs = modelConfig.inputDescArr.size();
    std::vector<const aclTensorDesc *> input_desc;
    for (size_t i = 0; i < numInputs; ++i) {
        input_desc.emplace_back(&modelConfig.inputDescArr[i]);
    }
    std::vector<const aclTensorDesc *> output_desc;
    auto numOutputs = modelConfig.outputDescArr.size();
    for (size_t i = 0; i < numOutputs; ++i) {
        output_desc.emplace_back(&modelConfig.outputDescArr[i]);
    }

    AclOp aclOp;
    aclOp.opType = modelConfig.opType;
    aclOp.opAttr = &modelConfig.opAttr;
    aclOp.numInputs = numInputs;
    aclOp.numOutputs = numOutputs;
    aclOp.inputDesc = input_desc.data();
    aclOp.outputDesc = output_desc.data();
    if (!isStaticRegister) {
        // only dynamic load need update timestamp, static load is ULLONG_MAX default no need aging
        modelConfig.timestamp = attr_utils::GetCurrentTimestamp();
    }

    auto modelDefPtr = shared_ptr<OpModelDef>(new (std::nothrow)OpModelDef(std::move(modelConfig)));
    ACL_REQUIRES_NOT_NULL(modelDefPtr);
    shared_ptr<OpModelDef> agingModelDef = nullptr;
    if (isDynamic) {
        ACL_LOG_INFO("The model is dynamic shape");
        std::vector<aclTensorShapeStatus> shapeStatus;
        SetTensorShapeStatus(aclOp, shapeStatus);
        SetTensorShapeRange(aclOp, shapeStatus);
        opDynamicModelDefs.Insert(aclOp, modelDefPtr, agingModelDef);
        if (agingModelDef != nullptr) {
            dynamicModelCache_.Delete(*agingModelDef);
        }
    } else {
        ACL_LOG_INFO("The model is static shape");
        opModelDefs.Insert(aclOp, modelDefPtr, agingModelDef);
        if (agingModelDef != nullptr) {
            modelCache_.Delete(*agingModelDef);
        }
    }
    bool castHasTruncate = !GetIfCastHasTruncateAttr() && (aclOp.opType == "Cast") &&
                           (aclOp.opAttr != nullptr) && (aclOp.opAttr->HasAttr("truncate"));
    if (castHasTruncate) {
        ACL_LOG_INFO("Find cast op whose attr contains truncate");
        SetCastHasTruncateAttr(true);
    }
    ACL_LOG_INFO("Register model. OpModelDef = %s", modelDefPtr->DebugString().c_str());
    return ACL_SUCCESS;
}

OpModel *OpModelManager::GetBasicOpModel(const std::string &opType)
{
    auto iter = basicModelMap_.find(opType);
    if (iter == basicModelMap_.end()) {
        return nullptr;
    }

    OpModel &opModel = iter->second;
    return &opModel;
}

aclError OpModelManager::SetTensorConst(aclTensorDesc *desc, const aclDataBuffer *dataBuffer)
{
    ACL_LOG_INFO("start to execute SetTensorConst");
    ACL_REQUIRES_NOT_NULL(desc);
    ACL_REQUIRES_NOT_NULL(dataBuffer);
    size_t length = dataBuffer->length;
    void *hostMem = dataBuffer->data;
    if (length == 0) {
        desc->isConst = true;
        return ACL_SUCCESS;
    }
    if (length < 0) {
        ACL_LOG_INNER_ERROR("[Check][Length]The length of const hostMem is invalid. size = %zu",
            length);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"length of const hostMem",
            std::to_string(length), "length < 0"}));
        return ACL_ERROR_INVALID_PARAM;
    }
    desc->isConst = true;

    auto *constData = new (std::nothrow) char[length];
    ACL_REQUIRES_NOT_NULL(constData);
    if (memcpy_s(constData, length, hostMem, length) != EOK) {
        ACL_LOG_INNER_ERROR("[Copy][Mem]Copy const data failed. size = %zu", length);
        ACL_DELETE_ARRAY_AND_SET_NULL(constData);
        return ACL_ERROR_FAILURE;
    }

    desc->constDataBuf.reset(constData, [](const char *p)
        { delete[]p; ACL_LOG_DEBUG("delete const data in SetTensorConst of OpModelManager"); });
    desc->constDataLen = length;
    return ACL_SUCCESS;
}

aclError OpModelManager::SetHostMemToConst(AclOp &aclopHostMemToConst, bool &isExistConst)
{
    for (int i = 0; i < aclopHostMemToConst.numInputs; ++i) {
        if ((aclopHostMemToConst.inputDesc[i]->memtype == ACL_MEMTYPE_HOST) &&
            (!aclopHostMemToConst.inputDesc[i]->isConst)) {
            isExistConst = true;
            // HostMem needs to be constructed as constTensor
            ACL_LOG_INFO("inputTensor %s is hostMem, index is %d", aclopHostMemToConst.opType.c_str(), i);
            ACL_REQUIRES_OK(SetTensorConst(const_cast<aclTensorDesc *>(aclopHostMemToConst.inputDesc[i]),
                aclopHostMemToConst.inputs[i]));
        }
    }
    for (int i = 0; i < aclopHostMemToConst.numOutputs; ++i) {
        if ((aclopHostMemToConst.outputDesc[i]->memtype == ACL_MEMTYPE_HOST) &&
            (!aclopHostMemToConst.outputDesc[i]->isConst)) {
            isExistConst = true;
            // HostMem needs to be constructed as constTensor
            ACL_LOG_INFO("outputTensor %s is hostMem, index is %d", aclopHostMemToConst.opType.c_str(), i);
            ACL_REQUIRES_OK(SetTensorConst(const_cast<aclTensorDesc *>(aclopHostMemToConst.outputDesc[i]),
                aclopHostMemToConst.outputs[i]));
        }
    }
    return ACL_SUCCESS;
}

aclError OpModelManager::GetOpModel(AclOp &aclOp)
{
    OpModel opModel;
    bool isDynamic = false;
    aclError ret = MatchOpModel(aclOp, opModel, isDynamic);
    if (ret == ACL_SUCCESS) {
        aclOp.opModel = opModel;
        aclOp.isMatched = true;
        aclOp.isDynamic = isDynamic;
        ACL_LOG_INFO("operator %s is already registered, isDynamicModel = %d", aclOp.opType.c_str(), isDynamic);
        return ret;
    }

    auto compileRet = BuildOpModel(aclOp);
    if (compileRet != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Build][Op]Fail to build op model");
        return compileRet;
    }

    return ACL_SUCCESS;
}

static bool CheckDimRange(std::pair<int64_t, int64_t> rangeLeft, std::pair<int64_t, int64_t> rangeRight)
{
    int64_t minRange = rangeLeft.first;
    int64_t maxRange = rangeLeft.second;
    // the range Max in model is -1
    if (rangeRight.second == UNKNOW_DIM) {
        if (minRange < rangeRight.first) {
            ACL_LOG_INFO("the range:[%ld, %ld] is not in range of the model:[%ld, -1]",
                minRange, maxRange, rangeRight.first);
            return false;
        }
    } else if (maxRange == UNKNOW_DIM) { // the input range max is -1
        ACL_LOG_INFO("the range:[%ld, -1] is not in range of the model:[%ld, %ld]",
            minRange, rangeRight.first, rangeRight.second);
        return false;
    } else if ((minRange < rangeRight.first) || // input range or model range is static
        (maxRange > rangeRight.second)) {
        ACL_LOG_WARN("the range:[%ld, %ld] in compile is not in range of the model:[%ld, %ld]",
            minRange, maxRange, rangeRight.first, rangeRight.second);
        return false;
    }
    return true;
}

static bool CheckRange(int tensorNum,
                       const aclTensorDesc *const tensorDesc[],
                       const std::vector<aclTensorShapeStatus> &tensorShapeStatus,
                       const std::vector<std::pair<int64_t, int64_t>> &shapeRange,
                       size_t &rangeIndex, size_t &statusIndex, size_t &tensorDimSize)
{
    for (int tensorIndex = 0; tensorIndex < tensorNum; ++tensorIndex, ++statusIndex) {
        if (tensorShapeStatus[statusIndex].isUnkownRank) {
            ACL_LOG_INFO("the tensor is isUnkownRank tensor, tensorId is %d", tensorIndex);
            continue;
        }
        tensorDimSize += tensorDesc[tensorIndex]->dims.size();
        if (tensorDimSize > shapeRange.size()) {
            ACL_LOG_WARN("the size of shapeRange[%zu] is smaller than dims[%zu]", shapeRange.size(), tensorDimSize);
            return false;
        }
        for (size_t dimIndex = 0; dimIndex < tensorDesc[tensorIndex]->dims.size(); ++dimIndex, ++rangeIndex) {
            if (tensorDesc[tensorIndex]->dims[dimIndex] == UNKNOW_DIM) {
                ACL_LOG_INFO("the dim is -1, maybe compiling, tensorId is %d", tensorIndex);
                if (tensorDesc[tensorIndex]->shapeRange.size() > dimIndex) {
                    if (!CheckDimRange(tensorDesc[tensorIndex]->shapeRange[dimIndex], shapeRange[rangeIndex])) {
                        return false;
                    }
                }
            } else if (tensorDesc[tensorIndex]->dims[dimIndex] == UNKNOW_RANK) {
                return false;
            } else {
                if (tensorDesc[tensorIndex]->dims[dimIndex] < shapeRange[rangeIndex].first) {
                    return false;
                } else if (shapeRange[rangeIndex].second == UNKNOW_DIM) {
                    continue;
                } else if (tensorDesc[tensorIndex]->dims[dimIndex] > shapeRange[rangeIndex].second) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool OpModelManager::CheckShapeRange(const AclOp &aclOp,
                                     const std::vector<aclTensorShapeStatus> &tensorShapeStatus,
                                     const std::vector<std::pair<int64_t, int64_t>> &shapeRange)
{
    // Check whether the dims input by the user is within the range of the model
    size_t rangeIndex = 0;
    size_t statusIndex = 0;
    // Before calling the CheckShapeRange, it has been guaranteed that the rangeIndex
    // will not exceed the length of shapeRange
    size_t tensorDimSize = 0;
    bool ret = CheckRange(aclOp.numInputs, aclOp.inputDesc,
        tensorShapeStatus, shapeRange, rangeIndex, statusIndex, tensorDimSize);
    if (!ret) {
        ACL_LOG_WARN("the dims is not in range of shapeRange");
        return false;
    }
    ACL_LOG_INFO("inputDesc check success");
    ret = CheckRange(aclOp.numOutputs, aclOp.outputDesc,
        tensorShapeStatus, shapeRange, rangeIndex, statusIndex, tensorDimSize);
    if (!ret) {
        ACL_LOG_WARN("the dims is not in range of shapeRange");
        return false;
    }
    return true;
}

static void FixedAclOp(int tensorNum,
                       const aclTensorDesc *const tensorDesc[],
                       const std::vector<aclTensorShapeStatus> &tensorShapeStatus,
                       const std::vector<std::pair<int64_t, int64_t>> &shapeRange,
                       std::vector<std::vector<int64_t>> &tensorDims,
                       std::vector<int64_t> &storageTensorDims, size_t &tensorStatusIndex,
                       size_t &shapeIndex, size_t &shapeBegin)
{
    size_t dimIndex = 0; // The index of dims
    for (int tensorIndex = 0; tensorIndex < tensorNum; ++tensorIndex, ++tensorStatusIndex) {
        size_t statusIndex = 0;
        std::vector<int64_t> dim;
        for (size_t i = 0; i < tensorDesc[tensorIndex]->dims.size(); ++i) {
            dim.emplace_back(tensorDesc[tensorIndex]->dims[i]);
        }
        tensorDims.emplace_back(dim);
        if (tensorShapeStatus[tensorStatusIndex].isUnkownRank) {
            const_cast<aclTensorDesc *>(tensorDesc[tensorIndex])->dims = {-2};
            const_cast<aclTensorDesc *>(tensorDesc[tensorIndex])->shapeRange.clear();
            if (tensorDesc[tensorIndex]->storageFormat != ACL_FORMAT_UNDEFINED) {
                const_cast<aclTensorDesc *>(tensorDesc[tensorIndex])->storageDims = {-2};
            }
            continue;
        }
        for (dimIndex = 0; dimIndex < tensorDesc[tensorIndex]->dims.size(); ++dimIndex) {
            if (!tensorShapeStatus[tensorStatusIndex].shapeStatus[statusIndex]) {
                // -1 means dynamic dim
                const_cast<aclTensorDesc *>(tensorDesc[tensorIndex])->dims[dimIndex] = -1;
            }
            statusIndex++;
            shapeIndex++;
        }
        // The length of shapeRange is equal to the sum of inputDesc.dims.size and outputDesc.dims.size
        std::vector<std::pair<int64_t, int64_t>> inputTensorShapeRange(shapeRange.begin() + shapeBegin,
            shapeRange.begin() + shapeIndex);
        shapeBegin = shapeIndex;
        const_cast<aclTensorDesc *>(tensorDesc[tensorIndex])->shapeRange = inputTensorShapeRange;

        for (dimIndex = 0; dimIndex < tensorDesc[tensorIndex]->storageDims.size(); ++dimIndex) {
            storageTensorDims.emplace_back(tensorDesc[tensorIndex]->storageDims[dimIndex]);
            if (!tensorShapeStatus[tensorStatusIndex].shapeStatus[statusIndex]) {
                // -1 means dynamic dim
                const_cast<aclTensorDesc *>(tensorDesc[tensorIndex])->storageDims[dimIndex] = -1;
            }
            statusIndex++;
        }
    }
}

void OpModelManager::FixedAclopMatch(AclOp &aclOpMatch,
                                     const std::vector<aclTensorShapeStatus> &tensorShapeStatus,
                                     const std::vector<std::pair<int64_t, int64_t>> &shapeRange,
                                     std::vector<std::vector<int64_t>> &tensorDims,
                                     std::vector<int64_t> &storageTensorDims)
{
    // Modify the dynamic dimension entered by the user to -1 in order to match the model
    size_t shapeIndex = 0; // The end of shapeRange
    size_t shapeBegin = 0; // The begin of shapeRange
    size_t tensorStatusIndex = 0; // The index of tensorShapeStatus
    // Before calling the FixedAclopMatch, it has been guaranteed that the statusIndex
    // will not exceed the length of shapeRange and tensorShapeStatus
    FixedAclOp(aclOpMatch.numInputs, aclOpMatch.inputDesc, tensorShapeStatus, shapeRange,
        tensorDims, storageTensorDims, tensorStatusIndex, shapeIndex, shapeBegin);
    ACL_LOG_INFO("InputDesc FixedAclopMatch success");
    FixedAclOp(aclOpMatch.numOutputs, aclOpMatch.outputDesc, tensorShapeStatus, shapeRange,
        tensorDims, storageTensorDims, tensorStatusIndex, shapeIndex, shapeBegin);
    ACL_LOG_INFO("OutputDesc FixedAclopMatch success");
}

static bool BackAclOp(int tensorNum,
                      const aclTensorDesc *const tensorDesc[],
                      const std::vector<aclTensorShapeStatus> &tensorShapeStatus,
                      const std::vector<std::vector<int64_t>> &tensorDims,
                      const std::vector<int64_t> &storageTensorDims,
                      size_t &oriStoDimsIndex, size_t &tensorStatusIndex)
{
    size_t dimIndex = 0; // The index of dims
    for (int tensorIndex = 0; tensorIndex < tensorNum; ++tensorIndex, ++tensorStatusIndex) {
        if (tensorShapeStatus[tensorStatusIndex].isUnkownRank) {
            const_cast<aclTensorDesc *>(tensorDesc[tensorIndex])->dims = tensorDims[tensorStatusIndex];
            continue;
        }
        if (tensorDims[tensorStatusIndex].size() != tensorDesc[tensorIndex]->dims.size()) {
            ACL_LOG_INNER_ERROR("[Back][Op]Fail to BackAclopMatch");
            return false;
        }
        for (dimIndex = 0; dimIndex < tensorDesc[tensorIndex]->dims.size(); ++dimIndex) {
            // -1 means dynamic dim
            if (tensorDesc[tensorIndex]->dims[dimIndex] == -1) {
                const_cast<aclTensorDesc *>(tensorDesc[tensorIndex])->dims[dimIndex] =
                    tensorDims[tensorStatusIndex][dimIndex];
            }
        }
        for (dimIndex = 0; dimIndex < tensorDesc[tensorIndex]->storageDims.size(); ++dimIndex) {
            if (tensorDesc[tensorIndex]->storageDims[dimIndex] == -1) {
                const_cast<aclTensorDesc *>(tensorDesc[tensorIndex])->storageDims[dimIndex] =
                    storageTensorDims[oriStoDimsIndex];
            }
            oriStoDimsIndex++;
        }
    }
    return true;
}

bool OpModelManager::BackAclopMatch(AclOp &aclOpMatch,
                                    const std::vector<aclTensorShapeStatus> &tensorShapeStatus,
                                    const std::vector<std::vector<int64_t>> &tensorDims,
                                    const std::vector<int64_t> &storageTensorDims)
{
    // After the matching is completed, the value of the dynamic dimension input is restored to the original value
    size_t oriStoDimsIndex = 0; // The index of storageTensorDims
    size_t tensorStatusIndex = 0; // The index of tensorShapeStatus
    // Before calling the CheckShapeRange, it has been guaranteed that the oriDimsIndex
    // will not exceed the length of shapeRange and shapeStatus
    bool ret = BackAclOp(aclOpMatch.numInputs, aclOpMatch.inputDesc, tensorShapeStatus,
                         tensorDims, storageTensorDims, oriStoDimsIndex, tensorStatusIndex);
    if (!ret) {
        ACL_LOG_INNER_ERROR("[Back][Op]Fail to inputDesc back aclOp");
        return false;
    }
    ACL_LOG_INFO("InputDesc BackAclopMatch success");
    ret = BackAclOp(aclOpMatch.numOutputs, aclOpMatch.outputDesc, tensorShapeStatus,
                    tensorDims, storageTensorDims, oriStoDimsIndex, tensorStatusIndex);
    if (!ret) {
        ACL_LOG_INNER_ERROR("[Back][Op]Fail to inputDesc back aclOp");
        return false;
    }
    return true;
}

aclError OpModelManager::MatchStaticOpModel(const AclOp &aclOp, OpModel &opModel,
    bool &isDynamic, bool &isNeedMatchDymaic)
{
    shared_ptr<OpModelDef> modelDef;
    AclOp aclopHostMemToConst = aclOp;
    aclError ret = ACL_SUCCESS;
    isNeedMatchDymaic = false;
    bool isExistConst = false;
    ACL_REQUIRES_OK(SetHostMemToConst(aclopHostMemToConst, isExistConst));
    if (isExistConst) {
        ret = opModels_.Get(aclopHostMemToConst, modelDef, true);
        if (ret == ACL_SUCCESS) {
            isDynamic = false;
            ACL_LOG_INFO("Match static model with const memory successfully. opType = %s, opModel = %s",
                aclOp.opType.c_str(), modelDef->modelPath.c_str());
            ret = modelCache_.GetOpModel(*modelDef, opModel);
            return ret;
        } else {
            ret = opModels_.Get(aclOp, modelDef, true);
            if (ret == ACL_SUCCESS) {
                isDynamic = false;
                ACL_LOG_INFO("Match static model successfully. opType = %s, opModel = %s",
                    aclOp.opType.c_str(), modelDef->modelPath.c_str());
                ret = modelCache_.GetOpModel(*modelDef, opModel);
                return ret;
            }
        }
    } else {
        ret = opModels_.Get(aclOp, modelDef, true);
        if (ret == ACL_SUCCESS) {
            isDynamic = false;
            ACL_LOG_INFO("Match static model successfully. opType = %s, opModel = %s",
                aclOp.opType.c_str(), modelDef->modelPath.c_str());
            ret = modelCache_.GetOpModel(*modelDef, opModel);
            return ret;
        }
    }
    ACL_LOG_INFO("Match static opModels fail, begin to match model from dynamic opModels. opType = %s",
        aclOp.opType.c_str());
    isNeedMatchDymaic = true;

    return ret;
}

aclError OpModelManager::MatchDynamicOpModel(const AclOp &aclOp, OpModel &opModel, bool &isDynamic)
{
    shared_ptr<OpModelDef> modelDef;
    aclError ret = ACL_SUCCESS;
    // check the input shape must be static when executing
    if (!aclOp.isCompile) {
        if (IsDynamicOpModel(aclOp)) {
            ACL_LOG_INNER_ERROR("[Check][Op]TensorDesc must be static when executing, "
                "tensorDesc is %s", aclOp.DebugString().c_str());
            return ACL_ERROR_INVALID_PARAM;
        }
    }
    // Need to refresh the shape(-1/-2) and go to map to find model when executing
    ACL_LOG_INFO("aclOp.numInputs is %d, aclOp.numOutputs is %d", aclOp.numInputs, aclOp.numOutputs);
    std::vector<std::vector<aclTensorShapeStatus>> shapeStatus;
    GetTensorShapeStatus(aclOp, shapeStatus);
    for (size_t i = 0; i < shapeStatus.size(); ++i) {
        std::vector<std::vector<std::pair<int64_t, int64_t>>> shapeRanges;
        GetTensorShapeRange(shapeStatus[i], shapeRanges);
        for (size_t j = 0; j < shapeRanges.size(); ++j) {
            ACL_LOG_INFO("shapeRanges[%d] size is %zu, shapeStatus[%d] size is %zu",
                j, shapeRanges[j].size(), i, shapeStatus[i].size());
            if (static_cast<size_t>(aclOp.numInputs + aclOp.numOutputs) != shapeStatus[i].size()) {
                ACL_LOG_WARN("The numbers of tensor[%zu] is not equal to tensor dims[%zu] in model",
                    static_cast<size_t>(aclOp.numInputs + aclOp.numOutputs), shapeStatus[i].size());
                continue;
            }
            if (!CheckShapeRange(aclOp, shapeStatus[i], shapeRanges[j])) {
                ACL_LOG_WARN("the dims is not in range of shapeRange");
                continue;
            }
            AclOp aclOpMatch = AclOp(aclOp);
            ACL_LOG_INFO("before FixedAclopMatch aclOpMatch = %s", aclOpMatch.DebugString().c_str());
            std::vector<std::vector<int64_t>> tensorDims;
            std::vector<int64_t> storageTensorDims;
            FixedAclopMatch(aclOpMatch, shapeStatus[i], shapeRanges[j], tensorDims, storageTensorDims);
            ACL_LOG_INFO("after FixedAclopMatch aclOpMatch = %s", aclOpMatch.DebugString().c_str());
            ret = dynamicOpModels_.Get(aclOpMatch, modelDef, true);
            if (ret == ACL_SUCCESS) {
                ACL_LOG_INFO("Match dynamic model success. opType = %s, opModel = %s",
                    aclOpMatch.opType.c_str(), modelDef->modelPath.c_str());
                isDynamic = true;
                ret = dynamicModelCache_.GetOpModel(*modelDef, opModel);
                return ret;
            } else {
                // aicpu is -2 but need to set const
                bool isExistConst = false;
                ACL_REQUIRES_OK(SetHostMemToConst(aclOpMatch, isExistConst));
                ret = dynamicOpModels_.Get(aclOpMatch, modelDef, true);
                if (ret == ACL_SUCCESS) {
                    ACL_LOG_INFO("Match dynamic model success111. opType = %s, opModel = %s",
                        aclOpMatch.opType.c_str(), modelDef->modelPath.c_str());
                    isDynamic = true;
                    ret = dynamicModelCache_.GetOpModel(*modelDef, opModel);
                    return ret;
                }
            }
        }
    }
    ACL_CHECK_WITH_MESSAGE_AND_NO_RETURN(aclOp.isCompile, "MatchOpModel fail from static map or dynamic map");
    return ACL_ERROR_OP_NOT_FOUND;
}

aclError OpModelManager::MatchOpModel(const AclOp &aclOp, OpModel &opModel, bool &isDynamic)
{
    bool isNeedMatchDymaic = false;
    aclError ret = MatchStaticOpModel(aclOp, opModel, isDynamic, isNeedMatchDymaic);
    if (!isNeedMatchDymaic) {
        return ret;
    }

    ret = MatchDynamicOpModel(aclOp, opModel, isDynamic);
    return ret;
}

aclError OpModelManager::ReadModelDefs(const std::string &configPath,
    std::vector<OpModelDef> &configList)
{
    vector<string> modelFilePaths;
    ACL_REQUIRES_OK(file_utils::ListFiles(configPath, OmFileFilterFn, modelFilePaths, OM_DIR_MAX_DEPTH));

    for (auto &path : modelFilePaths) {
        OpModel opModel;
        ACL_REQUIRES_OK(ReadOpModelFromFile(path, opModel));
        OpModelDef modelDef;
        modelDef.modelPath = path;
        auto ret = OpModelParser::ParseOpModel(opModel, modelDef);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_WARN("parse model failed. result = %d, model = %s. skip it", ret, modelDef.modelPath.c_str());
            continue;
        }

        configList.emplace_back(std::move(modelDef));
        if (IsDynamicOpModel(configList.back())) {
            ACL_LOG_INFO("Add dynamic model: %s", configList.back().modelPath.c_str());
            dynamicModelCache_.Add(configList.back(), opModel);
        } else {
            ACL_LOG_INFO("Add static model: %s", configList.back().modelPath.c_str());
            modelCache_.Add(configList.back(), opModel);
        }
    }

    return ACL_SUCCESS;
}

bool OpModelManager::MatchModelDefByAttr(const OpModelDef &modelDef, const aclopAttr *attr)
{
    ACL_LOG_DEBUG("Matching model: %s", modelDef.modelPath.c_str());
    auto &modelAttrs = modelDef.opAttr.Attrs();
    size_t opAttrNum = 0;
    if (attr != nullptr) {
        opAttrNum = attr->Attrs().size();
    }

    if (modelAttrs.size() != opAttrNum) {
        ACL_LOG_INFO("the nums of model attr[%zu] is not equal to nums of op attr[%zu]", modelAttrs.size(), opAttrNum);
        return false;
    }

    if (opAttrNum == 0) {
        ACL_LOG_INFO("Match attr success. model = %s", modelDef.modelPath.c_str());
        return true;
    }

    for (const auto &iter : attr->Attrs()) {
        const string &attrName = iter.first;
        const ge::GeAttrValue &attrValue = iter.second;
        ACL_LOG_DEBUG("Matching attr %s", attrName.c_str());

        auto it = modelAttrs.find(attrName);
        if (it == modelAttrs.end()) {
            ACL_LOG_DEBUG("the value of Attr[%s] is invalid, it does't exist in model", attrName.c_str());
            return false;
        }

        // no overloaded operator != for GeAttrValue
        if (!attr_utils::AttrValueEquals(it->second, attrValue)) {
            ACL_LOG_INFO("Attr[%s] value mismatches. model's value = %s, required = %s", attrName.c_str(),
                         attr_utils::GeAttrValueToString(it->second).c_str(),
                         attr_utils::GeAttrValueToString(attrValue).c_str());
            return false;
        }
    }

    ACL_LOG_INFO("Match attr success. model = %s", modelDef.modelPath.c_str());
    return true;
}

aclError OpModelManager::BuildOpModel(const AclOp &aclOp)
{
    std::shared_ptr<void> modelData;
    size_t modelSize;
    auto ret = OpCompileService::GetInstance().CompileOp(aclOp, modelData, modelSize);
    if (ret != ACL_SUCCESS) {
        return ret;
    }
    // it is dynamic load
    ret = LoadModelFromSharedMem(modelData, modelSize, false);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Load][Model]load model from mem failed.");
        return ret;
    }

    ACL_LOG_INFO("Compile operator %s success", aclOp.opType.c_str());
    return ACL_SUCCESS;
}
}
