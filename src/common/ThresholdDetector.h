#pragma once

#include "common/SensorType.h"
#include "common/Status.h"
#include "common/Measurement.h"

struct ThresholdSpec {
    double criticalLow;
    double warningLow;
    double warningHigh;
    double criticalHigh;
};

struct ThresholdTable {
    ThresholdSpec temperature;
    ThresholdSpec pressure;
    ThresholdSpec voltage;
    const ThresholdSpec& forType(SensorType type) const;
};

ThresholdTable defaultThresholds();

class ThresholdDetector {
public:
    explicit ThresholdDetector(ThresholdTable table);
    Status evaluate(const Measurement& measurement) const;

private:
    ThresholdTable table_;
};