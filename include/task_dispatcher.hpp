#pragma once

#include <memory>

#include "queue/priority_queue.hpp"
#include "thread_pool/thread_pool.hpp"
#include "types.hpp"

namespace dispatcher {

class TaskDispatcher {
public:
    // Default config:
    static inline const std::map<TaskPriority, queue::QueueOptions> DefaultQueueConfig = {
        {TaskPriority::High, {true, 1000}},
        {TaskPriority::Normal, {false, std::nullopt}}
    };

    TaskDispatcher(size_t thread_count, const queue::PriorityOptionsMap& queue_config = DefaultQueueConfig);
    ~TaskDispatcher();

    void schedule(TaskPriority priority, std::function<void()> task);
    void complete_all_tasks() {
        thread_pool_.release();
    }

private:
    std::shared_ptr<queue::PriorityQueue> priority_queue_;
    std::unique_ptr<thread_pool::ThreadPool> thread_pool_;

};

}  // namespace dispatcher