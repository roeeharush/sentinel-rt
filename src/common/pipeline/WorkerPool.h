#pragma once

#include <atomic>
#include <cstddef>
#include <functional>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

#include "common/pipeline/BoundedQueue.h"
#include "common/infra/Logger.h"

template <typename T>
class WorkerPool {
public:
    WorkerPool(BoundedQueue<T>& queue, Logger& logger, std::function<void(T)> handler, std::size_t numWorkers);
    ~WorkerPool();
    void stop();

private:
    void workerLoop();

    BoundedQueue<T>& queue_;
    Logger& logger_;
    std::function<void(T)> handler_;
    std::vector<std::thread> workers_;
    std::atomic<bool> stopped_{false};
};

template <typename T>
WorkerPool<T>::WorkerPool(BoundedQueue<T>& queue, Logger& logger, std::function<void(T)> handler, std::size_t numWorkers)
    : queue_(queue), logger_(logger), handler_(std::move(handler)) {
    workers_.reserve(numWorkers);
    for (std::size_t i = 0; i < numWorkers; ++i) {
        workers_.emplace_back(&WorkerPool::workerLoop, this);
    }
}

template <typename T>
WorkerPool<T>::~WorkerPool() {
    stop();
}

template <typename T>
void WorkerPool<T>::stop() {
    if (stopped_.exchange(true)) {
        return;
    }
    queue_.close();
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

template <typename T>
void WorkerPool<T>::workerLoop() {
    while (true) {
        std::optional<T> item = queue_.pop();
        if (!item.has_value()) {
            break;
        }
        try {
            handler_(std::move(*item));
        } catch (const std::exception& e) {
            logger_.log(LogLevel::Error, e.what());
        } catch (...) {
            logger_.log(LogLevel::Error, "unknown exception in worker");
        }
    }
}
