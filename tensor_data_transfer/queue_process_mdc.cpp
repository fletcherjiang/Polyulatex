/**
* @file queue_process_host.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "queue_process_mdc.h"
#include "log_inner.h"
#include "runtime/rt_mem_queue.h"
#include "runtime/dev.h"
#include "aicpu/queue_schedule/qs_client.h"

namespace acl {

    aclError QueueProcessorMdc::acltdtDestroyQueue(uint32_t qid)
    {
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtEnqueueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout)
    {
        ACL_REQUIRES_NOT_NULL(buf);
        int32_t deviceId = 0;
        std::lock_guard<std::mutex> lock(muForQueueEnqueue);
        rtError_t rtRet = rtMemQueueEnQueue(deviceId, qid, buf->mbuf);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Enqueue][Queue]fail to enqueue result = %d", rtRet);
            return rtRet;
        }
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtDequeueBuf(uint32_t qid, acltdtBuf *buf, int32_t timeout)
    {
        ACL_REQUIRES_NOT_NULL(buf);
        int32_t deviceId = 0;
        std::lock_guard<std::mutex> lock(muForQueueEnqueue);
        rtError_t rtRet = rtMemQueueDeQueue(deviceId, qid, &buf->mbuf);
        if (rtRet != RT_ERROR_NONE) {
            ACL_LOG_CALL_ERROR("[Dequeue][Queue]fail to enqueue result = %d", rtRet);
            return rtRet;
        }
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtGrantQueue(uint32_t qid, int32_t pid, uint32_t permission, int32_t timeout)
    {
        int32_t deviceId = 0;
        uint64_t startTime = GetTimestamp();
        uint64_t endTime = 0;
        do {
            auto ret = rtMemQueueGrant(deviceId, qid, pid, permission);
            if (ret == RT_ERROR_NONE) {
                return ACL_SUCCESS;
            } else if (ret != 11111) {// 不需要重试?
                return ret;
            }
            endTime = GetTimestamp();
        } while ((endTime - startTime >= (timeout * 10000)));
        return ACL_SUCCESS;
    }

    aclError QueueProcessorMdc::acltdtAttachQueue(uint32_t qid, int32_t timeout, uint32_t *permission)
    {
        ACL_REQUIRES_NOT_NULL(permission);
        int32_t deviceId = 0;
        ACL_REQUIRES_CALL_RTS_OK(rtMemQueueAttach(deviceId, qid, timeout), rtMemQueueAttach);
        // 查询queue权限
        // 查询有cp则授权Q权限
        pid_t cpPid;
        rtBindHostpidInfo_t info = {0};
        info.hostPid = mmGetPid();
        info.cpType = RT_DEV_PROCESS_CP1;
        info.chipId = deviceId;
        // if (rtQueryDevpid(&info, &cpPid) == RT_ERROR_NONE) {

        // }
        return ACL_SUCCESS;
    }

    aclError QueueScheduleProcessorMdc::acltdtBindQueueRoutes(acltdtQueueRouteList *qRouteList)
    {
        ACL_REQUIRES_NOT_NULL(qRouteList);
        ACL_LOG_INFO("Start to acltdtBindQueueRoutes, queue route is %zu", qRouteList->routeList.size());

        return ACL_SUCCESS;
    }

    aclError QueueScheduleProcessorMdc::acltdtUnbindQueueRoutes(acltdtQueueRouteList *qRouteList)
    {
        return ACL_SUCCESS;
    }

    aclError QueueScheduleProcessorMdc::acltdtQueryQueueRoutes(const acltdtQueueRouteQueryInfo *queryInfo, acltdtQueueRouteList *qRouteList)
    {
        return ACL_SUCCESS;
    }

}