#include <gtest/gtest.h>
#include "common/WorkerPool.h"
#include "common/BoundedQueue.h"
#include "common/Logger.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <sstream>
#include <vector>

TEST(WorkerPool, ProcessesAllPushedItems) {
    BoundedQueue<int> queue(10);
    Logger logger;
    std::mutex resultsMutex;
    std::vector<int> results;

    WorkerPool<int> pool(queue, logger, [&](int item) {
        std::lock_guard<std::mutex> lock(resultsMutex);
        results.push_back(item);
    }, 2);

    queue.push(1);
    queue.push(2);
    queue.push(3);
    queue.close();

    pool.stop();

    std::sort(results.begin(), results.end());
    EXPECT_EQ(results, (std::vector<int>{1, 2, 3}));
}

TEST(WorkerPool, ExceptionInHandlerIsLoggedAndDoesNotStopProcessing) {
    BoundedQueue<int> queue(10);
    std::ostringstream captured;
    Logger logger(captured);
    std::atomic<int> successCount{0};

    WorkerPool<int> pool(queue, logger, [&](int item) {
        if (item == 2) {
            throw std::runtime_error("bad item");
        }
        ++successCount;
    }, 1);

    queue.push(1);
    queue.push(2);
    queue.push(3);
    queue.close();

    pool.stop();

    EXPECT_EQ(successCount, 2);
    EXPECT_NE(captured.str().find("bad item"), std::string::npos);
}

TEST(WorkerPool, StopIsIdempotent) {
    BoundedQueue<int> queue(10);
    Logger logger;
    WorkerPool<int> pool(queue, logger, [](int) {}, 2);

    queue.close();
    pool.stop();
    EXPECT_NO_THROW(pool.stop());
}

TEST(WorkerPool, DestructorStopsWorkersWithoutExplicitStop) {
    BoundedQueue<int> queue(10);
    Logger logger;
    std::atomic<int> processed{0};

    {
        WorkerPool<int> pool(queue, logger, [&](int) { ++processed; }, 2);
        queue.push(1);
        queue.push(2);
        queue.close();
    }

    EXPECT_EQ(processed, 2);
}
