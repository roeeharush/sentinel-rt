#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <cstddef>
#include <utility>

template <typename T>
class BoundedQueue {
  public:
    explicit BoundedQueue(std::size_t capacity);
    void push(T item);
    void pushDropOldest(T item);
    std::optional<T> pop();
    void close();
    std::size_t size() const;
    std::size_t droppedCount() const;

private:
    std::size_t capacity_;
    std::deque<T> items_;
    mutable std::mutex mutex_;
    std::condition_variable itemAvailable_;
    std::condition_variable spaceAvailable_;
    bool closed_ = false;
    std::size_t droppedCount_ = 0;

};

template <typename T>
BoundedQueue<T>::BoundedQueue(std::size_t capacity)
    : capacity_(capacity) {}

template <typename T>
void BoundedQueue<T>::push(T item) {
    std::unique_lock lock(mutex_);
    spaceAvailable_.wait(lock, [this]() {
        return items_.size() < capacity_;
    });
    items_.push_back(std::move(item));
    itemAvailable_.notify_one();
}

template <typename T>
void BoundedQueue<T>::pushDropOldest(T item) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (items_.size() >= capacity_) {
        items_.pop_front();
        ++droppedCount_;
    }
    items_.push_back(std::move(item));
    itemAvailable_.notify_one();
}

template <typename T>
std::optional<T> BoundedQueue<T>::pop() {
    std::unique_lock lock(mutex_);
    itemAvailable_.wait(lock, [this]() {
        return !items_.empty() || closed_;
    });

    if (items_.empty()) {
        return std::nullopt;
    }

    T item = std::move(items_.front());
    items_.pop_front();
    if (items_.size() <= capacity_ / 2) {
        spaceAvailable_.notify_one();
    }
    return item;
}

template <typename T>
void BoundedQueue<T>::close() {
    std::lock_guard lock(mutex_);
    closed_ = true;
    itemAvailable_.notify_all();
}

template <typename T>
std::size_t BoundedQueue<T>::size() const {
    std::lock_guard lock(mutex_);
    return items_.size();
}

template <typename T>
std::size_t BoundedQueue<T>::droppedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return droppedCount_;
}