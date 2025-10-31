#pragma once
#include <queue>

#include "queue/queue.hpp"
#include <queue>
#include <mutex>

namespace dispatcher::queue {

class UnboundedQueue : public IQueue {
public:
    explicit UnboundedQueue();

    void push(std::function<void()> task) override;
    std::optional<std::function<void()>> try_pop() override;
    bool not_full() const override;
    bool empty() const override;

    ~UnboundedQueue() override;;

private:
    std::mutex mtx_;
    std::queue<std::function<void()>> tasks_;
};

}  // namespace dispatcher::queue