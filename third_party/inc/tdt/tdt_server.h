﻿/**
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

#ifndef INC_TDT_TDT_SERVER_H
#define INC_TDT_TDT_SERVER_H

#include <list>
#include "tdt/status.h"

namespace tdt {
/**
* @ingroup TDTServerInit
* @brief Initialization functions, establish TDT Server,
* provide services such as access services, initialization and tuning channels
*
* @par Function
* Initialization functions, establish TDT Server,
* provide services such as access services, initialization and tuning channels
*
* @param deviceID [IN] type #unsigned int. Physical device ID
* @param bindCoreList [IN] type #List<unsigned int> bindCoreList.
* device CPU core sequence, the maximum value of the core sequence should not
* exceed the total number of CPU cores
* @retval 0 Success
* @retval OtherValues 0 Fail
*
* @par Dependency
* @li libtdtserver.so: Library to which the interface belongs.
* @li tdt_server.h: Header file where the interface declaration is located.
*/
TDT_LIB_EXPORT int32_t TDTServerInit(const uint32_t deviceID, const std::list<uint32_t> &bindCoreList);

/**
* @ingroup TDTServerInit
* @brief End TDT Server
*
* @par Function
* End TDT Server
*
* @param NA
* @retval 0 Success
* @retval OtherValues 0 Fail
*
* @par Dependency
* @li libtdtserver.so: Library to which the interface belongs.
* @li tdt_server.h: Header file where the interface declaration is located.
*/
TDT_LIB_EXPORT int32_t TDTServerStop();

class TdtServer {
 public:
 private:
  /**
  * @ingroup TdtServer
  * @brief TdtServer is a static class, all delete constructs and destructors
  */
  TdtServer() = delete;

  /**
  * @ingroup TdtServer
  * @brief TdtServer destructor
  */
  virtual ~TdtServer() = delete;
  TdtServer(const TdtServer &) = delete;
  TdtServer(TdtServer &&) = delete;
  TdtServer &operator=(const TdtServer &) = delete;
  TdtServer &operator=(TdtServer &&) = delete;
};
};      // namespace tdt
#endif  // INC_TDT_TDT_SERVER_H
