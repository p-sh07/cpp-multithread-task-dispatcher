#include "queue/unbounded_queue.hpp"

#include <functional>
#include <mutex>
#include <queue>
#include <semaphore>

namespace dispatcher::queue {

// здесь ваш код

UnboundedQueue::UnboundedQueue() {

}

void UnboundedQueue::push(std::function<void()> task) {

}

std::optional<std::function<void()>> UnboundedQueue::try_pop() {

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