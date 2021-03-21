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
using QueueScheduleProcessorPtr = std::shared_ptr<QueueScheduleProcessor>;

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
    QueueProcessorPtr GetQueueProcessor(uint32_t qid);

    aclError AddQueueProcessor(uint32_t qid);

    void DeleteQueueProcessor(uint32_t qid);

    QueueScheduleProcessorPtr GetQueueScheduleProcessor();

    QueueManager() = default;

    ~QueueManager() = default;

    // not allow copy constructor and assignment operators
    QueueManager(const QueueManager &) = delete;

    QueueManager &operator=(const QueueManager &) = delete;

    QueueManager(QueueManager &&) = delete;

    QueueManager &&operator=(QueueManager &&) = delete;

private:
    // queue processor
    std::mutex mu;
    std::map<uint32_t, QueueProcessorPtr> queueProcessMap;
    QueueScheduleProcessorPtr queueScheduleProc = nullptr;
    RunEnv env_ = ENV_UNKNOWN;
};

}

#endif //QUEUE_MANAGER_H