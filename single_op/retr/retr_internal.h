/**
 * @file retr_internal.h
 *
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ACL_SINGLE_OP_RETR_INTERNAL_H
#define ACL_SINGLE_OP_RETR_INTERNAL_H

#include "runtime/rt.h"
#include "aicpu/retr/retr_def.h"
#include "types/tensor_desc_internal.h"

struct aclfvFeatureInfo {
    aclDataBuffer dataBuffer = {nullptr, 0};
    aicpu::retr::AicpuFvFeatureInfo retrFeatureInfo;
};

struct aclfvRepoRange {
    aclDataBuffer dataBuffer = {nullptr, 0};
    aicpu::retr::AicpuFvRepoRange retrRepoRange;
};

struct aclfvQueryTable {
    aicpu::retr::AicpuFvQueryTable queryTable;
};

struct aclfvSearchInput {
    aclDataBuffer dataBuffer = {nullptr, 0};
    aicpu::retr::AicpuFvSearchInput searchInput;
};

struct aclfvSearchResult {
    aicpu::retr::AicpuFvSearchResult searchResult;
    aclDataBuffer dataBuffer = {nullptr, 0};
};

struct aclfvInitPara {
    aclDataBuffer dataBuffer = {nullptr, 0};
    aicpu::retr::AicpuFvInitPara initPara;
};

namespace acl {
namespace retr {
constexpr const char *RETR_KERNELS_SONAME = "libretr_kernels.so";

struct aclfvArgs {
    char *args = nullptr;
    uint32_t argsSize = 0;
    uint32_t configOffset = 0;
};

/**
 * create notify and generate notifyid
 * @param notify: use to wait for current task end
 * @param notifyId: id of current notify
 * @param stream: stream
 * @return ACL_SUCCESS:success other:failed
 */
aclError aclCreateNotify(rtNotify_t &notify, uint32_t &notifyId, aclrtStream &stream);

aclError aclFvNotifyWait(rtNotify_t &notify, aclrtStream &stream);

/**
 * check result of stream task, 0 represent success
 * @param retCode: result of current task
 * @return ACL_SUCCESS:success other:failed
 */
aclError aclCheckResult(const void *retCode);
}  // namespace retr
}  // namespace acl

#endif  // ACL_SINGLE_OP_RETR_INTERNAL_H