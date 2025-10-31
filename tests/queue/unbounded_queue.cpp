#include <gtest/gtest.h>

#include "queue/unbounded_queue.hpp"

#include <thread>
#include <vector>
#include <chrono>

using namespace dispatcher::queue;

// Вспомогательные константы
constexpr std::chrono::seconds kTimeout{5};

TEST(UnboundedQueueTest, Constructor) {
    UnboundedQueue queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_TRUE(queue.not_full());
}

TEST(UnboundedQueueTest, PushOneTask) {
    UnboundedQueue queue;

    queue.push([] {});

    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(queue.not_full());
}

TEST(UnboundedQueueTest, PushMultipleTasks) {
    UnboundedQueue queue;

    for(int i = 0; i < 5; ++i) {
        queue.push([i] {});
    }

    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(queue.not_full());
}

TEST(UnboundedQueueTest, TryPopFromNonEmpty) {
    UnboundedQueue queue;
    std::function<void()> expectedTask = [] {};

    queue.push(expectedTask);

    auto result = queue.try_pop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().target_type(), expectedTask.target_type());
}

TEST(UnboundedQueueTest, TryPopFromEmpty) {
    UnboundedQueue queue;

    auto result = queue.try_pop();
    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(queue.empty());
}

TEST(UnboundedQueueTest, FIFOSemantics) {
    UnboundedQueue queue;
    std::vector<int> executionOrder;

    for(int i = 1; i <= 3; ++i) {
        queue.push([&executionOrder, i] {
            executionOrder.push_back(i);
        });
    }

    while(auto task = queue.try_pop()) {
        task.value()();
    }

    ASSERT_EQ(executionOrder.size(), 3);
    EXPECT_EQ(executionOrder[0], 1);
    EXPECT_EQ(executionOrder[1], 2);
    EXPECT_EQ(executionOrder[2], 3);
}

TEST(UnboundedQueueTest, NotFullAlwaysTrue) {
    UnboundedQueue queue;

    queue.push([] {});
    queue.push([] {});

    EXPECT_TRUE(queue.not_full());
}

TEST(UnboundedQueueTest, EmptyReflectsState) {
    UnboundedQueue queue;

    EXPECT_TRUE(queue.empty());

    queue.push([] {
    });
    EXPECT_FALSE(queue.empty());

    queue.try_pop();
    EXPECT_TRUE(queue.empty());
}

TEST(UnboundedQueueTest, RepeatedPushPop) {
    UnboundedQueue queue;

    for(int i = 0; i < 10; ++i) {
        queue.push([i] {
            /* task */
        });
        auto task = queue.try_pop();
        ASSERT_TRUE(task.has_value());
    }

    EXPECT_TRUE(queue.empty());
}
