#include "common/config/ScenarioRunner.h"
#include "common/detection/MeasurementJudge.h"
#include "common/infra/Logger.h"
#include "common/infra/StatsCollector.h"
#include "common/pipeline/BoundedQueue.h"
#include "common/pipeline/WorkerPool.h"
#include "common/sensors/FaultSchedule.h"
#include "common/sensors/SimulatedDevice.h"
#include <chrono>
#include <cstddef>

namespace {

    constexpr std::size_t kQueueCapacity = 1024;
    constexpr std::size_t kWorkerCount = 1;

}

ScenarioOutcome runScenario(const Scenario& scenario) {
    SimulatedDevice simulated = makeSimulatedDevice();
    FaultSchedule faultSchedule(scenario.config.fault);
    MeasurementJudge judge(defaultThresholds(), kDefaultWindowCapacity);
    StatsCollector stats;
    Logger logger;
    BoundedQueue<Measurement> queue(kQueueCapacity);

    WorkerPool<Measurement> pool(queue, logger, [&](Measurement measurement) {
        stats.record(judge.judge(measurement));
    }, kWorkerCount);

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::chrono::milliseconds period(scenario.config.periodMs);

    for (int cycle = 0; cycle < scenario.config.cycles; ++cycle) {
        std::chrono::milliseconds elapsed = cycle * period;

        if (auto fault = faultSchedule.due(elapsed)) {
            simulated.temperature->setFault(*fault);
        }

        for (const Measurement& measurement : simulated.device.sample(start + elapsed)) {
            queue.push(measurement);
        }
    }

    pool.stop();

    return ScenarioOutcome{
        stats.okCount(),
        stats.warningCount(),
        stats.criticalCount(),
        stats.invalidCount(),
        stats.totalCount(),
        evaluateExpectations(scenario.expectations, stats)};
}
