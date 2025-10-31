#include <gtest/gtest.h>

#include "task_dispatcher.hpp"

namespace dispatcher {
static constexpr std::chrono::microseconds TEST_TIMEOUT {10000};
static constexpr int TASK_COUNT = 500;

//Use mutex to ensure Normal task doesnt randomly start first in a different thread
TEST(TaskDispatcherTest, HighAndNormalPriorityOrder) {
    TaskDispatcher dispatcher(std::thread::hardware_concurrency());
    std::vector<int> executionOrder;
    std::mutex orderMutex;

    dispatcher.schedule(TaskPriority::Normal, [&] {
        std::lock_guard lock(orderMutex);
        executionOrder.push_back(2);
    });

    dispatcher.schedule(TaskPriority::High, [&] {
        std::lock_guard lock(orderMutex);
        executionOrder.push_back(1);
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));

    ASSERT_EQ(executionOrder.size(), 2);
    EXPECT_EQ(executionOrder[0], 1);  // High
    EXPECT_EQ(executionOrder[1], 2);  // Normal

}

TEST(TaskDispatcherTest, ScheduleMultipleTasks) {
    TaskDispatcher dispatcher(4);

    std::vector<bool> executed(TASK_COUNT, false);

    for (int i = 0; i < TASK_COUNT; ++i) {
        dispatcher.schedule(
            (i % 2 == 0) ? TaskPriority::High : TaskPriority::Normal,
            [&, i]() { executed[i] = true; }
        );
    }

    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < TEST_TIMEOUT) {
        if (rg::none_of(executed, std::logical_not())) { //none of are false - avoid lambda or custom func.
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    for (bool wasExecuted : executed) {
        EXPECT_TRUE(wasExecuted) << "Task was not executed";
    }
}

TEST(TaskDispatcherTest, ScheduleToFullHighPriorityQueue) {
    std::map<TaskPriority, dispatcher::queue::QueueOptions> config = {
        {TaskPriority::High, {true, 1}},
        {TaskPriority::Normal, {false, std::nullopt}}
    };

    TaskDispatcher dispatcher(1, config);

    bool firstExecuted = false;
    dispatcher.schedule(TaskPriority::High, [&firstExecuted]() {
        firstExecuted = true;
    });

    dispatcher.schedule(TaskPriority::High, []{});

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_TRUE(firstExecuted);
}

TEST(TaskDispatcherTest, DestructorNoThrow) {
    {
        TaskDispatcher dispatcher(2);

        for (int i = 0; i < 5; ++i) {
            dispatcher.schedule(TaskPriority::Normal, []{
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            });
        }
    } // ~TaskDispatcher

    //Destructor no-throw, finished successfully
    //TODO: check threads/etc?
    EXPECT_TRUE(true);
}

TEST(TaskDispatcherTest, ConcurrentScheduleFromMultipleThreads) {
    TaskDispatcher dispatcher(4);
    constexpr int TOTAL_TASKS = 100;
    std::vector<bool> executed(TOTAL_TASKS, false);

    std::vector<std::thread> senders;

    for (int t = 0; t < 4; ++t) {
        senders.emplace_back([&, t]() {
            for (int i = t * 25; i < (t + 1) * 25; ++i) {
                dispatcher.schedule(
                    (i % 2 == 0) ? TaskPriority::High : TaskPriority::Normal,
                    [&, i]() { executed[i] = true; }
                );
            }
        });
    }

    for (auto& sender : senders) {
        sender.join();
    }

    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < TEST_TIMEOUT) {
        if (std::all_of(executed.begin(), executed.end(), [](bool b) { return b; })) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    for (bool wasExecuted : executed) {
        EXPECT_TRUE(wasExecuted) << "Task not executed in concurrent test";
    }
}

//TODO: Test suite with combinations of threads / tasks / configs

TEST(TaskDispatcherTest, OneThreadCorrectOrder) {
    constexpr int tasks_num = 10;

    //TODO: This test fails ?1/!?1
    std::vector<std::pair<int, TaskPriority>> executed_order(tasks_num);
    std::atomic<int> exec_order {0};
    std::mutex ord_mtx;
    {
        TaskDispatcher dispatcher(1);
        for(int i = 0; i < 2; ++i) {
            for (int j = 0; j < tasks_num / 2; ++j) {
                int task_id = i * (tasks_num / 2) + j;
                auto priority = (i == 0) ? TaskPriority::High : TaskPriority::Normal;
                std::println("Sheduling:");
                std::println("#{}, {}", task_id, priority == TaskPriority::High ? "high" : "norm");
                dispatcher.schedule(priority,
                    [&]() {
                        auto lk = std::lock_guard(ord_mtx);
                        executed_order[task_id] = std::make_pair(exec_order.load(), priority);
                        ++exec_order;
                    }
                );
            }
        }
    } //~destroyed, so all tasks should be finished

    std::println("Completed:");
    for(const auto& [order_completed, pr] : executed_order) {
        std::println("#{}, {}", order_completed, pr == TaskPriority::High ? "high" : "norm");
    }

    int count = 0;
    for (const auto& [order_completed, pr] : executed_order) {
        if(count < tasks_num / 2) {
            EXPECT_EQ(pr, TaskPriority::High);
        } else {
            EXPECT_EQ(pr, TaskPriority::Normal);
        }
        EXPECT_EQ(order_completed, count);
        ++count;
    }
    EXPECT_EQ(count, tasks_num);
}

}//nnamespace dispatchee