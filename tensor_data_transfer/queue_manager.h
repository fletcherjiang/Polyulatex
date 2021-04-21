/**
* @file queue_manager.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
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
using QueueProcessorPtr = std::shared_ptr<QueueProcessor>;

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
    QueueProcessorPtr GetQueueProcessor();

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
    RunEnv env_ = ENV_UNKNOWN;
};

}

#endif // QUEUE_MANAGER_H