#include "common/detection/TimeoutMonitor.h"

TimeoutMonitor::TimeoutMonitor(std::chrono::milliseconds timeoutThreshold)
    : timeoutThreshold_(timeoutThreshold) {}

void TimeoutMonitor::recordArrival(std::uint32_t deviceId, std::chrono::steady_clock::time_point now) {
    std::lock_guard<std::mutex> lock(mutex_);

    DeviceState& state = devices_[deviceId];
    state.lastSeen = now;
    state.timedOut = false;
}

std::vector<std::uint32_t> TimeoutMonitor::checkTimeouts(std::chrono::steady_clock::time_point now) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::uint32_t> newlyTimedOut;

    for (auto& [deviceId, state] : devices_) {
        auto elapsed = now - state.lastSeen;
        if (elapsed > timeoutThreshold_ && !state.timedOut) {
            state.timedOut = true;
            newlyTimedOut.push_back(deviceId);
        }
    }

    return newlyTimedOut;
}
