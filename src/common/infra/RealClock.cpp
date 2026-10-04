#include "common/infra/RealClock.h"
#include <thread>

std::chrono::steady_clock::time_point RealClock::now() const {
    return std::chrono::steady_clock::now();
}

void RealClock::sleepUntil(std::chrono::steady_clock::time_point wakeTime){
  std::this_thread::sleep_until(wakeTime);
  }