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

#define WEAKFUC __attribute__((weak))
#define RT_MQ_MAX_NAME_LEN 128
#define RT_MQ_DEPTH_MIN 2
#define RT_MQ_MODE_PUSH 1
#define RT_MQ_MODE_PULL 2
#define RT_MQ_MODE_DEFAULT RT_MQ_MODE_PUSH





typedef struct tagMemQueueAttr {
    char name[RT_MQ_MAX_NAME_LEN];
    uint32_t depth;
    uint32_t workMode;
    bool flowCtrlFlag;
    uint32_t flowCtrlDropTime;
    bool overWriteFlag;
} rtMemQueueAttr_t;

/**
 * @ingroup rt_model
 * @brief tell runtime Model has been Loaded
 * @param [in] model   model to execute
 * @return RT_ERROR_NONE for ok
 */

RTS_API rtError_t rtMemQueueInit(int32_t devId) WEAKFUC;

RTS_API rtError_t rtMemQueueCreate(int32_t devId, const rtMemQueueAttr_t *queAttr, uint32_t *qid) WEAKFUC;

RTS_API rtError_t rtMemQueueDestroy(int32_t devId, uint32_t qid) WEAKFUC;

RTS_API rtError_t rtMemQueueEnqueue(int32_t devId, uint32_t qid, void *mbuf) WEAKFUC;

RTS_API rtError_t rtMemQueueDequeue(int32_t devId, uint32_t qid, void **mbuf) WEAKFUC;

RTS_API rtError_t rtMemQueueAllocMbuf(size_t size, void **buf) WEAKFUC;

RTS_API rtError_t rtMemQueueFreeMbuf(void *buf) WEAKFUC;

RTS_API rtError_t rtMemQueueGetMbufAddr(void *buf, void **databuf) WEAKFUC;

RTS_API rtError_t rtMemQueueGetMbufSize(void *buf, size_t *size) WEAKFUC;

RTS_API rtError_t rtMemQueueGetPrivInfo(void *buf, void **privBuf, size_t *size) WEAKFUC;

#if defined(__cplusplus) && !defined(COMPILE_OMG_PACKAGE)
}
#endif


#endif // 