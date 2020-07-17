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

#ifndef HOST_INNER_INC_TDT_DEVICE_H_
#define HOST_INNER_INC_TDT_DEVICE_H_

#include <string.h>
#include <memory>
#include <vector>
#include "tdt/data_common.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

namespace tdt {
/**
 * @ingroup  TdtDevicePushData
 * @brief Tdt device push data to queue for ops.
 *
 * @par Function
 * Tdt device push data to queue for ops.
 *
 * @param channelName [IN] type #String. queue channel name
 * @param items [IN] type #vector<DataItem> DataItem is defined in data_common.h.  input data
 * @retval 0 Success
 * @retval OtherValues Fail
 *
 * @par Dependency
 * @li libtdtdevice.so: Library to which the interface belongs.
 * @li tdt_device.h: Header file where the interface declaration is located.
 * @li data_common.h: Header file where 'DataItem' defined
 *
 */
int32_t TdtDevicePushData(const std::string &channelName, std::vector<DataItem> &items);
}  // namespace tdt
#ifdef __cplusplus
}
#endif  // __cplusplus
#endif  // HOST_INNER_INC_TDT_DEVICE_H_
