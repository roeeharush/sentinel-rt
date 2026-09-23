#include <gtest/gtest.h>
#include "common/BoundedQueue.h"
#include <thread>

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
