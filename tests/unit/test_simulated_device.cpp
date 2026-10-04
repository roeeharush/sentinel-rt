#include <gtest/gtest.h>
#include "common/sensors/SimulatedDevice.h"

#include <chrono>

TEST(SimulatedDevice, SampleGivesOneMeasurementPerSensorInOrder) {
    SimulatedDevice simulated = makeSimulatedDevice();

    std::vector<Measurement> measurements = simulated.device.sample(std::chrono::steady_clock::now());

    ASSERT_EQ(measurements.size(), 3u);
    EXPECT_EQ(measurements[0].sensorType, SensorType::Temperature);
    EXPECT_EQ(measurements[1].sensorType, SensorType::Pressure);
    EXPECT_EQ(measurements[2].sensorType, SensorType::Voltage);
    EXPECT_EQ(measurements[0].deviceId, 1u);
}

TEST(SimulatedDevice, TwoDevicesFromTheFactoryProduceIdenticalReadings) {
    SimulatedDevice first = makeSimulatedDevice();
    SimulatedDevice second = makeSimulatedDevice();
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    for (int i = 0; i < 5; ++i) {
        std::vector<Measurement> a = first.device.sample(now);
        std::vector<Measurement> b = second.device.sample(now);

        ASSERT_EQ(a.size(), b.size());
        for (std::size_t sensor = 0; sensor < a.size(); ++sensor) {
            EXPECT_DOUBLE_EQ(a[sensor].value, b[sensor].value);
        }
    }
}

TEST(SimulatedDevice, TemperatureHandleControlsOnlyTheTemperatureSensor) {
    SimulatedDevice faulty = makeSimulatedDevice();
    SimulatedDevice healthy = makeSimulatedDevice();
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    faulty.temperature->setFault(FaultType::Spike);

    std::vector<Measurement> a = faulty.device.sample(now);
    std::vector<Measurement> b = healthy.device.sample(now);

    ASSERT_EQ(a.size(), 3u);
    ASSERT_EQ(b.size(), 3u);
    EXPECT_DOUBLE_EQ(a[0].value, b[0].value * 5.0);
    EXPECT_DOUBLE_EQ(a[1].value, b[1].value);
    EXPECT_DOUBLE_EQ(a[2].value, b[2].value);
}
