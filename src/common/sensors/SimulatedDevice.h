#pragma once

#include "common/sensors/Device.h"
#include "common/sensors/FaultySensor.h"

struct SimulatedDevice {
    Device device;
    FaultySensor* temperature;
};

SimulatedDevice makeSimulatedDevice();
