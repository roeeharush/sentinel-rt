#include "common/detection/ThresholdDetector.h"
#include <cmath>

const ThresholdSpec& ThresholdTable::forType(SensorType type) const {
    switch (type) {
        case SensorType::Temperature:
          return temperature;
        case SensorType::Pressure:
          return pressure;
        case SensorType::Voltage:
          return voltage;
    }
    return voltage;
}

ThresholdTable defaultThresholds(){
    return ThresholdTable{
        ThresholdSpec{-20.0, 0.0, 70.0, 85.0},
        ThresholdSpec{50.0, 80.0, 130.0, 160.0},
        ThresholdSpec{18.0, 21.0, 27.0, 30.0}
    };
}

ThresholdDetector::ThresholdDetector(ThresholdTable table)
    : table_(table) {
}

Status ThresholdDetector::evaluate(const Measurement& measurement) const {
    if (std::isnan(measurement.value)) {
        return Status::Invalid;
    }
    const ThresholdSpec& spec = table_.forType(measurement.sensorType);
    double value = measurement.value;

    if (value <= spec.criticalLow || value >= spec.criticalHigh) {
        return Status::Critical;
    }
    if (value <= spec.warningLow || value >= spec.warningHigh) {
        return Status::Warning;
    }
    return Status::Ok;
}