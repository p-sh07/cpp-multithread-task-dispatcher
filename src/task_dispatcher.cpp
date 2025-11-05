#include "task_dispatcher.hpp"

namespace dispatcher {
using queue::PriorityOptionsMap;
using queue::PriorityQueue;
using thread_pool::ThreadPool;

TaskDispatcher::TaskDispatcher(size_t thread_count, const PriorityOptionsMap& queue_config)
    : priority_queue_(std::make_shared<PriorityQueue>(queue_config))
    , thread_pool_(std::make_unique<ThreadPool>(thread_count, priority_queue_)) {}

TaskDispatcher::~TaskDispatcher() {}

void TaskDispatcher::schedule(TaskPriority priority, std::function<void()> task) {
    priority_queue_->push(priority, task);
}
} // namespace dispatcher