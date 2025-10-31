#pragma once

#include "queue/priority_queue.hpp"

#include <functional>
#include <mutex>
#include <thread>

namespace dispatcher::thread_pool {
using queue::PriorityQueue;

class ThreadPool {
public:
    explicit ThreadPool(std::shared_ptr<PriorityQueue> queue, size_t num_threads);

    ~ThreadPool();

private:
    void Worker() const;

    std::shared_ptr<PriorityQueue> task_queue_;
    std::vector<std::jthread> workers_;
};

} // namespace dispatcher::thread_pool
