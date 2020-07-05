/**
* @file op_model_parser.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ACL_OP_EXEC_OP_MODEL_PARSER_H_
#define ACL_OP_EXEC_OP_MODEL_PARSER_H_

#include "op_model_manager.h"

#include "graph/model.h"

namespace acl {
class OpModelParser {
public:
    static aclError ParseOpModel(OpModel &opModel, OpModelDef &modelDef);

private:
    static aclError DeserializeModel(const OpModel &opModel, ge::Model &model);

    static aclError ToModelConfig(ge::Model &model, OpModelDef &modelDef);

    static aclError ParseModelContent(const OpModel &opModel, uint32_t &modelSize, uint8_t *&modelData);

    static aclError ParseGeTensorDesc(std::vector<ge::GeTensorDesc> &geTensorDescList,
                                      std::vector<aclTensorDesc> &output, std::string &opType);

    static void ParseOpAttrs(const ge::Model &model, aclopAttr &attrs);
};
} // namespace acl

#endif // ACL_OP_EXEC_OP_MODEL_PARSER_H_
