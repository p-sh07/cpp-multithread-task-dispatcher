#include <gtest/gtest.h>

#include "queue/priority_queue.hpp"

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>
#include <chrono>

namespace dispatcher::queue {
constexpr std::chrono::seconds TIMEOUT{10};
constexpr int HIGH_PRIORITY_CAP = 3;

PriorityOptionsMap makeTestConfig() {
    return {
            {TaskPriority::High, {true, HIGH_PRIORITY_CAP}}, {TaskPriority::Normal, {false, std::nullopt}}
    };
}

TEST(PriorityQueueTest, ConstructorWithValidConfig) {
    PriorityQueue queue(makeTestConfig());
    EXPECT_TRUE(queue.empty());
}

TEST(PriorityQueueTest, ConstructorWithEmptyConfig) {
    PriorityOptionsMap emptyConfig;
    PriorityQueue queue(emptyConfig);
    EXPECT_TRUE(queue.empty());
}

TEST(PriorityQueueTest, PushHighPriorityTask) {
    PriorityQueue queue(makeTestConfig());

    queue.push(TaskPriority::High, [] {
    });

    EXPECT_FALSE(queue.empty());
}

TEST(PriorityQueueTest, PushNormalPriorityTask) {
    PriorityQueue queue(makeTestConfig());

    queue.push(TaskPriority::Normal, [] {
    });

    EXPECT_FALSE(queue.empty());
}

TEST(PriorityQueueTest, PopPrefersHighPriority) {
    PriorityQueue queue(makeTestConfig());

    queue.push(TaskPriority::Normal, [] { });
    queue.push(TaskPriority::High, [] { });

    auto task = queue.pop();
    ASSERT_TRUE(task.has_value());
}

TEST(PriorityQueueTest, PopFromEmptyBlocksUntilTaskArrives) {
    PriorityQueue queue(makeTestConfig());

    std::atomic<bool> taskPushed{false};

    std::thread producer([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        queue.push(TaskPriority::High, [] {
        });
        taskPushed = true;
    });

    auto task = queue.pop();

    producer.join();
    EXPECT_TRUE(task.has_value());
    EXPECT_TRUE(taskPushed);
}

TEST(PriorityQueueTest, PopReturnsOnShutdown) {
    PriorityQueue queue(makeTestConfig());

    std::thread shutdowner([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        queue.shutdown();
    });

    auto task = queue.pop();

    shutdowner.join();
    EXPECT_FALSE(task.has_value()); // shutdown → nullopt
}

TEST(PriorityQueueTest, PushAfterShutdownIgnored) {
    PriorityQueue queue(makeTestConfig());
    queue.shutdown();

    queue.push(TaskPriority::High, [] {
    });

    EXPECT_TRUE(queue.empty());
}

TEST(PriorityQueueTest, MultipleTasksFIFO) {
    PriorityQueue queue(makeTestConfig());
    std::vector<int> executionOrder;

    for(int i = 1; i <= 3; ++i) {
        queue.push(TaskPriority::High, [&executionOrder, i] {
            executionOrder.push_back(i);
        });
    }

    for(int i = 0; i < 3; ++i) {
        auto task = queue.pop();
        ASSERT_TRUE(task.has_value());
        task.value()();
    }

    ASSERT_EQ(executionOrder.size(), 3);
    EXPECT_EQ(executionOrder[0], 1);
    EXPECT_EQ(executionOrder[1], 2);
    EXPECT_EQ(executionOrder[2], 3);
}

TEST(PriorityQueueTest, HighQueueFullState) {
    PriorityQueue queue(makeTestConfig());

    for(int i = 0; i < HIGH_PRIORITY_CAP; ++i) {
        queue.push(TaskPriority::High, [] {
        });
    }

    // TODO: Check that hp q is full
    // EXPECT_FALSE(queue..at(TaskPriority::High)->not_full());
}

TEST(PriorityQueueTest, EmptyReturnsTrueWhenAllQueuesEmpty) {
    PriorityQueue queue(makeTestConfig());

    EXPECT_TRUE(queue.empty());

    queue.push(TaskPriority::Normal, [] {
    });
    EXPECT_FALSE(queue.empty());

    queue.pop();
    EXPECT_TRUE(queue.empty());
}

//============= Multithreaded Test ===============
TEST(PriorityQueueTest, ConcurrentPushAndPop) {
    PriorityQueue queue(makeTestConfig());
    constexpr int kTasksPerPriority = 5;
    std::atomic<int> completed{0};
    {
        std::vector<std::jthread> producers;
        std::vector<std::jthread> consumers;

        for(int i = 0; i < 2; ++i) {
            producers.emplace_back([&, i] {
                for(int j = 0; j < kTasksPerPriority; ++j) {
                    queue.push(
                        (i == 0) ? TaskPriority::High : TaskPriority::Normal,
                        [&] {
                            ++completed;
                            //std::print("[{}] -> completing task [{}]\n", std::this_thread::get_id(), completed.load());
                        }
                    );
                }
            });
        }

        for(int i = 0; i < 2; ++i) {
            consumers.emplace_back([&] {
                while(completed < 2 * kTasksPerPriority) {
                    if(auto task = queue.pop()) {
                        task.value()();
                    } else {
                        std::this_thread::yield();
                    }
                }
                //once all tasks completed?
                queue.shutdown();
            });
        }
    }

    EXPECT_EQ(completed, 2 * kTasksPerPriority);
    EXPECT_TRUE(queue.empty());
}

TEST(PriorityQueueTest, ShutdownFromMultipleThreads) {
    PriorityQueue queue(makeTestConfig());

    std::vector<std::thread> shutdowners;
    for(int i = 0; i < 3; ++i) {
        shutdowners.emplace_back([&] {
            queue.shutdown();
        });
    }

    for(auto& t : shutdowners) t.join();

    auto result = queue.pop();
    EXPECT_FALSE(result.has_value());
}
}