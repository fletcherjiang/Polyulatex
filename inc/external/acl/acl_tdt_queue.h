/**
* @file acl_tdt_queue.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef INC_EXTERNAL_ACL_ACL_TDT_QUEUE_H_
#define INC_EXTERNAL_ACL_ACL_TDT_QUEUE_H_

#include "acl/acl_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagMemQueueAttr acltdtQueueAttr;
typedef void *acltdtBuf;
typedef struct acltdtQueueRouteList acltdtQueueRouteList;
typedef struct acltdtQueueRouteQueryInfo acltdtQueueRouteQueryInfo;
typedef struct acltdtQueueRoute acltdtQueueRoute;

#define ACL_TDTQUEUE_PERMISSION_MANAGER 1
#define ACL_TDTQUEUE_PERMISSION_READ 2
#define ACL_TDTQUEUE_PERMISSION_WRITE 4


enum acltdtQueueAttrType {
    ACL_QUEUE_NAME_PTR = 0,
    ACL_QUEUE_DEPTH_UINT32
};

enum acltdtQueueRouteKind {
    ACL_QUEUE_SRC_ID = 0,
    ACL_QUEUE_DST_ID = 1
};

enum acltdtQueueRouteQueryMode {
    ACL_QUEUE_ROUTE_QUERY_SRC = 0,
    ACL_QUEUE_ROUTE_QUERY_DST,
    ACL_QUEUE_ROUTE_QUERY_SRC_DST
};

enum acltdtQueueRouteQueryInfoParamType {
    ACL_QUEUE_ROUTE_QUERY_MODE_ENUM = 0,
    ACL_QUEUE_ROUTE_QUERY_SRC_ID_UINT32,
    ACL_QUEUE_ROUTE_QUERY_DST_ID_UINT32
};


ACL_FUNC_VISIBILITY aclError acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *qid);

ACL_FUNC_VISIBILITY aclError acltdtDestroyQueue(uint32_t qid);

ACL_FUNC_VISIBILITY aclError acltdtEnqueueBuf(uint32_t qid, acltdtBuf buf, int32_t timeout);

ACL_FUNC_VISIBILITY aclError acltdtDequeueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout);

ACL_FUNC_VISIBILITY aclError acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t permission, int32_t timeout);

ACL_FUNC_VISIBILITY aclError acltdtAttachQueue(uint32_t qid, int32_t timeout, uint32_t *permission);

ACL_FUNC_VISIBILITY aclError acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList);

ACL_FUNC_VISIBILITY aclError acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList);

ACL_FUNC_VISIBILITY aclError acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo,
                                                    acltdtQueueRouteList *qRouteList);

ACL_FUNC_VISIBILITY aclError acltdtAllocBuf(size_t size, acltdtBuf *buf);

ACL_FUNC_VISIBILITY aclError acltdtFreeBuf(acltdtBuf buf);

ACL_FUNC_VISIBILITY aclError acltdtGetBufData(const acltdtBuf buf, void **dataPtr, size_t *size);

ACL_FUNC_VISIBILITY aclError acltdtGetBufPrivData(const acltdtBuf buf, void **privBuf, size_t *size);

ACL_FUNC_VISIBILITY acltdtQueueAttr *acltdtCreateQueueAttr();

ACL_FUNC_VISIBILITY aclError acltdtDestroyQueueAttr(const acltdtQueueAttr *attr);

ACL_FUNC_VISIBILITY aclError acltdtSetQueueAttr(acltdtQueueAttr *attr,
                                                acltdtQueueAttrType type,
                                                size_t len,
                                                const void *param);

ACL_FUNC_VISIBILITY aclError acltdtGetQueueAttr(const acltdtQueueAttr *attr,
                                                acltdtQueueAttrType type,
                                                size_t len,
                                                size_t *paramRetSize,
                                                void *param);

ACL_FUNC_VISIBILITY acltdtQueueRoute* acltdtCreateQueueRoute(uint32_t srcQid, uint32_t dstQid);

ACL_FUNC_VISIBILITY aclError acltdtDestroyQueueRoute(const acltdtQueueRoute *route);

ACL_FUNC_VISIBILITY aclError acltdtGetqidFromQueueRoute(const acltdtQueueRoute *route,
                                                   acltdtQueueRouteKind srcDst,
                                                   uint32_t *qid);

ACL_FUNC_VISIBILITY aclError acltdtGetQueueRouteStatus(const acltdtQueueRoute *route, int32_t *routeStatus);

ACL_FUNC_VISIBILITY acltdtQueueRouteList* acltdtCreateQueueRouteList();

ACL_FUNC_VISIBILITY aclError acltdtDestroyQueueRouteList(const acltdtQueueRouteList *routeList);

ACL_FUNC_VISIBILITY aclError acltdtAddQueueRoute(acltdtQueueRouteList *routeList, const acltdtQueueRoute *route);

ACL_FUNC_VISIBILITY aclError acltdtGetQueueRoute(const acltdtQueueRouteList *routeList,
                                                 size_t index,
                                                 acltdtQueueRoute *route);

ACL_FUNC_VISIBILITY  acltdtQueueRouteQueryInfo* acltdtCreateQueueRouteQueryInfo();

ACL_FUNC_VISIBILITY aclError acltdtDestroyQueueRouteQueryInfo(const acltdtQueueRouteQueryInfo *param);

ACL_FUNC_VISIBILITY aclError acltdtSetQueueRouteQueryInfo(acltdtQueueRouteQueryInfo *param,
                                                          acltdtQueueRouteQueryInfoParamType type,
                                                          size_t len,
                                                          const void *value);


#ifdef __cplusplus
}
#endif

#endif //INC_EXTERNAL_ACL_ACL_TDT_QUEUE_H_