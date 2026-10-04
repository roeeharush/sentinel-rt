#include "common/sensors/FaultSchedule.h"

FaultSchedule::FaultSchedule(std::optional<TimedFault> fault)
    : fault_(fault) {}

std::optional<FaultType> FaultSchedule::due(std::chrono::milliseconds elapsed) {
    if (!fault_ || activated_) {
        return std::nullopt;
    }

    if (elapsed < fault_->activateAfter) {
        return std::nullopt;
    }

    activated_ = true;
    return fault_->type;
}
