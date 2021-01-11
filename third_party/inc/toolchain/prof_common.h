/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file prof_common.h
 * @brief declaraion of profiling common datas and functions
 */

#ifndef MSPROFILER_PROF_COMMON_H
#define MSPROFILER_PROF_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#include "stddef.h"
#include "stdint.h"

/**
 * @name MsprofAclApiType
 * @brief acl api types
 */
enum MsprofAclApiType {
    MSPROF_ACL_API_TYPE_OP = 1,
    MSPROF_ACL_API_TYPE_MODEL,
    MSPROF_ACL_API_TYPE_RUNTIME,
    MSPROF_ACL_API_TYPE_OTHERS,
};

/**
 * @name AclProfData
 * @brief struct of data to reported by acl
 */
#define ACL_PROF_DATA_RESERVE_BYTES 24
struct MsprofAclProfData {
    uint16_t magicNumber;
    uint16_t dataTag;
    uint32_t apiType;
    uint64_t apiHashValue;
    uint64_t beginTime;
    uint64_t endTime;
    uint32_t processId;
    uint32_t threadId;
    uint8_t reserve[ACL_PROF_DATA_RESERVE_BYTES];
};

#ifdef __cplusplus
}
#endif

#endif  // MSPROFILER_PROF_COMMON_H_