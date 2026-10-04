#pragma once

#include "common/config/DeviceSimArgs.h"
#include "common/core/FaultType.h"
#include <chrono>
#include <optional>

class FaultSchedule {
public:
    explicit FaultSchedule(std::optional<TimedFault> fault);

    std::optional<FaultType> due(std::chrono::milliseconds elapsed);

private:
    std::optional<TimedFault> fault_;
    bool activated_ = false;
};
