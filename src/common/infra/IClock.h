#pragma once

#include <chrono>

class IClock {
public:
    virtual std::chrono::steady_clock::time_point now() const = 0;
    virtual void sleepUntil(std::chrono::steady_clock::time_point wakeTime) = 0;
    virtual ~IClock() = default;
};