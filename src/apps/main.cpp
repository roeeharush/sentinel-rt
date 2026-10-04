#include "common/config/CliArgs.h"
#include "common/sensors/Device.h"
#include "common/sensors/Sensor.h"
#include "common/detection/ThresholdDetector.h"
#include "common/infra/Logger.h"
#include "common/infra/StatsCollector.h"
#include "common/infra/RealClock.h"
#include "common/pipeline/MonitorLoop.h"
#include "common/pipeline/BoundedQueue.h"
#include "common/pipeline/WorkerPool.h"

#include <iostream>
#include <memory>
#include <vector>
#include <chrono>

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

int main(int argc, char* argv[]) {
    std::optional<AppConfig> config = parseArgs(argc, argv);
    if (!config) {
        std::cerr << "Usage: monitor_app [--cycles N] [--period-ms N]" << std::endl;
        return 1;
    }

    std::vector<std::unique_ptr<ISensor>> sensors;
    sensors.push_back(std::make_unique<Sensor>(SensorType::Temperature, temperatureSpec(), 42));
    sensors.push_back(std::make_unique<Sensor>(SensorType::Pressure, pressureSpec(), 7));
    sensors.push_back(std::make_unique<Sensor>(SensorType::Voltage, voltageSpec(), 99));
    Device device(1, std::move(sensors));

    ThresholdDetector detector(defaultThresholds());
    Logger logger;
    StatsCollector stats(std::chrono::milliseconds(20));
    BoundedQueue<Measurement> queue(static_cast<std::size_t>(config->queueCapacity));
    RealClock clock;

    WorkerPool<Measurement> pool(queue, logger, [&](Measurement measurement) {
    auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - measurement.timestamp);
    stats.recordLatency(latency);

    Status status = detector.evaluate(measurement);
    stats.record(status);
    if (status != Status::Ok) {
        logger.log(levelForStatus(status), "Measurement outside normal range");
    }
    }, static_cast<std::size_t>(config->workers));

    runMonitoringLoop(device, queue, clock, config->cycles, std::chrono::milliseconds(config->periodMs), config->blockWhenFull);
    queue.close();
    pool.stop();

    std::cout << "Finished " << config->cycles << " cycles." << std::endl;
    std::cout << "OK: " << stats.okCount()
              << ", Warning: " << stats.warningCount()
              << ", Critical: " << stats.criticalCount()
              << ", Invalid: " << stats.invalidCount() << std::endl;
    std::cout << "Dropped: " << queue.droppedCount() << std::endl;

    return 0;
}