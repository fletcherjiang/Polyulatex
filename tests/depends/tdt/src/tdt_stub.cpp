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

#include "tdt/tdt_host_interface.h"

namespace tdt {
    int32_t TdtHostInit(uint32_t deviceId)
    {
        return 0;
    }

    int32_t TdtHostPreparePopData()
    {
        return 0;
    }

    int32_t TdtHostStop(const std::string &channelName)
    {
        return 0;
    }

    int32_t TdtHostDestroy()
    {
        return 0;
    }

    int32_t TdtHostPushData(const std::string &channelName, const std::vector<DataItem> &item, uint32_t deviceId)
    {
        return 0;
    }

    int32_t TdtHostPopData(const std::string &channelName, std::vector<DataItem> &item)
    {
        return 0;
    }

} //namespace tdt
