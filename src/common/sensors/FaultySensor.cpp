#include "common/sensors/FaultySensor.h"
#include <utility>
#include <limits>

namespace {
    constexpr double kSpikeMultiplier = 5.0;
    constexpr double kDropMultiplier = 0.1;
    constexpr double kDriftStepPerRead = 1.0;
    constexpr double kNoiseRelativeStdDev = 0.2;
}

FaultySensor::FaultySensor(std::unique_ptr<ISensor> inner, std::uint32_t seed)
    : inner_(std::move(inner)), currentFault_(FaultType::None), driftOffset_(0.0),
      rng_(seed), noise_(0.0, kNoiseRelativeStdDev) {}

SensorType FaultySensor::type() const {
    return inner_->type();
}

void FaultySensor::setFault(FaultType fault) {
    currentFault_ = fault;
    stuckValue_.reset();
    driftOffset_ = 0.0;
}

double FaultySensor::read() {
    double value = inner_->read();

    switch (currentFault_) {
        case FaultType::None:
            return value;
        case FaultType::Spike:
            return value * kSpikeMultiplier;
        case FaultType::Drop:
            return value * kDropMultiplier;
        case FaultType::Stuck:
            if (!stuckValue_.has_value())
              stuckValue_ = value;
            return *stuckValue_;
        case FaultType::Drift:
            driftOffset_ += kDriftStepPerRead;
            return value + driftOffset_;
        case FaultType::Noise:
          return value * (1.0 + noise_(rng_));
        case FaultType::Invalid:
            return std::numeric_limits<double>::quiet_NaN();


    }

    return value;
}