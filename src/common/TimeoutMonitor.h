#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <vector>

class TimeoutMonitor {
public:
    explicit TimeoutMonitor(std::chrono::milliseconds timeoutThreshold);

    void recordArrival(std::uint32_t deviceId, std::chrono::steady_clock::time_point now);
    std::vector<std::uint32_t> checkTimeouts(std::chrono::steady_clock::time_point now);

private:
    struct DeviceState {
        std::chrono::steady_clock::time_point lastSeen;
        bool timedOut = false;
    };

    std::chrono::milliseconds timeoutThreshold_;
    std::map<std::uint32_t, DeviceState> devices_;
    std::mutex mutex_;
};
