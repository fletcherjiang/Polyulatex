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

#ifndef TSD_EZCOM_H_
#define TSD_EZCOM_H_

#include <string>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

namespace tdt {
enum AlarmLevel { CRITICAL = 1, MAJOR, MINOR, SUGGESTION };
enum AlarmReason { PROC_EXIT_ABNORMAL = 1, PROC_START_FAIL };
enum AlarmModule {
    QS = 1,
    AICPU_SD,
    HCCP,
    CUSTOM_AICPU_SD,
};
struct AlarmMessage {
    AlarmModule module;
    AlarmLevel level;
    uint32_t procId;
    AlarmReason reason;
    uint32_t addtionalSize;
};

enum AlarmMessageType {
    PROCESS_NAME = 1,
    ALARM_MESSAGE = 2,
    TSD_STARTED = 3,
};

struct Tsd2dmReqMsg {
    std::string pidAppName;
    AlarmMessageType msgType;
    AlarmMessage alarmMessage;
    int32_t qsPid;
    uint32_t rosNodePid;
};

using TsdCallbackFuncType = void (*)(struct Tsd2dmReqMsg *req);

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
int32_t RegisterTsdEzcomCallBack(TsdCallbackFuncType callBack);
}  // namespace tdt
#ifdef __cplusplus
}
#endif  // __cplusplus
#endif  // TSD_EZCOM_H_
