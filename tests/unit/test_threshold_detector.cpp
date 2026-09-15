#include <gtest/gtest.h>
#include "common/ThresholdDetector.h"
#include <chrono>
#include <limits>

TEST(ThresholdDetector, NormalValueIsOk) {
    ThresholdDetector detector(defaultThresholds());
    Measurement m{1, SensorType::Temperature, std::chrono::steady_clock::time_point{}, 55.0};

    EXPECT_EQ(detector.evaluate(m), Status::Ok);
}

TEST(ThresholdDetector, ThresholdValueItselfBelongsToTheMoreSevereBand) {
    ThresholdDetector detector(defaultThresholds());
    Measurement m{1, SensorType::Temperature, std::chrono::steady_clock::time_point{}, 70.0};

    EXPECT_EQ(detector.evaluate(m), Status::Warning);
}

TEST(ThresholdDetector, NonFiniteValueIsInvalid) {
    ThresholdDetector detector(defaultThresholds());
    Measurement m{1, SensorType::Temperature, std::chrono::steady_clock::time_point{},
                  std::numeric_limits<double>::quiet_NaN()};

    EXPECT_EQ(detector.evaluate(m), Status::Invalid);
}