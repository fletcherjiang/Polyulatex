/**
* @file op.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/acl_op.h"

#include <mutex>

#include "framework/common/ge_format_util.h"
#include "common/log_inner.h"
#include "types/tensor_desc_internal.h"
#include "types/acl_op.h"
#include "op_executor.h"
#include "utils/array_utils.h"
#include "op_model_manager.h"
#include "single_op/compile/op_kernel_registry.h"
#include "single_op/compile/op_kernel_selector.h"
#include "error_codes_inner.h"
#include "toolchain/profiling_manager.h"
#include "toolchain/resource_statistics.h"
#include "common/ge_types.h"
#include "graph/operator.h"
#include "graph/tensor.h"
#include "graph/operator_factory.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/opsproto_manager.h"
#include "graph/utils/tensor_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "framework/common/util.h"

using namespace acl;

static bool g_aclInitFlag = false;
static std::mutex g_aclInitMutex;

namespace {
    bool aclLoadOpsProtoFlag = false;
    std::mutex aclLoadOpsProtoMutex;
}

struct aclopHandle {
    aclopHandle() : opHandle(nullptr) {}
    ~aclopHandle()
    {
        ACL_DELETE_AND_SET_NULL(opHandle);
    }

    OpHandle *opHandle;
};

aclError aclopSetModelDir(const char *modelDir)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclopSetModelDir");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(modelDir);
    std::unique_lock<std::mutex> lk(g_aclInitMutex);
    if (g_aclInitFlag) {
        ACL_LOG_INNER_ERROR("[Check][InitFlag]repeatedly set model dir.");
        return ACL_ERROR_REPEAT_INITIALIZE;
    }

    auto ret = OpModelManager::GetInstance().LoadAllModels(modelDir);
    if (ret == ACL_SUCCESS) {
        g_aclInitFlag = true;
    }
    return ret;
}

aclError aclopLoad(const void *model, size_t modelSize)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_STAGES_REG(acl::ACL_STAGE_LOAD, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclopLoad");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(model);
    if (modelSize == 0) {
        ACL_LOG_ERROR("[Check][ModelSize]the value of modelSize[%zu] can't be zero", modelSize);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"modelSize", std::to_string(modelSize), "can't be zero"}));
        return ACL_ERROR_INVALID_PARAM;
    }
    // it is static load, true means no need aging
    auto ret = OpModelManager::GetInstance().LoadModelFromMem(model, modelSize, true);
    if (ret == ACL_SUCCESS) {
        ACL_LOG_INFO("load opModels from memory successfully");
        return ret;
    }
    ACL_LOG_INNER_ERROR("[Load][opModels]fail to load opModels from memory, result = %d", ret);
    return ret;
}

aclError aclopCreateHandle(const char *opType,
                           int numInputs,
                           const aclTensorDesc *const inputDesc[],
                           int numOutputs,
                           const aclTensorDesc *const outputDesc[],
                           const aclopAttr *opAttr,
                           aclopHandle **handle)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_STAGES_REG(acl::ACL_STAGE_CREATE, acl::ACL_STAGE_DEFAULT);
    ACL_ADD_APPLY_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_HANDLE);
    ACL_LOG_INFO("start to execute aclopCreateHandle");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(opType);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(handle);
    ACL_REQUIRES_NON_NEGATIVE(numInputs);
    ACL_REQUIRES_NON_NEGATIVE(numOutputs);
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numInputs, inputDesc));
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numOutputs, outputDesc));
    if (array_utils::IsHostMemTensorDesc(numInputs, inputDesc) != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Check][HostMemTensorDesc]aclopCreateHandle ACL_MEMTYPE_HOST "
            "placeMent in inputDesc not support");
        return ACL_ERROR_API_NOT_SUPPORT;
    }
    if (array_utils::IsHostMemTensorDesc(numOutputs, outputDesc) != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Check][HostMemTensorDesc]aclopCreateHandle ACL_MEMTYPE_HOST "
            "placeMent in outputDesc not support");
        return ACL_ERROR_API_NOT_SUPPORT;
    }

    AclOp aclOp;
    aclOp.opType = std::string(opType);
    aclOp.numInputs = numInputs;
    aclOp.inputDesc = inputDesc;
    aclOp.numOutputs = numOutputs;
    aclOp.outputDesc = outputDesc;
    aclOp.opAttr = opAttr;
    aclOp.isCompile = false;

    OpHandle *opHandle = nullptr;
    ACL_REQUIRES_OK(OpExecutor::CreateOpHandle(aclOp, &opHandle));

    auto* const newHandle = new(std::nothrow) aclopHandle();
    ACL_CHECK_MALLOC_RESULT(newHandle);

    newHandle->opHandle = opHandle;
    *handle = newHandle;
    ACL_ADD_APPLY_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_HANDLE);
    return ACL_SUCCESS;
}

void aclopDestroyHandle(aclopHandle *handle)
{
    ACL_STAGES_REG(acl::ACL_STAGE_DESTROY, acl::ACL_STAGE_DEFAULT);
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_ADD_RELEASE_TOTAL_COUNT(ACL_STATISTICS_CREATE_DESTROY_HANDLE);
    ACL_DELETE_AND_SET_NULL(handle);
    ACL_ADD_RELEASE_SUCCESS_COUNT(ACL_STATISTICS_CREATE_DESTROY_HANDLE);
}

aclError aclopExecWithHandle(aclopHandle *handle,
                             int numInputs,
                             const aclDataBuffer *const inputs[],
                             int numOutputs,
                             aclDataBuffer *const outputs[],
                             aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_STAGES_REG(acl::ACL_STAGE_EXEC, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclopExecWithHandle");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(handle);
    ACL_REQUIRES_NON_NEGATIVE_WITH_INPUT_REPORT(numInputs);
    ACL_REQUIRES_NON_NEGATIVE_WITH_INPUT_REPORT(numOutputs);
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numInputs, inputs));
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numOutputs, outputs));
    if (array_utils::IsAllTensorEmpty(numOutputs, outputs)) {
        ACL_LOG_INFO("all ouput tensor are empty");
        return ACL_SUCCESS;
    }

    auto &opHandle = *handle->opHandle;
    if (numInputs != opHandle.numInputs) {
        ACL_LOG_ERROR("[Check][NumInputs]input num mismatch: expect %d, but %d", opHandle.numInputs, numInputs);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("input num mismatch: expect %d", opHandle.numInputs);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"input num", std::to_string(numInputs), errMsg}));
        return ACL_ERROR_OP_INPUT_NOT_MATCH;
    }

    if (numOutputs != opHandle.numOutputs) {
        ACL_LOG_ERROR("[Check][NumOutputs]output num mismatch: expect %d, but %d", opHandle.numOutputs, numOutputs);
        std::string errMsg = acl::AclErrorLogManager::FormatStr("input num mismatch: expect %d", opHandle.numOutputs);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"output num", std::to_string(numOutputs), errMsg}));
        return ACL_ERROR_OP_OUTPUT_NOT_MATCH;
    }

    return OpExecutor::ExecuteAsync(opHandle, inputs, outputs, stream);
}

aclError aclopExecute(const char *opType,
                      int numInputs,
                      const aclTensorDesc *const inputDesc[],
                      const aclDataBuffer *const inputs[],
                      int numOutputs,
                      const aclTensorDesc *const outputDesc[],
                      aclDataBuffer *const outputs[],
                      const aclopAttr *attr,
                      aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_STAGES_REG(acl::ACL_STAGE_EXEC, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclopExecute");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(opType);
    ACL_REQUIRES_NON_NEGATIVE_WITH_INPUT_REPORT(numInputs);
    ACL_REQUIRES_NON_NEGATIVE_WITH_INPUT_REPORT(numOutputs);
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numInputs, inputDesc));
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numOutputs, outputDesc));
    if (array_utils::IsAllTensorEmpty(numOutputs, outputDesc)) {
        ACL_LOG_INFO("all ouput tensor are empty");
        return ACL_SUCCESS;
    }

    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numInputs, inputs));
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numOutputs, outputs));

    AclOp aclOp;
    aclOp.opType = std::string(opType);
    aclOp.numInputs = numInputs;
    aclOp.inputDesc = inputDesc;
    aclOp.numOutputs = numOutputs;
    aclOp.outputDesc = outputDesc;
    aclOp.inputs = inputs;
    aclOp.outputs = outputs;
    aclOp.opAttr = attr;
    aclOp.isCompile = false;

    return OpExecutor::ExecuteAsync(aclOp, inputs, outputs, stream);
}

aclError aclopExecuteV2(const char *opType,
                        int numInputs,
                        aclTensorDesc *inputDesc[],
                        aclDataBuffer *inputs[],
                        int numOutputs,
                        aclTensorDesc *outputDesc[],
                        aclDataBuffer *outputs[],
                        aclopAttr *attr,
                        aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_STAGES_REG(acl::ACL_STAGE_EXEC, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclopExecuteV2");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(opType);
    ACL_REQUIRES_NON_NEGATIVE(numInputs);
    ACL_REQUIRES_NON_NEGATIVE(numOutputs);
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numInputs, inputDesc));
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numOutputs, outputDesc));
    if (array_utils::IsAllTensorEmpty(numOutputs, outputDesc)) {
        ACL_LOG_INFO("all ouput tensor are empty");
        return ACL_SUCCESS;
    }

    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numInputs, inputs));
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numOutputs, outputs));

    AclOp aclOp;
    aclOp.opType = std::string(opType);
    aclOp.numInputs = numInputs;
    aclOp.inputDesc = inputDesc;
    aclOp.numOutputs = numOutputs;
    aclOp.outputDesc = outputDesc;
    aclOp.inputs = inputs;
    aclOp.outputs = outputs;
    aclOp.opAttr = attr;
    aclOp.isCompile = false;
    aclOp.exeucteType = ACL_OP_EXECUTE_V2;
    ACL_REQUIRES_OK(OpExecutor::ExecuteAsync(aclOp, inputs, outputs, stream));
    return ACL_SUCCESS;
}

aclError aclTransTensorDescFormat(const aclTensorDesc *srcDesc, aclFormat dstFormat, aclTensorDesc **dstDesc)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_STAGES_REG(acl::ACL_STAGE_GET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(srcDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dstDesc);

    ge::Shape shape(srcDesc->dims);
    auto srcFormat = static_cast<ge::Format>(srcDesc->format);
    auto dataType = static_cast<ge::DataType >(srcDesc->dataType);
    ge::TensorDesc desc(shape, srcFormat, dataType);

    std::vector<int64_t> dstShape;
    auto geRet = ge::GeFormatUtil::TransShape(desc, static_cast<ge::Format>(dstFormat), dstShape);
    if (geRet != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Call][TransShape]invoke TransShape failed. ge result = %u",
            geRet);
        return ACL_GET_ERRCODE_GE(static_cast<int32_t>(geRet));
    }

    *dstDesc = aclCreateTensorDesc(srcDesc->dataType, static_cast<int32_t>(dstShape.size()),
                                   dstShape.data(), srcDesc->format);
    if (*dstDesc == nullptr) {
        ACL_LOG_INNER_ERROR("[Create][Desc]aclCreateTensorDesc failed.");
        return ACL_ERROR_BAD_ALLOC;
    }

    return ACL_SUCCESS;
}

aclError aclopCreateKernel(const char *opType,
                           const char *kernelId,
                           const char *kernelName,
                           void *binData,
                           int binSize,
                           aclopEngineType enginetype,
                           aclDataDeallocator deallocator)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_STAGES_REG(acl::ACL_STAGE_CREATE, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclopCreateKernel");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(opType);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(kernelId);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(kernelName);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(binData);

    auto* const registration = new(std::nothrow) OpKernelRegistration();
    ACL_CHECK_MALLOC_RESULT(registration);

    registration->opType = opType;
    registration->kernelId = kernelId;
    registration->kernelName = kernelName;
    registration->binData = binData;
    registration->binSize = static_cast<uint64_t>(binSize);
    registration->enginetype = enginetype;
    registration->deallocator = deallocator;

    ACL_REQUIRES_OK(OpKernelRegistry::GetInstance().Register(std::unique_ptr<OpKernelRegistration>(registration)));
    ACL_LOG_DEBUG("Successfully created kernel. opType = %s, kernelId = %s, kernelName = %s", opType, kernelId,
                  kernelName);
    return ACL_SUCCESS;
}

aclError aclopUpdateParams(const char *opType,
                           int numInputs,
                           const aclTensorDesc *const inputDesc[],
                           int numOutputs,
                           const aclTensorDesc *const outputDesc[],
                           const aclopAttr *attr)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_STAGES_REG(acl::ACL_STAGE_CREATE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(opType);
    ACL_REQUIRES_NON_NEGATIVE_WITH_INPUT_REPORT(numInputs);
    ACL_REQUIRES_NON_NEGATIVE_WITH_INPUT_REPORT(numOutputs);
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numInputs, inputDesc));
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numOutputs, outputDesc));

    ACL_LOG_INFO("start to execute aclopUpdateParams. opType = %s", opType);
    AclOp aclOp;
    aclOp.opType = std::string(opType);
    aclOp.numInputs = numInputs;
    aclOp.inputDesc = inputDesc;
    aclOp.numOutputs = numOutputs;
    aclOp.outputDesc = outputDesc;
    aclOp.opAttr = attr;

    return OpKernelSelector::GetInstance().SelectOpKernel(aclOp);
}

aclError aclopSetKernelArgs(aclopKernelDesc *kernelDesc,
                            const char *kernelId,
                            uint32_t blockDim,
                            const void *args,
                            uint32_t argSize)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(kernelDesc);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(args);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(kernelId);

    ACL_LOG_DEBUG("start to execute aclopSetKernelArgs, kernelId = %s, blockDim = %u, argSize = %u", kernelId, blockDim,
                  argSize);
    kernelDesc->kernelId = std::string(kernelId);
    kernelDesc->blockDim = blockDim;
    kernelDesc->extendArgs = std::string(reinterpret_cast<const char *>(args), argSize);

    return ACL_SUCCESS;
}

aclError aclopSetKernelWorkspaceSizes(aclopKernelDesc *kernelDesc, int numWorkspaces, size_t *workspaceSizes)
{
    ACL_STAGES_REG(acl::ACL_STAGE_SET, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_DEBUG("start to execute aclopSetKernelWorkspaceSizes");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(kernelDesc);
    ACL_REQUIRES_NON_NEGATIVE_WITH_INPUT_REPORT(numWorkspaces);
    if (numWorkspaces != 0) {
        ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(workspaceSizes);
    }

    for (int i = 0; i < numWorkspaces; ++i) {
        kernelDesc->workspaceSizes.emplace_back(workspaceSizes[i]);
    }

    return ACL_SUCCESS;
}

aclError aclopUnregisterCompileFunc(const char *opType)
{
    ACL_STAGES_REG(acl::ACL_STAGE_COMP, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(opType);
    ACL_LOG_INFO("aclopUnregisterCompileFunc in, opType = %s", opType);
    acl::OpKernelSelector::GetInstance().Unregister(opType);
    ACL_LOG_INFO("Unregistering compile function successfully. op type = %s", opType);
    return ACL_SUCCESS;
}

aclError aclopRegisterCompileFunc(const char *opType, aclopCompileFunc func)
{
    ACL_STAGES_REG(acl::ACL_STAGE_COMP, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(opType);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(func);
    bool ret = acl::OpKernelSelector::GetInstance().Register(opType, func);
    if (ret) {
        ACL_LOG_INFO("Registering compile function successfully. op type = %s", opType);
        return ACL_SUCCESS;
    } else {
        ACL_LOG_INNER_ERROR("[Register][Function]Failed to register compile function due to repeat registration. "
            "op type = %s", opType);
        return ACL_ERROR_BIN_SELECTOR_ALREADY_REGISTERED;
    }
}

static bool GetOpsProtoPath(std::string &opsProtoPath)
{
    ACL_LOG_DEBUG("Start to get ops proto path schedule.");
    const char* const pathEnv = std::getenv("ASCEND_OPP_PATH");
    if (pathEnv != nullptr) {
        std::string path = pathEnv;
        if (path.empty()) {
            ACL_LOG_INNER_ERROR("[Check][Path]File path is empty string.");
            return false;
        }
        std::string filePath = ge::RealPath(path.c_str());
        if (filePath.empty()) {
            ACL_LOG_INNER_ERROR("[Check][Path]File path %s is invalid.", path.c_str());
            return false;
        }
        opsProtoPath = (filePath + "/op_proto/custom/" + ":") + (filePath + "/op_proto/built-in/");
        ACL_LOG_INFO("Get opsproto so path from path : %s", opsProtoPath.c_str());
        return true;
    }
    return false;
}

static aclError LoadOpsProto()
{
    ACL_LOG_INFO("LoadOpsProtoLib begin");
    string opsprotoPath;
    if (!GetOpsProtoPath(opsprotoPath)) {
        ACL_LOG_INNER_ERROR("[Check][ProtoPath]The environment variable(ASCEND_OPP_PATH) is not set or path is "\
            "invalid.");
        return ACL_ERROR_INVALID_OPP_PATH;
    }
    ge::OpsProtoManager* const protoManager = ge::OpsProtoManager::Instance();
    std::map<std::string, std::string> optionTmp;
    optionTmp.emplace(std::pair<std::string, std::string>(string("ge.opsProtoLibPath"), opsprotoPath));
    bool isProtoInit = protoManager->Initialize(optionTmp);
    if (!isProtoInit) {
        ACL_LOG_INNER_ERROR("[Init][Manager]Load ops_proto lib failed, ops proto path[%s] is invalid.",
            opsprotoPath.c_str());
        return ACL_ERROR_FAILURE;
    }
    ACL_LOG_INFO("LoadOpsProtoLib success");

    std::vector<ge::AscendString> allOp;
    ge::graphStatus ret = ge::OperatorFactory::GetOpsTypeList(allOp);
    if (ret != ge::GRAPH_SUCCESS) {
        ACL_LOG_CALL_ERROR("[Get][OpsType]GetOpsTypeList failed.");
        return ACL_GET_ERRCODE_GE(static_cast<int32_t>(ret));
    }
    ACL_LOG_INFO("OpsTypeListSize is %zu", allOp.size());
    return ACL_SUCCESS;
}

static aclError UpdateOutPutDesc(ge::Operator inferOp, int numOutputs, aclTensorDesc *outputDesc[])
{
    ACL_LOG_INFO("Begin to update OutPutDesc, numOutputs is %d", numOutputs);
    // update outputDesc
    std::stringstream ss;
    for (int i = 0; i < numOutputs; ++i) {
        ge::AscendString ascendString;
        // get inferOutputDesc after inferShape
        auto inferOutputDesc = inferOp.GetOutputDesc(static_cast<uint32_t>(i));
        ge::Format outputFormat = inferOutputDesc.GetFormat();
        ge::DataType outputDType = inferOutputDesc.GetDataType();
        auto ret = inferOutputDesc.GetName(ascendString);
        if (ret != ge::GRAPH_SUCCESS) {
            ACL_LOG_CALL_ERROR("[Get][Name]the %d tensor GetName failed.", i);
            return ACL_GET_ERRCODE_GE(static_cast<int32_t>(ret));
        }
        std::string outputName;
        if (ascendString.GetString() != nullptr) {
            outputName = std::string(ascendString.GetString());
        }
        ge::Shape outputShape = inferOutputDesc.GetShape();
        std::vector<std::pair<int64_t, int64_t>> outputRange;
        ret = inferOutputDesc.GetShapeRange(outputRange);
        if (ret != ge::GRAPH_SUCCESS) {
            ACL_LOG_CALL_ERROR("[Get][ShapeRange]the %d tensor GetShapeRange failed.", i);
            return ACL_GET_ERRCODE_GE(static_cast<int32_t>(ret));
        }

        // update outputDesc
        outputDesc[i]->dataType = static_cast<aclDataType>(outputDType);
        outputDesc[i]->format = static_cast<aclFormat>(outputFormat);
        outputDesc[i]->name = outputName;
        std::vector<int64_t> outputDims = outputShape.GetDims();
        ACL_LOG_INFO("inferShapeDimSize is %zu", outputDims.size());
        outputDesc[i]->dims.clear();
        for (size_t j = 0; j < outputDims.size(); ++j) {
            outputDesc[i]->dims.emplace_back(outputDims[j]);
        }
        outputDesc[i]->shapeRange.clear();
        for (size_t j = 0; j < outputRange.size(); ++j) {
            outputDesc[i]->shapeRange.emplace_back(outputRange[j]);
        }
        if (acl::AclLog::IsLogOutputEnable(ACL_INFO)) {
            ss << "inferOutputDesc[" << i << "]: ";
            ss << outputDesc[i]->DebugString() << " ";
        }
    }

    ACL_LOG_INFO("inferOutputDesc is %s", ss.str().c_str());
    return ACL_SUCCESS;
}

static void AddOpDesc(aclTensorDesc *tensorDesc, ge::OpDescPtr &opDesc, bool isInput)
{
    ge::Format geFormat = ge::FORMAT_RESERVED;
    if (tensorDesc->format != ACL_FORMAT_UNDEFINED) {
        geFormat = static_cast<::ge::Format>(tensorDesc->format);
    }
    ge::DataType geDataType = ge::DT_UNDEFINED;
    if (tensorDesc->dataType != ACL_DT_UNDEFINED) {
        geDataType = static_cast<::ge::DataType>(tensorDesc->dataType);
    }
    ge::GeTensorDesc geTensorDesc(ge::GeShape(tensorDesc->dims),
                                  geFormat,
                                  geDataType);
    geTensorDesc.SetOriginFormat(geFormat);
    ge::TensorUtils::SetRealDimCnt(geTensorDesc, static_cast<uint32_t>(tensorDesc->dims.size()));

    if (isInput) {
        ge::TensorUtils::SetInputTensor(geTensorDesc, true);
        ge::TensorUtils::SetOutputTensor(geTensorDesc, false);
    } else {
        ge::TensorUtils::SetInputTensor(geTensorDesc, false);
        ge::TensorUtils::SetOutputTensor(geTensorDesc, true);
    }
    if (!tensorDesc->name.empty()) {
        if (isInput) {
            (void)opDesc->AddInputDesc(tensorDesc->name, geTensorDesc);
        } else {
            (void)opDesc->AddOutputDesc(tensorDesc->name, geTensorDesc);
        }
    } else {
        if (isInput) {
            (void)opDesc->AddInputDesc(geTensorDesc);
        } else {
            (void)opDesc->AddOutputDesc(geTensorDesc);
        }
    }
}

static aclError AddDataInput(aclTensorDesc *inputDesc,
                             aclDataBuffer *inputs,
                             std::unique_ptr<uint8_t[]> &constData,
                             ge::Operator &constOp)
{
    // create const operator
    ge::TensorDesc constDesc(ge::Shape(inputDesc->dims),
                             static_cast<::ge::Format>(inputDesc->format),
                             static_cast<::ge::DataType >(inputDesc->dataType));
    size_t tensorSize = inputs->length;
    if (tensorSize <= 0) {
        ACL_LOG_ERROR("[Check][TensorSize]tensorSize must be positive, tensorSize = %zu", tensorSize);
        acl::AclErrorLogManager::ReportInputError(acl::INVALID_PARAM_MSG,
            std::vector<std::string>({"param", "value", "reason"}),
            std::vector<std::string>({"tensorSize", std::to_string(tensorSize), "must be positive"}));
        return ACL_ERROR_INVALID_PARAM;
    }
    auto args = std::unique_ptr<uint8_t[]>(new(std::nothrow) uint8_t[tensorSize]);
    if (args == nullptr) {
        ACL_LOG_INNER_ERROR("[Allocate][Mem]Allocate memory failed.");
        return static_cast<int32_t>(ACL_ERROR_BAD_ALLOC);
    }
    constData = std::move(args);
    if (memcpy_s(constData.get(), tensorSize, inputs->data, tensorSize) != EOK) {
        ACL_LOG_INNER_ERROR("[Copy][Mem]Copy input data failed. size = %zu", tensorSize);
        return ACL_ERROR_FAILURE;
    }
    ge::Tensor constTensor(constDesc, constData.get(), tensorSize);
    std::string valueName = ge::ATTR_NAME_WEIGHTS;
    (void)constOp.SetAttr(valueName.c_str(), constTensor);
    auto retConstInfer = constOp.InferShapeAndType();
    if (retConstInfer != ge::GRAPH_SUCCESS) {
        ACL_LOG_CALL_ERROR("the constOp inferShape failed. ge result = %u", retConstInfer);
        return ACL_GET_ERRCODE_GE(static_cast<int32_t>(retConstInfer));
    }
    return ACL_SUCCESS;
}

aclError aclopInferShape(const char *opType,
                         int numInputs,
                         aclTensorDesc *inputDesc[],
                         aclDataBuffer *inputs[],
                         int numOutputs,
                         aclTensorDesc *outputDesc[],
                         aclopAttr *attr)
{
    ACL_STAGES_REG(acl::ACL_STAGE_INFER, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclopInferShape");
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_REQUIRES_NON_NEGATIVE(numInputs);
    ACL_REQUIRES_NON_NEGATIVE(numOutputs);
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numInputs, inputDesc));
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numOutputs, outputDesc));
    ACL_REQUIRES_OK(array_utils::CheckPtrArray(numInputs, inputs));
    {
        std::unique_lock<std::mutex> lk(aclLoadOpsProtoMutex);
        if (!aclLoadOpsProtoFlag) {
            aclError ret = LoadOpsProto();
            if (ret != ACL_SUCCESS) {
                ACL_LOG_INNER_ERROR("[Load][OpsProto]Load opsProto lib fail.");
                return ret;
            }
            aclLoadOpsProtoFlag = true;
        } else {
            ACL_LOG_INFO("OpsProto lib has been successfully loaded");
        }
    }
    ge::Operator inferOp = ge::OperatorFactory::CreateOperator(opType, opType);
    ge::OpDescPtr opDesc = ge::OpDescUtils::GetOpDescFromOperator(inferOp);
    ACL_REQUIRES_NOT_NULL(opDesc);
    size_t factoryInputSize = opDesc->GetAllInputName().size();
    ACL_LOG_INFO("size of GetAllInputName is %zu, numInputs of entered by user is %d", factoryInputSize, numInputs);
    if (factoryInputSize < static_cast<size_t>(numInputs)) {
        ACL_MAKE_SHARED(opDesc = std::make_shared<ge::OpDesc>(opType, opType), return ACL_ERROR_BAD_ALLOC);
        ACL_CHECK_MALLOC_RESULT(opDesc);
        for (int i = 0; i < numInputs; ++i) {
            AddOpDesc(inputDesc[i], opDesc, true);
        }
        ACL_LOG_INFO("addInputOpDesc success，numInputs is %d", numInputs);
        for (int i = 0; i < numOutputs; ++i) {
            AddOpDesc(outputDesc[i], opDesc, false);
        }
        ACL_LOG_INFO("addOutputOpDesc success numOutputs is %d", numOutputs);
        inferOp.BreakConnect();
        inferOp = ge::OpDescUtils::CreateOperatorFromOpDesc(opDesc);
    }

    if (attr != nullptr) {
        for (const auto &it : attr->Attrs()) {
            (void)opDesc->SetAttr(it.first, it.second);
        }
    }
    std::unique_ptr<uint8_t[]> *constData = new(std::nothrow) std::unique_ptr<uint8_t[]>[numInputs];
    ACL_CHECK_MALLOC_RESULT(constData);
    std::vector<ge::Operator> constOps;
    for (int i = 0; i < numInputs; ++i) {
        constOps.push_back(ge::OperatorFactory::CreateOperator("Const", "Const"));
        ge::Operator constOp = constOps.back();
        aclError ret = AddDataInput(inputDesc[i], inputs[i], constData[i], constOp);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Add][Data]add data fail, index = %d", i);
            ACL_DELETE_ARRAY_AND_SET_NULL(constData);
            return ret;
        }
        ACL_LOG_INFO("opDesc.GetInputNameByIndex, index = %d", i);
        std::string inferOpName = opDesc->GetInputNameByIndex(static_cast<uint32_t>(i));
        ACL_LOG_INFO("opDesc.GetInputNameByIndex, inferOpName = %s", inferOpName.c_str());
        (void)inferOp.SetInput(inferOpName.c_str(), constOp, "y");
    }
    ACL_LOG_INFO("create constData success");
    ge::graphStatus retInfer = inferOp.InferShapeAndType();
    if (retInfer != ge::GRAPH_SUCCESS) {
        ACL_LOG_CALL_ERROR("[Infer][ShapeAndType]the op:%s inferShape failed. ge result = %u",
            opType, retInfer);
        ACL_DELETE_ARRAY_AND_SET_NULL(constData);
        return ACL_GET_ERRCODE_GE(static_cast<int32_t>(retInfer));
    }
    for (size_t i = static_cast<uint64_t>(0); i < constOps.size(); ++i) {
        constOps[i].BreakConnect();
    }
    ACL_LOG_INFO("the op:%s inferShape success", opType);
    aclError ret = UpdateOutPutDesc(inferOp, numOutputs, outputDesc);
    if (ret != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("[Update][OutPutDesc]the op:%s update outputDesc failed", opType);
        ACL_DELETE_ARRAY_AND_SET_NULL(constData);
        return ret;
    }
    ACL_LOG_INFO("the op:%s update outputDesc success", opType);
    inferOp.BreakConnect();
    ACL_DELETE_ARRAY_AND_SET_NULL(constData);
    return ACL_SUCCESS;
}
