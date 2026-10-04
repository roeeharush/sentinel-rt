#include <gtest/gtest.h>
#include "common/sensors/Sensor.h"

TEST(Sensor, SameSeedProducesIdenticalFirstReading) {
    Sensor a(SensorType::Temperature, temperatureSpec(), 42);
    Sensor b(SensorType::Temperature, temperatureSpec(), 42);

    EXPECT_EQ(a.read(), b.read());
}

TEST(Sensor, DifferentSeedsProduceDifferentReadings) {
    Sensor a(SensorType::Temperature, temperatureSpec(), 1);
    Sensor b(SensorType::Temperature, temperatureSpec(), 2);

    EXPECT_NE(a.read(), b.read());
}