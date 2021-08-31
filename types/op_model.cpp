/**
* @file op_model.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "op_model.h"
#include <sstream>
#include "framework/common/util.h"
#include "utils/attr_utils.h"


namespace acl {
std::string OpModelDef::DebugString() const
{
    std::stringstream ss;
    ss << "[OpModelDef] Path: " << modelPath;
    ss << ", OpType: " << opType << ", ";
    for (size_t i = 0U; i < inputDescArr.size(); ++i) {
        const aclTensorDesc &desc = inputDescArr.at(i);
        ss << "InputDesc[" << i << "]: ";
        ss << desc.DebugString() << " ";
    }

    for (size_t i = 0U; i < outputDescArr.size(); ++i) {
        const aclTensorDesc &desc = outputDescArr.at(i);
        ss << "OutputDesc[" << i << "]: ";
        ss << desc.DebugString() << " ";
    }

    ss << ", Attr: " << opAttr.DebugString();
    ss << ", isStaticModelWithFuzzCompile: " << isStaticModelWithFuzzCompile;
    return ss.str();
}

aclError ReadOpModelFromFile(const std::string &path, OpModel &model)
{
    int32_t fileSize;
    char *data = nullptr;
    ACL_LOG_DEBUG("start to call ge::ReadBytesFromBinaryFile");
    if (!ge::ReadBytesFromBinaryFile(path.c_str(), &data, fileSize)) {
        ACL_LOG_CALL_ERROR("[Read][Bytes]Read model file failed. path = %s", path.c_str());
        return ACL_ERROR_READ_MODEL_FAILURE;
    }

    std::shared_ptr<void> p_data(data, [](const char *p) { delete[]p; });
    model.data = p_data;
    model.size = static_cast<uint32_t>(fileSize);
    model.name = path;

    ACL_LOG_INFO("Read model file succeeded. file = %s, length = %d", path.c_str(), fileSize);
    return ACL_SUCCESS;
}
} // namespace acl