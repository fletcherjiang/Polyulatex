/**
* @file math_utils.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "math_utils.h"
#include "acl/acl_base.h"
#include "common/log_inner.h"

namespace acl {
aclError GetAlignedSize(size_t size, size_t &alignedSize)
{
    // check overflow, the max value of size must be less than 0xFFFFFFFFFFFFFFFF-32*2
    if (size + DATA_MEMORY_ALIGN_SIZE * 2 < size) {
        ACL_LOG_ERROR("[Check][Size]size too large: %zu", size);
        return ACL_ERROR_INVALID_PARAM;
    }

    // align size to multiple of 32 and puls 32
    alignedSize = (size + DATA_MEMORY_ALIGN_SIZE * 2 - 1) / DATA_MEMORY_ALIGN_SIZE * DATA_MEMORY_ALIGN_SIZE;
    return ACL_SUCCESS;
}
} // namespace acl

