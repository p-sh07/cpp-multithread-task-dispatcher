#pragma once
#include <algorithm>

#include "queue/bounded_queue.hpp"
#include "queue/unbounded_queue.hpp"
#include "types.hpp"

#include <atomic>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <stdexcept>

namespace dispatcher::queue {

using PriorityOptionsMap = std::map<TaskPriority, QueueOptions>;

class PriorityQueue {
public:
    explicit PriorityQueue(const PriorityOptionsMap& options_map);
    void push(TaskPriority priority, std::function<void()> task);

    // block on pop until shutdown is called after that return std::nullopt on empty queue
    std::optional<std::function<void()>> pop();

    bool empty();
    void shutdown();

    ~PriorityQueue();
private:
    std::mutex queue_mutex_;
    std::condition_variable not_empty_;    //at least one task in either queue
    std::condition_variable hpq_not_full_; //high-priority queue is not full
    std::atomic<bool> shutdown_active_ = false;

    std::map<TaskPriority, std::unique_ptr<IQueue>> priority_queues_;
};

}  // namespace dispatcher::queue