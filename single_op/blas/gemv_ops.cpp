/**
* @file gemv_ops.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "acl/ops/acl_cblas.h"
#include "common/log_inner.h"
#include "toolchain/profiling_manager.h"

aclError aclblasGemvEx(aclTransType transA, int m, int n, const void *alpha,
    const void *a, int lda, aclDataType dataTypeA,
    const void *x, int incx, aclDataType dataTypeX,
    const void *beta, void *y, int incy, aclDataType dataTypeY,
    aclComputeType type, aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_STAGES_REG(acl::ACL_STAGE_BLAS, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclblasGemvEx");
    return aclblasGemmEx(transA, ACL_TRANS_N, ACL_TRANS_N,
        m, 1, n, alpha, a, lda, dataTypeA, x, incx,
        dataTypeX, beta, y, incy, dataTypeY, type, stream);
}

aclError aclblasCreateHandleForGemvEx(aclTransType transA, int m, int n,
                                      aclDataType dataTypeA,
                                      aclDataType dataTypeX,
                                      aclDataType dataTypeY,
                                      aclComputeType type,
                                      aclopHandle **handle)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_STAGES_REG(acl::ACL_STAGE_CREATE, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclblasCreateHandleForGemvEx");
    return aclblasCreateHandleForGemmEx(transA, ACL_TRANS_N, ACL_TRANS_N, m, 1, n,
        dataTypeA, dataTypeX, dataTypeY, type, handle);
}

aclError aclblasHgemv(aclTransType transA,
    int m, int n, const aclFloat16 *alpha, const aclFloat16 *a,
    int lda, const aclFloat16 *x, int incx, const aclFloat16 *beta,
    aclFloat16 *y, int incy, aclComputeType type, aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_STAGES_REG(acl::ACL_STAGE_BLAS, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclblasHgemv");
    return aclblasGemvEx(transA, m, n, alpha, a, lda, ACL_FLOAT16, x, incx,
        ACL_FLOAT16, beta, y, incy, ACL_FLOAT16, type, stream);
}

aclError aclblasCreateHandleForHgemv(aclTransType transA,
                                     int m,
                                     int n,
                                     aclComputeType type,
                                     aclopHandle **handle)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_STAGES_REG(acl::ACL_STAGE_CREATE, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclblasCreateHandleForHgemv");
    return aclblasCreateHandleForGemvEx(transA, m, n, ACL_FLOAT16, ACL_FLOAT16, ACL_FLOAT16, type, handle);
}

aclError aclblasCreateHandleForS8gemv(aclTransType transA,
                                      int m,
                                      int n,
                                      aclComputeType type,
                                      aclopHandle **handle)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_STAGES_REG(acl::ACL_STAGE_CREATE, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclblasCreateHandleForS8gemv");
    return aclblasCreateHandleForGemvEx(transA, m, n, ACL_INT8, ACL_INT8, ACL_INT32, type, handle);
}

aclError aclblasS8gemv(aclTransType transA,
    int m, int n, const int32_t *alpha, const int8_t *a,
    int lda, const int8_t *x, int incx, const int32_t *beta,
    int32_t *y, int incy, aclComputeType type, aclrtStream stream)
{
    ACL_PROFILING_REG(ACL_PROF_FUNC_OP);
    ACL_STAGES_REG(acl::ACL_STAGE_BLAS, acl::ACL_STAGE_DEFAULT);
    ACL_LOG_INFO("start to execute aclblasS8gemv");
    return aclblasGemvEx(transA, m, n, alpha, a, lda, ACL_INT8, x, incx,
        ACL_INT8, beta, y, incy, ACL_INT32, type, stream);
}
