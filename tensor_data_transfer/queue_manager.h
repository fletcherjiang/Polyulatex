/**
* @file queue_manager.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef QUEUE_MANAGER_H
#define QUEUE_MANAGER_H

#include <mutex>
#include <memory>
#include <map>
#include "acl/acl_rt.h"
#include "queue_process.h"
#include "queue.h"

namespace acl {
using QueueProcessorPtr = std::unique_ptr<QueueProcessor>;

enum RunEnv {
    ACL_ACL_ENV_UNKNOWN = -1,
    ACL_ENV_HOST = 0,
    ACL_ENV_DEVICE_CCPU = 1,
    ACL_ENV_DEVICE_MDC = 2,
};

class QueueManager {
public:
    /**
     * Get queue manager instance
     * @return QueueManager reference
     */
    static QueueManager& GetInstance();

    /**
     * Get queue processor
     * @return queueProcessorPtr queue processor pointer
     */
    QueueProcessor *GetQueueProcessor();

    static aclError GetRunningEnv(RunEnv &runningEnv);

    QueueManager() = default;

    ~QueueManager() = default;

    // not allow copy constructor and assignment operators
    QueueManager(const QueueManager &) = delete;

    QueueManager &operator=(const QueueManager &) = delete;

    QueueManager(QueueManager &&) = delete;

    QueueManager &&operator=(QueueManager &&) = delete;

private:
    // queue processor
    QueueProcessorPtr queueProcessProc_ = nullptr;
    RunEnv env_ = ACL_ACL_ENV_UNKNOWN;
};

}

#endif // QUEUE_MANAGER_H