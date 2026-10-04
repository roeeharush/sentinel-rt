#include "common/detection/SlidingWindow.h"
#include <algorithm>
#include <cmath>

namespace {
    constexpr double kStuckEpsilon = 0.01;
    constexpr double kDriftThreshold = 1.0;
}

SlidingWindow::SlidingWindow(std::size_t capacity)
    : capacity_(capacity) {}

void SlidingWindow::push(double value) {
    if (values_.size() >= capacity_) {
        values_.pop_front();
    }
    values_.push_back(value);
}

std::size_t SlidingWindow::size() const {
    return values_.size();
}

bool SlidingWindow::isStuck() const {
    if (values_.size() < capacity_) {
        return false;
    }

    double minValue = *std::min_element(values_.begin(), values_.end());
    double maxValue = *std::max_element(values_.begin(), values_.end());
    return (maxValue - minValue) < kStuckEpsilon;
}

bool SlidingWindow::isDrifting() const {
    if (values_.size() < capacity_) {
        return false;
    }

    std::size_t half = values_.size() / 2;
    double firstHalfSum = 0.0;
    double secondHalfSum = 0.0;

    for (std::size_t i = 0; i < half; ++i) {
        firstHalfSum += values_[i];
    }
    for (std::size_t i = half; i < values_.size(); ++i) {
        secondHalfSum += values_[i];
    }

    double firstHalfAvg = firstHalfSum / half;
    double secondHalfAvg = secondHalfSum / (values_.size() - half);

    return std::abs(secondHalfAvg - firstHalfAvg) > kDriftThreshold;
}