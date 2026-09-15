#pragma once

#include "common/ISensor.h"
#include "common/Measurement.h"
#include <memory>
#include <chrono>
#include <vector>

class Device {
public:
    Device(std::uint32_t deviceId, std::vector<std::unique_ptr<ISensor>> sensors);
    std::uint32_t id() const;
    std::size_t sensorCount() const;
    std::vector<Measurement> sample(std::chrono::steady_clock::time_point now);
private:
    std::uint32_t deviceId_;
    std::vector<std::unique_ptr<ISensor>> sensors_;
};