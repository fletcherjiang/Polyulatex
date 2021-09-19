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

#ifndef CCE_RUNTIME_MEM_QUEUE_H
#define CCE_RUNTIME_MEM_QUEUE_H
#include <sys/types.h>
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

typedef struct tagMemQueueInfo
{
    int32_t id;
    int32_t size;
    uint32_t depth;
    int32_t status;
} rtMemQueueInfo_t;

typedef struct tagMemQueueAttr {
    char name[RT_MQ_MAX_NAME_LEN];
    uint32_t depth;
    uint32_t workMode;
    bool flowCtrlFlag;
    uint32_t flowCtrlDropTime;
    bool overWriteFlag;
} rtMemQueueAttr_t;

typedef struct tagMemQueueShareAttr
{
    int manage:1;
    int read:1;
    int write:1;
    int rsv:29;
}rtMemQueueShareAttr_t;

typedef struct tagMemQueueBuffInfo {
    void *addr;
    size_t len;
} rtMemQueueBuffInfo;

typedef struct tagMemQueueBuff {
    void *contextAddr;
    size_t contextLen;
    uint32_t buffCount;
    rtMemQueueBuffInfo *buffInfo;
} rtMemQueueBuff_t;

typedef enum tagMemQueueQueryCmd {
    RT_MQ_QUERY_QUE_ATTR_OF_CUR_PROC = 0,
    RT_MQ_QUERY_QUES_OF_CUR_PROC =1,
    RT_MQ_QUERY_CMD_MAX = 2
} rtMemQueueQueryCmd_t;

#define RT_MQ_EVENT_QS_MSG 27

#define RT_MQ_SCHED_PRIORITY_LEVEL0 0
#define RT_MQ_SCHED_PRIORITY_LEVEL1 1
#define RT_MQ_SCHED_PRIORITY_LEVEL2 2
#define RT_MQ_SCHED_PRIORITY_LEVEL3 3
#define RT_MQ_SCHED_PRIORITY_LEVEL4 4
#define RT_MQ_SCHED_PRIORITY_LEVEL5 5
#define RT_MQ_SCHED_PRIORITY_LEVEL6 6
#define RT_MQ_SCHED_PRIORITY_LEVEL7 7

#define RT_MQ_DST_ENGINE_ACPU_DEVICE 0
#define RT_MQ_DST_ENGINE_ACPU_HOST 1
#define RT_MQ_DST_ENGINE_CCPU_DEVICE 2
#define RT_MQ_DST_ENGINE_CCPU_HOST 3
#define RT_MQ_DST_ENGINE_DCPU_DEVICE 4

#define RT_MQ_SCHED_EVENT_QS_MSG 25

typedef struct tagEschedEventSummary {
    int32_t pid;
    uint32_t grpId;
    int32_t eventId;
    uint32_t subeventId;
    uint32_t msgLen;
    char *msg;
    uint32_t dstEngine;
    int32_t policy;
} rtEschedEventSummary_t;

typedef struct tagEschedEventReply {
    char *buf;
    uint32_t bufLen;
    uint32_t replyLen;
} rtEschedEventReply_t;

#define RT_DEV_PROCESS_CP1 0
#define RT_DEV_PROCESS_CP2 1
#define RT_DEV_PROCESS_DEV_ONLY 2
#define RT_DEV_PROCESS_QS 3
#define RT_DEV_PROCESS_SIGN_LENGTH 49

typedef struct tagBindHostpidInfo {
    int32_t hostPid;
    uint32_t vfid;
    uint32_t chipId;
    int32_t mode;
    int32_t cpType;
    uint32_t len;
    char sign[RT_DEV_PROCESS_SIGN_LENGTH];
} rtBindHostpidInfo_t;

#define RT_MEM_BUFF_MAX_CFG_NUM 64

typedef struct {
    uint32_t cfgId;
    uint32_t totalSize;
    uint32_t blkSize;
    uint32_t maxBufSize;
    uint32_t pageType;
    int32_t elasticEnable;
    int32_t elasticRate;
    int32_t elasticRateMax;
    int32_t elasticHighLevel;
    int32_t elasticLowLevel;
} rtMemZoneCfg_t;

typedef struct {
    rtMemZoneCfg_t cfg[RT_MEM_BUFF_MAX_CFG_NUM];
} rtMemBuffCfg_t;

typedef void *rtMbufPtr_t;

RTS_API rtError_t rtMemQueueInitQS(int32_t devId) WEAKFUC;

RTS_API rtError_t rtMemQueueCreate(int32_t devId, const rtMemQueueAttr_t *queAttr, uint32_t *qid) WEAKFUC;

RTS_API rtError_t rtMemQueueDestroy(int32_t devId, uint32_t qid) WEAKFUC;

RTS_API rtError_t rtMemQueueInit(int32_t devId) WEAKFUC;

RTS_API rtError_t rtMemQueueEnQueue(int32_t devId, uint32_t qid, void *mbuf) WEAKFUC;

RTS_API rtError_t rtMemQueueDeQueue(int32_t devId, uint32_t qid, void **mbuf) WEAKFUC;

RTS_API rtError_t rtMemQueuePeek(int32_t devId, uint32_t qid, size_t *bufLen, int32_t timeout) WEAKFUC;

RTS_API rtError_t rtMemQueueEnQueueBuff(int32_t devId, uint32_t qid, rtMemQueueBuff_t *inBuf, int32_t timeout) WEAKFUC;

RTS_API rtError_t rtMemQueueDeQueueBuff(int32_t devId, uint32_t qid, rtMemQueueBuff_t *outBuf, int32_t timeout) WEAKFUC;

RTS_API rtError_t rtMemQueueQuery(int32_t devId, rtMemQueueQueryCmd_t cmd, void *inBuff, uint32_t inLen,
                                  void *outBuff, uint32_t *outLen) WEAKFUC;

RTS_API rtError_t rtMemQueueQueryInfo(int32_t device, uint32_t qid, rtMemQueueInfo_t *queueInfo) WEAKFUC;

RTS_API rtError_t rtMemQueueGrant(int32_t devId, uint32_t qid, int32_t pid, rtMemQueueShareAttr_t *attr) WEAKFUC;

RTS_API rtError_t rtMemQueueAttach(int32_t devId, uint32_t qid, int32_t timeout) WEAKFUC;

RTS_API rtError_t rtEschedSubmitEventSync(int32_t devId, rtEschedEventSummary_t *event, rtEschedEventReply_t *ack) WEAKFUC;

RTS_API rtError_t rtQueryDevPid(rtBindHostpidInfo_t *info, int32_t *devPid) WEAKFUC;

RTS_API rtError_t rtMbufInit(rtMemBuffCfg_t *cfg) WEAKFUC;

RTS_API rtError_t rtMbufAlloc(rtMbufPtr_t *mbuf, uint64_t size) WEAKFUC;

RTS_API rtError_t rtMbufFree(rtMbufPtr_t mbuf) WEAKFUC;

RTS_API rtError_t rtMbufGetBuffAddr(rtMbufPtr_t mbuf, void **databuf) WEAKFUC;

RTS_API rtError_t rtMbufGetBuffSize(rtMbufPtr_t mbuf, uint64_t *size) WEAKFUC;

RTS_API rtError_t rtMbufGetPrivInfo(rtMbufPtr_t mbuf, void **priv, uint64_t *size) WEAKFUC;

typedef struct {
    uint64_t maxMemSize;
} rtMemGrpConfig_t;

typedef struct {
    int32_t admin:1;
    int32_t read:1;
    int32_t write:1;
    int32_t alloc:1;
    int32_t rsv:28;
} rtMemGrpShareAttr_t;

#define RT_MEM_GRP_QUERY_GROUPS_OF_PROCESS 1

typedef struct {
    int pid;
} rtMemGrpQueryByProc_t;

typedef union {
    rtMemGrpQueryByProc_t grpQueryByProc;
} rtMemGrpQueryInput_t;

#define RT_MEM_GRP_NAME_LEN 32

typedef struct {
    char groupName[RT_MEM_GRP_NAME_LEN];
    rtMemGrpShareAttr_t attr;
} rtMemGrpOfProc_t;

typedef struct {
    rtMemGrpOfProc_t *groupsOfProc;
    size_t maxNum;
    size_t resultNum;
} rtMemGrpQueryOutput_t;

RTS_API rtError_t rtMemGrpCreate(const char *name, const rtMemGrpConfig_t *cfg) WEAKFUC;

RTS_API rtError_t rtMemGrpAddProc(const char *name, int32_t pid, const rtMemGrpShareAttr_t *attr) WEAKFUC;

RTS_API rtError_t rtMemGrpAttach(const char *name, int32_t timeout) WEAKFUC;

RTS_API rtError_t rtMemGrpQuery(int32_t cmd, const rtMemGrpQueryInput_t *input, rtMemGrpQueryOutput_t *output) WEAKFUC;

#if defined(__cplusplus) && !defined(COMPILE_OMG_PACKAGE)
}
#endif
#endif // CCE_RUNTIME_MEM_QUEUE_H