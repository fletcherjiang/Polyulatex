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


typedef void *acltdtBuf;
typedef struct tagMemQueueAttr acltdtQueueAttr;
typedef struct acltdtQueueRouteList acltdtQueueRouteList;
typedef struct acltdtQueueRouteQueryInfo acltdtQueueRouteQueryInfo;
typedef struct acltdtQueueRoute acltdtQueueRoute;

#define ACL_TDT_QUEUE_PERMISSION_MANAGE 1
#define ACL_TDT_QUEUE_PERMISSION_DEQUEUE 2
#define ACL_TDT_QUEUE_PERMISSION_ENQUEUE 4

enum acltdtQueueAttrType {
    ACL_QUEUE_NAME_PTR = 0,
    ACL_QUEUE_DEPTH_UINT32
};

enum acltdtQueueRouteParamType {
    ACL_TDT_QUEUE_ROUTE_SRC_UINT32 = 0,
    ACL_TDT_QUEUE_ROUTE_DST_UINT32,
    ACL_TDT_QUEUE_ROUTE_STATUS_INT32
};

enum acltdtQueueRouteQueryMode {
    ACL_TDT_QUEUE_ROUTE_QUERY_SRC = 0,
    ACL_TDT_QUEUE_ROUTE_QUERY_DST,
    ACL_TDT_QUEUE_ROUTE_QUERY_SRC_AND_DST
};

enum acltdtQueueRouteQueryInfoParamType {
    ACL_QUEUE_ROUTE_QUERY_MODE_ENUM = 0,
    ACL_QUEUE_ROUTE_QUERY_SRC_ID_UINT32,
    ACL_QUEUE_ROUTE_QUERY_DST_ID_UINT32
};


ACL_FUNC_VISIBILITY aclError acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *qid);

ACL_FUNC_VISIBILITY aclError acltdtDestroyQueue(uint32_t qid);

ACL_FUNC_VISIBILITY aclError acltdtEnqueue(uint32_t qid, acltdtBuf buf, int32_t timeout);

ACL_FUNC_VISIBILITY aclError acltdtDequeue(uint32_t qid, acltdtBuf *buf, int32_t timeout);

ACL_FUNC_VISIBILITY aclError acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t permission, int32_t timeout);

ACL_FUNC_VISIBILITY aclError acltdtAttachQueue(uint32_t qid, int32_t timeout, uint32_t *permission);

ACL_FUNC_VISIBILITY aclError acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList);

ACL_FUNC_VISIBILITY aclError acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList);

ACL_FUNC_VISIBILITY aclError acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo,
                                                    acltdtQueueRouteList *qRouteList);

ACL_FUNC_VISIBILITY aclError acltdtAllocBuf(size_t size, acltdtBuf *buf);

ACL_FUNC_VISIBILITY aclError acltdtFreeBuf(acltdtBuf buf);

ACL_FUNC_VISIBILITY aclError acltdtGetBufData(const acltdtBuf buf, void **dataPtr, size_t *size);

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

ACL_FUNC_VISIBILITY acltdtQueueRoute* acltdtCreateQueueRoute(uint32_t srcId, uint32_t dstId);

ACL_FUNC_VISIBILITY aclError acltdtDestroyQueueRoute(const acltdtQueueRoute *route);

ACL_FUNC_VISIBILITY aclError acltdtGetQueueRouteParam(const acltdtQueueRoute *route,
                                                      acltdtQueueRouteParamType type,
                                                      size_t len,
                                                      size_t *paramRetSize,
                                                      void *param);

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