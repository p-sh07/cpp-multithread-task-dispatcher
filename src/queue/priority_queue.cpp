#include "queue/priority_queue.hpp"

namespace dispatcher::queue {
PriorityQueue::PriorityQueue(const PriorityOptionsMap& options_map) {
    for(const auto& [priority, options] : options_map) {
        std::unique_ptr<IQueue> q_ptr = nullptr;
        if(options.bounded) {
            q_ptr = std::make_unique<BoundedQueue>(*options.capacity);
        } else {
            q_ptr = std::make_unique<UnboundedQueue>();
        }
        auto lk = std::unique_lock(queue_mutex_);
        priority_queues_.try_emplace(priority, std::move(q_ptr)); //push under lock
    }
}

void PriorityQueue::push(TaskPriority priority, std::function<void()> task) {
    auto lk = std::unique_lock(queue_mutex_);
    auto& queue = priority_queues_.at(priority);

    //For high priority task the queue is bounded, so have to wait for not_full
    if(priority == TaskPriority::High) {
        hpq_not_full_.wait(lk, [this, &queue] {
            return !queue->not_full() || shutdown_active_;
        });
    }

    //Stop accepting pushes on shutdown
    if(shutdown_active_) {
        return;
    }

    queue->push(std::move(task));
    not_empty_.notify_one(); //wake sleeping thread??
}

std::optional<std::function<void()>> PriorityQueue::pop() {
    auto lk = std::unique_lock(queue_mutex_);

    //wait until at least one of the qs is not empty
    not_empty_.wait(lk, [this] {
        return !Empty() || shutdown_active_;
    });

    //Return HP task if not empty
    if(auto high_priority_task = priority_queues_.at(TaskPriority::High)->try_pop()) {
        hpq_not_full_.notify_one();
        return high_priority_task;
    }

    //Else return NP task [or nullopt, _only_ when shutdown is active]
    return priority_queues_.at(TaskPriority::Normal)->try_pop();
}

void PriorityQueue::shutdown() {
    shutdown_active_ = true;

    not_empty_.notify_all();
    hpq_not_full_.notify_all();
}

PriorityQueue::~PriorityQueue() {

}


bool PriorityQueue::Empty() {
    auto lk = std::unique_lock(queue_mutex_);
    return std::ranges::all_of(priority_queues_ | vw::values, &IQueue::empty);
}
} // namespace dispatcher::queue
