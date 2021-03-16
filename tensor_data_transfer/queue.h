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

struct acltdtBuf {
    void *buf;
    uint32_t depth;
};

struct acltdtQueueRouteList {
    std::vector<acltdtQueueRoute> routeList;
};

struct acltdtQueueRouteQueryInfo {
    int32_t mode;
    uint32_t srcId;
    uint32_t dstId;
};

struct acltdtQueueRoute {
    uint32_t srcId;
    uint32_t dstId;
    int32_t status;
};


#endif //QUEUE_H