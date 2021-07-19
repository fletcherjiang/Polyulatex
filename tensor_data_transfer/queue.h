/**
* @file queue.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ACL_QUEUE_H
#define ACL_QUEUE_H

#include <vector>
#include "acl/acl_base.h"
#include "acl/acl_tdt_queue.h"
#include "acl/acl_rt.h"
#include "runtime/rt_mem_queue.h"

struct acltdtQueueRouteList {
    std::vector<acltdtQueueRoute> routeList;
};

struct acltdtQueueRouteQueryInfo {
    int32_t mode;
    uint32_t srcId;
    uint32_t dstId;
    bool isConfigMode;
    bool isConfigSrc;
    bool isConfigDst;
};

struct acltdtQueueRoute {
    uint32_t srcId;
    uint32_t dstId;
    int32_t status;
};
#endif // ACL_QUEUE_H