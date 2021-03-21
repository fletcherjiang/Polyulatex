/**
* @file queue_process.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "queue_process.h"
#include "log_inner.h"
#include "runtime/rt_mem_queue.h"
#include "runtime/dev.h"
#include "aicpu/queue_schedule/qs_client.h"

namespace acl {
    aclError QueueProcessor::acltdtCreateQueue(const acltdtQueueAttr *attr, uint32_t *qid)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtDestroyQueue(uint32_t qid)
    {
        std::lock_guard<std::recursive_mutex> lock(muForQueueCtrl);
        int32_t deviceId = 0;
        GET_CURRENT_DEVICE_ID(deviceId);
        // TODO 有绑定结果需要报错
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueDestroy(deviceId, qid), rtMemQueueDestroy);
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtEnqueueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtDequeueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t flag, int32_t)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessor::acltdtAttachQueue(uint32_t qid, int32_t timeout, uint32_t *flag)
    {
        return ACL_SUCCESS;
    }




    aclError QueueScheduleProcessor::acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList)
    {
        return ACL_SUCCESS;
    }

    aclError QueueScheduleProcessor::acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList)
    {
        return ACL_SUCCESS;
    }

    aclError QueueScheduleProcessor::acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo, acltdtQueueRouteList *qRouteList)
    {
        return ACL_SUCCESS;
    }

    // acltdtBuf* QueueProcessor::acltdtCreateBuf(size_t size)
    // {
    //     return nullptr;
    // }

    // aclError QueueProcessor::acltdtDestroyBuf(acltdtBuf *buf)
    // {
    //     return ACL_SUCCESS;
    // }

    // aclError QueueProcessor::acltdtGetBufData(const acltdtBuf *buf, void **dataPtr, size_t *size)
    // {
    //     return ACL_SUCCESS;
    // }

    // aclError QueueProcessor::acltdtGetBufPrivData(const acltdtBuf *buf, void **privBuf, size_t *size)
    // {
    //     return ACL_SUCCESS;
    // }

}