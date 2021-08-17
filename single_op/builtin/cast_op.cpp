/**
* @file cast_op.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/ops/acl_cblas.h"
#include "types/op_attr.h"
#include "types/tensor_desc_internal.h"
#include "common/common_inner.h"
#include "toolchain/profiling_manager.h"

namespace {
    constexpr char const *OP_NAME_CAST = "Cast";
    constexpr char const *ATTR_NAME_TRUNCATE = "truncate";
    constexpr char const *ATTR_NAME_DST_TYPE = "dst_type";
    constexpr int32_t CAST_INPUT_NUM = 1;
    constexpr int32_t CAST_OUTPUT_NUM = 1;
}

aclError aclopCast(const aclTensorDesc * const srcDesc,
                   const aclDataBuffer * const srcBuffer,
                   const aclTensorDesc * const dstDesc,
                   aclDataBuffer * const dstBuffer,
                   uint8_t truncate,
                   aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_STAGES_REG(acl::ACL_STAGE_EXEC, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dstDesc);
    const aclTensorDesc *inputDesc[CAST_INPUT_NUM] = {srcDesc};
    const aclTensorDesc *outputDesc[CAST_OUTPUT_NUM] = {dstDesc};
    const aclDataBuffer *inputs[CAST_OUTPUT_NUM] = {srcBuffer};
    aclDataBuffer *outputs[CAST_OUTPUT_NUM] = {dstBuffer};
    aclopAttr opAttr;
    if (GetIfCastHasTruncateAttr()) {
        ACL_LOG_INFO("Need to set truncate attr in aclopCast");
        (void)opAttr.SetAttr(ATTR_NAME_TRUNCATE, static_cast<bool>(truncate));
    }
    (void)opAttr.SetAttr(ATTR_NAME_DST_TYPE, static_cast<int64_t>(dstDesc->dataType));
    return aclopExecuteV2(OP_NAME_CAST,
                          CAST_INPUT_NUM,
                          const_cast<aclTensorDesc **>(inputDesc),
                          const_cast<aclDataBuffer **>(inputs),
                          CAST_OUTPUT_NUM,
                          const_cast<aclTensorDesc **>(outputDesc),
                          outputs,
                          &opAttr,
                          stream);
}

aclError aclopCreateHandleForCast(aclTensorDesc * const srcDesc,
                                  aclTensorDesc * const dstDesc,
                                  uint8_t truncate,
                                  aclopHandle ** const handle)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_STAGES_REG(acl::ACL_STAGE_CREATE, acl::ACL_STAGE_DEFAULT);
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(dstDesc);
    const aclTensorDesc *inputDesc[CAST_INPUT_NUM] = {srcDesc};
    const aclTensorDesc *outputDesc[CAST_OUTPUT_NUM] = {dstDesc};
    aclopAttr opAttr;
    if (GetIfCastHasTruncateAttr()) {
        ACL_LOG_INFO("Need to set truncate attr in aclopCreateHandleForCast");
        (void)opAttr.SetAttr(ATTR_NAME_TRUNCATE, static_cast<bool>(truncate));
    }
    (void)opAttr.SetAttr(ATTR_NAME_DST_TYPE, static_cast<int64_t>(dstDesc->dataType));
    return aclopCreateHandle(OP_NAME_CAST,
                             CAST_INPUT_NUM,
                             inputDesc,
                             CAST_OUTPUT_NUM,
                             outputDesc,
                             &opAttr,
                             handle);
}
