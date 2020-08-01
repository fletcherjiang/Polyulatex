/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description:
 * Author: huawei
 * Create: 2020-8-27
 */
#ifndef __ASCEND_HAL_DEFINE_H__
#define __ASCEND_HAL_DEFINE_H__

#include "ascend_hal_error.h"

/*========================== Queue Manage ===========================*/
#define DRV_ERROR_QUEUE_INNER_ERROR  DRV_ERROR_INNER_ERR             /**< queue error code */
#define DRV_ERROR_QUEUE_PARA_ERROR  DRV_ERROR_PARA_ERROR
#define DRV_ERROR_QUEUE_OUT_OF_MEM  DRV_ERROR_OUT_OF_MEMORY
#define DRV_ERROR_QUEUE_NOT_INIT  DRV_ERROR_UNINIT
#define DRV_ERROR_QUEUE_OUT_OF_SIZE  DRV_ERROR_OVER_LIMIT
#define DRV_ERROR_QUEUE_REPEEATED_INIT  DRV_ERROR_REPEATED_INIT
#define DRV_ERROR_QUEUE_IOCTL_FAIL  DRV_ERROR_IOCRL_FAIL
#define DRV_ERROR_QUEUE_NOT_CREATED  DRV_ERROR_NOT_EXIST
#define DRV_ERROR_QUEUE_RE_SUBSCRIBED  DRV_ERROR_REPEATED_SUBSCRIBED
#define DRV_ERROR_QUEUE_MULIPLE_ENTRY  DRV_ERROR_BUSY
#define DRV_ERROR_QUEUE_NULL_POINTER  DRV_ERROR_INVALID_HANDLE

/*=========================== Event Sched ===========================*/
#define EVENT_MAX_MSG_LEN  128  /* Maximum message length */
#define DRV_ERROR_SCHED_INNER_ERR  DRV_ERROR_INNER_ERR                   /**< event sched add error*/
#define DRV_ERROR_SCHED_PARA_ERR DRV_ERROR_PARA_ERROR
#define DRV_ERROR_SCHED_OUT_OF_MEM  DRV_ERROR_OUT_OF_MEMORY
#define DRV_ERROR_SCHED_UNINIT  DRV_ERROR_UNINIT
#define DRV_ERROR_SCHED_NO_PROCESS DRV_ERROR_NO_PROCESS
#define DRV_ERROR_SCHED_PROCESS_EXIT DRV_ERROR_PROCESS_EXIT
#define DRV_ERROR_SCHED_NO_SUBSCRIBE_THREAD DRV_ERROR_NO_SUBSCRIBE_THREAD
#define DRV_ERROR_SCHED_NON_SCHED_GRP_MUL_THREAD DRV_ERROR_NON_SCHED_GRP_MUL_THREAD
#define DRV_ERROR_SCHED_GRP_INVALID DRV_ERROR_NO_GROUP
#define DRV_ERROR_SCHED_PUBLISH_QUE_FULL DRV_ERROR_QUEUE_FULL
#define DRV_ERROR_SCHED_NO_GRP DRV_ERROR_NO_GROUP
#define DRV_ERROR_SCHED_GRP_EXIT DRV_ERROR_GROUP_EXIST
#define DRV_ERROR_SCHED_THREAD_EXCEEDS_SPEC DRV_ERROR_THREAD_EXCEEDS_SPEC
#define DRV_ERROR_SCHED_RUN_IN_ILLEGAL_CPU DRV_ERROR_RUN_IN_ILLEGAL_CPU
#define DRV_ERROR_SCHED_WAIT_TIMEOUT  DRV_ERROR_WAIT_TIMEOUT
#define DRV_ERROR_SCHED_WAIT_FAILED   DRV_ERROR_INNER_ERR
#define DRV_ERROR_SCHED_WAIT_INTERRUPT DRV_ERROR_WAIT_INTERRUPT
#define DRV_ERROR_SCHED_THREAD_NOT_RUNNIG DRV_ERROR_THREAD_NOT_RUNNIG
#define DRV_ERROR_SCHED_PROCESS_NOT_MATCH DRV_ERROR_PROCESS_NOT_MATCH
#define DRV_ERROR_SCHED_EVENT_NOT_MATCH DRV_ERROR_EVENT_NOT_MATCH
#define DRV_ERROR_SCHED_PROCESS_REPEAT_ADD DRV_ERROR_PROCESS_REPEAT_ADD
#define DRV_ERROR_SCHED_GRP_NON_SCHED DRV_ERROR_GROUP_NON_SCHED
#define DRV_ERROR_SCHED_NO_EVENT DRV_ERROR_NO_EVENT
#define DRV_ERROR_SCHED_COPY_USER DRV_ERROR_COPY_USER_FAIL
#define DRV_ERROR_SCHED_SUBSCRIBE_THREAD_TIMEOUT DRV_ERROR_SUBSCRIBE_THREAD_TIMEOUT
typedef enum group_type {
    /* Bound to a AICPU, multiple threads can be woken up simultaneously within a group */
    GRP_TYPE_BIND_DP_CPU = 1,
    GRP_TYPE_BIND_CP_CPU,             /* Bind to the control CPU */
    GRP_TYPE_BIND_DP_CPU_EXCLUSIVE    /* Bound to a AICPU, intra-group threads are mutex awakened */
} GROUP_TYPE;

/* Events can be released between different systems. This parameter specifies the destination type of events
   to be released. The destination type is defined based on the CPU type of the destination system. */
typedef enum schedule_dst_engine {
    ACPU_DEVICE = 0,
    ACPU_HOST = 1,
    CCPU_DEVICE = 2,
    CCPU_HOST = 3,
    DCPU_DEVICE = 4,
    TS_CPU = 5,
    DVPP_CPU = 6,
    DST_ENGINE_MAX,
} SCHEDULE_DST_ENGINE;

/* When the destination engine is AICPU, select a policy.
   ONLY: The command is executed only on the local AICPU.
   FIRST: The local AICPU is preferentially executed. If the local AICPU is busy, the remote AICPU can be used. */
typedef enum schedule_policy {
    ONLY = 0,
    FIRST = 1,
    POLICY_MAX,
} SCHEDULE_POLICY;

typedef enum event_id {
    EVENT_RANDOM_KERNEL,      /* Random operator event */
    EVENT_DVPP_MSG,           /* operator events commited by DVPP */
    EVENT_FR_MSG,             /* operator events commited by Feature retrieves */
    EVENT_TS_HWTS_KERNEL,     /* operator events commited by ts/hwts */
    EVENT_AICPU_MSG,          /* aicpu activates its own stream events */
    EVENT_TS_CTRL_MSG,        /* controls message events of TS */
    EVENT_QUEUE_ENQUEUE,      /* entry event of Queue(consumer) */
    EVENT_QUEUE_FULL_TO_NOT_FULL,   /* full to non-full events of Queue(producers) */
    EVENT_QUEUE_EMPTY_TO_NOT_EMPTY,   /* empty to non-empty event of Queue(consumer) */
    EVENT_TDT_ENQUEUE,        /* data entry event of TDT */
    EVENT_TIMER,              /* ros timer */
    EVENT_HCFI_SCHED_MSG,     /* scheduling events of HCFI */
    EVENT_HCFI_EXEC_MSG,      /* performs the event of HCFI */
    EVENT_ROS_MSG_LEVEL0,
    EVENT_ROS_MSG_LEVEL1,
    EVENT_ROS_MSG_LEVEL2,
    EVENT_ACPU_MSG_TYPE0,
    EVENT_ACPU_MSG_TYPE1,
    EVENT_ACPU_MSG_TYPE2,
    EVENT_CCPU_CTRL_MSG,
    EVENT_SPLIT_KERNEL,
    EVENT_DVPP_MPI_MSG,
    EVENT_CDQ_MSG,
    /* Add a new event here */
    EVENT_TEST,               /* Reserve for test */
    EVENT_MAX_NUM
} EVENT_ID;

typedef enum schedule_priority {
    PRIORITY_LEVEL0,
    PRIORITY_LEVEL1,
    PRIORITY_LEVEL2,
    PRIORITY_LEVEL3,
    PRIORITY_LEVEL4,
    PRIORITY_LEVEL5,
    PRIORITY_LEVEL6,
    PRIORITY_LEVEL7,
    PRIORITY_MAX,
} SCHEDULE_PRIORITY;

/*=========================== Queue Manage ===========================*/

typedef enum QueEventCmd {
    QUE_PAUSE_EVENT = 1,    /* pause enque event publish in group */
    QUE_RESUME_EVENT        /* resume enque event publish */
} QUE_EVENT_CMD;

/*=========================== Buffer Manage ===========================*/

#define UNI_ALIGN_MAX       4096
#define UNI_ALIGN_MIN       32
#define BUFF_POOL_NAME_LEN 128
#define BUFF_RESERVE_LEN 8

typedef enum group_id_type
{
    GROUP_ID_CREATE,
    GROUP_ID_ADD
}GROUP_ID_TYPE;

enum BuffConfCmdType {
    BUFF_CONF_MBUF_TIMEOUT_CHECK = 0,
    BUFF_CONF_MEMZONE_BUFF_CFG = 1,
    BUFF_CONF_MAX,
};

enum BuffGetCmdType {
    BUFF_GET_MBUF_TIMEOUT_INFO = 0,
    BUFF_GET_MBUF_USE_INFO = 1,
    BUFF_GET_MAX,
};

struct MbufTimeoutCheckPara {
    unsigned int enableFlag;     /* enable: 1; disable: 0 */
    unsigned int maxRecordNum;   /* maximum number timeout mbuf info recored */
    unsigned int timeout;        /* mbuf timeout value,  unit:ms, minimum 10ms, default 1000 ms */
    unsigned int checkPeriod;    /* mbuf check thread work period, uinit:ms, minimum 1000ms, default:1s */
};

struct MbufDataInfo {
    void *mp;
    int owner;
    unsigned int ref;
    unsigned int blkSize;
    unsigned int totalBlkNum;
    unsigned int availableNum;
    unsigned int allocFailCnt;
};

struct MbufDebugInfo {
    unsigned long long timeStamp;
    void *mbuf;
    int usePid;
    int allocPid;
    unsigned int useTime;
    unsigned int round;
    struct MbufDataInfo dataInfo;
    char poolName[BUFF_POOL_NAME_LEN];
    int reserve[BUFF_RESERVE_LEN];
};

struct MbufUseInfo {
    int allocPid;                /* mbuf alloc pid */
    int usePid;                  /* mbuf use pid */
    unsigned int ref;              /* mbuf reference num */
    unsigned int status;           /* mbuf status, 1 means in use, not support other status currently */
    unsigned long long timestamp;  /* mbuf alloc timestamp, cpu tick */
    int reserve[BUFF_RESERVE_LEN]; /* for reserve */
};

#endif
