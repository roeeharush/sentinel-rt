#include <gtest/gtest.h>
#include "common/detection/SlidingWindowDetector.h"

#include <chrono>

namespace {

Measurement makeMeasurement(std::uint32_t deviceId, SensorType type, double value) {
    Measurement measurement;
    measurement.deviceId = deviceId;
    measurement.sensorType = type;
    measurement.timestamp = std::chrono::steady_clock::now();
    measurement.value = value;
    return measurement;
}

}

TEST(SlidingWindowDetector, FirstMeasurementsBeforeWindowFullReturnOk) {
    SlidingWindowDetector detector(4);

    EXPECT_EQ(detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0)), Status::Ok);
    EXPECT_EQ(detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0)), Status::Ok);
    EXPECT_EQ(detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0)), Status::Ok);
}

TEST(SlidingWindowDetector, RepeatedIdenticalValuesReturnCriticalOnceWindowFull) {
    SlidingWindowDetector detector(4);

    detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0));
    detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0));
    detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0));
    Status result = detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0));

    EXPECT_EQ(result, Status::Critical);
}

TEST(SlidingWindowDetector, GradualIncreaseReturnsWarningOnceWindowFull) {
    SlidingWindowDetector detector(4);

    detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0));
    detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.1));
    detector.evaluate(makeMeasurement(1, SensorType::Temperature, 59.0));
    Status result = detector.evaluate(makeMeasurement(1, SensorType::Temperature, 59.2));

    EXPECT_EQ(result, Status::Warning);
}

TEST(SlidingWindowDetector, HealthyVaryingValuesReturnOk) {
    SlidingWindowDetector detector(4);

    detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0));
    detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.1));
    detector.evaluate(makeMeasurement(1, SensorType::Temperature, 54.9));
    Status result = detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.2));

    EXPECT_EQ(result, Status::Ok);
}

TEST(SlidingWindowDetector, DifferentDevicesHaveIndependentWindows) {
    SlidingWindowDetector detector(4);

    detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0));
    detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0));
    detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0));
    Status stuckDeviceResult = detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0));

    detector.evaluate(makeMeasurement(2, SensorType::Temperature, 55.0));
    detector.evaluate(makeMeasurement(2, SensorType::Temperature, 55.1));
    detector.evaluate(makeMeasurement(2, SensorType::Temperature, 54.9));
    Status healthyDeviceResult = detector.evaluate(makeMeasurement(2, SensorType::Temperature, 55.2));

    EXPECT_EQ(stuckDeviceResult, Status::Critical);
    EXPECT_EQ(healthyDeviceResult, Status::Ok);
}

TEST(SlidingWindowDetector, DifferentSensorTypesOnSameDeviceHaveIndependentWindows) {
    SlidingWindowDetector detector(4);

    detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0));
    detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0));
    detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0));
    Status temperatureResult = detector.evaluate(makeMeasurement(1, SensorType::Temperature, 55.0));

    detector.evaluate(makeMeasurement(1, SensorType::Pressure, 101.3));
    detector.evaluate(makeMeasurement(1, SensorType::Pressure, 101.4));
    detector.evaluate(makeMeasurement(1, SensorType::Pressure, 101.2));
    Status pressureResult = detector.evaluate(makeMeasurement(1, SensorType::Pressure, 101.5));

    EXPECT_EQ(temperatureResult, Status::Critical);
    EXPECT_EQ(pressureResult, Status::Ok);
}
