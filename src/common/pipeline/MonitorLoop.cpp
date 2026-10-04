#include "common/pipeline/MonitorLoop.h"

void runMonitoringLoop(Device& device, BoundedQueue<Measurement>& queue, IClock& clock, int cycleCount, std::chrono::milliseconds period, bool blockWhenFull) {
    std::chrono::steady_clock::time_point nextWakeTime = clock.now();
    for (int cycle = 0; cycle < cycleCount; ++cycle) {
        std::vector<Measurement> measurements = device.sample(clock.now());
        for (const Measurement& measurement : measurements) {
            if (blockWhenFull) {
                queue.push(measurement);
            } else {
                queue.pushDropOldest(measurement);
            }
        }
        nextWakeTime += period;
        clock.sleepUntil(nextWakeTime);
    }
}