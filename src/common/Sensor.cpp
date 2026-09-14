#include "common/Sensor.h"

SensorSpec temperatureSpec() {
    return SensorSpec{55.0, 2.0, -40.0, 150.0};
}

SensorSpec pressureSpec() {
    return SensorSpec{101.3, 1.5, 0.0, 500.0};
}

SensorSpec voltageSpec() {
    return SensorSpec{24.0, 0.3, 0.0, 60.0};
}

Sensor::Sensor(SensorType type, SensorSpec spec, std::uint32_t seed)
    : type_(type), spec_(spec), rng_(seed), noise_(spec.baseline, spec.noiseStdDev){}

SensorType Sensor::type() const {
  return type_;
}

double Sensor::read() {
    double value = noise_(rng_);
    if (value < spec_.min) {
        value = spec_.min;
    }
    if (value > spec_.max) {
        value = spec_.max;
    }
    return value;
}