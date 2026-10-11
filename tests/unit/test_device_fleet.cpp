#include <gtest/gtest.h>
#include "common/sensors/DeviceFleet.h"
#include "common/sensors/SimulatedDevice.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

Clock::time_point someTime() {
    return Clock::time_point{} + std::chrono::seconds(100);
}

}

TEST(DeviceFleet, ReturnsTheRequestedNumberOfDevices) {
    EXPECT_EQ(makeDeviceFleet(0).size(), 0u);
    EXPECT_EQ(makeDeviceFleet(1).size(), 1u);
    EXPECT_EQ(makeDeviceFleet(10).size(), 10u);
}

TEST(DeviceFleet, IdsAreOneToNInOrder) {
    std::vector<Device> fleet = makeDeviceFleet(5);

    ASSERT_EQ(fleet.size(), 5u);
    for (std::size_t i = 0; i < fleet.size(); ++i) {
        EXPECT_EQ(fleet[i].id(), static_cast<std::uint32_t>(i + 1));
    }
}

TEST(DeviceFleet, EveryDeviceHasTemperaturePressureAndVoltageInOrder) {
    std::vector<Device> fleet = makeDeviceFleet(3);

    ASSERT_EQ(fleet.size(), 3u);
    for (Device& device : fleet) {
        std::vector<Measurement> measurements = device.sample(someTime());

        ASSERT_EQ(measurements.size(), 3u);
        EXPECT_EQ(measurements[0].sensorType, SensorType::Temperature);
        EXPECT_EQ(measurements[1].sensorType, SensorType::Pressure);
        EXPECT_EQ(measurements[2].sensorType, SensorType::Voltage);
        EXPECT_EQ(measurements[0].deviceId, device.id());
    }
}

TEST(DeviceFleet, TwoFleetsOfTheSameSizeProduceIdenticalReadings) {
    std::vector<Device> first = makeDeviceFleet(4);
    std::vector<Device> second = makeDeviceFleet(4);

    ASSERT_EQ(first.size(), 4u);
    ASSERT_EQ(second.size(), 4u);
    for (int round = 0; round < 5; ++round) {
        for (std::size_t d = 0; d < first.size(); ++d) {
            std::vector<Measurement> a = first[d].sample(someTime());
            std::vector<Measurement> b = second[d].sample(someTime());

            ASSERT_EQ(a.size(), b.size());
            for (std::size_t s = 0; s < a.size(); ++s) {
                EXPECT_DOUBLE_EQ(a[s].value, b[s].value);
            }
        }
    }
}

TEST(DeviceFleet, DifferentDevicesProduceDifferentReadings) {
    std::vector<Device> fleet = makeDeviceFleet(3);

    ASSERT_EQ(fleet.size(), 3u);
    double first = fleet[0].sample(someTime())[0].value;
    double second = fleet[1].sample(someTime())[0].value;
    double third = fleet[2].sample(someTime())[0].value;

    EXPECT_NE(first, second);
    EXPECT_NE(second, third);
    EXPECT_NE(first, third);
}

TEST(DeviceFleet, TheFirstDeviceMatchesTheScenarioDevice) {
    std::vector<Device> fleet = makeDeviceFleet(1);
    SimulatedDevice scenario = makeSimulatedDevice();

    ASSERT_EQ(fleet.size(), 1u);
    for (int round = 0; round < 5; ++round) {
        std::vector<Measurement> a = fleet[0].sample(someTime());
        std::vector<Measurement> b = scenario.device.sample(someTime());

        ASSERT_EQ(a.size(), b.size());
        for (std::size_t s = 0; s < a.size(); ++s) {
            EXPECT_DOUBLE_EQ(a[s].value, b[s].value);
        }
    }
}
