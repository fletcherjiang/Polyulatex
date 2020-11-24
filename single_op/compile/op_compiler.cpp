/**
* @file op_compiler.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "op_compiler.h"
#include <string>
#include <map>
#include "graph/utils/tensor_utils.h"
#include "graph/utils/type_utils.h"
#include "types/tensor_desc_internal.h"
#include "common/ge_types.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_attr_value.h"
#include "utils/array_utils.h"

using namespace ge;

namespace {
    const std::map<GeAttrValue::ValueType,  std::string> ATTR_TYPES_MAP = {
        {GeAttrValue::VT_NONE, "string"},
        {GeAttrValue::VT_STRING, "string"},
        {GeAttrValue::VT_FLOAT, "float"},
        {GeAttrValue::VT_BOOL, "bool"},
        {GeAttrValue::VT_INT, "int"},
        {GeAttrValue::VT_DATA_TYPE, "datatype"},
        {GeAttrValue::VT_LIST_STRING, "liststring"},
        {GeAttrValue::VT_LIST_FLOAT, "listfloat"},
        {GeAttrValue::VT_LIST_BOOL, "listbool"},
        {GeAttrValue::VT_LIST_INT, "listint"},
        {GeAttrValue::VT_LIST_DATA_TYPE, "listdatatype"},
    };
    int32_t compileFlag = 0;
}

void SetGlobalCompileFlag(int32_t flag)
{
    compileFlag = flag;
}

int32_t GetGlobalCompileFlag()
{
    return compileFlag;
}

namespace acl {
static void MakeHostMemTensor(const aclTensorDesc *desc, const aclDataBuffer *dataBuffer,
                              int32_t compileFlag, GeTensorDesc &geTensorDesc)
{
    if ((desc->memtype == ACL_MEMTYPE_HOST) && (!desc->isConst)) {
        if (compileFlag == 0) {
            // During static compilation, change hostMem to const input.
            ACL_LOG_INFO("compleFlag is ACL_OP_COMPILE_DEFAULT, change hostMem to const.");
            AttrUtils::SetBool(geTensorDesc, ge::CONST_ATTR_NAME_INPUT, true);
            ge::ConstGeTensorPtr constTensor = nullptr;
            ACL_MAKE_SHARED(constTensor = std::make_shared<GeTensor>(geTensorDesc,
                static_cast<uint8_t *>(dataBuffer->data), dataBuffer->length), ;);
            ge::AttrUtils::SetTensor(geTensorDesc, ge::ATTR_NAME_WEIGHTS, constTensor);
        } else {
            // During fuzzy compilation, change hostMem to data input.
            ACL_LOG_INFO("compleFlag is ACL_OP_COMPILE_FUZZ, change hostMem to data.");
            ge::ConstGeTensorPtr dataTensor = nullptr;
            ACL_MAKE_SHARED(dataTensor = std::make_shared<GeTensor>(geTensorDesc,
                static_cast<uint8_t *>(dataBuffer->data), dataBuffer->length), ;);
            ge::AttrUtils::SetTensor(geTensorDesc, ge::ATTR_NAME_VALUE, dataTensor);
        }
    }
}

static void OptimizeTensorDescForTransdata(const AclOp &aclOp, bool isInput, GeTensorDesc &geTensorDesc)
{
    if ((aclOp.opType != "TransData") ||
        (aclOp.inputDesc[0] == nullptr) || (aclOp.outputDesc[0] == nullptr)) {
        return;
    }
    if ((aclOp.inputDesc[0]->storageFormat == ACL_FORMAT_UNDEFINED) &&
        (aclOp.outputDesc[0]->storageFormat == ACL_FORMAT_UNDEFINED)) {
        // This TransData is not set storageFormat, old process
        ge::Format inOriFormat = ge::FORMAT_RESERVED;
        if (aclOp.inputDesc[0]->format != ACL_FORMAT_UNDEFINED) {
            inOriFormat = static_cast<::ge::Format>(aclOp.inputDesc[0]->format);
        }
        ge::Format outOriFormat = ge::FORMAT_RESERVED;
        if (aclOp.outputDesc[0]->format != ACL_FORMAT_UNDEFINED) {
            outOriFormat = static_cast<::ge::Format>(aclOp.outputDesc[0]->format);
        }
        ACL_LOG_INFO("Find input origin format %d, output origin format %d", inOriFormat, outOriFormat);
        ge::Format transdataOriFormat = FORMAT_RESERVED;
        // if output is oringin format,input is not, need update input
        if (ge::TypeUtils::IsInternalFormat(inOriFormat) && !ge::TypeUtils::IsInternalFormat(outOriFormat)) {
            transdataOriFormat = outOriFormat;
            if (isInput) {
                geTensorDesc.SetOriginFormat(transdataOriFormat);
                geTensorDesc.SetOriginShape(GeShape(aclOp.outputDesc[0]->dims));
            }
        // update output
        } else {
            transdataOriFormat = inOriFormat;
            if (!isInput) {
                geTensorDesc.SetOriginFormat(transdataOriFormat);
                geTensorDesc.SetOriginShape(GeShape(aclOp.inputDesc[0]->dims));
            }
        }
        ACL_LOG_INFO("Find origin format is %d", transdataOriFormat);
    }
    return;
}

static aclError MakeInputCompileParam(const AclOp &aclOp, CompileParam &param,
                                      OpDesc *opDesc, int32_t compileFlag)
{
    for (int i = 0; i < aclOp.numInputs; ++i) {
        const aclTensorDesc *desc = aclOp.inputDesc[i];
        if (!desc->CheckShapeRange()) {
            ACL_LOG_INNER_ERROR("the number of shapeRange is not equal to number of dims");
            return ACL_ERROR_INVALID_PARAM;
        }
        ge::Format geFormat = ge::FORMAT_RESERVED;
        if (desc->format != ACL_FORMAT_UNDEFINED) {
            geFormat = static_cast<::ge::Format>(desc->format);
        }
        ge::DataType geDataType = ge::DT_UNDEFINED;
        if (desc->dataType != ACL_DT_UNDEFINED) {
            geDataType = static_cast<::ge::DataType>(desc->dataType);
        }
        GeTensorDesc geTensorDesc(GeShape(desc->dims),
                                  geFormat,
                                  geDataType);
        geTensorDesc.SetOriginFormat(geFormat);
        if (aclOp.opType == "TransData") {
            ACL_LOG_INFO("This op is TransData of input");
            if (desc->storageFormat != ACL_FORMAT_UNDEFINED) {
                ACL_LOG_INFO("TransData create aclop inputDesc");
                geTensorDesc.SetShape(GeShape(desc->storageDims));
                geTensorDesc.SetFormat(static_cast<::ge::Format>(desc->storageFormat));
                geTensorDesc.SetDataType(geDataType);
                geTensorDesc.SetOriginShape(GeShape(desc->dims));
                geTensorDesc.SetOriginDataType(geDataType);
            }
        }
        if (geTensorDesc.SetShapeRange(desc->shapeRange) != ge::GRAPH_SUCCESS) {
            ACL_LOG_INNER_ERROR("set shape range fail, opType: %s", aclOp.opType.c_str());
            return ACL_ERROR_GE_FAILURE;
        }
        AttrUtils::SetInt(geTensorDesc, ge::ATTR_NAME_PLACEMENT, static_cast<int64_t>(desc->memtype));
        TensorUtils::SetRealDimCnt(geTensorDesc, desc->dims.size());
        TensorUtils::SetInputTensor(geTensorDesc, true);
        TensorUtils::SetOutputTensor(geTensorDesc, false);
        if (desc->storageFormat != ACL_FORMAT_UNDEFINED) {
            AttrUtils::SetInt(geTensorDesc, ge::ATTR_NAME_STORAGE_FORMAT, static_cast<int64_t>(desc->storageFormat));
            AttrUtils::SetListInt(geTensorDesc, ge::ATTR_NAME_STORAGE_SHAPE, desc->storageDims);
        }
        if ((aclOp.inputs != nullptr) && (aclOp.inputs[i] != nullptr)) {
            MakeHostMemTensor(desc, aclOp.inputs[i], compileFlag, geTensorDesc);
        } else {
            ACL_LOG_INFO("the aclop maybe is create in aclopCompile, no inputs");
        }

        if (desc->isConst) {
            AttrUtils::SetBool(geTensorDesc, ge::CONST_ATTR_NAME_INPUT, true);
            ge::ConstGeTensorPtr constTensor = nullptr;
            ACL_MAKE_SHARED(constTensor = std::make_shared<GeTensor>(geTensorDesc,
                static_cast<uint8_t *>(desc->constDataBuf.get()), desc->constDataLen), ;);
            ge::AttrUtils::SetTensor(geTensorDesc, ge::ATTR_NAME_WEIGHTS, constTensor);
        }

        OptimizeTensorDescForTransdata(aclOp, true, geTensorDesc);

        if (!desc->name.empty()) {
            opDesc->AddInputDesc(desc->name, geTensorDesc);
        } else {
            opDesc->AddInputDesc(geTensorDesc);
        }

        param.inputs.emplace_back(geTensorDesc);
    }
    return ACL_SUCCESS;
}

static aclError MakeOutputCompileParam(const AclOp &aclOp, CompileParam &param,
                                       OpDesc *opDesc, int32_t compileFlag)
{
    for (int i = 0; i < aclOp.numOutputs; ++i) {
        const aclTensorDesc *desc = aclOp.outputDesc[i];
        GeTensorDesc geTensorDesc(GeShape(desc->dims),
                                  static_cast<::ge::Format>(desc->format),
                                  static_cast<::ge::DataType >(desc->dataType));
        geTensorDesc.SetOriginFormat(static_cast<::ge::Format>(desc->format));
        if (aclOp.opType == "TransData") {
            ACL_LOG_INFO("This op is TransData of output");
            if (desc->storageFormat != ACL_FORMAT_UNDEFINED) {
                ACL_LOG_INFO("TransData create aclop outputDesc");
                geTensorDesc.SetShape(GeShape(desc->storageDims));
                geTensorDesc.SetFormat(static_cast<::ge::Format>(desc->storageFormat));
                geTensorDesc.SetDataType(static_cast<::ge::DataType >(desc->dataType));
                geTensorDesc.SetOriginShape(GeShape(desc->dims));
                geTensorDesc.SetOriginDataType(static_cast<::ge::DataType >(desc->dataType));
            }
        }
        if (geTensorDesc.SetShapeRange(desc->shapeRange) != ge::GRAPH_SUCCESS) {
            ACL_LOG_INNER_ERROR("set shape range fail, opType: %s", aclOp.opType.c_str());
            return ACL_ERROR_GE_FAILURE;
        }

        AttrUtils::SetInt(geTensorDesc, ge::ATTR_NAME_PLACEMENT, static_cast<int64_t>(desc->memtype));
        TensorUtils::SetRealDimCnt(geTensorDesc, desc->dims.size());
        TensorUtils::SetInputTensor(geTensorDesc, false);
        TensorUtils::SetOutputTensor(geTensorDesc, true);
        if (desc->storageFormat != ACL_FORMAT_UNDEFINED) {
            AttrUtils::SetInt(geTensorDesc, ge::ATTR_NAME_STORAGE_FORMAT, static_cast<int64_t>(desc->storageFormat));
            AttrUtils::SetListInt(geTensorDesc, ge::ATTR_NAME_STORAGE_SHAPE, desc->storageDims);
        }
        if ((aclOp.outputs != nullptr) && (aclOp.outputs[i] != nullptr)) {
            MakeHostMemTensor(desc, aclOp.outputs[i], compileFlag, geTensorDesc);
        } else {
            ACL_LOG_INFO("the aclop maybe is create in aclopCompile, no inputs");
        }
        if (desc->isConst) {
            AttrUtils::SetBool(geTensorDesc, ge::CONST_ATTR_NAME_INPUT, true);
            ge::ConstGeTensorPtr constTensor = nullptr;
            ACL_MAKE_SHARED(constTensor = std::make_shared<GeTensor>(geTensorDesc,
                static_cast<uint8_t *>(desc->constDataBuf.get()), desc->constDataLen), ;);
            ge::AttrUtils::SetTensor(geTensorDesc, ge::ATTR_NAME_WEIGHTS, constTensor);
        }

        OptimizeTensorDescForTransdata(aclOp, false, geTensorDesc);

        if (!desc->name.empty()) {
            opDesc->AddOutputDesc(desc->name, geTensorDesc);
        } else {
            opDesc->AddOutputDesc(geTensorDesc);
        }
        param.outputs.emplace_back(geTensorDesc);
    }
    return ACL_SUCCESS;
}

aclError OpCompiler::MakeCompileParam(const AclOp &aclOp, CompileParam &param, int32_t compileFlag)
{
    OpDescPtr opDesc = nullptr;
    ACL_MAKE_SHARED(opDesc = std::make_shared<ge::OpDesc>(aclOp.opType, aclOp.opType), return ACL_ERROR_BAD_ALLOC);
    ACL_CHECK_MALLOC_RESULT(opDesc);

    aclError inputRet = MakeInputCompileParam(aclOp, param, opDesc.get(), compileFlag);
    if (inputRet != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("make input compile param failed, result = %d", inputRet);
        return inputRet;
    }

    aclError outputRet = MakeOutputCompileParam(aclOp, param, opDesc.get(), compileFlag);
    if (outputRet != ACL_SUCCESS) {
        ACL_LOG_INNER_ERROR("make output compile param failed, result = %d", outputRet);
        return outputRet;
    }

    std::string attrTypeList;
    if (aclOp.opAttr != nullptr) {
        for (const auto &it : aclOp.opAttr->Attrs()) {
            opDesc->SetAttr(it.first, it.second);
            if (aclOp.compileType == OP_COMPILE_UNREGISTERED) {
                GeAttrValue::ValueType valType = it.second.GetValueType();
                auto valTypeIt = ATTR_TYPES_MAP.find(valType);
                if (valTypeIt == ATTR_TYPES_MAP.end()) {
                    ACL_LOG_INNER_ERROR("Invalid attr value type, valType: %d", static_cast<int32_t>(valType));
                    return ACL_ERROR_INVALID_PARAM;
                }
                attrTypeList.append(it.first).append(":")
                            .append(valTypeIt->second).append(";");
            }
        }
        // delete ; from end of attrTypeList
        if (attrTypeList.length() != 0) {
            attrTypeList = attrTypeList.substr(0, attrTypeList.length() - 1);
        }
    }

    if (aclOp.compileType == OP_COMPILE_UNREGISTERED) {
        (void)AttrUtils::SetStr(opDesc, ge::ATTR_NAME_UNREGST_OPPATH, aclOp.opPath);
        (void)AttrUtils::SetStr(opDesc, ge::ATTR_NAME_UNREGST_ATTRLIST, attrTypeList);
    }

    // set dynamic input attr
    array_utils::DynamicInputIndexPair indexPair;
    bool ret = array_utils::GetDynamicInputIndex(aclOp.numInputs, aclOp.inputDesc, indexPair);
    if (ret != true) {
        ACL_LOG_INFO("failed to get dynamic input index, invalid dynamic input attr, op type: %s",
            aclOp.opType.c_str());
        return ACL_ERROR_INVALID_PARAM;
    }

    if (indexPair.first.size() > 0) {
        AttrUtils::SetListInt(opDesc, ge::ATTR_NAME_DYNAMIC_INPUT_START, indexPair.first);
        AttrUtils::SetListInt(opDesc, ge::ATTR_NAME_DYNAMIC_INPUT_END, indexPair.second);
    }

    param.opDesc = std::move(opDesc);
    param.engineType = static_cast<ge::OpEngineType>(aclOp.engineType);
    param.compileFlag = compileFlag;
    return ACL_SUCCESS;
}

aclError OpCompiler::CompileOp(const AclOp &aclOp, std::shared_ptr<void> &modelData, size_t &modelSize)
{
    int32_t compileFlag = GetGlobalCompileFlag();
    ACL_LOG_INFO("To compile op: %s, compileFlag is %d", aclOp.opType.c_str(), compileFlag);
    CompileParam param;
    ACL_REQUIRES_OK(MakeCompileParam(aclOp, param, compileFlag));
    ACL_REQUIRES_OK(DoCompile(param, modelData, modelSize));
    return ACL_SUCCESS;
}
} // namespace acl

