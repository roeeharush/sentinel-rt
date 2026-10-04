#include "common/network/Codec.h"
#include <string>
#include <charconv>

namespace {

    const char* sensorTypeToCode(SensorType type) {
        switch (type) {
            case SensorType::Temperature:
                return "TEMP";
            case SensorType::Pressure:
                return "PRES";
            case SensorType::Voltage:
                return "VOLT";
        }
        return "";
    }

    std::optional<SensorType> codeToSensorType(const std::string& code) {
        if (code == "TEMP") {
            return SensorType::Temperature;
        }
        if (code == "PRES") {
            return SensorType::Pressure;
        }
        if (code == "VOLT") {
            return SensorType::Voltage;
        }
        return std::nullopt;
    }

    std::int64_t wallClockSeconds(std::chrono::steady_clock::time_point sampledAt) {
        auto elapsed = std::chrono::steady_clock::now() - sampledAt;
        auto wallTime = std::chrono::system_clock::now() - elapsed;
        return std::chrono::duration_cast<std::chrono::seconds>(wallTime.time_since_epoch()).count();
    }

    std::chrono::steady_clock::time_point steadyClockFromWallSeconds(std::int64_t seconds) {
        auto wallTime = std::chrono::system_clock::time_point(std::chrono::seconds(seconds));
        auto elapsed = std::chrono::system_clock::now() - wallTime;
        return std::chrono::steady_clock::now() - elapsed;
    }
}

std::vector<std::uint8_t> encodeMeasurement(const Measurement& measurement) {
    char numberBuffer[32];
    std::string text;

    auto idResult = std::to_chars(numberBuffer, numberBuffer + sizeof(numberBuffer), measurement.deviceId);
    text.append(numberBuffer, idResult.ptr);
    text += '|';

    text += sensorTypeToCode(measurement.sensorType);
    text += '|';

    std::int64_t seconds = wallClockSeconds(measurement.timestamp);
    auto timeResult = std::to_chars(numberBuffer, numberBuffer + sizeof(numberBuffer), seconds);
    text.append(numberBuffer, timeResult.ptr);
    text += '|';

    auto valueResult = std::to_chars(numberBuffer, numberBuffer + sizeof(numberBuffer), measurement.value);
    text.append(numberBuffer, valueResult.ptr);

    return std::vector<std::uint8_t>(text.begin(), text.end());
}

std::optional<Measurement> decodeMeasurement(const std::vector<std::uint8_t>& bytes) {
    std::string text(bytes.begin(), bytes.end());

    std::vector<std::string> fields;
    std::size_t start = 0;
    while (true) {
        std::size_t separator = text.find('|', start);
        if (separator == std::string::npos) {
            fields.push_back(text.substr(start));
            break;
        }
        fields.push_back(text.substr(start, separator - start));
        start = separator + 1;
    }

    if (fields.size() != 4) {
        return std::nullopt;
    }

    std::uint32_t deviceId;
    auto idResult = std::from_chars(fields[0].data(), fields[0].data() + fields[0].size(), deviceId);
    if (idResult.ec != std::errc{}) {
        return std::nullopt;
    }

    std::optional<SensorType> sensorType = codeToSensorType(fields[1]);
    if (!sensorType) {
        return std::nullopt;
    }

    std::int64_t seconds;
    auto timeResult = std::from_chars(fields[2].data(), fields[2].data() + fields[2].size(), seconds);
    if (timeResult.ec != std::errc{}) {
        return std::nullopt;
    }

    double value;
    auto valueResult = std::from_chars(fields[3].data(), fields[3].data() + fields[3].size(), value);
    if (valueResult.ec != std::errc{}) {
        return std::nullopt;
    }

    Measurement measurement;
    measurement.deviceId = deviceId;
    measurement.sensorType = *sensorType;
    measurement.timestamp = steadyClockFromWallSeconds(seconds);
    measurement.value = value;

    return measurement;
}
