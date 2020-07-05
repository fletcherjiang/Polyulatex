/**
* @file transdata_op.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/ops/acl_cblas.h"
#include <map>
#include "types/op_attr.h"
#include "types/tensor_desc_internal.h"

namespace {
constexpr char const *OP_NAME_TRANSDATA = "TransData";
constexpr int TRANSDATA_INPUT_NUM = 1;
constexpr int TRANSDATA_OUTPUT_NUM = 1;

const char *ATTR_SRC_FORMAT = "src_format";
const char *ATTR_DST_FORMAT = "dst_format";

std::map<aclFormat, const char *> FORMAT_DICT {
        {ACL_FORMAT_NCHW,       "NCHW"},
        {ACL_FORMAT_NHWC,       "NHWC"},
        {ACL_FORMAT_ND,         "NCHW"}, // ND is treated as NCHW
        {ACL_FORMAT_NC1HWC0,    "NC1HWC0"},
        {ACL_FORMAT_FRACTAL_Z,  "FRACTAL_Z"},
        {ACL_FORMAT_FRACTAL_NZ, "FRACTAL_NZ"},
};

aclError GetFormatName(aclFormat format, const char **name)
{
    auto it = FORMAT_DICT.find(format);
    if (it == FORMAT_DICT.end()) {
        ACL_LOG_ERROR("Invalid format: %d", format);
        return ACL_ERROR_INVALID_PARAM;
    }

    *name = it->second;
    return ACL_SUCCESS;
}
}

aclError aclopTransData(aclTensorDesc *srcDesc,
                        aclDataBuffer *srcBuffer,
                        aclTensorDesc *dstDesc,
                        aclDataBuffer *dstBuffer,
                        aclrtStream stream)
{
    ACL_REQUIRES_NOT_NULL(srcDesc);
    ACL_REQUIRES_NOT_NULL(srcBuffer);
    ACL_REQUIRES_NOT_NULL(dstDesc);
    ACL_REQUIRES_NOT_NULL(dstBuffer);
    const char *srcFormat = nullptr;
    ACL_REQUIRES_OK(GetFormatName(srcDesc->format, &srcFormat));
    const char *dstFormat = nullptr;
    ACL_REQUIRES_OK(GetFormatName(dstDesc->format, &dstFormat));

    aclopAttr opAttr;
    aclopSetAttrString(&opAttr, ATTR_SRC_FORMAT, srcFormat);
    aclopSetAttrString(&opAttr, ATTR_DST_FORMAT, dstFormat);
    aclTensorDesc *inputDesc[TRANSDATA_INPUT_NUM] = {srcDesc};
    aclTensorDesc *outputDesc[TRANSDATA_OUTPUT_NUM] = {dstDesc};
    aclDataBuffer *inputs[TRANSDATA_INPUT_NUM] = {srcBuffer};
    aclDataBuffer *outputs[TRANSDATA_OUTPUT_NUM] = {dstBuffer};
    return aclopExecuteV2(OP_NAME_TRANSDATA,
                          TRANSDATA_INPUT_NUM,
                          inputDesc,
                          inputs,
                          TRANSDATA_OUTPUT_NUM,
                          outputDesc,
                          outputs,
                          &opAttr,
                          stream);
}

aclError aclopCreateHandleForTransData(aclTensorDesc *srcDesc,
                                       aclTensorDesc *dstDesc,
                                       aclopHandle **handle)
{
    ACL_REQUIRES_NOT_NULL(srcDesc);
    ACL_REQUIRES_NOT_NULL(dstDesc);
    ACL_REQUIRES_NOT_NULL(handle);
    const char *srcFormat = nullptr;
    ACL_REQUIRES_OK(GetFormatName(srcDesc->format, &srcFormat));
    const char *dstFormat = nullptr;
    ACL_REQUIRES_OK(GetFormatName(dstDesc->format, &dstFormat));

    aclopAttr opAttr;
    aclopSetAttrString(&opAttr, ATTR_SRC_FORMAT, srcFormat);
    aclopSetAttrString(&opAttr, ATTR_DST_FORMAT, dstFormat);
    const aclTensorDesc *inputDesc[TRANSDATA_INPUT_NUM] = {srcDesc};
    const aclTensorDesc *outputDesc[TRANSDATA_OUTPUT_NUM] = {dstDesc};
    return aclopCreateHandle(OP_NAME_TRANSDATA,
                             TRANSDATA_INPUT_NUM,
                             inputDesc,
                             TRANSDATA_OUTPUT_NUM,
                             outputDesc,
                             &opAttr,
                             handle);
}