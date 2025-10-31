#include "queue/unbounded_queue.hpp"

#include <functional>
#include <mutex>
#include <queue>
#include <semaphore>

namespace dispatcher::queue {

// здесь ваш код

UnboundedQueue::UnboundedQueue() {}

void UnboundedQueue::push(std::function<void()> task) {
    tasks_.push(task);
}

std::optional<std::function<void()>> UnboundedQueue::try_pop() {
    if (tasks_.empty()) {
        return std::nullopt;
    }
    auto top_task = std::move(tasks_.front());
    tasks_.pop();

    return top_task;
}

bool UnboundedQueue::not_full() const {
    return true;
}

bool UnboundedQueue::empty() const {
    return tasks_.empty();
}

UnboundedQueue::~UnboundedQueue() {

}
} // namespace dispatcher::queue