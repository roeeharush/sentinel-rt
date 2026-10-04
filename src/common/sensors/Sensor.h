#pragma once

#include "common/core/SensorType.h"
#include "common/sensors/ISensor.h"
#include <cstdint>
#include <random>

struct SensorSpec {
    double baseline;
    double noiseStdDev;
    double min;
    double max;
};

SensorSpec temperatureSpec();
SensorSpec pressureSpec();
SensorSpec voltageSpec();

class Sensor final : public ISensor {
  public:
    Sensor(SensorType type, SensorSpec spec, std::uint32_t seed);

    SensorType type() const override;
    double read() override;


private:
    SensorType type_;
    SensorSpec spec_;
    std::mt19937 rng_;
    std::normal_distribution<double> noise_;
};
