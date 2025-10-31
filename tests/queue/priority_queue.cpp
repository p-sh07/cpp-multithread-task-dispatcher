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

/**mt test 2
 *namespace ConsumeProduce {
class Producer {
public:
    Producer(std::shared_ptr<PriorityQueue> queue, size_t total) : queue_(queue), remaining_(total) {
        thread_ = std::jthread([this] { Run(); });
    }

    void Wait() {
        if (thread_.joinable())
            thread_.join();
    }

    size_t Size() const { return remaining_; }

private:
    void Run() {
        for (auto i = remaining_.load(); i > 0; --remaining_) {  // ????????? from book
            queue_->push(
                (i % 2 == 0) ? TaskPriority::High : TaskPriority::Normal,
                []{});
        }
    }

    std::shared_ptr<PriorityQueue> queue_;
    std::atomic<size_t> remaining_{0};
    std::jthread thread_;
};

class CountedConsumer {
public:
    explicit CountedConsumer(std::shared_ptr<PriorityQueue> queue) : queue_(std::move(queue)) {
        thread_ = std::jthread([this] { Run(); });
    }

    void Wait() {
        if (thread_.joinable())
            thread_.join();
    }

    size_t Size() const { return count_; }

private:
    void Run() {
        while (auto item = queue_->pop()) {

            if (item.has_value()) {
                ++count_;
            }
        }
    }

    std::shared_ptr<PriorityQueue> queue_;
    std::atomic<size_t> count_{0};
    std::jthread thread_;
};


TEST(PriorityQueueTest, ConcurrentPushAndPop) {
    auto queue = std::make_shared<PriorityQueue>(makeTestConfig());
    constexpr int kTasksPerPriority = 5;
    std::atomic<int> completed{0};
    {

        Producer prod{queue, 10'000};  // Создаём одного производителя, который добавит в очередь 10'000 элементов
        CountedConsumer cons{queue};  // Создаём одного потребителя, который будет обрабатывать добавленные в очередь элементы

        // Ждём, пока производитель добавит все задачи в очередь, и останавливаем обработку новых задач
        prod.Wait();
        queue->shutdown();

        // Ждём, пока CountedConsumer обработает все задачи
        cons.Wait();

        // Проверяем, что Producer добавил все свои задачи в очередь
        if (prod.Size() != 0) {
            std::println("Ошибка! Производитель отправил не все задачи на исполнение");
        }

        // Проверяем, что CountedConsumer обработал все задачи
        if (cons.Size() != 10'000) {
            std::println("Ошибка! Потребитель обработал не все задачи");
        }

        EXPECT_EQ(prod.Size(), 0);
        EXPECT_EQ(cons.Size(), 10'000);
        EXPECT_TRUE(queue->empty());
    }
}
}*/

/* PQ code with prints: - in case of further debug
 * #include "queue/priority_queue.hpp"
#include <print>

namespace dispatcher::queue {
PriorityQueue::PriorityQueue(const PriorityOptionsMap& options_map) {
    for(const auto& [priority, options] : options_map) {
        std::unique_ptr<IQueue> q_ptr = nullptr;
        if(options.bounded) {
            q_ptr = std::make_unique<BoundedQueue>(*options.capacity);
        } else {
            q_ptr = std::make_unique<UnboundedQueue>();
        }
        auto lk = std::lock_guard(queue_mutex_);
        priority_queues_.try_emplace(priority, std::move(q_ptr)); //push under lock
    }
}

void PriorityQueue::push(TaskPriority priority, std::function<void()> task) {
    auto lk = std::unique_lock(queue_mutex_);
    auto& queue = priority_queues_.at(priority);

    //For high priority task the queue is bounded, so have to wait for not_full
    if(priority == TaskPriority::High) {
        std::print("[{}]: pushing HP task -> ", std::this_thread::get_id());
        hpq_not_full_.wait(lk, [this, &queue] {
            std::print("wait() returns {}\n", queue->not_full() || shutdown_active_);
            return queue->not_full() || shutdown_active_;
        });
    }

    //Stop accepting pushes on shutdown
    if(shutdown_active_) {
        std::println("[{}]: Rejected task ", std::this_thread::get_id());
        return;
    }

    std::println("[{}]: Accepted task -> ", std::this_thread::get_id());
    queue->push(std::move(task));
    not_empty_.notify_one(); //wake sleeping thread??
}

std::optional<std::function<void()>> PriorityQueue::pop() {
    auto lk = std::unique_lock(queue_mutex_);

    //wait until at least one of the qs is not empty
    not_empty_.wait(lk, [this] {
        return !empty() || shutdown_active_;
    });

    std::print("[{}]: Exited wait for pop -> ", std::this_thread::get_id());
    //Return HP task if not empty
    if(auto high_priority_task = priority_queues_.at(TaskPriority::High)->try_pop()) {
        std::print("getting HP task, send Notify\n");
        hpq_not_full_.notify_one();
        return high_priority_task;
    }

    //Else return NP task [or nullopt, _only_ when shutdown is active]
    std::print("getting NP task, try_pop -> q is empty: {}", priority_queues_.at(TaskPriority::Normal)->empty());
    auto task = priority_queues_.at(TaskPriority::Normal)->try_pop();
    std::print(" / has_value: {}\n", task.has_value());
    return task;
}

void PriorityQueue::shutdown() {
    shutdown_active_ = true;

    std::println("[{}]: notified all -> ", std::this_thread::get_id());
    not_empty_.notify_all();
    hpq_not_full_.notify_all();
}

PriorityQueue::~PriorityQueue() {

}

bool PriorityQueue::empty() {
    return std::ranges::all_of(priority_queues_ | vw::values, &IQueue::empty);
}
} // namespace dispatcher::queue
*/