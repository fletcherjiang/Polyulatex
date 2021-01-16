/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved
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

#ifndef MSPROFILER_PROF_COMMON_H_
#define MSPROFILER_PROF_COMMON_H_

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define MSPROF_DATA_HEAD_MAGIC_NUM 0x5a5a

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
 * @name MsprofAclProfData
 * @brief struct of data reported by acl
 */
#define MSPROF_ACL_DATA_RESERVE_BYTES 24
struct MsprofAclProfData {
    uint16_t magicNumber;    // MSPROF_DATA_HEAD_MAGIC_NUM
    uint16_t dataTag;        // the value must be 0
    uint32_t apiType;        // enum MsprofAclApiType
    uint64_t apiHashValue;
    uint64_t beginTime;
    uint64_t endTime;
    uint32_t processId;
    uint32_t threadId;
    uint8_t reserve[MSPROF_ACL_DATA_RESERVE_BYTES];
};

#ifdef __cplusplus
}
#endif

#endif  // MSPROFILER_PROF_COMMON_H_