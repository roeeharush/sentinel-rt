#pragma once

#include "common/infra/IClock.h"

class RealClock final : public IClock {
public:
    std::chrono::steady_clock::time_point now() const override;
    void sleepUntil(std::chrono::steady_clock::time_point wakeTime) override;
};