/**
* @file qs_client.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef BUFFER_QUEUE_SCHEDULE_CLIENT_H
#define BUFFER_QUEUE_SCHEDULE_CLIENT_H

#include <stdint.h>

namespace bqs {

const int32_t BQS_OK = 0;
const int32_t BQS_PARAM_INVALID = 1;
const int32_t BQS_INNER_ERROR = 2;
const int32_t BQS_DRIVER_ERROR = 3;


enum QueueSubEventType {
    ACL_BIND_QUEUE = 2049,
    ACL_BIND_QUEUE_INIT,
    ACL_UNBIND_QUEUE,
    ACL_QUERY_QUEUE,
    ACL_QUERY_QUEUE_NUM
};

enum QsQueryType {
    BQS_QUERY_TYPE_SRC,
    BQS_QUERY_TYPE_DST,
    BQS_QUERY_TYPE_SRC_AND_DST,
    BQS_QUERY_TYPE_SRC_OR_DST,
};

enum EventGroupId {
    ENQUEUEGRPID = 0,
    F2NFGRPID,
    BINDQUEUEGRPID
};

struct QsBindInit
{
    int32_t pid;
    uint32_t grpId;
    char rsv[32];
};

struct QueueRoute
{
    uint32_t srcId;
    uint32_t dstId;
    int32_t status;
    uint64_t comHandle;
    char rsv[32];
};

struct QueueRouteList
{
    uint32_t routeNum;
    uint64_t routeListAddr;
    char rsv[28];
};

struct QueueRouteQuery
{
    uint32_t queryType;
    uint32_t srcId;
    uint32_t dstId;
    uint32_t routeNum;
    uint64_t routeListAddr;
    char rsv[16];
};

struct QsProcMsgRsp
{
    uint32_t eventId;
    int32_t retCode;
    uint32_t retValue; // qid, queryNum
    char rsv[32];
};

};
#endif