#include "common/CliArgs.h"
#include "common/Device.h"
#include "common/Sensor.h"
#include "common/ThresholdDetector.h"
#include "common/Logger.h"
#include "common/StatsCollector.h"
#include "common/RealClock.h"
#include "common/MonitorLoop.h"

#include <iostream>
#include <memory>
#include <vector>
#include <chrono>

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
    StatsCollector stats;
    RealClock clock;

    runMonitoringLoop(device, detector, logger, stats, clock,config->cycles, std::chrono::milliseconds(config->periodMs));

    std::cout << "Finished " << config->cycles << " cycles." << std::endl;
    std::cout << "OK: " << stats.okCount()
              << ", Warning: " << stats.warningCount()
              << ", Critical: " << stats.criticalCount()
              << ", Invalid: " << stats.invalidCount() << std::endl;

    return 0;
}