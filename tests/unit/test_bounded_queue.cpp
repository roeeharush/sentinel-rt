#include <gtest/gtest.h>
#include "common/pipeline/BoundedQueue.h"
#include <thread>
#include <atomic>
#include <chrono>

TEST(BoundedQueue, PushThenPopReturnsSameItem) {
    BoundedQueue<int> queue(5);
    queue.push(42);

    std::optional<int> result = queue.pop();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

TEST(BoundedQueue, MaintainsFifoOrder) {
    BoundedQueue<int> queue(5);
    queue.push(1);
    queue.push(2);
    queue.push(3);

    EXPECT_EQ(*queue.pop(), 1);
    EXPECT_EQ(*queue.pop(), 2);
    EXPECT_EQ(*queue.pop(), 3);
}

TEST(BoundedQueue, SizeReflectsPushesAndPops) {
    BoundedQueue<int> queue(5);
    EXPECT_EQ(queue.size(), 0u);

    queue.push(1);
    queue.push(2);
    EXPECT_EQ(queue.size(), 2u);

    queue.pop();
    EXPECT_EQ(queue.size(), 1u);
}

TEST(BoundedQueue, PopOnClosedEmptyQueueReturnsNullopt) {
    BoundedQueue<int> queue(5);
    queue.close();

    std::optional<int> result = queue.pop();

    EXPECT_FALSE(result.has_value());
}

TEST(BoundedQueue, DrainsRemainingItemsAfterClose) {
    BoundedQueue<int> queue(5);
    queue.push(1);
    queue.push(2);
    queue.push(3);
    queue.close();

    EXPECT_EQ(queue.pop(), std::optional<int>(1));
    EXPECT_EQ(queue.pop(), std::optional<int>(2));
    EXPECT_EQ(queue.pop(), std::optional<int>(3));
    EXPECT_FALSE(queue.pop().has_value());
}

TEST(BoundedQueue, PopBlocksUntilItemPushedFromAnotherThread) {
    BoundedQueue<int> queue(5);

    std::thread producer([&queue]() {
        queue.push(99);
    });

    std::optional<int> result = queue.pop();

    producer.join();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 99);
}

TEST(BoundedQueue, PushBlocksWhenFullUntilSpaceFreed) {
    BoundedQueue<int> queue(1);
    queue.push(1);

    std::thread consumer([&queue]() {
        queue.pop();
    });

    queue.push(2);

    consumer.join();

    EXPECT_EQ(queue.size(), 1u);
}

TEST(BoundedQueue, PushDropOldestAddsItemWhenNotFull) {
    BoundedQueue<int> queue(3);
    queue.pushDropOldest(1);
    queue.pushDropOldest(2);

    EXPECT_EQ(queue.size(), 2u);
    EXPECT_EQ(*queue.pop(), 1);
    EXPECT_EQ(*queue.pop(), 2);
}

TEST(BoundedQueue, PushDropOldestEvictsOldestWhenFull) {
    BoundedQueue<int> queue(3);
    queue.pushDropOldest(1);
    queue.pushDropOldest(2);
    queue.pushDropOldest(3);
    queue.pushDropOldest(4);

    EXPECT_EQ(queue.size(), 3u);
    EXPECT_EQ(*queue.pop(), 2);
    EXPECT_EQ(*queue.pop(), 3);
    EXPECT_EQ(*queue.pop(), 4);
}

TEST(BoundedQueue, PushDropOldestNeverExceedsCapacity) {
    BoundedQueue<int> queue(2);
    queue.pushDropOldest(1);
    queue.pushDropOldest(2);
    queue.pushDropOldest(3);
    queue.pushDropOldest(4);

    EXPECT_EQ(queue.size(), 2u);
}

TEST(BoundedQueue, DroppedCountTracksEvictions) {
    BoundedQueue<int> queue(2);
    queue.pushDropOldest(1);
    queue.pushDropOldest(2);
    EXPECT_EQ(queue.droppedCount(), 0u);

    queue.pushDropOldest(3);
    queue.pushDropOldest(4);
    EXPECT_EQ(queue.droppedCount(), 2u);
}

// ---- Wake-up policy of pop(): the producer is woken only when the queue is at
// ---- half capacity or less (and always when it is empty), not on every pop.

TEST(BoundedQueue, BlockedProducerIsNotWokenWhileQueueIsAboveHalfAfterPop) {
    BoundedQueue<int> queue(4);
    for (int i = 0; i < 4; ++i) queue.push(i);

    std::atomic<bool> pushed{false};
    std::thread producer([&]() {
        queue.push(100);
        pushed = true;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));   // let it block on the full queue

    queue.pop();                                                   // size 3 > 2: no wake-up yet
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(pushed.load());

    queue.pop();                                                   // size 2 <= 2: producer is woken
    producer.join();
    EXPECT_TRUE(pushed.load());
    EXPECT_EQ(queue.size(), 3u);
}

TEST(BoundedQueue, BlockedProducerIsWokenWhenPopEmptiesTheQueue) {
    BoundedQueue<int> queue(1);
    queue.push(1);

    std::atomic<bool> pushed{false};
    std::thread producer([&]() {
        queue.push(2);
        pushed = true;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    queue.pop();                                                   // size 0: must wake the producer
    producer.join();                                               // a missed wake-up would hang here
    EXPECT_TRUE(pushed.load());
}

TEST(BoundedQueue, ProducerAndTwoConsumersDeliverEveryItemExactlyOnce) {
    constexpr int kItems = 20000;
    BoundedQueue<int> queue(8);
    std::atomic<int> received{0};
    std::atomic<long long> sum{0};

    auto consume = [&]() {
        while (std::optional<int> item = queue.pop()) {
            ++received;
            sum += *item;
        }
    };
    std::thread consumerA(consume);
    std::thread consumerB(consume);

    for (int i = 1; i <= kItems; ++i) queue.push(i);
    queue.close();
    consumerA.join();
    consumerB.join();

    EXPECT_EQ(received.load(), kItems);
    EXPECT_EQ(sum.load(), static_cast<long long>(kItems) * (kItems + 1) / 2);
}
