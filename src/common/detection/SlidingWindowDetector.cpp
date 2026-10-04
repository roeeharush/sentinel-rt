#include "common/detection/SlidingWindowDetector.h"

SlidingWindowDetector::SlidingWindowDetector(std::size_t windowCapacity)
    : windowCapacity_(windowCapacity) {}

Status SlidingWindowDetector::evaluate(const Measurement& measurement) {
    std::lock_guard<std::mutex> lock(mutex_);

    WindowKey key(measurement.deviceId, measurement.sensorType);
    auto it = windows_.find(key);
    if (it == windows_.end()) {
        it = windows_.emplace(key, SlidingWindow(windowCapacity_)).first;
    }

    SlidingWindow& window = it->second;
    window.push(measurement.value);

    if (window.isStuck()) {
        return Status::Critical;
    }
    if (window.isDrifting()) {
        return Status::Warning;
    }
    return Status::Ok;
}