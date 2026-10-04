#include "common/infra/ManualClock.h"

ManualClock::ManualClock(std::chrono::steady_clock::time_point startTime)
    : currentTime_(startTime) {}

std::chrono::steady_clock::time_point ManualClock::now() const {
     return currentTime_;
}

void ManualClock::sleepUntil(std::chrono::steady_clock::time_point wakeTime) {
    currentTime_ = wakeTime;
}