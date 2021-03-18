/**
 * Copyright 2020 Huawei Technologies Co., Ltd

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#ifndef RT_MBUFF_QUEUE_H
#define RT_MBUFF_QUEUE_H
#include "base.h"

#if defined(__cplusplus) && !defined(COMPILE_OMG_PACKAGE)
extern "C" {
#endif

#define RT_MQ_MAX_NAME_LEN 128


typedef struct tagMQueueAttr {
    char name[RT_MQ_MAX_NAME_LEN];
    uint32_t depth;
    uint32_t workMode;
    bool flowCtrlFlag;
    uint32_t flowCtrlDropTime;
    bool overWriteFlag;
} rtMQueueAttr_t;

/**
 * @ingroup rt_model
 * @brief tell runtime Model has been Loaded
 * @param [in] model   model to execute
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtMqueueCreate(int32_t devId, const rtMQueueAttr_t *queAttr, uint32_t *qid);

RTS_API rtError_t rtMqueueDestroy(int32_t devId, uint32_t qid);



#if defined(__cplusplus) && !defined(COMPILE_OMG_PACKAGE)
}
#endif


#endif // 