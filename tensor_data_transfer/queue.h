/**
* @file queue.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ACL_QUEUE_H
#define ACL_QUEUE_H
#include <string.h>
#include <string>
#include <vector>

#include "acl/acl_base.h"
#include "acl/acl_tdt.h"
#include "acl/acl_rt.h"


#define GET_CURRENT_DEVICE_ID(devId) \
    do { \
        rtError_t rtRet = rtGetDevice(&devId); \
        if (rtRet != RT_ERROR_NONE) { \
            ACL_LOG_CALL_ERROR("[Get][DeviceId]fail to get deviceId result = %d", rtRet); \
            return rtRet; \
        } \
    } while (false)


enum RunEnv {
    ENV_UNKNOWN = -1,
    ENV_HOST = 0,
    ENV_DEVICE_DEFAULT = 1,
    ENV_DEVICE_MDC = 2,
};

struct acltdtBuf {
    acltdtBuf(void *buf) 
    {
        mbuf = buf;
    }
    ~acltdtBuf() = default;
    void *mbuf;
};

struct acltdtQueueRouteList {
    std::vector<acltdtQueueRoute> routeList;
};

struct acltdtQueueRouteQueryInfo {
    acltdtQueueRouteQueryMode mode;
    uint32_t srcId;
    uint32_t dstId;
};

struct acltdtQueueRoute {
    acltdtQueueRoute(uint32_t src, uint32_t dst) 
    {
        srcId = src;
        dstId = dst;
        status = 0;
    }
    ~acltdtQueueRoute() = default;
    uint32_t srcId;
    uint32_t dstId;
    int32_t status;
};

aclError GetRunningEnv(RunEnv &runEnv);


#endif //QUEUE_H