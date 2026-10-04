#pragma once

#include "common/core/Measurement.h"
#include "common/core/Status.h"
#include "common/detection/SlidingWindowDetector.h"
#include "common/detection/ThresholdDetector.h"
#include <cstddef>

class MeasurementJudge {
public:
    MeasurementJudge(ThresholdTable thresholds, std::size_t windowCapacity);

    Status judge(const Measurement& measurement);

private:
    ThresholdDetector thresholdDetector_;
    SlidingWindowDetector windowDetector_;
};
