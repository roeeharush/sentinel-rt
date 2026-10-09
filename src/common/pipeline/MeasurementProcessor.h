#pragma once

#include "common/core/Measurement.h"
#include "common/core/Status.h"
#include "common/detection/MeasurementJudge.h"
#include "common/detection/TimeoutMonitor.h"
#include "common/infra/Logger.h"
#include "common/infra/StatsCollector.h"
#include <chrono>

class MeasurementProcessor {
public:
    MeasurementProcessor(MeasurementJudge& judge, StatsCollector& stats,TimeoutMonitor& timeoutMonitor, Logger& logger);

    Status process(const Measurement& measurement, std::chrono::steady_clock::time_point now);

private:
    MeasurementJudge& judge_;
    StatsCollector& stats_;
    TimeoutMonitor& timeoutMonitor_;
    Logger& logger_;
};
