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
#include "ge_tensor_cache.h"
#include "single_op/executor/stream_executor.h"
#include "error_codes_inner.h"
#include "utils/string_utils.h"
#include "utils/array_utils.h"

namespace acl {
namespace {
constexpr int32_t MAX_CACHED_NUM = 128;
}

aclError OpExecutor::DoExecuteAsync(ge::SingleOp *singleOp,
                                    const AclOp &aclOp,
                                    const aclDataBuffer *const inputs[],
                                    aclDataBuffer *const outputs[],
                                    bool executeWithExactModel)
{
    std::vector<ge::DataBuffer> inputVec;
    std::vector<ge::DataBuffer> outputVec;

    for (int32_t i = 0; i < aclOp.numInputs; ++i) {
        // skip optional input
        if (aclOp.inputDesc[i]->IsOptinalTensor()) {
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

    for (int32_t i = 0; i < aclOp.numOutputs; ++i) {
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
        ACL_LOG_CALL_ERROR("[Exec][Op]Execute op failed. ge result = %u", ret);
        return ACL_GET_ERRCODE_GE(static_cast<int32_t>(ret));
    }

    return ACL_SUCCESS;
}

static void GetInputAndOutputNum(const AclOp &aclOp, bool executeWithExactModel, size_t &inputNum, size_t &outputNum)
{
    inputNum =0;
    for (int32_t i = 0; i < aclOp.numInputs; ++i) {
        // skip optional input
        if (aclOp.inputDesc[i]->IsOptinalTensor()) {
            ACL_LOG_INFO("unused optional input, index %d", i);
            continue;
        }
        if (aclOp.inputDesc[i]->CheckConstTensor(executeWithExactModel)) {
            ACL_LOG_INFO("the inputTensor is const tensor, index %d", i);
            continue;
        }
        ++inputNum;
    }

    outputNum =0;
    for (int32_t i = 0; i < aclOp.numOutputs; ++i) {
        if (aclOp.outputDesc[i]->CheckConstTensor(executeWithExactModel)) {
            ACL_LOG_INFO("the outputTensor is const tensor, index %d", i);
            continue;
        }
        ++outputNum;
    }
}

static void FixGeTensorDesc(const aclTensorDesc &aclDesc, ge::GeTensorDesc &geTensorDesc)
{
    ge::Format geFormat = ge::FORMAT_RESERVED;
    if (aclDesc.storageFormat != ACL_FORMAT_UNDEFINED) {
        geFormat = static_cast<::ge::Format>(aclDesc.storageFormat);
    }
    ge::DataType geDataType = ge::DT_UNDEFINED;
    if (aclDesc.dataType != ACL_DT_UNDEFINED) {
        geDataType = static_cast<::ge::DataType>(aclDesc.dataType);
    }
    ge::Format geOriginFormat = ge::FORMAT_RESERVED;
    if (aclDesc.format != ACL_FORMAT_UNDEFINED) {
        geOriginFormat = static_cast<::ge::Format>(aclDesc.format);
    }
    ACL_LOG_DEBUG("use storageDims to construct GeShape in op execute");
    if ((geFormat != ge::FORMAT_RESERVED) && (geOriginFormat != geFormat)) {
        ACL_LOG_DEBUG("geOriginFormat is %d, geFormat is %d, they are not equal",
            static_cast<int32_t>(geOriginFormat), static_cast<int32_t>(geFormat));
        WrapGeShape(geTensorDesc.MutableShape(), aclDesc.storageDims);
        geTensorDesc.SetFormat(geFormat);
        const ge::GeShape &shape = geTensorDesc.GetOriginShape();
        WrapGeShape(const_cast<ge::GeShape &>(shape), aclDesc.dims);
        geTensorDesc.SetOriginShape(shape);
    } else {
        ACL_LOG_DEBUG("geOriginFormat is %d, geFormat is %d, they are equal",
            static_cast<int32_t>(geOriginFormat), static_cast<int32_t>(geFormat));
        WrapGeShape(geTensorDesc.MutableShape(), aclDesc.dims);
        geTensorDesc.SetFormat(geOriginFormat);
        geTensorDesc.SetOriginShape(geTensorDesc.GetShape());
    }
    geTensorDesc.SetOriginFormat(geOriginFormat);
    geTensorDesc.SetDataType(geDataType);
}

static void FixGeDataBuffer(const aclDataBuffer *aclBuf, aclMemType memType, ge::DataBuffer &geBuf)
{
    geBuf.data = aclBuf->data;
    geBuf.length = aclBuf->length;
    geBuf.placement = memType;
}
aclError OpExecutor::DoExecuteAsync(ge::DynamicSingleOp *singleOp,
                                    const AclOp &aclOp,
                                    const aclDataBuffer *const inputs[],
                                    aclDataBuffer *const outputs[],
                                    bool executeWithExactModel)
{
    size_t inputNum = 0U;
    size_t outputNum = 0U;
    GetInputAndOutputNum(aclOp, executeWithExactModel, inputNum, outputNum);
    GeTensorDescVecPtr inputDesc = GeTensorDescCache::GetInstance().GetDescVecPtr(inputNum);
    ACL_CHECK_WITH_MESSAGE_AND_RETURN(inputDesc != nullptr, ACL_ERROR_BAD_ALLOC, "get input tensor desc failed!");
    GeTensorDescVecPtr outputDesc = GeTensorDescCache::GetInstance().GetDescVecPtr(outputNum);
    ACL_CHECK_WITH_MESSAGE_AND_RETURN(outputDesc != nullptr, ACL_ERROR_BAD_ALLOC, "get output tensor desc failed!");
    std::vector<ge::DataBuffer> inputVec(inputNum);
    std::vector<ge::DataBuffer> outputVec(outputNum);

    size_t inCnt = 0U;
    for (int32_t i = 0U; (i < aclOp.numInputs) && (inCnt < inputNum); ++i) {
        // skip optional input
        if (aclOp.inputDesc[i]->IsOptinalTensor()) {
            continue;
        }
        if (aclOp.inputDesc[i]->CheckConstTensor(executeWithExactModel)) {
            continue;
        }
        auto &vecDesc = *inputDesc;
        const aclTensorDesc *aclDesc = aclOp.inputDesc[i];
        FixGeTensorDesc(*aclDesc, vecDesc[inCnt]);
        FixGeDataBuffer(inputs[i], aclDesc->memtype, inputVec[inCnt]);
        ++inCnt;
    }
    ACL_LOG_INFO("inputBuffer and inputDesc for ge are ready");

    size_t outCnt = 0U;
    for (int32_t i = 0U; (i < aclOp.numOutputs) && (outCnt < outputNum); ++i) {
        if (aclOp.outputDesc[i]->CheckConstTensor(executeWithExactModel)) {
            continue;
        }
        auto &vecDesc = *outputDesc;
        const aclTensorDesc *aclDesc = aclOp.outputDesc[i];
        FixGeTensorDesc(*aclDesc, vecDesc[outCnt]);
        FixGeDataBuffer(outputs[i], aclDesc->memtype, outputVec[outCnt]);
        ++outCnt;
    }
    ACL_LOG_INFO("outputBuffer and outputDesc for ge are ready");

    ACL_LOG_INFO("to invoke GeExecutor::ExecuteAsync");
    ge::Status ret = ge::GeExecutor::ExecuteAsync(singleOp, *inputDesc, inputVec, *outputDesc, outputVec);
    if (ret != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Exec][Op]Execute op failed, ge result = %u", ret);
        GeTensorDescCache::GetInstance().ReleaseDescVecPtr(inputDesc);
        GeTensorDescCache::GetInstance().ReleaseDescVecPtr(outputDesc);
        return ACL_GET_ERRCODE_GE(static_cast<int32_t>(ret));
    }

    if (aclOp.exeucteType == ACL_OP_EXECUTE_V2) {
        ACL_LOG_INFO("begin to update outputDesc");
        for (size_t i = 0U; i < outputDesc->size(); ++i) {
            ge::GeShape outputShape = (*outputDesc)[i].GetShape();
            std::vector<int64_t> outputDims = outputShape.GetDims();
            ACL_LOG_INFO("update outputDesc[%zu] dims is [%s]", i, string_utils::VectorToString(outputDims).c_str());
            const_cast<aclTensorDesc *>(aclOp.outputDesc[i])->dims = outputDims;
        }
        ACL_LOG_INFO("update outputDesc successfully");
    }

    GeTensorDescCache::GetInstance().ReleaseDescVecPtr(inputDesc);
    GeTensorDescCache::GetInstance().ReleaseDescVecPtr(outputDesc);
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
        ACL_LOG_CALL_ERROR("[Load][Op]Load operator failed. model = %s, ge result = %u",
            modelInfo.name.c_str(), status);
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
        ACL_LOG_CALL_ERROR("[Load][Op]Load dynamic operator failed. model = %s, ge result = %u",
            modelInfo.name.c_str(), status);
    }

    return singleOp;
}

aclError OpExecutor::ExecuteAsync(const AclOp &aclOp,
                                  const aclDataBuffer *const inputs[],
                                  aclDataBuffer *const outputs[],
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

    if (!aclOp.isMatched) {
        ACL_REQUIRES_OK(OpModelManager::GetInstance().MatchOpModel(aclOp, opModel, isDynamic));
        ACL_LOG_DEBUG("match opModel success, opType = %s, isDynamic = %d", aclOp.opType.c_str(),
            static_cast<int32_t>(isDynamic));
    } else {
        opModel = aclOp.opModel;
        isDynamic = aclOp.isDynamic;
        ACL_LOG_INFO("opType = %s has been matched in the  op compile phase", aclOp.opType.c_str());
    }

    bool isExactModel = (opModel.isStaticModelWithFuzzCompile == 0U) ? true : false;
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
        ret = DoExecuteAsync(executor, aclOp, inputs, outputs, isExactModel);
    }

    return ret;
}

aclError OpExecutor::ExecuteAsync(OpHandle &opHandle,
                                  const aclDataBuffer *const inputs[],
                                  aclDataBuffer *const outputs[],
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
        {
            std::lock_guard<std::mutex> lk(opHandle.mutexForDynamic);
            auto it = cachedExecutors.find(stream);
            if (it != cachedExecutors.end()) {
                singleOp = it->second;
            } else {
                singleOp = LoadDynamicSingleOp(opHandle.opModel, stream);
                if (singleOp == nullptr) {
                    ACL_LOG_INNER_ERROR("[Load][Op]LoadSingleOp failed");
                    return ACL_ERROR_OP_LOAD_FAILED;
                }

                // Just for protection. Preventing from cache grows to large.
                if (cachedExecutors.size() > static_cast<size_t>(MAX_CACHED_NUM)) {
                    auto toErase = cachedExecutors.begin();
                    ACL_LOG_WARN("cache[%zu] reaches max size[%zu], evict one object. stream = %p",
                        cachedExecutors.size(), MAX_CACHED_NUM, toErase->first);
                    (void)cachedExecutors.erase(toErase);
                }

                ACL_LOG_INFO("cache operator executor. model = %s, stream = %p", opHandle.opModel.name.c_str(), stream);
                (void)opHandle.cachedDynamicOperators.emplace(stream, singleOp);
            }
        }
        ret = DoExecuteAsync(singleOp, opHandle.aclOp, inputs, outputs);
    } else {
        ACL_LOG_INFO("begin to load static model. model = %s, stream = %p", opHandle.opModel.name.c_str(), stream);
        auto &cachedExecutors = opHandle.cachedOperators;
        ge::SingleOp *singleOp = nullptr;
        {
            std::lock_guard<std::mutex> lk(opHandle.mutexForStatic);
            auto it = cachedExecutors.find(stream);
            if (it != cachedExecutors.end()) {
                singleOp = it->second;
            } else {
                singleOp = LoadSingleOp(opHandle.opModel, stream);
                if (singleOp == nullptr) {
                    ACL_LOG_INNER_ERROR("[Load][Op]LoadSingleOp failed");
                    return ACL_ERROR_OP_LOAD_FAILED;
                }

                // Just for protection. Preventing from cache grows to large.
                if (cachedExecutors.size() > static_cast<size_t>(MAX_CACHED_NUM)) {
                    auto toErase = cachedExecutors.begin();
                    ACL_LOG_INFO("cache[%zu] reaches max size[%zu], evict one object. stream = %p",
                        cachedExecutors.size(), MAX_CACHED_NUM, toErase->first);
                    (void)cachedExecutors.erase(toErase);
                }

                ACL_LOG_INFO("cache operator executor. model = %s, stream = %p", opHandle.opModel.name.c_str(), stream);
                (void)opHandle.cachedOperators.emplace(stream, singleOp);
            }
        }
        ret = DoExecuteAsync(singleOp, opHandle.aclOp, inputs, outputs);
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

    if (handlePtr->kernelDesc == nullptr) {
        ACL_REQUIRES_OK(OpModelManager::GetInstance().MatchOpModel(aclOp, handlePtr->opModel, isDynamic));
        ACL_LOG_INFO("Match opModel success, opType = %s, isDynamic = %d", aclOp.opType.c_str(),
            static_cast<int32_t>(isDynamic));
    }
    handlePtr->isDynamic = isDynamic;
    *handle = handlePtr.release();
    return ACL_SUCCESS;
}
}
