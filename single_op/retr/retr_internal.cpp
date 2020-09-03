/**
 * @file retr_internal.cpp
 *
 * Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "retr_internal.h"
#include "acl/acl_rt.h"
#include "common/log_inner.h"

namespace acl {
namespace retr {
/**
 * create notify and generate notifyid
 * @return ACL_SUCCESS:success other:failed
 */
aclError aclCreateNotify(rtNotify_t &notify, uint32_t &notifyId, aclrtStream &stream)
{
    ACL_LOG_INFO("aclCreateNotify start.");
    // create notify event
    rtEvent_t event = nullptr;
    auto rtRet = rtEventCreateWithFlag(&event, RT_EVENT_WITH_FLAG);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[Create][Notify]fail to create notify event, ret = %d", rtRet);
        return ACL_ERROR_RT_FAILURE;
    }
    notify = static_cast<rtNotify_t>(event);

    // get eventId for notify event
    uint32_t eventId = 0;
    rtRet = rtGetEventID(event, &eventId);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[Get][Notify]fail to get notify eventId, ret = %d", rtRet);
        (void)rtEventDestroy(event);
        return ACL_ERROR_RT_FAILURE;
    }
    notifyId = eventId;
    ACL_LOG_INFO("aclCreateNotify success, notifyId:%u", notifyId);

    // reset event
    rtRet = rtEventReset(event, stream);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_WARN("reset a event to stream failed, result = %d", rtRet);
        return ACL_SUCCESS;
    }

    return ACL_SUCCESS;
}

aclError aclFvNotifyWait(rtNotify_t &notify, aclrtStream &stream)
{
    ACL_LOG_INFO("aclFvNotifyWait start.");
    rtEvent_t event = static_cast<rtEvent_t>(notify);
    auto rtRet = rtStreamWaitEvent(stream, event);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[Wait][Event]wait for a event to stream failed, result = %d", rtRet);
        return ACL_ERROR_RT_FAILURE;
    }
    rtRet = rtEventReset(event, stream);
    if (rtRet != RT_ERROR_NONE) {
        ACL_LOG_CALL_ERROR("[Reset][Event]reset a event to stream failed, result = %d", rtRet);
        return ACL_ERROR_RT_FAILURE;
    }
    ACL_LOG_INFO("aclFvNotifyWait success.");
    return ACL_SUCCESS;
}

/**
 * check result of task, 0 represent success
 * @return ACL_SUCCESS:success other:failed
 */
aclError aclCheckResult(const void *retCode)
{
    ACL_LOG_INFO("aclCheckResult start.");
    ACL_REQUIRES_NOT_NULL_WITH_INPUT_REPORT(retCode);
    int32_t result = 0;
    aclrtRunMode runMode;
    aclError ret = aclrtGetRunMode(&runMode);
    if (ret != ACL_SUCCESS) {
        return ret;
    }
    if (runMode == ACL_HOST) {
        ret = aclrtMemcpy(&result, sizeof(int32_t), retCode, sizeof(int32_t), ACL_MEMCPY_DEVICE_TO_HOST);
        if (ret != ACL_SUCCESS) {
            ACL_LOG_INNER_ERROR("[Copy][Mem]memcpy retCode to host failed, result = %d", ret);
            return ret;
        }
    } else {
        result = *(reinterpret_cast<const int32_t *>(retCode));
    }

    if (result != 0) {
        ACL_LOG_INNER_ERROR("[Check][Result]execute aclCheckResult failed, result = %d", result);
        return ACL_ERROR_FAILURE;
    }
    ACL_LOG_INFO("aclCheckResult success.");
    return ACL_SUCCESS;
}
}  // namespace retr
}  // namespace acl