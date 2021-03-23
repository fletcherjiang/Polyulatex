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

    QueueProcessorPtr QueueManager::GetQueueProcessor()
    {
        if (queueProcessProc_ != nullptr) {
            return queueProcessProc_;
        }
        try {
            switch(env_) {
                case ENV_HOST:
                    queueProcessProc_ = std::shared_ptr<QueueProcessorHost>(new (std::nothrow)QueueProcessorHost());
                    break;
                case ENV_DEVICE_MDC:
                    queueProcessProc_ = std::shared_ptr<QueueProcessorMdc>(new (std::nothrow)QueueProcessorMdc());
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
        return queueProcessProc_;
    }
}
