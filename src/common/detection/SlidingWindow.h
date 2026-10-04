#pragma once

#include <cstddef>
#include <deque>

class SlidingWindow {
public:
    explicit SlidingWindow(std::size_t capacity);

    void push(double value);
    bool isStuck() const;
    bool isDrifting() const;
    std::size_t size() const;

private:
    std::size_t capacity_;
    std::deque<double> values_;
};