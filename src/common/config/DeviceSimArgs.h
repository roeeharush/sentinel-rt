#pragma once

#include "common/core/FaultType.h"
#include <chrono>
#include <optional>
#include <string>

struct TimedFault {
    FaultType type;
    std::chrono::milliseconds activateAfter;
};

struct DeviceSimConfig {
    int cycles{};
    int periodMs{};
    std::optional<TimedFault> fault;
};

std::optional<FaultType> parseFaultType(const std::string& text);
std::optional<DeviceSimConfig> parseDeviceSimArgs(int argc, char* argv[]);
