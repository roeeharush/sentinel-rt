#include "common/sensors/DeviceFleet.h"
#include "common/core/SensorType.h"
#include "common/sensors/Sensor.h"
#include <cstdint>
#include <memory>
#include <utility>

namespace
{
    constexpr std::uint32_t kTemperatureSeed = 42;
    constexpr std::uint32_t kPressureSeed = 7;
    constexpr std::uint32_t kVoltageSeed = 99;

    Device makeDevice(std::uint32_t id) {
        std::uint32_t offset = id - 1;

        std::vector<std::unique_ptr<ISensor>> sensors;
        sensors.push_back(std::make_unique<Sensor>(SensorType::Temperature, temperatureSpec(), kTemperatureSeed + offset));
        sensors.push_back(std::make_unique<Sensor>(SensorType::Pressure, pressureSpec(), kPressureSeed + offset));
        sensors.push_back(std::make_unique<Sensor>(SensorType::Voltage, voltageSpec(), kVoltageSeed + offset));

        return Device(id, std::move(sensors));
    }
}

    std::vector<Device> makeDeviceFleet(std::size_t count) {
        std::vector<Device> fleet;
        fleet.reserve(count);

        for (std::size_t i = 1; i <= count; ++i) {
            fleet.push_back(makeDevice(static_cast<std::uint32_t>(i)));
        }

        return fleet;
    }
