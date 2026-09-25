#include <gtest/gtest.h>
#include "common/MonitorLoop.h"
#include "common/Device.h"
#include "common/Sensor.h"
#include "common/BoundedQueue.h"
#include "common/ManualClock.h"
#include <memory>

TEST(MonitorLoop, AdvancesClockByExactlyCycleCountTimesPeriod) {
    std::vector<std::unique_ptr<ISensor>> sensors;
    sensors.push_back(std::make_unique<Sensor>(SensorType::Temperature, temperatureSpec(), 42));
    Device device(1, std::move(sensors));

    BoundedQueue<Measurement> queue(100);

    std::chrono::steady_clock::time_point start{std::chrono::milliseconds(0)};
    ManualClock clock(start);

    std::chrono::milliseconds period(100);
    int cycleCount = 5;

    runMonitoringLoop(device, queue, clock, cycleCount, period, true);

    std::chrono::steady_clock::time_point expectedEnd = start + cycleCount * period;
    EXPECT_EQ(clock.now(), expectedEnd);
}

TEST(MonitorLoop, PushesOneMeasurementPerSensorPerCycleToQueue) {
    std::vector<std::unique_ptr<ISensor>> sensors;
    sensors.push_back(std::make_unique<Sensor>(SensorType::Temperature, temperatureSpec(), 42));
    sensors.push_back(std::make_unique<Sensor>(SensorType::Voltage, voltageSpec(), 7));
    Device device(1, std::move(sensors));

    BoundedQueue<Measurement> queue(100);
    ManualClock clock(std::chrono::steady_clock::time_point{});

    runMonitoringLoop(device, queue, clock, 3, std::chrono::milliseconds(100), true);

    EXPECT_EQ(queue.size(), 6u);
}
