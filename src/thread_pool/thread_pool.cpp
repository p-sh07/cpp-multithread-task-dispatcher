#include "thread_pool/thread_pool.hpp"

namespace dispatcher::thread_pool {

ThreadPool::ThreadPool(size_t num_threads, std::shared_ptr<PriorityQueue> queue)
    : task_queue_(std::move(queue))
{
    workers_.reserve(num_threads);
    generate_n(
        std::back_inserter(workers_),
        num_threads,
        [this] { return std::jthread(&ThreadPool::Worker, this); }
    );
    // for(auto i : vw::iota(0u, num_threads)) {
    //     workers_.emplace_back(&ThreadPool::Worker, this);
    // }
}

ThreadPool::~ThreadPool() {
    task_queue_->shutdown();

    for(auto& worker : workers_) {
        if(worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::Worker() const {
    //Do tasks while not std::nullopt from Q
    while(auto task = task_queue_->pop()) {
        (*task)(); //perform task
    }
}
} // namespace dispatcher::thread_pool