#pragma once
#include "queue/queue.hpp"
#include <stack>

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
    std::stack<std::function<void()>> tasks_;
};

}  // namespace dispatcher::queue