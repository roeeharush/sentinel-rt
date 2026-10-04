#include <gtest/gtest.h>
#include "common/detection/MeasurementJudge.h"

#include <chrono>
#include <limits>

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

TEST(MeasurementJudge, HealthyMeasurementIsOk) {
    MeasurementJudge judge(defaultThresholds(), 4);

    EXPECT_EQ(judge.judge(makeMeasurement(1, SensorType::Temperature, 55.0)), Status::Ok);
}

TEST(MeasurementJudge, OutOfRangeValueIsCriticalEvenBeforeWindowIsFull) {
    MeasurementJudge judge(defaultThresholds(), 4);

    EXPECT_EQ(judge.judge(makeMeasurement(1, SensorType::Temperature, 90.0)), Status::Critical);
}

TEST(MeasurementJudge, ThresholdWarningSurvivesWhenWindowSaysOk) {
    MeasurementJudge judge(defaultThresholds(), 4);

    EXPECT_EQ(judge.judge(makeMeasurement(1, SensorType::Temperature, 75.0)), Status::Warning);
}

TEST(MeasurementJudge, FrozenHealthyValueBecomesCriticalOnlyThroughTheWindow) {
    MeasurementJudge judge(defaultThresholds(), 4);

    EXPECT_EQ(judge.judge(makeMeasurement(1, SensorType::Temperature, 55.0)), Status::Ok);
    EXPECT_EQ(judge.judge(makeMeasurement(1, SensorType::Temperature, 55.0)), Status::Ok);
    EXPECT_EQ(judge.judge(makeMeasurement(1, SensorType::Temperature, 55.0)), Status::Ok);
    EXPECT_EQ(judge.judge(makeMeasurement(1, SensorType::Temperature, 55.0)), Status::Critical);
}

TEST(MeasurementJudge, InvalidValueBeatsAnOkWindow) {
    MeasurementJudge judge(defaultThresholds(), 4);

    double nan = std::numeric_limits<double>::quiet_NaN();
    EXPECT_EQ(judge.judge(makeMeasurement(1, SensorType::Temperature, nan)), Status::Invalid);
}
