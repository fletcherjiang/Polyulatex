/**
* @file queue_manager.cpp
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "log_inner.h"
#include "queue.h"
#include "queue_manager.h"
#include "queue_process.h"
#include "queue_process_host.h"
#include "queue_process_mdc.h"

namespace acl {
    QueueManager& QueueManager::GetInstance()
    {
        static QueueManager instance;
        (void)GetRunningEnv(instance.env_);
        return instance;
    }

    QueueProcessorPtr QueueManager::GetQueueProcessor(uint32_t qid)
    {
        std::unique_lock<std::mutex> lock(mu);
        auto it = queueProcessMap.find(qid);
        if (it == queueProcessMap.end()) {
            ACL_LOG_INNER_ERROR("[Check][Runenv]can not find qid %u", qid);
            return nullptr;
        }
        return it->second;
    }

    aclError QueueManager::AddQueueProcessor(uint32_t qid)
    {
        if (env_ == ENV_UNKNOWN) {
            ACL_LOG_INNER_ERROR("[Check][Runenv]check runenv failed.");
            return ACL_ERROR_FAILURE;
        }
        std::unique_lock<std::mutex> lock(mu);
        if (queueProcessMap.find(qid) != queueProcessMap.end()) {
            return ACL_ERROR_FAILURE;
        }
        QueueProcessorPtr queueProcessor = nullptr;
         try {
            switch(env_) {
                case ENV_HOST:
                    queueProcessor = std::shared_ptr<QueueProcessorHost>(new (std::nothrow)QueueProcessorHost());
                    break;
                case ENV_DEVICE_MDC:
                    queueProcessor = std::shared_ptr<QueueProcessorMdc>(new (std::nothrow)QueueProcessorMdc());
                    break;
                case ENV_DEVICE_DEFAULT:
                    // TDOD
                    break;
                default:
                    ACL_LOG_INNER_ERROR("[Check][Runenv]check runenv failed.");
                    return ACL_ERROR_FAILURE;
            }
        } catch (...) {
            ACL_LOG_INNER_ERROR("[Define][Object]define object queue processor with unique_ptr failed.");
            return ACL_ERROR_BAD_ALLOC;
        }
        ACL_CHECK_MALLOC_RESULT(queueProcessor);
        queueProcessMap[qid] = queueProcessor;
        return ACL_SUCCESS;
    }

    void QueueManager::DeleteQueueProcessor(uint32_t qid)
    {
        QueueProcessorPtr tmpQueueProcessor = nullptr;
        {
            std::unique_lock<std::mutex> lock(mu);
            auto it = queueProcessMap.find(qid);
            if (it == queueProcessMap.end()) {
                ACL_LOG_INFO("can not find queue process, qid is %u", qid);
                return;
            }
            tmpQueueProcessor = it->second;
            queueProcessMap.erase(it);
        }
        return;
    }

    QueueScheduleProcessorPtr QueueManager::GetQueueScheduleProcessor()
    {
        if (queueScheduleProc != nullptr) {
            return queueScheduleProc;
        }
         try {
            switch(env_) {
                case ENV_HOST:
                    queueScheduleProc =
                        std::shared_ptr<QueueScheduleProcessorHost>(new (std::nothrow)QueueScheduleProcessorHost());
                    break;
                case ENV_DEVICE_MDC:
                    queueScheduleProc =
                        std::shared_ptr<QueueScheduleProcessorMdc>(new (std::nothrow)QueueScheduleProcessorMdc());
                    break;
                case ENV_DEVICE_DEFAULT:
                    // TDOD
                    break;
                default:
                    ACL_LOG_INNER_ERROR("[Check][Runenv]check runenv failed.");
                    return nullptr;
            }
        } catch (...) {
            ACL_LOG_INNER_ERROR("[Define][Object]define object queue processor with unique_ptr failed.");
            return nullptr;
        }
        return queueScheduleProc;
    }
}