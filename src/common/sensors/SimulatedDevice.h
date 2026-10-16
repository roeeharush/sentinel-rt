#pragma once

#include "common/sensors/Device.h"
#include "common/sensors/FaultySensor.h"

#include <cstdint>

struct SimulatedDevice {
    Device device;
    FaultySensor* temperature;
};

SimulatedDevice makeSimulatedDevice(std::uint32_t deviceId = 1);
