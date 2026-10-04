#include "common/detection/MeasurementJudge.h"

namespace {

    Status worse(Status a, Status b) {
        return static_cast<int>(b) > static_cast<int>(a) ? b : a;
    }

}

MeasurementJudge::MeasurementJudge(ThresholdTable thresholds, std::size_t windowCapacity)
    : thresholdDetector_(thresholds), windowDetector_(windowCapacity) {}

Status MeasurementJudge::judge(const Measurement& measurement) {
    Status thresholdStatus = thresholdDetector_.evaluate(measurement);
    Status windowStatus = windowDetector_.evaluate(measurement);
    return worse(thresholdStatus, windowStatus);
}
