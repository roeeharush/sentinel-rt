#include <gtest/gtest.h>
#include "common/Codec.h"

#include <chrono>
#include <string>
#include <vector>

namespace {

std::vector<std::uint8_t> toBytes(const std::string& text) {
    return std::vector<std::uint8_t>(text.begin(), text.end());
}

}

TEST(Codec, RoundTripPreservesFields) {
    Measurement original;
    original.deviceId = 42;
    original.sensorType = SensorType::Temperature;
    original.timestamp = std::chrono::steady_clock::now();
    original.value = 73.5;

    std::vector<std::uint8_t> encoded = encodeMeasurement(original);
    std::optional<Measurement> decoded = decodeMeasurement(encoded);

    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->deviceId, original.deviceId);
    EXPECT_EQ(decoded->sensorType, original.sensorType);
    EXPECT_DOUBLE_EQ(decoded->value, original.value);

    auto diff = decoded->timestamp - original.timestamp;
    EXPECT_LE(std::chrono::abs(diff), std::chrono::seconds(2));
}

TEST(Codec, RoundTripWorksForEachSensorType) {
    Measurement pressure;
    pressure.deviceId = 1;
    pressure.sensorType = SensorType::Pressure;
    pressure.timestamp = std::chrono::steady_clock::now();
    pressure.value = 101.3;

    Measurement voltage;
    voltage.deviceId = 2;
    voltage.sensorType = SensorType::Voltage;
    voltage.timestamp = std::chrono::steady_clock::now();
    voltage.value = 24.0;

    std::optional<Measurement> decodedPressure = decodeMeasurement(encodeMeasurement(pressure));
    std::optional<Measurement> decodedVoltage = decodeMeasurement(encodeMeasurement(voltage));

    ASSERT_TRUE(decodedPressure.has_value());
    ASSERT_TRUE(decodedVoltage.has_value());
    EXPECT_EQ(decodedPressure->sensorType, SensorType::Pressure);
    EXPECT_EQ(decodedVoltage->sensorType, SensorType::Voltage);
}

TEST(Codec, RejectsWrongFieldCount) {
    EXPECT_FALSE(decodeMeasurement(toBytes("17|TEMP|123")).has_value());
}

TEST(Codec, RejectsUnknownSensorTypeCode) {
    EXPECT_FALSE(decodeMeasurement(toBytes("17|FOO|123|45.6")).has_value());
}

TEST(Codec, RejectsNonNumericDeviceId) {
    EXPECT_FALSE(decodeMeasurement(toBytes("abc|TEMP|123|45.6")).has_value());
}

TEST(Codec, RejectsNonNumericValue) {
    EXPECT_FALSE(decodeMeasurement(toBytes("17|TEMP|123|notanumber")).has_value());
}
