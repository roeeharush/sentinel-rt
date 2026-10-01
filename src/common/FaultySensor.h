#pragma once

#include "common/FaultType.h"
#include "common/ISensor.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <random>

class FaultySensor final : public ISensor {
public:
    explicit FaultySensor(std::unique_ptr<ISensor> inner, std::uint32_t seed);

    SensorType type() const override;
    double read() override;

    void setFault(FaultType fault);

private:
    std::unique_ptr<ISensor> inner_;
    FaultType currentFault_;
    std::optional<double> stuckValue_;
    double driftOffset_;
    std::mt19937 rng_;
    std::normal_distribution<double> noise_;
};