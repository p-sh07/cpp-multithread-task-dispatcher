#include "queue/bounded_queue.hpp"

namespace dispatcher::queue {
BoundedQueue::BoundedQueue(int capacity)
    : capacity_(capacity)
    // , slots_(capacity)
{}

//Returns true of pushed successfully, otherwise false - better than exception
void BoundedQueue::push(std::function<void()> task) {
    // if (slots_.try_acquire()) { - if used with slots
    auto lk = std::lock_guard(mtx_);
    if(tasks_.size() >= capacity_) {
        //should be controlled by Higher-level class, avoid push when full (use c_var)
        throw std::runtime_error("Queue is full");
    }
    tasks_.push(task);
}

std::optional<std::function<void()>> BoundedQueue::try_pop() {
    auto lk = std::lock_guard(mtx_);
    if (tasks_.empty()) {
        return std::nullopt;
    }
    auto top_task = std::move(tasks_.front());
    tasks_.pop();
    // slots_.release(); //free capacity slot

    return top_task;
}

bool BoundedQueue::not_full() const {
    return tasks_.size() < capacity_;
}

bool BoundedQueue::empty() const {
    return tasks_.empty();
}

BoundedQueue::~BoundedQueue() {}
} // namespace dispatcher::queue

