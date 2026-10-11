#include <gtest/gtest.h>
#include "common/benchmark/BenchmarkRunner.h"

#include <cstdint>
#include <optional>

namespace {

constexpr std::uint64_t kQuickDurationMs = 250;
constexpr std::uint64_t kQueueCapacity = 1024;

BenchmarkConfig quickConfig(BenchmarkMode mode) {
    BenchmarkConfig config;
    config.mode = mode;
    config.workers = 2;
    config.devices = 3;
    config.targetRate = (mode == BenchmarkMode::Rate) ? 2000 : 0;
    config.durationMs = kQuickDurationMs;
    return config;
}

void expectNotGreater(const std::optional<std::uint64_t>& smaller, const std::optional<std::uint64_t>& larger) {
    if (smaller.has_value() && larger.has_value()) {
        EXPECT_LE(*smaller, *larger);
    }
}

}

TEST(BenchmarkRunner, EverythingSentIsProcessedInMaxMode) {
    BenchmarkResult result = runBenchmark(quickConfig(BenchmarkMode::Max));

    ASSERT_GT(result.sent, 0u);
    EXPECT_EQ(result.processed, result.sent);
}

TEST(BenchmarkRunner, EverythingSentIsProcessedInRateMode) {
    BenchmarkResult result = runBenchmark(quickConfig(BenchmarkMode::Rate));

    ASSERT_GT(result.sent, 0u);
    EXPECT_EQ(result.processed, result.sent);
}

TEST(BenchmarkRunner, EverythingSentIsProcessedWithManyDevices) {
    BenchmarkConfig config = quickConfig(BenchmarkMode::Max);
    config.devices = 100;

    BenchmarkResult result = runBenchmark(config);

    ASSERT_GT(result.sent, 0u);
    EXPECT_EQ(result.processed, result.sent);
}

TEST(BenchmarkRunner, EverythingSentIsProcessedAcrossManyShortRuns) {
    BenchmarkConfig config = quickConfig(BenchmarkMode::Max);
    config.workers = 4;
    config.durationMs = 20;

    for (int run = 0; run < 40; ++run) {
        BenchmarkResult result = runBenchmark(config);

        ASSERT_GT(result.sent, 0u) << "run " << run;
        ASSERT_EQ(result.processed, result.sent) << "run " << run;
    }
}

TEST(BenchmarkRunner, TotalTimeIncludesWaitingInTheQueue) {
    BenchmarkConfig config = quickConfig(BenchmarkMode::Max);
    config.workers = 1;

    BenchmarkResult result = runBenchmark(config);

    ASSERT_GT(result.processed, 0u);
    ASSERT_TRUE(result.total.maxUs.has_value());
    ASSERT_TRUE(result.service.maxUs.has_value());
    EXPECT_GT(*result.total.maxUs, *result.service.maxUs);
}

TEST(BenchmarkRunner, StopsPromptlyEvenWhenTheRequestedRateIsFarAboveCapacity) {
    BenchmarkConfig config = quickConfig(BenchmarkMode::Rate);
    config.targetRate = 5'000'000'000;

    BenchmarkResult result = runBenchmark(config);

    ASSERT_GT(result.processed, 0u);
    EXPECT_GE(result.durationMs, kQuickDurationMs);
    EXPECT_LE(result.durationMs, kQuickDurationMs + 750);
}

TEST(BenchmarkRunner, TheConfigurationIsEchoedInTheResult) {
    BenchmarkConfig config = quickConfig(BenchmarkMode::Rate);
    config.workers = 3;
    config.devices = 5;
    config.targetRate = 1500;

    BenchmarkResult result = runBenchmark(config);

    ASSERT_GT(result.processed, 0u);
    EXPECT_EQ(result.mode, BenchmarkMode::Rate);
    EXPECT_EQ(result.workers, 3u);
    EXPECT_EQ(result.devices, 5u);
    EXPECT_EQ(result.targetRate, 1500u);
}

TEST(BenchmarkRunner, RateModeNeverSendsMoreThanTheScheduleAllows) {
    BenchmarkConfig config = quickConfig(BenchmarkMode::Rate);

    BenchmarkResult result = runBenchmark(config);

    ASSERT_GT(result.sent, 0u);
    std::uint64_t scheduled = config.targetRate * result.durationMs / 1000;
    std::uint64_t oneDeviceSample = 3;
    std::uint64_t roundingOfTheMeasuredDuration = config.targetRate / 1000 + 1;
    EXPECT_LE(result.sent, scheduled + oneDeviceSample + roundingOfTheMeasuredDuration);
}

TEST(BenchmarkRunner, RateModeSendsAtLeastHalfOfTheRequestedAmount) {
    BenchmarkConfig config = quickConfig(BenchmarkMode::Rate);

    BenchmarkResult result = runBenchmark(config);

    ASSERT_GT(result.sent, 0u);
    std::uint64_t expected = config.targetRate * config.durationMs / 1000;
    EXPECT_GE(result.sent, expected / 2);
}

TEST(BenchmarkRunner, LatencyPercentilesAreOrdered) {
    BenchmarkResult result = runBenchmark(quickConfig(BenchmarkMode::Max));

    ASSERT_GT(result.processed, 0u);
    ASSERT_TRUE(result.total.p50Us.has_value());
    ASSERT_TRUE(result.service.p50Us.has_value());

    expectNotGreater(result.total.p50Us, result.total.p95Us);
    expectNotGreater(result.total.p95Us, result.total.p99Us);
    expectNotGreater(result.total.p99Us, result.total.maxUs);
    expectNotGreater(result.service.p50Us, result.service.p95Us);
    expectNotGreater(result.service.p95Us, result.service.p99Us);
    expectNotGreater(result.service.p99Us, result.service.maxUs);
}

TEST(BenchmarkRunner, ProcessingTimeIsNeverLongerThanTotalTime) {
    BenchmarkResult result = runBenchmark(quickConfig(BenchmarkMode::Max));

    ASSERT_GT(result.processed, 0u);
    ASSERT_TRUE(result.total.p50Us.has_value());
    ASSERT_TRUE(result.service.p50Us.has_value());

    expectNotGreater(result.service.p50Us, result.total.p50Us);
    expectNotGreater(result.service.p95Us, result.total.p95Us);
    expectNotGreater(result.service.p99Us, result.total.p99Us);
    expectNotGreater(result.service.maxUs, result.total.maxUs);
}

TEST(BenchmarkRunner, QueueStatisticsAreConsistent) {
    BenchmarkResult result = runBenchmark(quickConfig(BenchmarkMode::Max));

    ASSERT_GT(result.processed, 0u);
    EXPECT_LE(result.queueAvg, result.queueMax);
    EXPECT_LE(result.queueMax, kQueueCapacity);
}

TEST(BenchmarkRunner, MeasuredDurationCoversTheRequestedTimeButNotMuchMore) {
    BenchmarkResult result = runBenchmark(quickConfig(BenchmarkMode::Max));

    ASSERT_GT(result.processed, 0u);
    EXPECT_GE(result.durationMs, kQuickDurationMs);
    EXPECT_LE(result.durationMs, kQuickDurationMs + 2000);
}

TEST(BenchmarkRunner, ThroughputMatchesProcessedOverTheMeasuredDuration) {
    BenchmarkResult result = runBenchmark(quickConfig(BenchmarkMode::Max));

    ASSERT_GT(result.processed, 0u);
    ASSERT_GT(result.durationMs, 0u);
    double expected = static_cast<double>(result.processed) * 1000.0 / static_cast<double>(result.durationMs);
    EXPECT_NEAR(static_cast<double>(result.throughputPerSec), expected, expected * 0.02 + 2.0);
}
