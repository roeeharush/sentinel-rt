#pragma once
#include "common/core/SensorType.h"
#include <chrono>
#include <cstdint>

struct Measurement {
    std::uint32_t deviceId;
    SensorType sensorType;
    std::chrono::steady_clock::time_point timestamp;
    double value;
   };