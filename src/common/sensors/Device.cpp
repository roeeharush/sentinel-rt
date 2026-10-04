#include "common/sensors/Device.h"
#include <utility>

Device::Device(std::uint32_t deviceId, std::vector<std::unique_ptr<ISensor>> sensors)
    : deviceId_(deviceId), sensors_(std::move(sensors)) {}

std::uint32_t Device::id() const {
  return deviceId_;
  }

std::size_t Device::sensorCount() const {
  return sensors_.size();
  }

 std::vector<Measurement>Device::sample(std::chrono::steady_clock::time_point now) {
   std::vector<Measurement> result;
   for (const std::unique_ptr<ISensor>& sensor : sensors_) {
       result.push_back(Measurement{deviceId_, sensor->type(), now, sensor->read()});
    }
    return result;
 }