#pragma once

#include "queue/priority_queue.hpp"

#include <functional>
#include <mutex>
#include <thread>

namespace dispatcher::thread_pool {
using queue::PriorityQueue;

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads, std::shared_ptr<PriorityQueue> queue);

    ~ThreadPool();

private:
    void Worker() const;

    std::shared_ptr<PriorityQueue> task_queue_;
    std::vector<std::jthread> workers_;
};

} // namespace dispatcher::thread_pool
