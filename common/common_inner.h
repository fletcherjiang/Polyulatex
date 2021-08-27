/**
* @file common_inner.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ACL_COMMON_INNER_API_H_
#define ACL_COMMON_INNER_API_H_

#include "acl/acl_base.h"

constexpr size_t SOC_VERSION_LEN = 128U;

std::string GetSocVersion();

aclError InitSocVersion();

typedef aclError (*GeFinalizeCallback)();

ACL_FUNC_VISIBILITY void SetGeFinalizeCallback(GeFinalizeCallback func);

bool GetAclInitFlag();

ACL_FUNC_VISIBILITY void SetThreadCompileOpts(const std::map<std::string, std::string> &options);

void SetCastHasTruncateAttr(bool hasTruncate);

bool GetIfCastHasTruncateAttr();

void SetGlobalCompileFlag(int32_t flag);

int32_t GetGlobalCompileFlag();

#endif // ACL_COMMON_INNER_API_H_
