/**
* @file op_model_parser.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "op_model_parser.h"

#include "framework/common/types.h"
#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/helper/om_file_helper.h"
#include "graph/utils/attr_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_error_codes.h"

#include "common/log_inner.h"

using ge::MODEL_FILE_MAGIC_NUM;
using ge::ModelFileHeader;
using ge::OmFileLoadHelper;
using ge::ModelPartition;
using ge::ModelPartitionType;

using namespace std;

namespace acl {
namespace {
const string ATTR_KEY_OP_TYPE = "ATTR_MODEL_OP_TYPE";
const string ATTR_KEY_INPUT_TENSOR_DESC = "ATTR_MODEL_TENSOR_INPUTS";
const string ATTR_KEY_OUTPUT_TENSOR_DESC = "ATTR_MODEL_TENSOR_OUTPUTS";
const string ATTR_KEY_PREFIX = "ATTR_MODEL_";
const string ATTR_SUPPORT_DYNAMICSHAPE = "support_dynamicshape";
}

aclError OpModelParser::ParseOpModel(OpModel &opModel, OpModelDef &modelDef)
{
    ge::Model model;
    ACL_REQUIRES_OK(DeserializeModel(opModel, model));
    ACL_REQUIRES_OK(ToModelConfig(model, modelDef));
    ACL_LOG_INFO("parse op model success, model name = %s, ModelDef = %s", opModel.name.c_str(),
        modelDef.DebugString().c_str());
    opModel.isStaticModelWithFuzzCompile = modelDef.isStaticModelWithFuzzCompile;
    return ACL_SUCCESS;
}

aclError OpModelParser::DeserializeModel(const OpModel &opModel, ge::Model &model)
{
    uint8_t *modelData = nullptr;
    uint32_t modelSize;
    ACL_REQUIRES_OK(ParseModelContent(opModel, modelSize, modelData));

    OmFileLoadHelper helper;
    auto geRet = helper.Init(modelData, modelSize);
    if (geRet != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Init][OmFileLoadHelper]Init OmFileLoadHelper failed. modelSize = %u, ge result = %u",
            modelSize, geRet);
        return ACL_ERROR_DESERIALIZE_MODEL;
    }

    ModelPartition modelPartition;
    geRet = helper.GetModelPartition(ModelPartitionType::MODEL_DEF, modelPartition);
    if (geRet != ge::SUCCESS) {
        ACL_LOG_CALL_ERROR("[Get][Model]Get MODEL_DEF Partition failed. modelSize = %u, ge result = %u", 
            modelSize, geRet);
        return ACL_ERROR_DESERIALIZE_MODEL;
    }

    auto ret = ge::Model::Load(modelPartition.data, modelPartition.size, model);
    if (ret != ge::GRAPH_SUCCESS) {
        ACL_LOG_CALL_ERROR("[Load][Model]Load model failed. ge result = %u", ret);
        return ACL_ERROR_DESERIALIZE_MODEL;
    }

    return ACL_SUCCESS;
}

aclError OpModelParser::ParseModelContent(const OpModel &opModel, uint32_t &modelSize, uint8_t *&modelData)
{
    if (opModel.size <= sizeof(ModelFileHeader)) {
        ACL_LOG_INNER_ERROR("[Check][Size]invalid model. length[%zu] is smaller than or equal to "
            "ModelFileHeader size[%zu]", opModel.size, sizeof(ModelFileHeader));
        return ACL_ERROR_PARSE_MODEL;
    }

    auto *file_header = reinterpret_cast<ModelFileHeader *>(opModel.data.get());
    modelSize = file_header->length;
    if (file_header->length + sizeof(ModelFileHeader) != opModel.size) {
        ACL_LOG_INNER_ERROR("[Check][Length]invalid model. header size = %u, model size = %u,"
            "file size = %u", sizeof(ModelFileHeader), modelSize, opModel.size);
        return ACL_ERROR_PARSE_MODEL;
    }

    modelData = reinterpret_cast<uint8_t *>(opModel.data.get()) + sizeof(ModelFileHeader);
    return ACL_SUCCESS;
}

static void GetFuzzBuildRet(ge::Model &model,
                            ge::GeAttrValue::LIST_NAMED_ATTRS &inputSupportAttrs,
                            ge::GeAttrValue::LIST_NAMED_ATTRS &outputSupportAttrs)
{
    ge::GeAttrValue::LIST_NAMED_ATTRS fuzzBuildAttrs;
    (void)ge::AttrUtils::GetListNamedAttrs(model, ge::ATTR_NAME_FUZZ_BUILD_RES_ATTRS, fuzzBuildAttrs);
    if (fuzzBuildAttrs.empty()) {
        ACL_LOG_INFO("the model maybe not by fuzzy compiling");
        return;
    }
    // ignore output
    ge::GeAttrValue::NAMED_ATTRS fuzzBuildAttr = fuzzBuildAttrs[0];

    (void)ge::AttrUtils::GetListNamedAttrs(fuzzBuildAttr, ge::ATTR_NAME_FUZZ_INPUTS_SUPPORTED_ATTRS, inputSupportAttrs);
    (void)ge::AttrUtils::GetListNamedAttrs(fuzzBuildAttr,
                                           ge::ATTR_NAME_FUZZ_OUTPUTS_SUPPORTED_ATTRS, outputSupportAttrs);
    ACL_LOG_INFO("inputSupportAttrs is %zu, outputSupportAttrs is %zu",
                 inputSupportAttrs.size(), outputSupportAttrs.size());
}

static aclError UpdateTensorAttrs(std::vector<aclTensorDesc> &tensorDescs,
                                  const ge::GeAttrValue::LIST_NAMED_ATTRS &tensorSupportAttrs)
{
    size_t tensorNum = 0;
    for (size_t i = 0; i < tensorSupportAttrs.size(); ++i) {
        ge::GeAttrValue::LIST_NAMED_ATTRS tensorAttrs;
        (void)ge::AttrUtils::GetListNamedAttrs(tensorSupportAttrs[i], "tensor", tensorAttrs);
        ACL_LOG_INFO("the number of dynamicInput is %zu, index is %zu", tensorAttrs.size(), i);
        tensorNum += tensorAttrs.size();
    }
    ACL_LOG_INFO("tensorSupportAttrs size is %zu, tensorDesc size is %zu", tensorNum, tensorDescs.size());
    if (tensorNum != tensorDescs.size()) {
        ACL_LOG_INNER_ERROR("[Check][Size]The size [%zu] of attrs is not equal to size [%zu] of inputs.",
            tensorNum, tensorDescs.size());
        return ACL_ERROR_PARSE_MODEL;
    }
    int64_t index = 0;
    for (size_t i = 0; i < tensorSupportAttrs.size(); ++i) {
        ge::GeAttrValue::NAMED_ATTRS supportAttr = tensorSupportAttrs[i];
        ge::GeAttrValue::LIST_NAMED_ATTRS tensorAttrs;
        (void)ge::AttrUtils::GetListNamedAttrs(supportAttr, "tensor", tensorAttrs);
        for (size_t k = 0; k < tensorAttrs.size(); ++k) {
            ge::GeAttrValue::NAMED_ATTRS tensorAttr = tensorAttrs[k];

            ge::GeAttrValue::LIST_INT shape;
            ge::GeAttrValue::LIST_LIST_INT shapeRange;
            std::string shape_key_name = "shape";
            std::string shape_range_key_name = "shapeRange";
            // shape item must be existed
            if (tensorAttr.GetItem(shape_key_name).GetValue<ge::GeAttrValue::LIST_INT>(shape) != ge::SUCCESS) {
                ACL_LOG_INNER_ERROR("[Get][Item]Can not find attr of shape.");
                return ACL_ERROR_PARSE_MODEL;
            }
            // change LIST_INT to vector<int64>
            bool needCheckRange = false;
            std::vector<int64_t> shapeByAttr;
            for (size_t indexShape = 0; indexShape < shape.size(); ++indexShape) {
                // if shape is -2, no need to check shape range
                if (shape.at(indexShape) == -1) {
                    needCheckRange = true;
                }
                ACL_LOG_INFO("shape is %ld", shape.at(indexShape));
                shapeByAttr.emplace_back(shape.at(indexShape));
            }

            // maybe no range item
            (void)tensorAttr.GetItem(shape_range_key_name).GetValue<ge::GeAttrValue::LIST_LIST_INT>(shapeRange);
            // change LIST_LIST_INT to vector<pair<int64, int64>>
            std::vector<std::pair<int64_t, int64_t>> rangesByAttr;
            for (size_t indexRange = 0; indexRange < shapeRange.size(); ++indexRange) {
                std::pair<int64_t, int64_t> range;
                range.first = shapeRange[indexRange][0];
                range.second = shapeRange[indexRange][1];
                ACL_LOG_INFO("shape_ranges is [%ld, %ld].", range.first, range.second);
                rangesByAttr.emplace_back(range);
            }

            if (needCheckRange && (shapeRange.size() != shape.size())) {
                ACL_LOG_INNER_ERROR("[Check][Range]the number[%zu] of shape is not equal to number[%zu] of "
                    "shapeRange in model.", shape.size(), shapeRange.size());
                return ACL_ERROR_PARSE_MODEL;
            }
            ACL_LOG_INFO("the number of shape is [%zu], number of shapeRange is [%zu].",
                         shape.size(), shapeRange.size());
            tensorDescs[index].UpdateTensorShape(shapeByAttr);
            tensorDescs[index].UpdateTensorShapeRange(rangesByAttr);
            if ((ge::AttrUtils::HasAttr(tensorAttr, "value_range")) && (ge::AttrUtils::HasAttr(tensorAttr, "value"))) {
                ACL_LOG_INNER_ERROR("value and value_range can not be existed at the same time");
                return ACL_ERROR_PARSE_MODEL;
            }
            if (ge::AttrUtils::HasAttr(tensorAttr, "value_range")) {
                ACL_LOG_INFO("find value_range attr in %ld", index);
                tensorDescs[index].valueRange[RANGE_TYPE] = tensorAttr.GetItem("value_range");
            }
            if (ge::AttrUtils::HasAttr(tensorAttr, "value")) {
                ACL_LOG_INFO("find value attr in %ld", index);
                tensorDescs[index].valueRange[VALUE_TYPE] = tensorAttr.GetItem("value");
            }
            index++;
        }
    }
    ACL_LOG_INFO("UpdateTensorAttrs succ");
    return ACL_SUCCESS;
}

aclError OpModelParser::ToModelConfig(ge::Model &model, OpModelDef &modelDef)
{
    vector<ge::GeTensorDesc> inputTensorDescList;
    vector<ge::GeTensorDesc> outputTensorDescList;
    vector<ge::ConstGeTensorPtr> inputTensorList;
    vector<ge::ConstGeTensorPtr> outputTensorList;
    bool missingAttrs = !ge::AttrUtils::GetListTensor(model, ATTR_KEY_INPUT_TENSOR_DESC, inputTensorList) ||
                        !ge::AttrUtils::GetListTensor(model, ATTR_KEY_OUTPUT_TENSOR_DESC, outputTensorList) ||
                        !ge::AttrUtils::GetStr(model, ATTR_KEY_OP_TYPE, modelDef.opType);
    if (missingAttrs) {
        ACL_LOG_WARN("Missing required attr. model = %s", modelDef.modelPath.c_str());
        return ACL_ERROR_MODEL_MISSING_ATTR;
    }

    bool supportDynamic = true;
    if (ge::AttrUtils::HasAttr(model, ATTR_SUPPORT_DYNAMICSHAPE)) {
        (void)ge::AttrUtils::GetBool(model, ATTR_SUPPORT_DYNAMICSHAPE, supportDynamic);
    } else {
        ACL_LOG_INFO("model[%s] does't have support_dynamicshape attr", modelDef.opType.c_str());
    }
    int32_t buildMode = 0; // 0:ACL_OP_COMPILE_DEFAULT ACL_OP_COMPILE_FUZZ
    (void)ge::AttrUtils::GetInt(model, ge::ATTR_NAME_BUILD_MODE, buildMode);
    if ((!supportDynamic) && (buildMode == 0)) {
        ACL_LOG_INNER_ERROR("[Check][SupportDynamic]model[%s] does't support dynamic shape",
            modelDef.opType.c_str());
        return ACL_ERROR_OP_UNSUPPORTED_DYNAMIC;
    }

    for (auto &tensor : inputTensorList) {
        inputTensorDescList.emplace_back(tensor->GetTensorDesc());
    }

    for (auto &tensor : outputTensorList) {
        outputTensorDescList.emplace_back(tensor->GetTensorDesc());
    }

    // parse input desc
    ACL_REQUIRES_OK(ParseGeTensorDesc(inputTensorDescList, modelDef.inputDescArr, modelDef.opType));

    // parse output desc
    ACL_REQUIRES_OK(ParseGeTensorDesc(outputTensorDescList, modelDef.outputDescArr, modelDef.opType));

    if (buildMode == 1) {
        bool staticModel = !ge::AttrUtils::HasAttr(model, ge::ATTR_NAME_FUZZ_BUILD_RES_ATTRS) &&
                           !ge::AttrUtils::HasAttr(model, "_AllShape");
        if (staticModel) {
            // ACL_OP_COMPILE_FUZZ mode but model is static
            ACL_LOG_INFO("isStaticModelWithFuzzCompile is 1, the model is static model with fuzz compile");
            modelDef.isStaticModelWithFuzzCompile = 1;
        } else {
            // ACL_OP_COMPILE_FUZZ mode and model is dynamic,maybe aicpu or aicore which return fuzz res
            ACL_LOG_INFO("The model [%s] is dynamic model with fuzz compile", modelDef.opType.c_str());
            modelDef.isStaticModelWithFuzzCompile = 2;
            if (ge::AttrUtils::HasAttr(model, ge::ATTR_NAME_FUZZ_BUILD_RES_ATTRS)) {
                ACL_LOG_INFO("The model [%s] is dynamic model with fuzz result", modelDef.opType.c_str());
                ge::GeAttrValue::LIST_NAMED_ATTRS inputSupportAttrs;
                ge::GeAttrValue::LIST_NAMED_ATTRS outputSupportAttrs;
                GetFuzzBuildRet(model, inputSupportAttrs, outputSupportAttrs);
                if (inputSupportAttrs.size() != 0) {
                    ACL_REQUIRES_OK(UpdateTensorAttrs(modelDef.inputDescArr, inputSupportAttrs));
                }
                if (outputSupportAttrs.size() != 0) {
                    ACL_REQUIRES_OK(UpdateTensorAttrs(modelDef.outputDescArr, outputSupportAttrs));
                }
            }
        }
    }

    model.DelAttr(ATTR_KEY_INPUT_TENSOR_DESC);
    model.DelAttr(ATTR_KEY_OUTPUT_TENSOR_DESC);
    model.DelAttr(ATTR_KEY_OP_TYPE);
    ACL_LOG_INFO("after delete attrs");

    // parse attr
    ParseOpAttrs(model, modelDef.opAttr);
    ACL_LOG_INFO("after parseOpAttrs");
    return ACL_SUCCESS;
}

static aclError ParseConstTensor(const ge::GeTensorDesc &tensorDesc, aclTensorDesc &outputDesc)
{
    ge::ConstGeTensorPtr constTensor;
    if (!ge::AttrUtils::GetTensor(tensorDesc, ge::ATTR_NAME_WEIGHTS, constTensor)) {
        ACL_LOG_INNER_ERROR("[Get][Tensor]get const tensor failed");
        return ACL_ERROR_PARSE_MODEL;
    }
    size_t constDataLen = constTensor->GetData().GetSize();
    if (constDataLen == 0) {
        ACL_LOG_INFO("parse constDataLen is 0");
        outputDesc.isConst = true;
        return ACL_SUCCESS;
    }
    const uint8_t *constDataBuf = constTensor->GetData().GetData();
    ACL_CHECK_MALLOC_RESULT(constDataBuf);
    if (constDataLen < 0) {
        ACL_LOG_INNER_ERROR("[Get][Data]get const dataLen failed, constDataLen:[%zu] <= 0", constDataLen);
        return ACL_ERROR_PARSE_MODEL;
    }

    auto *constData = new (std::nothrow) char[constDataLen];
    ACL_CHECK_MALLOC_RESULT(constData);
    if (memcpy_s(constData, constDataLen, constDataBuf, constDataLen) != EOK) {
        ACL_LOG_INNER_ERROR("[Copy][Mem]Copy const data failed. size = %zu", constDataLen);
        ACL_DELETE_ARRAY_AND_SET_NULL(constData);
        return ACL_ERROR_PARSE_MODEL;
    }
    outputDesc.constDataBuf.reset(constData, [](const char *p)
        { delete[]p; ACL_LOG_DEBUG("delete const data in ParseConstTensor"); });

    ACL_LOG_INFO("parse constDataLen is %zu", constDataLen);
    outputDesc.isConst = true;
    outputDesc.constDataLen = constDataLen;
    return ACL_SUCCESS;
}

aclError OpModelParser::ParseGeTensorDesc(vector<ge::GeTensorDesc> &geTensorDescList,
                                          vector<aclTensorDesc> &output, std::string &opType)
{
    size_t tensorNum = 0;
    for (auto &tensorDesc : geTensorDescList) {
        aclFormat format = ACL_FORMAT_UNDEFINED;
        aclFormat storageFormat = ACL_FORMAT_UNDEFINED;
        aclDataType dataType = ACL_DT_UNDEFINED;
        std::vector<int64_t> storageShape;
        ge::GeShape shape;
        if ((opType == "TransData") && ge::AttrUtils::HasAttr(tensorDesc, ge::ATTR_NAME_STORAGE_FORMAT)) {
            ACL_LOG_INFO("now parse tensor op is new TransData");
            if (tensorDesc.GetOriginFormat() != ge::FORMAT_RESERVED) {
                format = static_cast<aclFormat>(tensorDesc.GetOriginFormat());
            }
            if (tensorDesc.GetFormat() != ge::FORMAT_RESERVED) {
                storageFormat = static_cast<aclFormat>(tensorDesc.GetFormat());
            }
            if (tensorDesc.GetDataType() != ge::DT_UNDEFINED) {
                dataType = static_cast<aclDataType>(tensorDesc.GetDataType());
            }
            shape = tensorDesc.GetOriginShape();
            auto &currentShape = tensorDesc.MutableShape();
            for (size_t i = 0; i < currentShape.GetDims().size(); ++i) {
                storageShape.emplace_back(currentShape.GetDims().at(i));
            }
        } else {
            if (tensorDesc.GetFormat() != ge::FORMAT_RESERVED) {
                format = static_cast<aclFormat>(tensorDesc.GetFormat());
            }
            int64_t storageFormatVal;
            if (ge::AttrUtils::GetInt(tensorDesc, ge::ATTR_NAME_STORAGE_FORMAT, storageFormatVal)) {
                storageFormat = static_cast<aclFormat>(storageFormatVal);
            }
            if (tensorDesc.GetDataType() != ge::DT_UNDEFINED) {
                dataType = static_cast<aclDataType>(tensorDesc.GetDataType());
            }
            shape = tensorDesc.GetShape();
            (void)ge::AttrUtils::GetListInt(tensorDesc, ge::ATTR_NAME_STORAGE_SHAPE, storageShape);
        }
        output.emplace_back(dataType, shape.GetDims().size(), shape.GetDims().data(), format);
        output.back().storageFormat = storageFormat;
        output.back().storageDims.swap(storageShape);
        std::vector<std::pair<int64_t, int64_t>> shapeRange;
        (void)tensorDesc.GetShapeRange(shapeRange);
        output.back().shapeRange.swap(shapeRange);
        int64_t memType;
        if (ge::AttrUtils::GetInt(tensorDesc, ge::ATTR_NAME_PLACEMENT, memType)) {
            output.back().memtype = static_cast<aclMemType>(memType);
        }
        bool isConst = false;
        if (!ge::AttrUtils::GetBool(tensorDesc, ge::CONST_ATTR_NAME_INPUT, isConst)) {
            ACL_LOG_INFO("the tensor maybe not const tensor, tensor id:[%d]", tensorNum++);
            continue;
        }
        if (isConst) {
            ACL_REQUIRES_OK(ParseConstTensor(tensorDesc, output.back()));
        }
    }
    return ACL_SUCCESS;
}

void OpModelParser::ParseOpAttrs(const ge::Model &model, aclopAttr &attrs)
{
    for (auto &attr : model.GetAllAttrs()) {
        const string &attrName = attr.first;
        if (attrName.find(ATTR_KEY_PREFIX) == 0) {
            auto stripped_name = attrName.substr(ATTR_KEY_PREFIX.size());
            if (stripped_name.empty() || stripped_name[0] == '_') {
                ACL_LOG_DEBUG("skip attribute: %s", stripped_name.c_str());
                continue;
            }
            attrs.EmplaceAttr(attrName.substr(ATTR_KEY_PREFIX.size()), attr.second);
        }
    }
}
} // namespace acl

