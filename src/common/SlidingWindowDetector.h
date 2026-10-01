#pragma once

#include "common/Measurement.h"
#include "common/SensorType.h"
#include "common/SlidingWindow.h"
#include "common/Status.h"
#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <utility>

class SlidingWindowDetector {
public:
    explicit SlidingWindowDetector(std::size_t windowCapacity);

    Status evaluate(const Measurement& measurement);

private:
    using WindowKey = std::pair<std::uint32_t, SensorType>;

    std::size_t windowCapacity_;
    std::map<WindowKey, SlidingWindow> windows_;
    std::mutex mutex_;
};