#include <gtest/gtest.h>
#include "common/MonitorLoop.h"
#include "common/Device.h"
#include "common/Sensor.h"
#include "common/ThresholdDetector.h"
#include "common/Logger.h"
#include "common/StatsCollector.h"
#include "common/ManualClock.h"
#include <memory>
#include <sstream>

TEST(MonitorLoop, AdvancesClockByExactlyCycleCountTimesPeriod) {
    std::vector<std::unique_ptr<ISensor>> sensors;
    sensors.push_back(std::make_unique<Sensor>(SensorType::Temperature, temperatureSpec(), 42));
    Device device(1, std::move(sensors));

    ThresholdDetector detector(defaultThresholds());
    std::ostringstream output;
    Logger logger(output);
    StatsCollector stats;

    std::chrono::steady_clock::time_point start{std::chrono::milliseconds(0)};
    ManualClock clock(start);

    std::chrono::milliseconds period(100);
    int cycleCount = 5;

    runMonitoringLoop(device, detector, logger, stats, clock, cycleCount, period);

    std::chrono::steady_clock::time_point expectedEnd = start + cycleCount * period;
    EXPECT_EQ(clock.now(), expectedEnd);
}

TEST(MonitorLoop, RecordsOneStatusPerSensorPerCycle) {
    std::vector<std::unique_ptr<ISensor>> sensors;
    sensors.push_back(std::make_unique<Sensor>(SensorType::Temperature, temperatureSpec(), 42));
    sensors.push_back(std::make_unique<Sensor>(SensorType::Voltage, voltageSpec(), 7));
    Device device(1, std::move(sensors));

    ThresholdDetector detector(defaultThresholds());
    std::ostringstream output;
    Logger logger(output);
    StatsCollector stats;

    ManualClock clock(std::chrono::steady_clock::time_point{});

    runMonitoringLoop(device, detector, logger, stats, clock, 3, std::chrono::milliseconds(100));

    EXPECT_EQ(stats.totalCount(), 6u);
}
