#include <gtest/gtest.h>
#include "common/infra/LatencyHistogram.h"

#include <chrono>
#include <cmath>
#include <limits>
#include <thread>
#include <vector>

namespace {

std::chrono::nanoseconds micros(int value) {
    return std::chrono::microseconds(value);
}

}

TEST(LatencyHistogram, NewHistogramIsEmpty) {
    LatencyHistogram histogram;

    EXPECT_EQ(histogram.count(), 0u);
    EXPECT_EQ(histogram.overflowCount(), 0u);
    EXPECT_FALSE(histogram.percentileMicros(50.0).has_value());
}

TEST(LatencyHistogram, EveryRecordedValueIsCounted) {
    LatencyHistogram histogram;

    for (int i = 0; i < 5; ++i) {
        histogram.record(micros(10));
    }

    EXPECT_EQ(histogram.count(), 5u);
}

TEST(LatencyHistogram, PercentilesMatchTheWorkedExample) {
    LatencyHistogram histogram;
    for (int i = 0; i < 95; ++i) {
        histogram.record(micros(1));
    }
    for (int i = 0; i < 5; ++i) {
        histogram.record(micros(100));
    }

    EXPECT_EQ(histogram.percentileMicros(50.0), 1u);
    EXPECT_EQ(histogram.percentileMicros(95.0), 1u);
    EXPECT_EQ(histogram.percentileMicros(99.0), 100u);
    EXPECT_EQ(histogram.percentileMicros(100.0), 100u);
}

TEST(LatencyHistogram, RoundsUpToTheNextMicrosecond) {
    LatencyHistogram zero;
    zero.record(std::chrono::nanoseconds(0));
    EXPECT_EQ(zero.percentileMicros(100.0), 0u);

    LatencyHistogram exact;
    exact.record(std::chrono::nanoseconds(1000));
    EXPECT_EQ(exact.percentileMicros(100.0), 1u);

    LatencyHistogram justOver;
    justOver.record(std::chrono::nanoseconds(1001));
    EXPECT_EQ(justOver.percentileMicros(100.0), 2u);

    LatencyHistogram middle;
    middle.record(std::chrono::nanoseconds(3200));
    EXPECT_EQ(middle.percentileMicros(100.0), 4u);
}

TEST(LatencyHistogram, NegativeLatencyCountsAsZero) {
    LatencyHistogram histogram;

    histogram.record(std::chrono::nanoseconds(-5));

    EXPECT_EQ(histogram.count(), 1u);
    EXPECT_EQ(histogram.percentileMicros(50.0), 0u);
}

TEST(LatencyHistogram, ValuesBeyondTheRangeGoToOverflow) {
    LatencyHistogram histogram(10);

    histogram.record(micros(9));
    histogram.record(micros(10));

    EXPECT_EQ(histogram.overflowCount(), 1u);
    EXPECT_EQ(histogram.count(), 2u);
}

TEST(LatencyHistogram, PercentileInsideOverflowHasNoValue) {
    LatencyHistogram histogram(10);
    for (int i = 0; i < 90; ++i) {
        histogram.record(micros(1));
    }
    for (int i = 0; i < 10; ++i) {
        histogram.record(micros(50));
    }

    EXPECT_EQ(histogram.count(), 100u);
    EXPECT_EQ(histogram.overflowCount(), 10u);
    EXPECT_EQ(histogram.percentileMicros(90.0), 1u);
    EXPECT_FALSE(histogram.percentileMicros(95.0).has_value());
}

TEST(LatencyHistogram, InvalidPercentHasNoValue) {
    LatencyHistogram histogram;
    histogram.record(micros(5));

    EXPECT_FALSE(histogram.percentileMicros(0.0).has_value());
    EXPECT_FALSE(histogram.percentileMicros(-1.0).has_value());
    EXPECT_FALSE(histogram.percentileMicros(100.5).has_value());
    EXPECT_FALSE(histogram.percentileMicros(std::numeric_limits<double>::quiet_NaN()).has_value());
    EXPECT_TRUE(histogram.percentileMicros(100.0).has_value());
}

TEST(LatencyHistogram, ManyThreadsRecordingConcurrentlyLoseNothing) {
    LatencyHistogram histogram;
    constexpr int kThreads = 4;
    constexpr int kPerThread = 10000;

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&histogram]() {
            for (int i = 0; i < kPerThread; ++i) {
                histogram.record(micros(1));
            }
        });
    }
    for (std::thread& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(histogram.count(), static_cast<std::uint64_t>(kThreads) * kPerThread);
    EXPECT_EQ(histogram.overflowCount(), 0u);
    EXPECT_EQ(histogram.percentileMicros(100.0), 1u);
}
