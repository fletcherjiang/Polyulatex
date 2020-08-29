/**
* @file op_executor.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "op_executor.h"
#include <graph/debug/ge_attr_define.h>
#include "types/tensor_desc_internal.h"
#include "types/acl_op.h"
#include "common/log_inner.h"
#include "op_model_manager.h"
#include "single_op/executor/stream_executor.h"
#include "error_codes_inner.h"
#include "utils/string_utils.h"
#include "utils/array_utils.h"

namespace acl {
namespace {
constexpr int MAX_CACHED_NUM = 128;
}

aclError OpExecutor::DoExecuteAsync(ge::SingleOp *singleOp,
                                    const AclOp &aclOp,
                                    const aclDataBuffer *const *inputs,
                                    aclDataBuffer *const *outputs,
                                    std::map<int32_t, bool> optionalInputMap,
                                    bool executeWithExactModel)
{
    std::vector<ge::DataBuffer> inputVec;
    std::vector<ge::DataBuffer> outputVec;

    for (int i = 0; i < aclOp.numInputs; ++i) {
        // skip optional input
        if (optionalInputMap[i] == true) {
            ACL_LOG_INFO("unused optional input, index %d", i);
            continue;
        }
        if (aclOp.inputDesc[i]->CheckConstTensor(executeWithExactModel)) {
            ACL_LOG_INFO("the inputTensor is const tensor, index %d", i);
            continue;
        }
        ge::DataBuffer buffer;
        buffer.data = inputs[i]->data;
        buffer.length = inputs[i]->length;
        buffer.placement = aclOp.inputDesc[i]->memtype;
        inputVec.emplace_back(buffer);
    }

    for (int i = 0; i < aclOp.numOutputs; ++i) {
        if (aclOp.outputDesc[i]->CheckConstTensor(executeWithExactModel)) {
            ACL_LOG_INFO("the outputTensor is const tensor, index %d", i);
            continue;
        }
        ge::DataBuffer buffer;
        buffer.data = outputs[i]->data;
        buffer.length = outputs[i]->length;
        buffer.placement = aclOp.outputDesc[i]->memtype;
        outputVec.emplace_back(buffer);
    }

    ACL_LOG_INFO("To invoke GeExecutor::ExecuteAsync");
    ge::Status ret = ge::GeExecutor::ExecuteAsync(singleOp, inputVec, outputVec);
    if (ret != ge::SUCCESS) {
        ACL_LOG_ERROR("[Exec][Op]Execute op failed. ge result = %u", ret);
        return ACL_GET_ERRCODE_GE(ret);
    }

    return ACL_SUCCESS;
}

aclError OpExecutor::DoExecuteAsync(ge::DynamicSingleOp *singleOp,
                                    const AclOp &aclOp,
                                    const aclDataBuffer *const inputs[],
                                    aclDataBuffer *const outputs[],
                                    bool executeWithExactModel)
{
    std::vector<ge::DataBuffer> inputVec;
    std::vector<ge::DataBuffer> outputVec;
    std::vector<ge::GeTensorDesc> inputDesc;
    std::vector<ge::GeTensorDesc> outputDesc;
    for (int i = 0; i < aclOp.numInputs; ++i) {
        if (aclOp.inputDesc[i]->CheckConstTensor(executeWithExactModel)) {
            ACL_LOG_INFO("the inputTensor is const tensor, index %d", i);
            continue;
        }
        ge::GeTensorDesc tensorDesc(ge::GeShape(aclOp.inputDesc[i]->dims));
        tensorDesc.SetOriginShape(tensorDesc.GetShape());
        ge::DataType geDataType = ge::DT_UNDEFINED;
        if (aclOp.inputDesc[i]->dataType != ACL_DT_UNDEFINED) {
            geDataType = static_cast<::ge::DataType>(aclOp.inputDesc[i]->dataType);
        }
        tensorDesc.SetDataType(geDataType);
        if (aclOp.inputDesc[i]->storageFormat != ACL_FORMAT_UNDEFINED) {
            ge::AttrUtils::SetInt(tensorDesc, ge::ATTR_NAME_STORAGE_FORMAT,
                static_cast<int64_t>(aclOp.inputDesc[i]->storageFormat));
            ge::AttrUtils::SetListInt(tensorDesc, ge::ATTR_NAME_STORAGE_SHAPE, aclOp.inputDesc[i]->storageDims);
        }
        ge::AttrUtils::SetInt(tensorDesc, ge::ATTR_NAME_PLACEMENT, static_cast<int64_t>(aclOp.inputDesc[i]->memtype));
        inputDesc.emplace_back(std::move(tensorDesc));

        ge::DataBuffer buffer;
        buffer.data = inputs[i]->data;
        buffer.length = inputs[i]->length;
        buffer.placement = aclOp.inputDesc[i]->memtype;
        inputVec.emplace_back(buffer);
    }
    ACL_LOG_INFO("Inputbuff and inputDesc are ready");
    for (int i = 0; i < aclOp.numOutputs; ++i) {
        if (aclOp.outputDesc[i]->CheckConstTensor(executeWithExactModel)) {
            ACL_LOG_INFO("the outputTensor is const tensor, index %d", i);
            continue;
        }
        ge::GeTensorDesc tensorDesc(ge::GeShape(aclOp.outputDesc[i]->dims));
        tensorDesc.SetOriginShape(tensorDesc.GetShape());
        ge::DataType geDataType = ge::DT_UNDEFINED;
        if (aclOp.outputDesc[i]->dataType != ACL_DT_UNDEFINED) {
            geDataType = static_cast<::ge::DataType>(aclOp.outputDesc[i]->dataType);
        }
        tensorDesc.SetDataType(geDataType);
        if (aclOp.outputDesc[i]->storageFormat != ACL_FORMAT_UNDEFINED) {
            ge::AttrUtils::SetInt(tensorDesc, ge::ATTR_NAME_STORAGE_FORMAT,
                static_cast<int64_t>(aclOp.outputDesc[i]->storageFormat));
            ge::AttrUtils::SetListInt(tensorDesc, ge::ATTR_NAME_STORAGE_SHAPE, aclOp.outputDesc[i]->storageDims);
        }
        ge::AttrUtils::SetInt(tensorDesc, ge::ATTR_NAME_PLACEMENT, static_cast<int64_t>(aclOp.outputDesc[i]->memtype));
        outputDesc.emplace_back(std::move(tensorDesc));

        ge::DataBuffer buffer;
        buffer.data = outputs[i]->data;
        buffer.length = outputs[i]->length;
        buffer.placement = aclOp.outputDesc[i]->memtype;
        outputVec.emplace_back(buffer);
    }

    ACL_LOG_INFO("To invoke GeExecutor::ExecuteAsync");
    ge::Status ret = ge::GeExecutor::ExecuteAsync(singleOp, inputDesc, inputVec, outputDesc, outputVec);
    if (ret != ge::SUCCESS) {
        ACL_LOG_ERROR("[Exec][Op]Execute op failed. ge result = %u", ret);
        return ACL_GET_ERRCODE_GE(ret);
    }

    if (aclOp.exeucteType == ACL_OP_EXECUTE_V2) {
        ACL_LOG_INFO("Begin to update outputDesc");
        for (size_t i = 0; i < outputDesc.size(); ++i) {
            ge::GeShape outputShape = outputDesc[i].GetShape();
            std::vector<int64_t> outputDims = outputShape.GetDims();
            ACL_LOG_INFO("update outputDesc[%zu] dims is [%s]", i, string_utils::VectorToString(outputDims).c_str());
            const_cast<aclTensorDesc *>(aclOp.outputDesc[i])->dims = outputDims;
        }
        ACL_LOG_INFO("Update outputDesc success");
    }
    ACL_LOG_INFO("GeExecutor::ExecuteAsync success");
    return ACL_SUCCESS;
}

ge::SingleOp *OpExecutor::LoadSingleOp(const OpModel &modelInfo, aclrtStream stream)
{
    ge::ModelData modelData;
    ge::SingleOp *singleOp = nullptr;
    modelData.model_data = modelInfo.data.get();
    modelData.model_len = modelInfo.size;
    ACL_LOG_INFO("call ge interface LoadSingleOp");
    auto status = ge::GeExecutor::LoadSingleOpV2(modelInfo.name, modelData, stream, &singleOp, modelInfo.opModelId);
    if (status != ge::SUCCESS) {
        ACL_LOG_ERROR("[Load][Op]Load operator failed. model = %s, ge result = %u", modelInfo.name.c_str(), status);
    }

    return singleOp;
}

ge::DynamicSingleOp *OpExecutor::LoadDynamicSingleOp(const OpModel &modelInfo, aclrtStream stream)
{
    ge::ModelData modelData;
    ge::DynamicSingleOp *singleOp = nullptr;
    modelData.model_data = modelInfo.data.get();
    modelData.model_len = modelInfo.size;
    ACL_LOG_INFO("call ge interface LoadDynamicSingleOp");
    auto status = ge::GeExecutor::LoadDynamicSingleOpV2(modelInfo.name, modelData, stream, &singleOp,
                                                        modelInfo.opModelId);
    if (status != ge::SUCCESS) {
        ACL_LOG_ERROR("[Load][Op]Load dynamic operator failed. model = %s, ge result = %u",
            modelInfo.name.c_str(), status);
    }

    return singleOp;
}

aclError OpExecutor::ExecuteAsync(const AclOp &aclOp,
                                  const aclDataBuffer *const *inputs,
                                  aclDataBuffer *const *outputs,
                                  aclrtStream stream)
{
    if (OpKernelSelector::GetInstance().HasSelectFunc(aclOp.opType)) {
        ACL_LOG_INFO("Dynamic kernel selector is registered for %s", aclOp.opType.c_str());
        aclrtContext context = nullptr;
        ACL_REQUIRES_OK(aclrtGetCurrentContext(&context));
        auto *streamExecutor = Executors::GetOrCreate(context, stream);
        ACL_CHECK_MALLOC_RESULT(streamExecutor);
        auto ret = streamExecutor->ExecuteAsync(aclOp, inputs, outputs);
        if (ret == ACL_SUCCESS) {
            return ACL_SUCCESS;
        }
    }
    ACL_LOG_INFO("OpExecutor::ExecuteAsync aclOp = %s", aclOp.DebugString().c_str());
    OpModel opModel;
    aclError ret;
    bool isDynamic = false;

    ACL_REQUIRES_OK(OpModelManager::GetInstance().MatchOpModel(aclOp, opModel, isDynamic));
    ACL_LOG_DEBUG("match opModel success, opType = %s, isDynamic = %d", aclOp.opType.c_str(), isDynamic);
    bool isExactModel = (opModel.isStaticModelWithFuzzCompile == 0) ? true : false;
    if (isDynamic) {
        ACL_LOG_INFO("begin to load dynamic model. model = %s", opModel.name.c_str());
        auto executor = LoadDynamicSingleOp(opModel, stream);
        if (executor == nullptr) {
            return ACL_ERROR_OP_LOAD_FAILED;
        }
        ret = DoExecuteAsync(executor, aclOp, inputs, outputs, isExactModel);
    } else {
        ACL_LOG_INFO("begin to load static model. model = %s", opModel.name.c_str());
        auto executor = LoadSingleOp(opModel, stream);
        if (executor == nullptr) {
            return ACL_ERROR_OP_LOAD_FAILED;
        }
        std::map <int32_t, bool> optionalInputMap;
        array_utils::GetOptionalInputMap(aclOp.numInputs, aclOp.inputDesc, optionalInputMap);
        ret = DoExecuteAsync(executor, aclOp, inputs, outputs, optionalInputMap, isExactModel);
    }

    return ret;
}

aclError OpExecutor::ExecuteAsync(OpHandle &opHandle,
                                  const aclDataBuffer *const *inputs,
                                  aclDataBuffer *const *outputs,
                                  aclrtStream stream)
{
    if (opHandle.kernelDesc != nullptr) {
        aclrtContext context = nullptr;
        ACL_REQUIRES_OK(aclrtGetCurrentContext(&context));
        auto *streamExecutor = Executors::GetOrCreate(context, stream);
        ACL_CHECK_MALLOC_RESULT(streamExecutor);
        return streamExecutor->ExecuteAsync(*opHandle.kernelDesc,
                                            opHandle.numInputs,
                                            inputs,
                                            opHandle.numOutputs,
                                            outputs);
    }
    aclError ret;
    if (opHandle.isDynamic) {
        ACL_LOG_INFO("begin to load dynamic model. model = %s, stream = %p", opHandle.opModel.name.c_str(), stream);
        auto &cachedExecutors = opHandle.cachedDynamicOperators;
        ge::DynamicSingleOp *singleOp = nullptr;
        auto it = cachedExecutors.find(stream);
        if (it != cachedExecutors.end()) {
            singleOp = it->second;
        } else {
            singleOp = LoadDynamicSingleOp(opHandle.opModel, stream);
            if (singleOp == nullptr) {
                ACL_LOG_ERROR("[Load][Op]LoadSingleOp failed");
                return ACL_ERROR_OP_LOAD_FAILED;
            }

            // Just for protection. Preventing from cache grows to large.
            if (cachedExecutors.size() > MAX_CACHED_NUM) {
                auto toErase = cachedExecutors.begin();
                ACL_LOG_WARN("cache[%zu] reaches max size[%zu], evict one object. stream = %p",
                    cachedExecutors.size(), MAX_CACHED_NUM, toErase->first);
                cachedExecutors.erase(toErase);
            }

            ACL_LOG_INFO("cache operator executor. model = %s, stream = %p", opHandle.opModel.name.c_str(), stream);
            opHandle.cachedDynamicOperators.emplace(stream, singleOp);
        }
        ret = DoExecuteAsync(singleOp, opHandle.aclOp, inputs, outputs);
    } else {
        ACL_LOG_INFO("begin to load static model. model = %s, stream = %p", opHandle.opModel.name.c_str(), stream);
        auto &cachedExecutors = opHandle.cachedOperators;
        ge::SingleOp *singleOp = nullptr;
        auto it = opHandle.cachedOperators.find(stream);
        if (it != cachedExecutors.end()) {
            singleOp = it->second;
        } else {
            singleOp = LoadSingleOp(opHandle.opModel, stream);
            if (singleOp == nullptr) {
                ACL_LOG_ERROR("[Load][Op]LoadSingleOp failed");
                return ACL_ERROR_OP_LOAD_FAILED;
            }

            // Just for protection. Preventing from cache grows to large.
            if (cachedExecutors.size() > MAX_CACHED_NUM) {
                auto toErase = cachedExecutors.begin();
                ACL_LOG_INFO("cache[%zu] reaches max size[%zu], evict one object. stream = %p",
                    cachedExecutors.size(), MAX_CACHED_NUM, toErase->first);
                cachedExecutors.erase(toErase);
            }

            ACL_LOG_INFO("cache operator executor. model = %s, stream = %p", opHandle.opModel.name.c_str(), stream);
            opHandle.cachedOperators.emplace(stream, singleOp);
        }
        ret = DoExecuteAsync(singleOp, opHandle.aclOp, inputs, outputs, opHandle.optionalInputMap);
    }

    return ret;
}

aclError OpExecutor::CreateOpHandle(const AclOp &aclOp, OpHandle **handle)
{
    std::unique_ptr<OpHandle> handlePtr = std::unique_ptr<OpHandle>(new(std::nothrow) OpHandle);
    ACL_CHECK_MALLOC_RESULT(handlePtr);
    handlePtr->numInputs = aclOp.numInputs;
    handlePtr->numOutputs = aclOp.numOutputs;
    handlePtr->opType = aclOp.opType;
    handlePtr->aclOp = aclOp;

    // try using dynamic shape
    if (OpKernelSelector::GetInstance().HasSelectFunc(aclOp.opType)) {
        ACL_LOG_INFO("Dynamic kernel selector is registered for %s", aclOp.opType.c_str());
        if (OpKernelSelector::GetInstance().GetOpKernelDesc(aclOp, handlePtr->kernelDesc) == ACL_SUCCESS) {
            ACL_LOG_INFO("Got compile kernel desc for handle. opType = %s", aclOp.opType.c_str());
        }
    }
    bool isDynamic = false;

    // get optional input from map
    array_utils::GetOptionalInputMap(aclOp.numInputs, aclOp.inputDesc, handlePtr->optionalInputMap);

    if (handlePtr->kernelDesc == nullptr) {
        ACL_REQUIRES_OK(OpModelManager::GetInstance().MatchOpModel(aclOp, handlePtr->opModel, isDynamic));
        ACL_LOG_INFO("Match opModel success, opType = %s, isDynamic = %d", aclOp.opType.c_str(), isDynamic);
    }
    handlePtr->isDynamic = isDynamic;
    *handle = handlePtr.release();
    return ACL_SUCCESS;
}
}
