#include "common/MonitorLoop.h"

namespace {

    LogLevel levelForStatus(Status status) {
        switch (status) {
            case Status::Warning:
                return LogLevel::Warning;
            case Status::Critical:
            case Status::Invalid:
                return LogLevel::Error;
            case Status::Ok:
                return LogLevel::Info;
        }
        return LogLevel::Info;
    }

}

void runMonitoringLoop(Device& device, ThresholdDetector& detector, Logger& logger, StatsCollector& stats, IClock& clock, int cycleCount, std::chrono::milliseconds period) {
    std::chrono::steady_clock::time_point nextWakeTime = clock.now();
    for (int cycle = 0; cycle < cycleCount; ++cycle) {
        std::vector<Measurement> measurements = device.sample(clock.now());
        for (const Measurement& measurement : measurements) {
            Status status = detector.evaluate(measurement);
            stats.record(status);
            if (status != Status::Ok) {
                logger.log(levelForStatus(status), "Measurement outside normal range");
            }
        }
        nextWakeTime += period;
        clock.sleepUntil(nextWakeTime);
    }
}