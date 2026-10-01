#pragma once

#include "common/FaultType.h"
#include <chrono>
#include <optional>

struct TimedFault {
    FaultType type;
    std::chrono::milliseconds activateAfter;
};

struct DeviceSimConfig {
    int cycles{};
    int periodMs{};
    std::optional<TimedFault> fault;
};

std::optional<DeviceSimConfig> parseDeviceSimArgs(int argc, char* argv[]);
