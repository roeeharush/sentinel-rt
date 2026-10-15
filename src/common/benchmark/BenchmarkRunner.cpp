#include "common/benchmark/BenchmarkRunner.h"
#include "common/benchmark/RatePacer.h"
#include "common/detection/MeasurementJudge.h"
#include "common/detection/TimeoutMonitor.h"
#include "common/infra/LatencyHistogram.h"
#include "common/infra/Logger.h"
#include "common/infra/StatsCollector.h"
#include "common/pipeline/BoundedQueue.h"
#include "common/pipeline/MeasurementProcessor.h"
#include "common/pipeline/WorkerPool.h"
#include "common/sensors/DeviceFleet.h"
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <functional>

namespace {

    constexpr std::size_t kQueueCapacity = 1024;
    constexpr std::chrono::milliseconds kTimeoutThreshold(2000);
    constexpr std::chrono::milliseconds kSampleInterval(1);
    constexpr std::chrono::microseconds kRateWait(100);

    struct FeederStats {
        std::uint64_t sent = 0;
        std::uint64_t maxLag = 0;
    };

    void feedMax(BoundedQueue<Measurement>& queue, std::vector<Device>& fleet,const std::atomic<bool>& stop, FeederStats& stats) {
        std::size_t next = 0;

        while (!stop.load()) {
            std::vector<Measurement> measurements = fleet[next].sample(std::chrono::steady_clock::now());
            for (const Measurement& measurement : measurements) {
                queue.push(measurement);
            }

            stats.sent += measurements.size();
            next = (next + 1) % fleet.size();
        }
    }

    void feedRate(BoundedQueue<Measurement>& queue, std::vector<Device>& fleet,const std::atomic<bool>& stop, std::uint64_t targetRate, FeederStats& stats) {
        RatePacer pacer(targetRate);
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        std::size_t next = 0;

        while (!stop.load()) {
            std::uint64_t scheduled = pacer.scheduledCount(std::chrono::steady_clock::now() - start);

            if (scheduled > stats.sent) {
                stats.maxLag = std::max(stats.maxLag, scheduled - stats.sent);
            }

            while (stats.sent < scheduled && !stop.load()) {
                std::vector<Measurement> measurements = fleet[next].sample(std::chrono::steady_clock::now());
                for (const Measurement& measurement : measurements) {
                    queue.push(measurement);
                }

                stats.sent += measurements.size();
                next = (next + 1) % fleet.size();
            }

            std::this_thread::sleep_for(kRateWait);
        }
    }

    LatencySummary summarize(const LatencyHistogram& histogram) {
        LatencySummary summary;
        summary.p50Us = histogram.percentileMicros(50.0);
        summary.p95Us = histogram.percentileMicros(95.0);
        summary.p99Us = histogram.percentileMicros(99.0);
        summary.maxUs = histogram.percentileMicros(100.0);
        summary.overflow = histogram.overflowCount();
        return summary;
    }
}

BenchmarkResult runBenchmark(const BenchmarkConfig& config)
{
    std::ostream nullStream(nullptr);
    Logger logger(nullStream);
    StatsCollector stats;
    MeasurementJudge judge(defaultThresholds(), kDefaultWindowCapacity);
    TimeoutMonitor timeoutMonitor(kTimeoutThreshold);
    MeasurementProcessor processor(judge, stats, timeoutMonitor, logger);
    BoundedQueue<Measurement> queue(kQueueCapacity);
    LatencyHistogram totalLatency;
    LatencyHistogram serviceLatency;
    std::atomic<std::uint64_t> calibrationCount{0};
    std::vector<Device> fleet = makeDeviceFleet(static_cast<std::size_t>(config.devices));

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    std::function<void(Measurement)> handler;
    if (config.mode == BenchmarkMode::Calibrate)
    {
        handler = [&calibrationCount](Measurement) {
            calibrationCount.fetch_add(1, std::memory_order_relaxed);
        };
    }

    else {
        handler = [&](Measurement measurement) {
            std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
            processor.process(measurement, begin);
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

            serviceLatency.record(end - begin);
            totalLatency.record(end - measurement.timestamp);
        };
    }

    WorkerPool<Measurement> pool(queue, logger, handler, static_cast<std::size_t>(config.workers));

    std::atomic<bool> stop{false};
    FeederStats feederStats;

    std::thread feeder([&]() {
        if (config.mode == BenchmarkMode::Rate) {
            feedRate(queue, fleet, stop, config.targetRate, feederStats);
        } else {
            feedMax(queue, fleet, stop, feederStats);
        }
    });

    std::uint64_t queueSizeSum = 0;
    std::uint64_t queueSamples = 0;
    std::uint64_t queueMax = 0;
    std::chrono::steady_clock::time_point deadline = start + std::chrono::milliseconds(static_cast<std::int64_t>(config.durationMs));

    while (std::chrono::steady_clock::now() < deadline) {
        std::uint64_t size = queue.size();
        queueSizeSum += size;
        ++queueSamples;
        queueMax = std::max(queueMax, size);
        std::this_thread::sleep_for(kSampleInterval);
    }

    stop.store(true);
    feeder.join();
    pool.stop();

    std::chrono::steady_clock::time_point finish = std::chrono::steady_clock::now();
    std::int64_t elapsedNanos = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();

    BenchmarkResult result;
    result.mode = config.mode;
    result.workers = config.workers;
    result.devices = config.devices;
    result.targetRate = config.targetRate;
    result.durationMs = static_cast<std::uint64_t>(elapsedNanos / 1'000'000);

    result.sent = feederStats.sent;
    if (config.mode == BenchmarkMode::Calibrate){
        result.processed = calibrationCount.load();
    }
    else {
        result.processed = stats.totalCount();
    }
    result.maxLag = feederStats.maxLag;
    result.queueMax = queueMax;
    if (queueSamples > 0) {
        result.queueAvg = queueSizeSum / queueSamples;
    }
    if (elapsedNanos > 0) {
        result.throughputPerSec = result.processed * 1'000'000'000ULL / static_cast<std::uint64_t>(elapsedNanos);
    }

    result.total = summarize(totalLatency);
    result.service = summarize(serviceLatency);

    return result;
}
