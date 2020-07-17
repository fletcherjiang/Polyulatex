/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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
 */

#ifndef INC_TDT_TSD_H_
#define INC_TDT_TSD_H_

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
* @ingroup  Tsdaemon.
*
* Identifies that HCCP or Compute_process is waiting for
* Tsdaemon to issue a shutdown command.
*/
typedef enum {
  TSD_HCCP = 0,    /**< HCCP*/
  TSD_COMPUTE = 1, /**< Compute_process*/
  TSD_CUSTOM_COMPUTE = 2, /**< Custom Compute_process*/
  TSD_WAITTYPE_MAX /**< Max*/
} TsdWaitType;

/**
* @ingroup TsdWaitForShutdown
* @brief Wait for the TSD process to issue the shutdown command
*
* @par Function
* Wait for the TSD process to issue the shutdown command
*
* @param NA
* @param deviceID [IN] type #unsigned int. Physical device ID
* @param waitType [IN] type #TsdWaitType. HCCP or CP
* @param hostPid [IN] type #unsigned int. Host pid
* @retval 0 Success
* @retval OtherValues 0 Fail
*
* @par Dependency
* @li libtsdppc.so: Library to which the interface belongs.
* @li tsd.h: Header file where the interface declaration is located.
*/
int32_t TsdWaitForShutdown(const uint32_t deviceId, const TsdWaitType waitType, const uint32_t hostPid);

/**
* @ingroup TsdDestory
* @brief tsd event client send abnormal msg to tsd event server
*
* @par Function
* tsd event client send abnormal msg to tsd event server
*
* @param NA
* @param deviceID [IN] type #unsigned int. Physical device ID
* @param waitType [IN] type #TsdWaitType. HCCP or CP
* @param hostPid [IN] type #unsigned int. Host pid
* @retval 0 Success
* @retval OtherValues 0 Fail
*
* @par Dependency
* @li libtsdppc.so: Library to which the interface belongs.
* @li tsd.h: Header file where the interface declaration is located.
*/
int32_t TsdDestory(const uint32_t deviceId, const TsdWaitType waitType, const uint32_t hostPid);

/**
* @ingroup CreateOrFindCustPid
* @brief inform tsdaemon start aicpu_cust_schedule
*
* @par Function
* inform tsdaemon start aicpu_cust_schedule
*
* @param NA
* @param deviceID [IN] type #unsigned int. Physical device ID
* @param loadLibNum [IN] type #unsigned int. Load so nums
* @param loadLibName [IN] type #char *. Load so names
* @param hostPid [IN] type #unsigned int. Host pid
* @retval 0 Success
* @retval OtherValues 0 Fail
*
* @par Dependency
* @li libtsdppc.so: Library to which the interface belongs.
* @li tsd.h: Header file where the interface declaration is located.
*/
int32_t CreateOrFindCustPid(const uint32_t deviceId, const uint32_t loadLibNum, char *loadLibName[], const uint32_t hostPid);

#ifdef __cplusplus
}
#endif  // __cplusplus
#endif  // INC_TDT_TSD_H_
