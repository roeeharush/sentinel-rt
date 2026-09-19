#pragma once

#include "common/IClock.h"

class ManualClock final : public IClock {
public:
    explicit ManualClock(std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::time_point{});
    std::chrono::steady_clock::time_point now() const override;
    void sleepUntil(std::chrono::steady_clock::time_point wakeTime) override;
private:
    std::chrono::steady_clock::time_point currentTime_;
};