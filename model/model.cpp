/**
* @file model.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/acl_mdl.h"
#include <vector>
#include <mutex>
#include <string>
#include <queue>
#include "securec.h"
#include "acl/acl_base.h"
#include "executor/ge_executor.h"
#include "common/ge_inner_error_codes.h"
#include "runtime/stream.h"
#include "graph/tensor.h"
#include "graph/types.h"
#include "model_desc_internal.h"
#include "error_codes_inner.h"
#include "toolchain/profiling_manager.h"
#include "toolchain/resource_statistics.h"
#include "framework/common/ge_types.h"
#include "utils/math_utils.h"
#include "utils/string_utils.h"
#include "model_config.h"

using namespace std;

namespace {
constexpr size_t MIN_OUTPUT_SHAPE_INFO_SIZE = 2;
constexpr size_t MAX_OUTPUT_SHAPE_INFO_SIZE = MIN_OUTPUT_SHAPE_INFO_SIZE + ACL_MAX_DIM_CNT;
constexpr size_t DYNAMIC_BATCH_SIZE = 1;
constexpr size_t DYNAMIC_HW_SIZE = 2;
constexpr size_t TENSOR_NAME_ATTR_NUM = 5;

constexpr const char *TENSOR_NAME_PREFIX = "acl";
constexpr const char *TENSOR_INPUT_STR = "input";
constexpr const char *TENSOR_OUTPUT_STR = "output";
constexpr const char *MODEL_ID_STR = "modelId";

std::mutex aclmdlGetOpAttrMutex;

enum DimsType {
    DIMS_TYPE_V1 = 0,
    DIMS_TYPE_V2
};

enum TensorType {
    INPUT_TENSOR_TYPE = 0,
    OUTPUT_TENSOR_TYPE
};
}

aclmdlDesc *aclmdlCreateDesc()
{
    ACL_STAGES_REG(acl::ACL_STAGE_CREATE, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DESC);
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DESC);
    return new(std::nothrow) aclmdlDesc();
}

aclError aclmdlDestroyDesc(aclmdlDesc *modelDesc)
{
    ACL_STAGES_REG(acl::ACL_STAGE_DESTROY, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DESC);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelDesc);
    ACL_DELETE_AND_SET_NULL(modelDesc);
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DESC);
    return ACL_SUCCESS;
}

static aclError ParseBatchInfo(aclmdlDesc *modelDesc, int32_t dynamicType, const vector<vector<int64_t>> &batchInfo)
{
    uint32_t modelId = modelDesc->modelId;
    if (dynamicType == static_cast<int32_t>(ge::DYNAMIC_DIMS)) { // dynamic dims, size can be [1, 4]
        size_t dimCount = batchInfo[0].size();
        for (size_t i = 0; i < batchInfo.size(); ++i) {
            if (batchInfo[i].size() != dimCount) {
                ACL_LOG_INNER_ERROR("[Check][Size]Get dynamic model info invalid, model id[%u], one dim count is %zu "
                    "while another is %zu", modelId, dimCount, batchInfo[i].size());
                modelDesc->dynamicDims.clear();
                return ACL_ERROR_GE_FAILURE;
            }
            vector<uint64_t> oneDims;
            for (size_t j = 0; j < dimCount; ++j) {
                oneDims.push_back(static_cast<uint64_t>(batchInfo[i][j]));
            }
            modelDesc->dynamicDims.push_back(oneDims);
        }
    } else if (batchInfo[0].size() == DYNAMIC_BATCH_SIZE) { // dynamic batch,size is 1
        for (size_t i = 0; i < batchInfo.size(); ++i) {
            if (batchInfo[i].size() != DYNAMIC_BATCH_SIZE) {
                ACL_LOG_INNER_ERROR("[Check][Size]get dynamic model info invalid, model id[%u]", modelId);
                modelDesc->dynamicBatch.clear();
                return ACL_ERROR_GE_FAILURE;
            }
            modelDesc->dynamicBatch.push_back(static_cast<uint64_t>(batchInfo[i][0]));
        }
    } else if (batchInfo[0].size() == DYNAMIC_HW_SIZE) { // dynamic hw,size is 2
        for (size_t i = 0; i < batchInfo.size(); ++i) {
            if (batchInfo[i].size() != DYNAMIC_HW_SIZE) { // dynamic hw,size is 2
                ACL_LOG_INNER_ERROR("[Check][Size]get dynamic model info invalid, model id[%u]", modelId);
                modelDesc->dynamicHW.clear();
                return ACL_ERROR_GE_FAILURE;
            }
            modelDesc->dynamicHW.push_back({static_cast<uint64_t>(batchInfo[i][0]),
                                            static_cast<uint64_t>(batchInfo[i][1])});
        }
    } else {
        ACL_LOG_INNER_ERROR("[Get][DynamicModel]get dynamic model info invalid, model id[%u]", modelId);
        return ACL_ERROR_GE_FAILURE;
    }

    return ACL_SUCCESS;
}

static aclError GetDynamicTensorInfo(aclmdlDesc *modelDesc)
{
    ACL_LOG_DEBUG("call ge interface executor.GetDynamicBatchInfo");
    std::vector<std::vector<int64_t>> batchInfo;
    ge::GeExecutor executor;
    uint32_t modelId = modelDesc->modelId;
    int32_t dynamicType = static_cast<int32_t>(ge::FIXED);
    ge::Status ret = executor.GetDynamicBatchInfo(modelId, batchInfo, dynamicType);
    if (ret != ge::SUCCESS) {
        ACL_LOG_WARN("get dynamic model info failed, ge result[%u], model id[%u]", ret, modelId);
    }
    vector<string> userDesignateShapeOrder;
    ret = executor.GetUserDesignateShapeOrder(modelId, userDesignateShapeOrder);
    if (ret != ge::SUCCESS) {
        ACL_LOG_WARN("get user designate shape order failed, ge result[%u], model id[%u]", ret, modelId);
    }
    modelDesc->dataNameOrder = userDesignateShapeOrder;

    if (batchInfo.empty()) {
        ACL_LOG_INFO("model is not dynamic, batchInfo is empty, modelId[%u]", modelId);
        return ACL_SUCCESS;
    }

    ACL_LOG_INFO("model is dynamic, modelId[%u]", modelId);
    aclError retVal = ParseBatchInfo(modelDesc, dynamicType, batchInfo);
    if (retVal != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Parse][BatchInfo]get model dynamic info failed, result[%d], model id[%u]",
            retVal, modelId);
        return retVal;
    }

    return ACL_SUCCESS;
}

static aclError GetModelOutputShapeInfo(aclmdlDesc *modelDesc)
{
    ACL_LOG_DEBUG("call ge interface executor.GetModelAttr");
    ge::GeExecutor executor;
    std::vector<std::string> geDynamicOutputShape;
    uint32_t modelId = modelDesc->modelId;
    ge::Status ret = executor.GetModelAttr(modelId, geDynamicOutputShape);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Get][ModelAttr]get model attribute failed, ge result[%u], model id[%u]", ret, modelId);
        return ACL_GET_ERRCODE_GE(ret);
    }

    if (geDynamicOutputShape.empty()) {
        ACL_LOG_INFO("model is not dynamic, geDynamicOutputShape is empty, modelId[%u]", modelId);
        return ACL_SUCCESS;
    }

    std::vector<std::vector<int64_t>> &dynamicOutputShape = modelDesc->dynamicOutputShape;
    for (auto &it : geDynamicOutputShape) {
        int64_t val = 0;
        int64_t negativeFlag = 1;
        std::vector<int64_t> outputShape;
        // ge uses string like "0:0:1,3,224,224" to represent output shape info,
        // acl converts string like "0:0:1,3,224,224" to vector<int64_t>
        for (auto &strIt : it) {
            if ((strIt >= '0') && (strIt <= '9')) { // numeric character
                val = val * 10 + (strIt - '0'); // character to number
            } else if (strIt == '-') { // '-' represents that dynamic model has static output
                negativeFlag = -1;
                ACL_LOG_DEBUG("dynamic model include static output");
            } else {
                val *= negativeFlag;
                outputShape.emplace_back(val);
                val = 0;
                negativeFlag = 1;
            }
        }
        val *= negativeFlag;
        outputShape.emplace_back(val); // last value
        dynamicOutputShape.emplace_back(outputShape);
    }

    return ACL_SUCCESS;
}

static aclError ModelLoadFromFileWithMem(const char *modelPath, uint32_t *modelId,
    void *workPtr, size_t workSize, void *weightPtr, size_t weightSize, int32_t priority)
{
    ACL_LOG_INFO("start to execute ModelLoadFromFileWithMem, modelPath[%s], "
        "workSize[%zu], weightSize[%zu], priority[%d]", modelPath, workSize, weightSize, priority);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelPath);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelId);

    ge::GeExecutor executor;
    uint32_t id = 0;
    ge::ModelData data;
    std::string path(modelPath);
    ACL_LOG_INFO("call ge interface executor.LoadDataFromFile, workSize[%zu], weightSize[%zu]",
        workSize, weightSize);
    ge::Status ret = executor.LoadDataFromFile(path, data);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Model][FromFile]load model from file[%s] failed, ge result[%u]", modelPath, ret);
        ACL_DELETE_ARRAY(data.model_data);
        return ACL_GET_ERRCODE_GE(ret);
    }
    data.priority = priority;
    ACL_LOG_INFO("call ge interface executor.LoadModelFromData, workSize[%zu], weightSize[%zu]",
        workSize, weightSize);
    ret = executor.LoadModelFromData(id, data, workPtr, workSize, weightPtr, weightSize);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Model][FromData]load model from data failed, ge result[%u]", ret);
        ACL_DELETE_ARRAY(data.model_data);
        return ACL_GET_ERRCODE_GE(ret);
    }

    *modelId = id;
    ACL_DELETE_ARRAY(data.model_data);
    ACL_LOG_INFO("successfully execute ModelLoadFromFileWithMem, workSize[%zu], weightSize[%zu], modelId[%u]",
        workSize, weightSize, *modelId);
    return ACL_SUCCESS;
}

static aclError ModelLoadFromMemWithMem(const void *model, size_t modelSize, uint32_t *modelId,
    void *workPtr, size_t workSize, void *weightPtr, size_t weightSize, int32_t priority)
{
    ACL_LOG_INFO("start to execute ModelLoadFromMemWithMem, workSize[%zu], weightSize[%zu], priority[%d]",
        workSize, weightSize, priority);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(model);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelId);
    if (modelSize == 0) {
        ACL_LOG_INNER_ERROR("[Check][ModelSize]modelSize[%zu] is invalid, should not be zero", modelSize);
        return ACL_ERROR_INVALID_PARAM;
    }

    ge::GeExecutor geExecutor;
    uint32_t id = 0;
    ge::ModelData modelData;
    modelData.model_data = const_cast<void *>(model);
    modelData.model_len = modelSize;
    modelData.priority = priority;
    ACL_LOG_INFO("call ge interface executor.LoadModelFromData, modelSize[%zu], workSize[%zu], weightSize[%zu]",
        modelSize, workSize, weightSize);
    ge::Status ret = geExecutor.LoadModelFromData(id, modelData, workPtr, workSize, weightPtr, weightSize);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Model][FromData]load model from data failed, ge result[%u]", ret);
        return ACL_GET_ERRCODE_GE(ret);
    }

    *modelId = id;
    ACL_LOG_INFO("successfully execute ModelLoadFromMemWithMem, modelSize[%zu], workSize[%zu], "
        "weightSize[%zu], modelId[%u]", modelSize, workSize, weightSize, *modelId);
    return ACL_SUCCESS;
}

static aclError ModelLoadFromFileWithQ(const char *modelPath, uint32_t *modelId, const uint32_t *inputQ,
    size_t inputQNum, const uint32_t *outputQ, size_t outputQNum, int32_t priority)
{
    ACL_LOG_INFO("start to execute ModelLoadFromFileWithQ, modelPath[%s], inputQNum[%zu], "
        "outputQNum[%zu], priority[%d]", modelPath, inputQNum, outputQNum, priority);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelPath);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelId);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(inputQ);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(outputQ);

    if ((inputQNum == 0) || (outputQNum == 0)) {
        ACL_LOG_INNER_ERROR("[Check][QNum]inputQNum[%zu] or outputQNum[%zu] is invalid, can't be zero",
            inputQNum, outputQNum);
        return ACL_ERROR_INVALID_PARAM;
    }

    ge::GeExecutor geExecutor;
    uint32_t id = 0;
    ge::ModelData modelData;
    std::string path(modelPath);
    ACL_LOG_INFO("call ge interface executor.LoadDataFromFile");
    ge::Status ret = geExecutor.LoadDataFromFile(path, modelData);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Load][FromFile]load model from file[%s], ge result[%u], failed", modelPath, ret);
        ACL_DELETE_ARRAY(modelData.model_data);
        return ACL_GET_ERRCODE_GE(ret);
    }
    modelData.priority = priority;
    std::vector<uint32_t> inputQVec(inputQ, inputQ + inputQNum);
    std::vector<uint32_t> outputQVec(outputQ, outputQ + outputQNum);

    ACL_LOG_INFO("call ge interface executor.LoadModelWithQ");
    ret = geExecutor.LoadModelWithQ(id, modelData, inputQVec, outputQVec);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Load][WithQ]execute LoadModelWithQ failed, ge result[%u]", ret);
        ACL_DELETE_ARRAY(modelData.model_data);
        return ACL_GET_ERRCODE_GE(ret);
    }

    *modelId = id;
    ACL_DELETE_ARRAY(modelData.model_data);

    ACL_LOG_INFO("successfully execute ModelLoadFromFileWithQ, modelPath[%s], inputQNum[%zu], outputQNum[%zu], "
        "modelId[%u]", modelPath, inputQNum, outputQNum, *modelId);
    return ACL_SUCCESS;
}

static aclError ModelLoadFromMemWithQ(const void *model, size_t modelSize, uint32_t *modelId,
    const uint32_t *inputQ, size_t inputQNum, const uint32_t *outputQ, size_t outputQNum, int32_t priority)
{
    ACL_LOG_INFO("start to execute ModelLoadFromMemWithQ, modelSize[%zu], inputQNum[%zu], outputQNum[%zu], "
        "priority[%d]", modelSize, inputQNum, outputQNum, priority);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(model);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelId);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(inputQ);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(outputQ);
    if ((modelSize == 0) || (inputQNum == 0) || (outputQNum == 0)) {
        ACL_LOG_INNER_ERROR("[Check][Params]modelSize[%zu] or inputQNum[%zu] or outputQNum[%zu] is invalid, "
            "can't be zero", modelSize, inputQNum, outputQNum);
        return ACL_ERROR_INVALID_PARAM;
    }

    ge::GeExecutor executor;
    uint32_t id = 0;
    ge::ModelData data;

    data.model_data = const_cast<void *>(model);
    data.model_len = modelSize;
    data.priority = priority;

    std::vector<uint32_t> inputQVec(inputQ, inputQ + inputQNum);
    std::vector<uint32_t> outputQVec(outputQ, outputQ + outputQNum);

    ACL_LOG_INFO("call ge interface executor.LoadModelWithQ, modelSize[%zu]", modelSize);
    ge::Status ret = executor.LoadModelWithQ(id, data, inputQVec, outputQVec);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Load][WithQ]load model with Q, ge result[%u], failed", ret);
        return ACL_GET_ERRCODE_GE(ret);
    }

    *modelId = id;
    ACL_LOG_INFO("successfully execute ModelLoadFromMemWithQ, modelSize[%zu], inputQNum[%zu], outputQNum[%zu], "
        "modelId[%u]", modelSize, inputQNum, outputQNum, *modelId);

    return ACL_SUCCESS;
}

aclError aclmdlGetDesc(aclmdlDesc *modelDesc, uint32_t modelId)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlGetDesc, model id[%u]", modelId);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelDesc);
    modelDesc->Clear();

    std::vector<ge::TensorDesc> inputDesc;
    std::vector<ge::TensorDesc> outputDesc;
    ge::GeExecutor executor;
    ge::Status ret = executor.GetModelDescInfo(modelId, inputDesc, outputDesc);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Get][ModelDescInfo]get model description failed, ge result[%u], model id[%u]",
            ret, modelId);
        return ACL_GET_ERRCODE_GE(ret);
    }

    std::vector<ge::TensorDesc> inputDescV2;
    std::vector<ge::TensorDesc> outputDescV2;
    ret = executor.GetModelDescInfo(modelId, inputDescV2, outputDescV2, true);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Get][ModelDescInfo]get model description v2 failed, ge result[%u], model id[%u]",
            ret, modelId);
        return ACL_GET_ERRCODE_GE(ret);
    }

    for (size_t i = 0; i < inputDesc.size(); ++i) {
        ge::AscendString inputName;
        aclmdlTensorDesc tensorDesc;
        tensorDesc.size = inputDesc[i].GetSize();
        ge::graphStatus ret = inputDesc[i].GetName(inputName);
        if (ret != ge::GRAPH_SUCCESS) {
            ACL_LOG_WARN("the %zu input tensor GetName failed.", i);
            return ACL_GET_ERRCODE_GE(ret);
        }
        std::string inputStr;
        if (inputName.GetString() != nullptr) {
            inputStr = std::string(inputName.GetString());
        }
        tensorDesc.name = inputStr;
        tensorDesc.format = static_cast<aclFormat>(inputDesc[i].GetFormat());
        tensorDesc.dataType = static_cast<aclDataType>(inputDesc[i].GetDataType());
        ge::Shape shape = inputDesc[i].GetShape();
        tensorDesc.dims = shape.GetDims();
        ge::Shape shapeV2 = inputDescV2[i].GetShape();
        tensorDesc.dimsV2 = shapeV2.GetDims();
        inputDesc[i].GetShapeRange(tensorDesc.shapeRanges);
        modelDesc->inputDesc.push_back(tensorDesc);
    }

    for (size_t i = 0; i < outputDesc.size(); ++i) {
        ge::AscendString outputName;
        aclmdlTensorDesc tensorDesc;
        tensorDesc.size = outputDesc[i].GetSize();
        ge::graphStatus ret = outputDesc[i].GetName(outputName);
        if (ret != ge::GRAPH_SUCCESS) {
            ACL_LOG_WARN("the %zu output tensor GetName failed.", i);
            return ACL_GET_ERRCODE_GE(ret);
        }
        std::string outputStr;
        if (outputName.GetString() != nullptr) {
            outputStr = std::string(outputName.GetString());
        }
        tensorDesc.name = outputStr;
        tensorDesc.format = static_cast<aclFormat>(outputDesc[i].GetFormat());
        tensorDesc.dataType = static_cast<aclDataType>(outputDesc[i].GetDataType());
        ge::Shape shape = outputDesc[i].GetShape();
        tensorDesc.dims = shape.GetDims();
        ge::Shape shapeV2 = outputDescV2[i].GetShape();
        tensorDesc.dimsV2 = shapeV2.GetDims();
        outputDesc[i].GetShapeRange(tensorDesc.shapeRanges);
        modelDesc->outputDesc.push_back(tensorDesc);
    }

    modelDesc->modelId = modelId;
    aclError retVal = GetDynamicTensorInfo(modelDesc);
    if (retVal != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Get][DynamicTensorInfo]get model dynamic info failed, result[%d], model id[%u]",
            retVal, modelId);
        return retVal;
    }

    retVal = GetModelOutputShapeInfo(modelDesc);
    if (retVal != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Get][ModelOutputShapeInfo]get model output shape info failed, result[%d], model id[%u]",
            retVal, modelId);
        return retVal;
    }

    ACL_LOG_INFO("successfully execute aclmdlGetDesc, model id[%u]", modelId);
    return ACL_SUCCESS;
}

size_t aclmdlGetNumInputs(aclmdlDesc *modelDesc)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (modelDesc == nullptr) {
        return 0;
    }

    return modelDesc->inputDesc.size();
}

size_t aclmdlGetNumOutputs(aclmdlDesc *modelDesc)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (modelDesc == nullptr) {
        return 0;
    }

    return modelDesc->outputDesc.size();
}

static size_t aclmdlGetTensorSize(aclmdlTensorDesc &tensorDesc, size_t index)
{
    std::vector<int64_t> &dims = tensorDesc.dims;
    bool needCalcByMaxShapeRange = false;
    for (size_t i = 0; i < dims.size(); ++i) {
        if (dims[i] < 0) {
            needCalcByMaxShapeRange = true;
            break;
        }
    }

    if (tensorDesc.shapeRanges.empty()) {
        needCalcByMaxShapeRange = false;
    }

    if (needCalcByMaxShapeRange) {
        std::vector<std::pair<int64_t, int64_t>> &shapeRanges = tensorDesc.shapeRanges;
        size_t elementTypeSize = aclDataTypeSize(tensorDesc.dataType);
        size_t outputSizeByMaxShapeRange = elementTypeSize;
        for (size_t i = 0; i < shapeRanges.size(); ++i) {
            if (shapeRanges[i].second <= 0) {
                ACL_LOG_INFO("max shape of shapeRanges[%zu] is [%ld], index[%zu]",
                             i, shapeRanges[i].second, index);
                return 0;
            }
            if (acl::CheckSizeTMultiOverflow(outputSizeByMaxShapeRange, shapeRanges[i].second,
                                             outputSizeByMaxShapeRange) == ACL_ERROR_FAILURE) {
                return 0;
            }
        }
        return outputSizeByMaxShapeRange;
    }

    return tensorDesc.size;
}

size_t aclmdlGetInputSizeByIndex(aclmdlDesc *modelDesc, size_t index)
{
    if ((modelDesc == nullptr) || (index >= modelDesc->inputDesc.size())) {
        ACL_LOG_INNER_ERROR("input param is invalid, modelDesc[%p], index[%zu]", modelDesc, index);
        return 0;
    }

    aclmdlTensorDesc &tensorDesc = modelDesc->inputDesc[index];
    return aclmdlGetTensorSize(tensorDesc, index);
}

size_t aclmdlGetOutputSizeByIndex(aclmdlDesc *modelDesc, size_t index)
{
    if ((modelDesc == nullptr) || (index >= modelDesc->outputDesc.size())) {
        ACL_LOG_INNER_ERROR("input param is invalid, index[%zu]", index);
        return 0;
    }
    aclmdlTensorDesc &tensorDesc = modelDesc->outputDesc[index];
    return aclmdlGetTensorSize(tensorDesc, index);
}

aclmdlDataset *aclmdlCreateDataset()
{
    ACL_STAGES_REG(acl::ACL_STAGE_CREATE, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DATASET);
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DATASET);
    return new(std::nothrow) aclmdlDataset();
}

aclError aclmdlDestroyDataset(const aclmdlDataset *dataset)
{
    ACL_STAGES_REG(acl::ACL_STAGE_DESTROY, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_DATASET);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dataset);
//    for (size_t i = 0; i < dataset->blobs.size(); ++i) {
//        ACL_DELETE_AND_SET_NULL((const_cast<aclmdlDataset *>(dataset))->blobs[i].tensorDesc);
//    }
    ACL_DELETE_AND_SET_NULL(dataset);
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_DATASET);
    return ACL_SUCCESS;
}

aclError aclmdlAddDatasetBuffer(aclmdlDataset *dataset, aclDataBuffer *dataBuffer)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dataset);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dataBuffer);
    AclModelTensor tensor = AclModelTensor(dataBuffer, nullptr);
    dataset->blobs.push_back(tensor);
    return ACL_SUCCESS;
}

size_t aclmdlGetDatasetNumBuffers(const aclmdlDataset *dataset)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if (dataset == nullptr) {
        ACL_LOG_ERROR("[Check][Dataset]input param[dataset] is null");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"dataset"}));
        return 0;
    }

    return dataset->blobs.size();
}

aclDataBuffer *aclmdlGetDatasetBuffer(const aclmdlDataset *dataset, size_t index)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    if ((dataset == nullptr) || (index >= dataset->blobs.size())) {
        ACL_LOG_ERROR("[Check][Params]input param is invalid, dataset[%p], index[%zu]", dataset, index);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("dataset[%p], index[%zu]", dataset, index);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"input param", errMsg, "check failed"}));
        return nullptr;
    }

    return dataset->blobs[index].dataBuf;
}

aclTensorDesc *aclmdlGetDatasetTensorDesc(aclmdlDataset *dataset, size_t index)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_RET_NULL_INPUT_REPORT(dataset);
    if (index >= dataset->blobs.size()) {
        ACL_LOG_ERROR("[Check][Index]input param index[%zu] must be smaller than output databuf size[%zu]",
                      index, dataset->blobs.size());
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
                                                  std::vector<std::string>({"param", "value", "reason"}),
                                                  std::vector<std::string>({"index", std::to_string(index),
                                                                            "must be smaller than output databuf size"}));
        return nullptr;
    }
    return dataset->blobs[index].tensorDesc;
}

aclError aclmdlSetDatasetTensorDesc(aclmdlDataset *dataset, aclTensorDesc *tensorDesc, size_t index)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL(dataset);
    if (index >= dataset->blobs.size()) {
        ACL_LOG_ERROR("[Check][Index]input param index[%zu] must be smaller than input databuf size[%zu]",
                      index, dataset->blobs.size());
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"index", std::to_string(index),
            "must be smaller than input databuf size"}));
        return ACL_ERROR_INVALID_PARAM;
    }

    if (tensorDesc == nullptr) {
        ACL_DELETE_AND_SET_NULL(dataset->blobs[index].tensorDesc);
        ACL_LOG_INFO("Set tensorDesc for tensor[%zu] successfully, tensorDesc is nullptr", index);
        return ACL_SUCCESS;
    }

    if (dataset->blobs[index].tensorDesc == nullptr) {
        dataset->blobs[index].tensorDesc = new(std::nothrow) aclTensorDesc(*tensorDesc);
        ACL_CHECK_MALLOC_RESULT(dataset->blobs[index].tensorDesc);
    } else {
        *(dataset->blobs[index].tensorDesc) = *tensorDesc;
    }
    ACL_LOG_INFO("Set tensorDesc for tensor[%zu] successfully", index);
    return ACL_SUCCESS;
}

aclError aclmdlLoadFromFile(const char *modelPath, uint32_t *modelId)
{
    ACL_STAGES_REG(acl::ACL_STAGE_LOAD, acl::ACL_STAGE_DEFAULT);
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL);
    ACL_LOG_INFO("start to execute aclmdlLoadFromFile");
    aclError ret = ModelLoadFromFileWithMem(modelPath, modelId, nullptr, 0, nullptr, 0, 0);
    if (ret != ACL_SUCCESS) {
        return ret;
    }
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL);
    ACL_LOG_INFO("successfully execute aclmdlLoadFromFile");
    return ACL_SUCCESS;
}

aclError aclmdlLoadFromFileWithMem(const char *modelPath, uint32_t *modelId,
                                   void *workPtr, size_t workSize,
                                   void *weightPtr, size_t weightSize)
{
    ACL_STAGES_REG(acl::ACL_STAGE_LOAD, acl::ACL_STAGE_DEFAULT);
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL);
    ACL_LOG_INFO("start to execute aclmdlLoadFromFileWithMem, workSize[%zu], weightSize[%zu]", workSize, weightSize);
    aclError ret = ModelLoadFromFileWithMem(modelPath, modelId, workPtr, workSize, weightPtr, weightSize, 0);
    if (ret != ACL_SUCCESS) {
        return ret;
    }
    ACL_LOG_INFO("Load model from file[%s] with memory success, modelId[%u]", modelPath, *modelId);
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL);
    return ACL_SUCCESS;
}

aclError aclmdlLoadFromMem(const void *model, size_t modelSize, uint32_t *modelId)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_STAGES_REG(acl::ACL_STAGE_LOAD, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL);
    ACL_LOG_INFO("start to execute aclmdlLoadFromMem, modelSize[%zu]", modelSize);
    aclError ret = ModelLoadFromMemWithMem(model, modelSize, modelId, nullptr, 0, nullptr, 0, 0);
    if (ret != ACL_SUCCESS) {
        return ret;
    }

    ACL_LOG_INFO("Load model from data success, modelId[%u]", *modelId);
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL);
    return ACL_SUCCESS;
}

aclError aclmdlLoadFromMemWithMem(const void *model, size_t modelSize,
                                  uint32_t *modelId, void *workPtr, size_t workSize,
                                  void *weightPtr, size_t weightSize)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_STAGES_REG(acl::ACL_STAGE_LOAD, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL);
    ACL_LOG_INFO("start to execute AaclmdlLoadFromMemWithMem, modelSize[%zu], workSize[%zu], weightSize[%zu]",
        modelSize, workSize, weightSize);
    aclError ret = ModelLoadFromMemWithMem(model, modelSize, modelId, workPtr, workSize, weightPtr, weightSize, 0);
    if (ret != ACL_SUCCESS) {
        return ret;
    }
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL);
    ACL_LOG_INFO("successfully execute aclmdlLoadFromMemWithMem, modelSize[%zu], workSize[%zu], weightSize[%zu]",
        modelSize, workSize, weightSize);
    return ACL_SUCCESS;
}

aclError aclmdlLoadFromFileWithQ(const char *modelPath, uint32_t *modelId, const uint32_t *inputQ,
                                 size_t inputQNum, const uint32_t *outputQ, size_t outputQNum)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_STAGES_REG(acl::ACL_STAGE_LOAD, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL);
    ACL_LOG_INFO("start to execute aclmdlLoadFromFileWithQ, inputQNum[%zu], outputQNum[%zu]", inputQNum, outputQNum);
    aclError ret = ModelLoadFromFileWithQ(modelPath, modelId, inputQ, inputQNum, outputQ, outputQNum, 0);
    if (ret != ACL_SUCCESS) {
        return ret;
    }
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL);
    ACL_LOG_INFO("successfully execute aclmdlLoadFromFileWithQ, inputQNum[%zu], outputQNum[%zu]",
        inputQNum, outputQNum);
    return ACL_SUCCESS;
}

aclError aclmdlLoadFromMemWithQ(const void *model, size_t modelSize, uint32_t *modelId,
    const uint32_t *inputQ, size_t inputQNum, const uint32_t *outputQ, size_t outputQNum)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_STAGES_REG(acl::ACL_STAGE_LOAD, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL);
    ACL_LOG_INFO("start to execute aclmdlLoadFromMemWithQ, modelSize[%zu], inputQNum[%zu], outputQNum[%zu]",
        modelSize, inputQNum, outputQNum);
    aclError ret = ModelLoadFromMemWithQ(model, modelSize, modelId, inputQ, inputQNum, outputQ, outputQNum, 0);
    if (ret != ACL_SUCCESS) {
        return ret;
    }
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL);
    ACL_LOG_INFO("successfully execute aclmdlLoadFromMemWithQ, modelSize[%zu], inputQNum[%zu], outputQNum[%zu]",
        modelSize, inputQNum, outputQNum);
    return ACL_SUCCESS;
}

static void SetInputData(const vector<AclModelTensor> &blobs, vector<ge::GeTensorDesc> &inputGeDesc, bool &isDynamic)
{
    for (size_t i = 0; i < blobs.size(); ++i) {
        if (blobs[i].tensorDesc != nullptr) {
            isDynamic = true;
            break;
        }
    }
    if (isDynamic) {
        for (size_t i = 0; i < blobs.size(); ++i) {
            if (blobs[i].tensorDesc != nullptr) {
                ge::GeShape shape(blobs[i].tensorDesc->dims);
                ge::GeTensorDesc geTensorDescTmp(shape);
                geTensorDescTmp.SetOriginShape(shape);
                inputGeDesc.push_back(geTensorDescTmp);
            } else {
                ge::GeTensorDesc geTensorDescTmp;
                inputGeDesc.push_back(geTensorDescTmp);
            }
        }
    }
    return;
}

aclError ModelExecute(uint32_t modelId, const aclmdlDataset *input,
    aclmdlDataset *output, bool async, aclrtStream stream)
{
    ACL_LOG_INFO("start to execute ModelExecute, modelId[%u]", modelId);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(input);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(output);

    ge::RunModelData inputData;
    inputData.timeout = 0;
    inputData.timestamp = 0;
    inputData.index = 0;
    inputData.modelId = modelId;

    inputData.dynamic_batch_size = input->dynamicBatchSize;
    inputData.dynamic_image_height = input->dynamicResolutionHeight;
    inputData.dynamic_image_width = input->dynamicResolutionWidth;
    inputData.dynamic_dims = input->dynamicDims;
    ACL_LOG_DEBUG("ModelExecute dynamic param: batch_size[%lu], height[%lu], width[%lu], dim_num[%zu]",
                  input->dynamicBatchSize, input->dynamicResolutionHeight, input->dynamicResolutionWidth,
                  input->dynamicDims.size());
    for (size_t i = 0; i < input->blobs.size(); ++i) {
        ge::DataBuffer inputBuffer;
        auto dataBuffer = input->blobs[i].dataBuf;
        if (dataBuffer == nullptr) {
            ACL_LOG_ERROR("[Check][dataBuffer]input dataset blobs is null, "
                "modelId[%d], index[%zu]", modelId, i);
            acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
                std::vector<std::string>({"param"}),
                std::vector<std::string>({"dataBuffer"}));
            return ACL_ERROR_INVALID_PARAM;
        }
        inputBuffer.data = dataBuffer->data;
        inputBuffer.length = dataBuffer->length;
        inputBuffer.isDataSupportMemShare = false;
        inputData.blobs.push_back(inputBuffer);
    }

    vector<ge::GeTensorDesc> inputGeDesc;
    bool dynamicFlag = false;
    SetInputData(input->blobs, inputGeDesc, dynamicFlag);

    ge::RunModelData outputData;
    outputData.modelId = modelId;
    for (size_t i = 0; i < output->blobs.size(); ++i) {
        ge::DataBuffer outputBuffer;
        auto dataBuffer = output->blobs[i].dataBuf;
        if (dataBuffer == nullptr) {
            ACL_LOG_ERROR("[Check][Databuffer]output dataset blobs is null, modelId[%d], index[%zu]",
                modelId, i);
            acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
                std::vector<std::string>({"param"}),
                std::vector<std::string>({"dataBuffer"}));
            return ACL_ERROR_INVALID_PARAM;
        }
        outputBuffer.data = dataBuffer->data;
        outputBuffer.length = dataBuffer->length;
        outputBuffer.isDataSupportMemShare = false;
        outputData.blobs.push_back(outputBuffer);
    }

    ge::GeExecutor executor;
    ACL_LOG_INFO("call ge interface executor.ExecModel, modelId[%u], asyncMode[%d]",
        modelId, static_cast<int32_t>(async));
    vector<ge::GeTensorDesc> outputGeDesc;
    ge::Status ret = executor.ExecModel(modelId, stream, inputData, inputGeDesc, outputData, outputGeDesc, async);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Exec][Model]Execute model failed, ge result[%u], modelId[%u]", ret, modelId);
        return ACL_GET_ERRCODE_GE(ret);
    }

    if (dynamicFlag) {
        int64_t shape = 1;
        for (size_t i = 0; i < output->blobs.size(); ++i) {
            if (output->blobs[i].tensorDesc != nullptr) {
                aclDestroyTensorDesc(output->blobs[i].tensorDesc);   
            }
            output->blobs[i].tensorDesc = aclCreateTensorDesc(ACL_FLOAT, 1, &shape, ACL_FORMAT_NCHW);
        }
    }

    for (size_t i = 0; i < outputGeDesc.size(); ++i) {
        if (output->blobs[i].tensorDesc != nullptr) {
            output->blobs[i].tensorDesc->dims.clear();
            std::vector<int64_t> dims = outputGeDesc[i].GetShape().GetDims();
            ge::Format format = outputGeDesc[i].GetFormat();
            ge::DataType dataType = outputGeDesc[i].GetDataType();
            for (auto dim : dims) {
                output->blobs[i].tensorDesc->dims.push_back(dim);
                if (format == ge::FORMAT_RESERVED) {
                    output->blobs[i].tensorDesc->format = ACL_FORMAT_UNDEFINED;
                } else {
                    output->blobs[i].tensorDesc->format = static_cast<aclFormat>(format);
                }
                if (dataType == ge::DT_UNDEFINED) {
                    output->blobs[i].tensorDesc->dataType = ACL_DT_UNDEFINED;
                } else { 
                    output->blobs[i].tensorDesc->dataType = static_cast<aclDataType>(dataType);
                }
            }
        }
    }

    ACL_LOG_INFO("successfully execute ModelExecute, modelId[%u]", modelId);
    return ACL_SUCCESS;
}

aclError aclmdlExecute(uint32_t modelId, const aclmdlDataset *input,
    aclmdlDataset *output)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_STAGES_REG(acl::ACL_STAGE_EXEC, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlExecute, modelId[%u]", modelId);
    aclError ret = ModelExecute(modelId, input, output, false, nullptr);
    if (ret == ACL_SUCCESS) {
        ACL_LOG_INFO("aclmdlExecute success, modelId[%u]", modelId);
    } else {
        ACL_LOG_INNER_ERROR("[Exec][Model]modelId[%u] execute failed, result[%d]", modelId, ret);
    }
    return ret;
}

aclError aclmdlExecuteAsync(uint32_t modelId, const aclmdlDataset *input,
                            aclmdlDataset *output, aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_STAGES_REG(acl::ACL_STAGE_EXEC, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlExecuteAsync, modelId[%u]", modelId);
    aclError ret = ModelExecute(modelId, input, output, true, stream);
    if (ret == ACL_SUCCESS) {
        ACL_LOG_INFO("aclmdlExecuteAsync success, modelId[%u]", modelId);
    } else {
        ACL_LOG_INNER_ERROR("[Exec][Model]aclmdlExecuteAsync failed, result[%d], modelId[%u]", ret, modelId);
    }
    return ret;
}

aclError aclmdlUnload(uint32_t modelId)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_STAGES_REG(acl::ACL_STAGE_UNLOAD, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL);
    ACL_LOG_INFO("start to execute ACL_ModelUnload, modelId[%u]", modelId);
    ge::GeExecutor executor;
    ACL_LOG_INFO("call ge interface executor.UnloadModel, modelId[%u]", modelId);
    ge::Status ret = executor.UnloadModel(modelId);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Unload][Model]model unload failed, ge result[%u], modelId[%u]", ret, modelId);
        return ACL_GET_ERRCODE_GE(ret);
    }

    ACL_LOG_INFO("aclmdlUnload success, modelId[%u]", modelId);
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL);
    return ACL_SUCCESS;
}

aclError aclmdlQuerySize(const char *fileName, size_t *workSize, size_t *weightSize)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlQuerySize");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(fileName);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(workSize);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(weightSize);

    ge::GeExecutor executor;
    std::string path(fileName);
    size_t work;
    size_t weight;
    ACL_LOG_DEBUG("call ge interface executor.GetMemAndWeightSize");
    ge::Status ret = executor.GetMemAndWeightSize(path, work, weight);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Get][MemAndWeightSize]query size failed, ge result[%u]", ret);
        return ACL_GET_ERRCODE_GE(ret);
    }

    *workSize = work;
    *weightSize = weight;
    ACL_LOG_INFO("success to get size from file[%s], work size[%zu], weight size[%zu]",
        fileName, *workSize, *weightSize);

    return ACL_SUCCESS;
}

aclError aclmdlQuerySizeFromMem(const void *model, size_t modelSize, size_t *workSize, size_t *weightSize)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute ACL_QueryModelSizeFromMem, modelSize[%zu]", modelSize);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(model);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(workSize);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(weightSize);

    ge::GeExecutor executor;
    size_t work;
    size_t weight;
    ACL_LOG_DEBUG("call ge interface executor.GetMemAndWeightSize, modelSize[%zu]", modelSize);
    ge::Status ret = executor.GetMemAndWeightSize(model, modelSize, work, weight);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Get][MemAndWeightSize]query size from mem failed, ge result[%u]", ret);
        return ACL_GET_ERRCODE_GE(ret);
    }

    *workSize = work;
    *weightSize = weight;
    ACL_LOG_INFO("success to get size from mem, work size[%zu], weight size[%zu]", *workSize, *weightSize);

    return ACL_SUCCESS;
}

aclError aclmdlSetDynamicBatchSize(uint32_t modelId, aclmdlDataset *dataset, size_t index, uint64_t batchSize)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlSetDynamicBatchSize, modelId[%u], index[%zu], batchSize[%lu]",
        modelId, index, batchSize);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dataset);

    if (batchSize == 0) {
        ACL_LOG_INNER_ERROR("[Check][Batchsize]input param[batchSize] invalid, batchSize can't be zero");
        return ACL_ERROR_INVALID_PARAM;
    }

    aclDataBuffer *buf = aclmdlGetDatasetBuffer(dataset, index);
    if (buf == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][buf]failed to get data buffer by index[%zu], dataset buffer is null", index);
        return ACL_ERROR_INVALID_PARAM;
    }

    void *devPtr = aclGetDataBufferAddr(buf);
    if (devPtr == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][devPtr]get addr by index[%zu] failed, data buffer addr can not be null", index);
        return ACL_ERROR_INVALID_PARAM;
    }
    uint64_t memSize = aclGetDataBufferSizeV2(buf);

    dataset->dynamicBatchSize = batchSize;
    dataset->dynamicResolutionHeight = 0;
    dataset->dynamicResolutionWidth = 0;
    ACL_LOG_DEBUG("call ge interface executor.SetDynamicBatchSize, batchSize[%lu]", batchSize);
    ge::GeExecutor executor;
    ge::Status ret = executor.SetDynamicBatchSize(modelId, devPtr, memSize, batchSize);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Set][DynamicBatchSize]set DynamicBatchSize failed, ge result[%u]", ret);
        return ACL_GET_ERRCODE_GE(ret);
    }

    ACL_LOG_INFO("successfully execute aclmdlSetDynamicBatchSize, modelId[%u], index[%zu], batchSize[%lu]",
        modelId, index, batchSize);
    return ACL_SUCCESS;
}

aclError aclmdlSetDynamicHWSize(uint32_t modelId, aclmdlDataset *dataset, size_t index,
    uint64_t height, uint64_t width)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlSetDynamicHWSize, modelId[%u], index[%zu], height[%lu], width[%lu]",
        modelId, index, height, width);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dataset);

    if ((height == 0) || (width == 0)) {
        ACL_LOG_INNER_ERROR("[Check][Params]height[%lu] or width[%lu] is invalid, can't be zero.", height, width);
        return ACL_ERROR_INVALID_PARAM;
    }

    aclDataBuffer *buffer = aclmdlGetDatasetBuffer(dataset, index);
    if (buffer == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][buffer]get data buffer by index[%zu] failed, dataset buffer can not be null",
            index);
        return ACL_ERROR_INVALID_PARAM;
    }

    void *devPtr = aclGetDataBufferAddr(buffer);
    if (devPtr == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][devPtr]get addr by index[%zu] failed, data buffer addr can not be nullptr", index);
        return ACL_ERROR_INVALID_PARAM;
    }
    uint64_t memSize = aclGetDataBufferSizeV2(buffer);

    dataset->dynamicBatchSize = 0;
    dataset->dynamicResolutionHeight = height;
    dataset->dynamicResolutionWidth = width;
    ACL_LOG_DEBUG("call ge interface executor.SetDynamicImageSize, height[%lu],width[%lu]", height, width);
    ge::GeExecutor executor;
    ge::Status ret = executor.SetDynamicImageSize(modelId, devPtr, memSize, height, width);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Set][DynamicImageSize]Set dynamic image size, ge result[%u]", index);
        return ACL_GET_ERRCODE_GE(ret);
    }

    ACL_LOG_INFO("successfully execute aclmdlSetDynamicHWSize, modelId[%u], index[%zu], height[%lu], width[%lu]",
        modelId, index, height, width);
    return ACL_SUCCESS;
}

aclError aclmdlSetInputDynamicDims(uint32_t modelId, aclmdlDataset *dataset, size_t index, const aclmdlIODims *dims)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dataset);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dims);
    if (dims->dimCount == 0) {
        ACL_LOG_ERROR("[Check][dimCount]dimCount[%u] is invalid, can't be zero.", dims->dimCount);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("dimCount[%u] can't be zero", dims->dimCount);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"dimCount", "0", errMsg}));
        return ACL_ERROR_INVALID_PARAM;
    }

    ACL_LOG_INFO("start to execute aclmdlSetInputDynamicDims, modelId[%u], index[%zu], dimCount[%u]",
                 modelId, index, dims->dimCount);
    aclDataBuffer *buffer = aclmdlGetDatasetBuffer(dataset, index);
    if (buffer == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][buffer]get data buffer by index[%zu] failed, dataset buffer can not be null",
            index);
        return ACL_ERROR_INVALID_PARAM;
    }

    void *devPtr = aclGetDataBufferAddr(buffer);
    if (devPtr == nullptr) {
        ACL_LOG_INNER_ERROR("[Get][devPtr]get addr by index[%zu] failed, data buffer addr can not be null", index);
        return ACL_ERROR_INVALID_PARAM;
    }
    uint64_t memSize = aclGetDataBufferSizeV2(buffer);

    dataset->dynamicBatchSize = 0;
    dataset->dynamicResolutionHeight = 0;
    dataset->dynamicResolutionWidth = 0;
    dataset->dynamicDims.clear();
    vector<uint64_t> curAllDims;
    for (size_t i = 0; i < static_cast<std::size_t>(dims->dimCount); ++i) {
        curAllDims.push_back(dims->dims[i]);
    }
    ACL_LOG_DEBUG("Call ge interface executor.SetDynamicDims, dimCount[%u]", dims->dimCount);
    ACL_LOG_DEBUG("Cur all dims size %zu", curAllDims.size());
    ge::GeExecutor executor;
    ge::Status ret = executor.SetDynamicDims(modelId, devPtr, memSize, curAllDims);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Set][DynamicDims]set dynamic dims failed, ge result[%u]", ret);
        return ACL_GET_ERRCODE_GE(ret);
    }

    vector<uint64_t> curDynmaicDims;
    ret = executor.GetCurDynamicDims(modelId, curAllDims, curDynmaicDims);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Get][CurDynamicDims]get current dynamic dims failed, ge result[%u]", ret);
        return ACL_GET_ERRCODE_GE(ret);
    }
    ACL_LOG_DEBUG("current dynamic dims size %zu", curDynmaicDims.size());
    dataset->dynamicDims = curDynmaicDims;

    ACL_LOG_INFO("successfully execute aclmdlSetInputDynamicDims, modelId[%u], index[%zu], dimCount[%u]",
                 modelId, index, dims->dimCount);
    return ACL_SUCCESS;
}

// get real tensor name from modelDesc, it will return nullptr if tensorName isn't in modelDesc
static const char *GetRealTensorName(const aclmdlDesc *modelDesc, const string &tensorName)
{
    for (size_t index = 0; index < modelDesc->inputDesc.size(); ++index) {
        if (modelDesc->inputDesc[index].name == tensorName) {
            return modelDesc->inputDesc[index].name.c_str();
        }
    }

    for (size_t index = 0; index < modelDesc->outputDesc.size(); ++index) {
        if (modelDesc->outputDesc[index].name == tensorName) {
            return modelDesc->outputDesc[index].name.c_str();
        }
    }
    return nullptr;
}

static bool IsConvertTensorNameLegal(const aclmdlDesc *modelDesc, const string &tensorName)
{
    return (GetRealTensorName(modelDesc, tensorName) == nullptr);
}

// current conversion tensor name illegal needs to be transformed
static bool TransConvertTensorNameToLegal(const aclmdlDesc *modelDesc, string &tensorName)
{
    size_t depth = 0;
    tensorName = tensorName + "_";
    queue<string> q;
    q.push(tensorName);
    constexpr size_t maxDepth = 3;
    while (!q.empty()) {
        if (depth == maxDepth) {
            ACL_LOG_INFO("reach max depth[%zu], cannot generate legal convert tensor name", maxDepth);
            tensorName = tensorName.substr(0, tensorName.size() - 1);
            return false;
        }
        size_t len = q.size();
        for (size_t index = 0; index < len; ++index) {
            string curTensorName = q.front();
            q.pop();
            for (char c = 'a'; c <= 'z'; ++c) {
                curTensorName += c;
                if (IsConvertTensorNameLegal(modelDesc, curTensorName)) {
                    tensorName = curTensorName;
                    return true;
                }
                q.push(curTensorName);
                curTensorName = curTensorName.substr(0, curTensorName.size() - 1);
            }
        }
        depth++;
    }
    return false;
}

// convert params to convertName
static void GetConvertTensorName(const aclmdlDesc *modelDesc, size_t index, TensorType tensorType, string &convertName)
{
    convertName = string(TENSOR_NAME_PREFIX) + "_" + string(MODEL_ID_STR) + "_" + std::to_string(modelDesc->modelId);
    if (tensorType == INPUT_TENSOR_TYPE) {
        convertName += ("_" + string(TENSOR_INPUT_STR));
    } else {
        convertName += ("_" + string(TENSOR_OUTPUT_STR));
    }
    convertName += ("_" + std::to_string(index));
    ACL_LOG_INFO("convert realname of tensor success, conversion name = %s", convertName.c_str());
}

// get tensor name to dims with or without realname
static aclError GetTensorDescNameToDims(const aclmdlDesc *modelDesc, const string &realName, TensorType tensorType,
    size_t index, aclmdlIODims *dims)
{
    size_t dimsNameLen = sizeof(dims->name);
    std::string tensorName;
    if ((realName.size() + 1) > dimsNameLen) {
        // use conversion name because realname is too long
        ACL_LOG_INFO("use conversion name because real tensor name is over than %zu", dimsNameLen);
        GetConvertTensorName(modelDesc, index, tensorType, tensorName);
        if (!IsConvertTensorNameLegal(modelDesc, tensorName)) {
            if (!TransConvertTensorNameToLegal(modelDesc, tensorName)) {
                ACL_LOG_WARN("cannot generate legal tensor name, use conversion name %s may has conflict risk",
                    tensorName.c_str());
            }
        }
    } else {
        tensorName = realName;
    }

    auto ret = strncpy_s(dims->name, dimsNameLen, tensorName.c_str(), tensorName.size());
    if (ret != EOK) {
        ACL_LOG_INNER_ERROR("[Copy][Str]call strncpy_s failed, result = %d", ret);
        return ACL_ERROR_FAILURE;
    }
    return ACL_SUCCESS;
}

static aclError GetDims(const aclmdlDesc *modelDesc, TensorType tensorType, DimsType dimsType, size_t index,
    aclmdlIODims *dims)
{
    ACL_REQUIRES_NOT_NULL(dims);
    std::vector<aclmdlTensorDesc> desc;
    if (tensorType == INPUT_TENSOR_TYPE) {
        desc = modelDesc->inputDesc;
    } else {
        desc = modelDesc->outputDesc;
    }

    size_t descSize = desc.size();
    if (index >= descSize) {
        ACL_LOG_INNER_ERROR("[Check][Params]GetDims failed, index[%zu] can not greater than or equal to tensor "
            "size[%zu]", index, descSize);
        return ACL_ERROR_INVALID_PARAM;
    }

    const aclmdlTensorDesc &tensorDesc = desc[index];
    auto ret = GetTensorDescNameToDims(modelDesc, tensorDesc.name, tensorType, index, dims);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Get][TensorDescName]get tensor desc name to dims failed, result = %d", ret);
        return ret;
    }
    std::vector<int64_t> tensorDims;
    if (dimsType == DIMS_TYPE_V1) {
        tensorDims = tensorDesc.dims;
    } else if (dimsType == DIMS_TYPE_V2) {
        tensorDims = tensorDesc.dimsV2;
    } else {
        ACL_LOG_INNER_ERROR("[Check][dimsType]dims type[%d] is invalid", static_cast<int32_t>(dimsType));
        return ACL_ERROR_FAILURE;
    }

    size_t dimSize = tensorDims.size();
    if (dimSize > ACL_MAX_DIM_CNT) {
        ACL_LOG_INNER_ERROR("[Check][dimSize]get dims failed, dims count[%zu] can not larger than max[%d]",
            dims->dimCount, ACL_MAX_DIM_CNT);
        return ACL_ERROR_STORAGE_OVER_LIMIT;
    }
    dims->dimCount = dimSize;

    for (size_t i = 0; i < dimSize; ++i) {
        dims->dims[i] = tensorDims[i];
    }

    return ACL_SUCCESS;
}

aclError aclmdlGetInputDims(const aclmdlDesc *modelDesc, size_t index, aclmdlIODims *dims)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dims);

    aclError ret = GetDims(modelDesc, INPUT_TENSOR_TYPE, DIMS_TYPE_V1, index, dims);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Get][Dims]get input dims failed, result[%d], index[%zu], modelId[%u]", ret, index,
            modelDesc->modelId);
    }

    return ret;
}

aclError aclmdlGetOutputDims(const aclmdlDesc *modelDesc, size_t index, aclmdlIODims *dims)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dims);

    aclError ret = GetDims(modelDesc, OUTPUT_TENSOR_TYPE, DIMS_TYPE_V1, index, dims);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Get][Dims]get output dims failed, result[%d], index[%zu], modelId[%u]",
            ret, index, modelDesc->modelId);
    }

    return ret;
}

aclError aclmdlGetInputDimsV2(const aclmdlDesc *modelDesc, size_t index, aclmdlIODims *dims)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dims);

    aclError ret = GetDims(modelDesc, INPUT_TENSOR_TYPE, DIMS_TYPE_V2, index, dims);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Get][Dims]get input dims(v2) failed, result[%d], index[%zu], modelId[%u]",
            ret, index, modelDesc->modelId);
    }

    return ret;
}

static aclError GetCurGearIndex(const aclmdlDesc *modelDesc, std::vector<uint64_t> shapeInfo,
                                int32_t dynamicType, size_t &curGearIndex)
{
    if (dynamicType == static_cast<int32_t>(ge::DYNAMIC_DIMS)) { // dynamic dims, type is 3
        ACL_LOG_DEBUG("Get dynamic dims gear index, dynamicType[%d], modelId[%u]",
                      dynamicType, modelDesc->modelId);
        for (size_t i = 0; i < modelDesc->dynamicDims.size(); ++i) {
            // shapeInfo is current dims
            if (shapeInfo == modelDesc->dynamicDims[i]) {
                curGearIndex = i;
                return ACL_SUCCESS;
            }
        }
    } else {
        size_t shapeSize = shapeInfo.size();
        if (shapeSize == DYNAMIC_BATCH_SIZE) { // dynamic batch, type is 1
            ACL_LOG_DEBUG("Get dynamic batch gear index, dynamicType[%d], modelId[%u]", dynamicType,
                          modelDesc->modelId);
            for (size_t i = 0; i < modelDesc->dynamicBatch.size(); ++i) {
                // shapeInfo[0] is current batch size
                if (shapeInfo[0] == modelDesc->dynamicBatch[i]) {
                    curGearIndex = i;
                    return ACL_SUCCESS;
                }
            }
        } else if (shapeSize == DYNAMIC_HW_SIZE) { // dynamic hw, type is 2
            ACL_LOG_DEBUG("Get dynamic hw gear index, dynamicType[%d], modelId[%u]", dynamicType, modelDesc->modelId);
            for (size_t i = 0; i < modelDesc->dynamicHW.size(); ++i) {
                // shapeInfo is current hw
                if (shapeInfo == modelDesc->dynamicHW[i]) {
                    curGearIndex = i;
                    return ACL_SUCCESS;
                }
            }
        } else {
            ACL_LOG_INNER_ERROR("[Check][dynamicType]dynamicType[%d] is invalid", dynamicType);
        }
    }

    return ACL_ERROR_FAILURE;
}

static aclError GetCurOuputShapeInfo(const aclmdlDesc *modelDesc, size_t index,
                                     size_t curGearIndex, aclmdlIODims *dims)
{
    ACL_LOG_DEBUG("curGearIndex is %zu, dynamicOutputShapeInfoSize is %zu , modelId is %u",
        curGearIndex, modelDesc->dynamicOutputShape.size(), modelDesc->modelId);
    for (auto &it : modelDesc->dynamicOutputShape) {
        if ((it.size() < MIN_OUTPUT_SHAPE_INFO_SIZE) || (it.size() > MAX_OUTPUT_SHAPE_INFO_SIZE)) {
            ACL_LOG_INNER_ERROR("[Check][dynamicOutputShape]output shape info size[%zu] is invalid, range is "
                "[%zu, %zu]", it.size(), MIN_OUTPUT_SHAPE_INFO_SIZE, MAX_OUTPUT_SHAPE_INFO_SIZE);
            return ACL_ERROR_FAILURE;
        }
        // -1 represents static output gear index value
        // it[0] is gear index and it[1] is output index
        if (((static_cast<int64_t>(curGearIndex) == it[0]) || (it[0] == -1)) &&
            (static_cast<int64_t>(index) == it[1])) {
            int32_t idx = 0;
            for (size_t i = 2; i < it.size(); ++i) { // from the third element is shape info
                dims->dims[idx++] = it[i];
            }
            dims->dimCount = it.size() - 2;
            const aclmdlTensorDesc &tensorDesc = modelDesc->outputDesc[index];
            auto ret = GetTensorDescNameToDims(modelDesc, tensorDesc.name, OUTPUT_TENSOR_TYPE, index, dims);
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Get][TensorDescName]get tensor desc name to dims failed, result = %d", ret);
                return ret;
            }
            return ACL_SUCCESS;
        }
    }

    return ACL_ERROR_FAILURE;
}

aclError aclmdlGetCurOutputDims(const aclmdlDesc *modelDesc, size_t index, aclmdlIODims *dims)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dims);
    uint32_t modelId = modelDesc->modelId;
    size_t descSize = modelDesc->outputDesc.size();
    if (index >= descSize) {
        ACL_LOG_INNER_ERROR("[Check][descSize]aclmdlGetCurOutputDims failed, index[%zu] should be smaller "
            "than tensor size[%zu], modelId[%u]", index, descSize, modelId);
        return ACL_ERROR_INVALID_PARAM;
    }

    std::vector<int64_t> geShapeInfo;
    ge::GeExecutor executor;
    int32_t dynamicType = static_cast<int32_t>(ge::FIXED);
    ge::Status geRet = executor.GetCurShape(modelId, geShapeInfo, dynamicType);
    if (geRet != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Get][CurShape]can not get current shape, ge result[%d], modelId[%u]", geRet, modelId);
        return ACL_GET_ERRCODE_GE(geRet);
    }
    // dynamic batch type is 1, dynamic hw type is 2, dynamic dims type is 3;
    // static model or not set dynamic shape info, dynamic type is 0, other value is invalid
    aclError aclRet;
    size_t shapeSize = geShapeInfo.size();
    if (dynamicType != static_cast<int32_t>(ge::DYNAMIC_DIMS) && shapeSize > 2) {
        ACL_LOG_INNER_ERROR("[Check][dynamicType]shapeSize[%zu] is invalid, modelId[%u]", shapeSize, modelId);
        return ACL_ERROR_GE_FAILURE;
    } else if (shapeSize == 0) {
        ACL_LOG_DEBUG("Dynamic type is 0, model[%u] is static or not set dynamic shape info", modelId);
        aclRet = GetDims(modelDesc, OUTPUT_TENSOR_TYPE, DIMS_TYPE_V1, index, dims);
        if (aclRet != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Get][Dims]get current output dims failed, result[%d], index[%zu], modelId[%u]",
                aclRet, index, modelId);
        }
        return aclRet;
    }

    size_t curGearIndex = 0;
    std::vector<uint64_t> shapeInfo;
    for (auto &it : geShapeInfo) {
        shapeInfo.emplace_back(static_cast<uint64_t>(it));
    }
    aclRet = GetCurGearIndex(modelDesc, shapeInfo, dynamicType, curGearIndex);
    if (aclRet != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Get][CurGearIndex]get current gear index failed, result[%d], index[%zu], "
            "modelId[%u], dynamicBatchSize[%zu], dynamicHWSize[%zu]", aclRet, index, modelId,
            modelDesc->dynamicBatch.size(), modelDesc->dynamicHW.size());
        return aclRet;
    }
    aclRet = GetCurOuputShapeInfo(modelDesc, index, curGearIndex, dims);
    if (aclRet != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Get][CurOuputShapeInfo]get current output shape info failed, result[%d], "
            "index[%zu], modelId[%u], the size of dynamicOutputShape[%zu]", aclRet, index, modelId,
            modelDesc->dynamicOutputShape.size());
        return aclRet;
    }

    return ACL_SUCCESS;
}

static const char *aclmdlGetNameByIndex(const std::vector<aclmdlTensorDesc> &desc, size_t index)
{
    if (index >= desc.size()) {
        ACL_LOG_ERROR("[Check][index]get name by index failed, index[%zu] is larger than or equal to desc size[%zu]",
            index, desc.size());
        std::string errMsg = acl::AclErrorLogManager::FormatStr("cannot larger than or equal to desc size[%zu]",
            desc.size());
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"index", std::to_string(index), errMsg}));
        return "";
    }

    return desc[index].name.c_str();
}

const char *aclmdlGetOpAttr(aclmdlDesc *modelDesc, const char *opName, const char *attr)
{
    ACL_LOG_INFO("start to execute aclmdlGetOpAttr");
    ACL_REQUIRES_NOT_NULL_RET_NULL(modelDesc);
    ACL_REQUIRES_NOT_NULL_RET_NULL(opName);
    ACL_REQUIRES_NOT_NULL_RET_NULL(attr);
    
    std::string opNameStr(opName);
    std::string attrStr(attr);
    if (attrStr != ACL_ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES) {
        ACL_LOG_INNER_ERROR("failed to execute aclmdlGetOpAttr, attr[%s] is invalid, only support "
            "ACL_ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES", attrStr.c_str());
        return nullptr;
    }

    std::unique_lock<std::mutex> lock(aclmdlGetOpAttrMutex);
    auto itOpName = modelDesc->opAttrValueMap.find(opName);
    if (itOpName != modelDesc->opAttrValueMap.end()) {
        auto itAttr = itOpName->second.find(attr);
        if (itAttr != itOpName->second.end()) {
            ACL_LOG_INFO("opName is [%s], the value of attr [%s] is %s", opName, attr, itAttr->second.c_str());
            return itAttr->second.c_str();
        }
    }

    ge::GeExecutor executor;
    uint32_t modelId = modelDesc->modelId;
    ACL_LOG_INFO("Call ge interface executor.GetOpAttr, modelId is [%u], opName is [%s], attr is [%s]",
        modelId, opName, attr);
    std::string attrValue;
    ge::Status ret = executor.GetOpAttr(modelId, opNameStr, attrStr, attrValue);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Get][Opattr]Execute GetOpAttr failed, ge result[%u], modelId[%u]", ret, modelId);
        return nullptr;
    }
    ACL_LOG_INFO("Execute aclmdlGetOpAttr successfully, opName is [%s], the value of attr[%s] is %s", opName, attr,
        attrValue.c_str());
    modelDesc->opAttrValueMap[opNameStr][attrStr] = attrValue;
    return modelDesc->opAttrValueMap[opNameStr][attrStr].c_str();
}

const char *aclmdlGetInputNameByIndex(const aclmdlDesc *modelDesc, size_t index)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlGetInputNameByIndex");
    if (modelDesc == nullptr) {
        ACL_LOG_ERROR("[Check][ModelDesc]modelDesc is null");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"modelDesc"}));
        return "";
    }

    return aclmdlGetNameByIndex(modelDesc->inputDesc, index);
}

const char *aclmdlGetOutputNameByIndex(const aclmdlDesc *modelDesc, size_t index)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlGetOutputNameByIndex");
    if (modelDesc == nullptr) {
        ACL_LOG_ERROR("[Check][ModelDesc]modelDesc is null");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"modelDesc"}));
        return "";
    }

    return aclmdlGetNameByIndex(modelDesc->outputDesc, index);
}

static aclFormat aclmdlGetFormat(const std::vector<aclmdlTensorDesc> &desc, size_t index)
{
    if (index >= desc.size()) {
        ACL_LOG_INNER_ERROR("[Check][index]get data format by index failed, index[%zu] is larger "
            "than or equal to desc size[%zu]", index, desc.size());
        return ACL_FORMAT_UNDEFINED;
    }

    return desc[index].format;
}

aclFormat aclmdlGetInputFormat(const aclmdlDesc *modelDesc, size_t index)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlGetInputFormat");
    if (modelDesc == nullptr) {
        ACL_LOG_ERROR("[Check][ModelDesc]modelDesc is null");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"modelDesc"}));
        return ACL_FORMAT_UNDEFINED;
    }

    return aclmdlGetFormat(modelDesc->inputDesc, index);
}

aclFormat aclmdlGetOutputFormat(const aclmdlDesc *modelDesc, size_t index)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlGetOutputFormat");
    if (modelDesc == nullptr) {
        ACL_LOG_ERROR("[Check][ModelDesc]modelDesc is null");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"params"}),
            std::vector<std::string>({"modelDesc"}));
        return ACL_FORMAT_UNDEFINED;
    }

    return aclmdlGetFormat(modelDesc->outputDesc, index);
}

static aclDataType aclmdlGetDataType(const std::vector<aclmdlTensorDesc> &desc, size_t index)
{
    if (index >= desc.size()) {
        ACL_LOG_INNER_ERROR("[Check][Index]get data type by index failed, index[%zu] is larger than or "
            "equal to desc size[%zu]", index, desc.size());
        return ACL_DT_UNDEFINED;
    }

    return desc[index].dataType;
}

aclDataType aclmdlGetInputDataType(const aclmdlDesc *modelDesc, size_t index)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlGetInputDataType");
    if (modelDesc == nullptr) {
        ACL_LOG_ERROR("[Check][ModelDesc]modelDesc is null");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}), std::vector<std::string>({"modelDesc"}));
        return ACL_DT_UNDEFINED;
    }

    return aclmdlGetDataType(modelDesc->inputDesc, index);
}

aclDataType aclmdlGetOutputDataType(const aclmdlDesc *modelDesc, size_t index)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlGetOutputDataType");
    if (modelDesc == nullptr) {
        ACL_LOG_ERROR("[Check][ModelDesc]modelDesc is null");
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_NULL_POINTER_MSG,
            std::vector<std::string>({"param"}),
            std::vector<std::string>({"modelDesc"}));
        return ACL_DT_UNDEFINED;
    }

    return aclmdlGetDataType(modelDesc->outputDesc, index);
}

static aclError aclmdlGetIndexByName(const std::vector<aclmdlTensorDesc> &desc, const char *name, size_t *index)
{
    ACL_REQUIRES_NOT_NULL(name);
    ACL_REQUIRES_NOT_NULL(index);

    std::string tensorName(name);
    for (size_t i = 0; i < desc.size(); ++i) {
        if (desc[i].name == tensorName) {
            *index = i;
            ACL_LOG_DEBUG("success to get tensor[%s] index[%zu]", name, *index);
            return ACL_SUCCESS;
        }
    }

    ACL_LOG_INNER_ERROR("[Get][Index]get index by name failed, cannot find tensor name[%s]", name);
    return ACL_ERROR_INVALID_PARAM;
}

aclError aclmdlGetInputIndexByName(const aclmdlDesc *modelDesc, const char *name, size_t *index)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlGetInputIndexByName");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(name);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(index);

    ACL_LOG_INFO("successfully execute aclmdlGetInputIndexByName");
    return aclmdlGetIndexByName(modelDesc->inputDesc, name, index);
}

aclError aclmdlGetOutputIndexByName(const aclmdlDesc *modelDesc, const char *name, size_t *index)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlGetOutputIndexByName");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(name);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(index);

    ACL_LOG_INFO("successfully execute aclmdlGetOutputIndexByName");
    return aclmdlGetIndexByName(modelDesc->outputDesc, name, index);
}

aclError aclmdlGetDynamicBatch(const aclmdlDesc *modelDesc, aclmdlBatch *batch)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlGetDynamicBatch");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(batch);

    size_t batchCnt = modelDesc->dynamicBatch.size();
    if (batchCnt > ACL_MAX_BATCH_NUM) {
        ACL_LOG_ERROR("[Check][batchCnt]aclmdlGetBatch failed, batch count[%zu] is larger than max batch num[%d]",
            batchCnt, ACL_MAX_BATCH_NUM);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("batch count[%zu] is larger than max batch num[%d]",
            batchCnt, ACL_MAX_BATCH_NUM);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"batch count", std::to_string(batchCnt), errMsg}));
        return ACL_ERROR_STORAGE_OVER_LIMIT;
    }

    batch->batchCount = batchCnt;
    if (batchCnt == 0) {
        ACL_LOG_WARN("batch count is 0");
        return ACL_SUCCESS;
    }

    for (size_t i = 0; i < batchCnt; ++i) {
        batch->batch[i] = modelDesc->dynamicBatch[i];
    }

    ACL_LOG_INFO("successfully execute aclmdlGetDynamicBatch");
    return ACL_SUCCESS;
}

aclError aclmdlGetDynamicHW(const aclmdlDesc *modelDesc, size_t index, aclmdlHW *hw)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlGetDynamicHW");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(hw);

    size_t hwCnt = modelDesc->dynamicHW.size();
    if (hwCnt > ACL_MAX_HW_NUM) {
        ACL_LOG_INNER_ERROR("[Check][hwCnt]aclmdlGetHW failed, hw count[%zu] is larger than max[%d]",
            hwCnt, ACL_MAX_HW_NUM);
        return ACL_ERROR_STORAGE_OVER_LIMIT;
    }

    hw->hwCount = hwCnt;
    if (hwCnt == 0) {
        ACL_LOG_WARN("hw count is 0");
        return ACL_SUCCESS;
    }

    for (size_t i = 0; i < hwCnt; ++i) {
        for (size_t j = 0; j < 2; ++j) { // dynamic hw,size is 2
            hw->hw[i][j] = modelDesc->dynamicHW[i][j];
        }
    }
    ACL_LOG_INFO("successfully execute aclmdlGetDynamicHW");
    return ACL_SUCCESS;
}

aclError aclmdlGetInputDynamicGearCount(const aclmdlDesc *modelDesc, size_t index, size_t *gearCount)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlGetInputDynamicGearCount");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(gearCount);

    if (index != static_cast<size_t>(-1)) {
        ACL_LOG_INNER_ERROR("[Check][index]aclmdlGetInputDynamicGearCount failed, index must be -1 while input is %zu.",
            index);
        return ACL_ERROR_INVALID_PARAM;
    }

    size_t dimCnt = modelDesc->dynamicDims.size();
    if (dimCnt > ACL_MAX_DIM_CNT) {
        ACL_LOG_INNER_ERROR("[Check][dimCnt]aclmdlGetInputDynamicGearCount failed, dimCnt[%zu] is "
            "larger than max[%d]", dimCnt, ACL_MAX_DIM_CNT);
        return ACL_ERROR_STORAGE_OVER_LIMIT;
    }

    if (dimCnt == 0) {
        *gearCount = 0;
        ACL_LOG_WARN("Gear count is 0");
        return ACL_SUCCESS;
    }
    *gearCount = dimCnt;
    ACL_LOG_INFO("successfully execute aclmdlGetInputDynamicGearCount");
    return ACL_SUCCESS;
}

aclError aclmdlGetInputDynamicDims(const aclmdlDesc *modelDesc, size_t index, aclmdlIODims *dims, size_t gearCount)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlGetInputDynamicDims");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dims);
    if (index != static_cast<std::size_t>(-1)) {
        ACL_LOG_INNER_ERROR("[Check][index]aclmdlGetInputDynamicDims failed, index must be -1 but it is %zu", index);
        return ACL_ERROR_INVALID_PARAM;
    }
    if (gearCount < modelDesc->dynamicDims.size()) {
        ACL_LOG_INNER_ERROR("[Check][gearCount]Gear count[%zu] can not less than model's dynamic gear count[%zu]",
            gearCount, modelDesc->dynamicDims.size());
        return ACL_ERROR_INVALID_PARAM;
    }
    vector<int64_t> allRawDims;
    for (auto &dataName : modelDesc->dataNameOrder) {
        for (auto &inputDesc : modelDesc->inputDesc) {
            if (inputDesc.name == dataName) {
                allRawDims.insert(allRawDims.end(), inputDesc.dims.begin(), inputDesc.dims.end());
            }
        }
    }

    for (size_t i = 0; i < modelDesc->dynamicDims.size(); ++i) {
        size_t begIndex = 0;
        for (size_t j = 0; j < allRawDims.size(); ++j) {
            if (allRawDims[j] < 0) {
                if (begIndex >= modelDesc->dynamicDims[i].size()) {
                    ACL_LOG_INNER_ERROR("[Check][begIndex]User input data index[%zu] shape size overflow", index);
                    return ACL_ERROR_INVALID_PARAM;
                }
                dims[i].dims[j] = modelDesc->dynamicDims[i][begIndex++];
            } else {
                dims[i].dims[j] = allRawDims[j];
            }
            dims[i].dimCount = allRawDims.size();
        }
    }
    ACL_LOG_INFO("successfully execute aclmdlGetInputDynamicDims");
    return ACL_SUCCESS;
}

aclError aclmdlCreateAndGetOpDesc(uint32_t deviceId, uint32_t streamId, uint32_t taskId, char *opName,
    size_t opNameLen, aclTensorDesc **inputDesc, size_t *numInputs, aclTensorDesc **outputDesc, size_t *numOutputs)
{
    ACL_LOG_INFO("start to execute aclmdlCreateAndGetOpDesc, deviceId[%u], streamId[%u], taskId[%u]",
        deviceId, streamId, taskId);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(opName);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(inputDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(outputDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(numInputs);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(numOutputs);

    ge::GeExecutor executor;
    ge::OpDescInfo opDescInfo;
    ACL_LOG_DEBUG("call ge interface executor.GetOpDescInfo");
    ge::Status geRet = executor.GetOpDescInfo(deviceId, streamId, taskId, opDescInfo);
    if (geRet != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Get][OpDescInfo]get op desc faild, ge result[%d], deviceId[%u], streamId[%u], taskId[%u]",
            geRet, deviceId, streamId, taskId);
        return ACL_GET_ERRCODE_GE(geRet);
    }

    if (opNameLen <= opDescInfo.op_name.length()) {
        ACL_LOG_INNER_ERROR("[Check][opNameLen]input length = %zu must be larger than op name real length = %zu",
            opNameLen, opDescInfo.op_name.length());
        return ACL_ERROR_INVALID_PARAM;
    }
    auto ret = strncpy_s(opName, opNameLen, opDescInfo.op_name.c_str(),
        opDescInfo.op_name.length());
    if (ret != EOK) {
        ACL_LOG_INNER_ERROR("[Copy][OpName]copy op name failed, copy result = %d, input opNameLen = %zu, "
            "real opNameLen = %zu", ret, opNameLen, opDescInfo.op_name.length());
        return ACL_ERROR_FAILURE;
    }

    size_t inputNum = opDescInfo.input_format.size();
    size_t outputNum = opDescInfo.output_format.size();

    ACL_REQUIRES_POSITIVE(inputNum);
    ACL_REQUIRES_POSITIVE(outputNum);
    *inputDesc = new(std::nothrow) aclTensorDesc[inputNum];
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(*inputDesc);
    *outputDesc = new(std::nothrow) aclTensorDesc[outputNum];
    if (*outputDesc == nullptr) {
        ACL_LOG_INNER_ERROR("[Check][outputDesc]alloc outputDesc memory failed");
        ACL_DELETE_ARRAY_AND_SET_NULL(*inputDesc);
        return ACL_ERROR_FAILURE;
    }

    for (size_t idx = 0; idx < inputNum; ++idx) {
        inputDesc[idx]->format = static_cast<aclFormat>(opDescInfo.input_format[idx]);
        inputDesc[idx]->dataType = static_cast<aclDataType>(opDescInfo.input_data_type[idx]);
        inputDesc[idx]->dims.assign(opDescInfo.input_shape[idx].begin(), opDescInfo.input_shape[idx].end());
        inputDesc[idx]->address = opDescInfo.input_addrs[idx];
    }
    for (size_t idx = 0; idx < outputNum; ++idx) {
        outputDesc[idx]->format = static_cast<aclFormat>(opDescInfo.output_format[idx]);
        outputDesc[idx]->dataType = static_cast<aclDataType>(opDescInfo.output_data_type[idx]);
        outputDesc[idx]->dims.assign(opDescInfo.output_shape[idx].begin(), opDescInfo.output_shape[idx].end());
        outputDesc[idx]->address = opDescInfo.output_addrs[idx];
    }
    *numInputs = inputNum;
    *numOutputs = outputNum;
    ACL_LOG_INFO("successfully execute aclmdlCreateAndGetOpDesc, deviceId[%u], streamId[%u], "
        "taskId[%u], numInputs[%zu], numOutputs[%zu]", deviceId, streamId, taskId, *numInputs, *numOutputs);
    return ACL_SUCCESS;
}

aclError aclmdlLoadWithConfig(const aclmdlConfigHandle *handle, uint32_t *modelId)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_MODEL);
    ACL_STAGES_REG(acl::ACL_STAGE_LOAD, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL);
    ACL_LOG_INFO("start to execute aclmdlLoadWithConfig");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(handle);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelId);
    if (!CheckMdlConfigHandle(handle)) {
        ACL_LOG_INNER_ERROR("[Check][ConfigHandle]model config is invalid because some param may not be set");
        return ACL_ERROR_INVALID_PARAM;
    }
    switch (handle->mdlLoadType) {
        case ACL_MDL_LOAD_FROM_FILE:
            return ModelLoadFromFileWithMem(handle->loadPath.c_str(), modelId,
                nullptr, 0, nullptr, 0, handle->priority);
        case ACL_MDL_LOAD_FROM_FILE_WITH_MEM:
            return ModelLoadFromFileWithMem(handle->loadPath.c_str(), modelId, handle->workPtr,
                handle->workSize, handle->weightPtr, handle->weightSize, handle->priority);
        case ACL_MDL_LOAD_FROM_MEM:
            return ModelLoadFromMemWithMem(handle->mdlAddr, handle->mdlSize, modelId,
                nullptr, 0, nullptr, 0, handle->priority);
        case ACL_MDL_LOAD_FROM_MEM_WITH_MEM:
            return ModelLoadFromMemWithMem(handle->mdlAddr, handle->mdlSize,
                modelId, handle->workPtr, handle->workSize,
                handle->weightPtr, handle->weightSize, handle->priority);
        case ACL_MDL_LOAD_FROM_FILE_WITH_Q:
            return ModelLoadFromFileWithQ(handle->loadPath.c_str(), modelId, handle->inputQ,
                handle->inputQNum, handle->outputQ, handle->outputQNum, handle->priority);
        case ACL_MDL_LOAD_FROM_MEM_WITH_Q:
            return ModelLoadFromMemWithQ(handle->mdlAddr, handle->mdlSize, modelId,
                handle->inputQ, handle->inputQNum,
                handle->outputQ, handle->outputQNum, handle->priority);
        default:
            ACL_LOG_INNER_ERROR("[Load][Model]model load type[%zu] is invalid, it should be in [%zu, %zu]",
                handle->mdlLoadType, ACL_MDL_LOAD_FROM_FILE, ACL_MDL_LOAD_FROM_MEM_WITH_Q);
            return ACL_ERROR_INVALID_PARAM;
    }
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_LOAD_UNLOAD_MODEL);
    ACL_LOG_INFO("successfully execute aclmdlLoadWithConfig, modelId[%u]", *modelId);
    return ACL_SUCCESS;
}

// try to transfer conversion name to real tensor name, it will return nullptr if conversion name isn't
// satisfy condition
static const char *TransTensorNameToReal(const aclmdlDesc *modelDesc, const string &tensorName)
{
    vector<string> valArr;
    acl::string_utils::Split(tensorName, '_', valArr);
    if ((valArr.size() != TENSOR_NAME_ATTR_NUM) && (valArr.size() != TENSOR_NAME_ATTR_NUM + 1)) {
        ACL_LOG_INNER_ERROR("[Check][Params]tensorName[%s] cannot be devided into %zu parts",
            tensorName.c_str(), TENSOR_NAME_ATTR_NUM);
        return nullptr;
    }
    if (valArr[0] != TENSOR_NAME_PREFIX) {
        ACL_LOG_INNER_ERROR("[Check][Param]cannot find Attr[%s] in tensorName[%s]",
            TENSOR_NAME_PREFIX, tensorName.c_str());
        return nullptr;
    }
    const int base = 10;
    if ((valArr[1] == MODEL_ID_STR) && (acl::string_utils::IsDigit(valArr[2]))) {
        auto modelId = strtoul(valArr[2].c_str(), nullptr, base);
        if (modelId != modelDesc->modelId) {
            ACL_LOG_INNER_ERROR("[Check][modelId]modelId[%lu] is invalid, tensorName[%s]", modelId, tensorName.c_str());
            return nullptr;
        }
    } else {
        ACL_LOG_INNER_ERROR("[Check][modelId]cannot find attr[%s] or modelId in tensorName[%s]",
            MODEL_ID_STR, tensorName.c_str());
        return nullptr;
    }
    if (acl::string_utils::IsDigit(valArr[4])) {
        auto index = strtoul(valArr[4].c_str(), nullptr, base);
        if (valArr[3] == TENSOR_INPUT_STR) {
            if (index >= modelDesc->inputDesc.size() || index < 0) {
                ACL_LOG_INNER_ERROR("[Check][index]inputDesc index[%lu] should be in [0, %zu), tensorName[%s]",
                    index, modelDesc->inputDesc.size(), tensorName.c_str());
                return nullptr;
            }
            return modelDesc->inputDesc[index].name.c_str();
        } else if (valArr[3] == TENSOR_OUTPUT_STR) {
            if (index >= modelDesc->outputDesc.size() || index < 0) {
                ACL_LOG_INNER_ERROR("[Check][index]outputDesc index[%lu] should be in [0, %zu), tensorName[%s]", index,
                    modelDesc->outputDesc.size(), tensorName.c_str());
                return nullptr;
            }
            return modelDesc->outputDesc[index].name.c_str();
        }
    }

    ACL_LOG_INNER_ERROR("[Find][Attr]cannot find [input_%s] or [ouput_%s] in tensorName[%s]", valArr[4].c_str(),
        valArr[4].c_str(), tensorName.c_str());
    return nullptr;
}

const char *aclmdlGetTensorRealName(const aclmdlDesc *modelDesc, const char *name)
{
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclmdlGetTensorName");
    ACL_REQUIRES_NOT_NULL_RET_NULL_INPUT_REPORT(modelDesc);
    ACL_REQUIRES_NOT_NULL_RET_NULL_INPUT_REPORT(name);
    const char *realTensorName = GetRealTensorName(modelDesc, name);
    if (realTensorName != nullptr) {
        ACL_LOG_INFO("successfully execute aclmdlGetTensorName, realTensorName = %s", realTensorName);
        return realTensorName;
    }
    realTensorName = TransTensorNameToReal(modelDesc, name);
    if (realTensorName != nullptr) {
        ACL_LOG_INFO("successfully execute aclmdlGetTensorName, realTensorName = %s", realTensorName);
        return realTensorName;
    }
    ACL_LOG_INNER_ERROR("[Get][TensorName]execute aclmdlGetTensorName failed, name[%s] is invalid.", name);
    return nullptr;
}
