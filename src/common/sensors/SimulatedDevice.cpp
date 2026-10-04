#include "common/sensors/SimulatedDevice.h"
#include "common/core/SensorType.h"
#include "common/sensors/Sensor.h"
#include <memory>
#include <utility>
#include <vector>

SimulatedDevice makeSimulatedDevice() {
    auto temperatureSensor = std::make_unique<FaultySensor>(std::make_unique<Sensor>(SensorType::Temperature, temperatureSpec(), 42), 123);
    FaultySensor* temperature = temperatureSensor.get();

    std::vector<std::unique_ptr<ISensor>> sensors;
    sensors.push_back(std::move(temperatureSensor));
    sensors.push_back(std::make_unique<Sensor>(SensorType::Pressure, pressureSpec(), 7));
    sensors.push_back(std::make_unique<Sensor>(SensorType::Voltage, voltageSpec(), 99));

    return SimulatedDevice{Device(1, std::move(sensors)), temperature};
}
