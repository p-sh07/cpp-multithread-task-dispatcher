#pragma once
#include "queue/queue.hpp"

#include <mutex>
#include <thread>
#include <semaphore>
#include <queue>

namespace dispatcher::queue {
//TODO: Least max value == numeric limits max (by default) / Provide at least for the number of cores?
static constexpr int least_max_capacity_ = 1; //Allow min 1 cap

class BoundedQueue : public IQueue {
public:
    explicit BoundedQueue(int capacity);
    void push(std::function<void()> task) override;
    std::optional<std::function<void()>> try_pop() override;
    bool not_full() const override;
    bool empty() const override;;

    ~BoundedQueue() override;

private:
    std::mutex mtx_;
    const int capacity_;
    std::queue<std::function<void()>> tasks_;

    // std::counting_semaphore<least_max_capacity_> slots_;


};

}  // namespace dispatcher::queue