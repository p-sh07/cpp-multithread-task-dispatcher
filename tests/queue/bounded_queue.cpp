#include <gtest/gtest.h>

#include "queue/bounded_queue.hpp"

using dispatcher::queue::BoundedQueue;

static constexpr size_t CAPACITY = 1000;

TEST(BoundedQueueTest, ConstructorWithMinCapacity) {
    const BoundedQueue queue(1);

    EXPECT_TRUE(queue.empty());
    EXPECT_TRUE(queue.not_full());
}

TEST(BoundedQueueTest, PushOneTask) {
    BoundedQueue queue(CAPACITY);

    queue.push([]{});

    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(queue.not_full());
}

TEST(BoundedQueueTest, FillQueueToCapacity) {
    BoundedQueue queue(CAPACITY);

    for (int i = 0; i < CAPACITY; ++i) {
        queue.push([i]{ /* task */ });
    }

    EXPECT_FALSE(queue.empty());
    EXPECT_FALSE(queue.not_full());
}

TEST(BoundedQueueTest, PushToFullQueueBlocks) {
    BoundedQueue queue(1);

    queue.push([]{});

    EXPECT_FALSE(queue.not_full());

    //TODO: time-out push?
}

TEST(BoundedQueueTest, PopFromNonEmptyQueue) {
    BoundedQueue queue(2);
    std::function<void()> task = []{};


    queue.push(task);

    auto popped = queue.try_pop();
    ASSERT_TRUE(popped.has_value());
    EXPECT_EQ(popped.value().target_type(), task.target_type());
}

TEST(BoundedQueueTest, PopFromEmptyQueue) {
    BoundedQueue queue(2);

    auto result = queue.try_pop();
    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(queue.empty());
}

TEST(BoundedQueueTest, FIFOSemantics) {
    BoundedQueue queue(3);
    std::vector<int> executionOrder;

    queue.push([&executionOrder]{ executionOrder.push_back(1); });
    queue.push([&executionOrder]{ executionOrder.push_back(2); });
    queue.push([&executionOrder]{ executionOrder.push_back(3); });

    while (auto task = queue.try_pop()) {
        task.value()();
    }

    ASSERT_EQ(executionOrder.size(), 3);
    EXPECT_EQ(executionOrder[0], 1);
    EXPECT_EQ(executionOrder[1], 2);
    EXPECT_EQ(executionOrder[2], 3);
}