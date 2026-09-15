#include <gtest/gtest.h>
#include "common/Device.h"
#include "common/Sensor.h"
#include <memory>

TEST(Device, SampleProducesOneMeasurementPerSensor) {
    std::vector<std::unique_ptr<ISensor>> sensors;
    sensors.push_back(std::make_unique<Sensor>(SensorType::Temperature, temperatureSpec(), 42));
    sensors.push_back(std::make_unique<Sensor>(SensorType::Voltage, voltageSpec(), 7));
    Device device(1, std::move(sensors));

    std::vector<Measurement> measurements = device.sample(std::chrono::steady_clock::time_point{});

    EXPECT_EQ(measurements.size(), 2u);
    EXPECT_EQ(measurements[0].sensorType, SensorType::Temperature);
    EXPECT_EQ(measurements[1].sensorType, SensorType::Voltage);
}