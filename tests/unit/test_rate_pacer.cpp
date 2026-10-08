#include <gtest/gtest.h>
#include "common/benchmark/RatePacer.h"

#include <chrono>
#include <cstdint>

namespace {

std::chrono::nanoseconds ns(std::int64_t value) {
    return std::chrono::nanoseconds(value);
}

std::chrono::nanoseconds seconds(std::int64_t value) {
    return std::chrono::seconds(value);
}

}

TEST(RatePacer, NothingIsDueAtTimeZero) {
    RatePacer pacer(1000);

    EXPECT_EQ(pacer.scheduledCount(ns(0)), 0u);
}

TEST(RatePacer, ExactlyTheRateIsDueAfterOneSecond) {
    RatePacer pacer(1000);

    EXPECT_EQ(pacer.scheduledCount(seconds(1)), 1000u);
    EXPECT_EQ(pacer.scheduledCount(seconds(2)), 2000u);
}

TEST(RatePacer, WorksForARateThatDoesNotDivideTheSecondEvenly) {
    RatePacer pacer(7);

    EXPECT_EQ(pacer.scheduledCount(seconds(1)), 7u);
    EXPECT_EQ(pacer.scheduledCount(seconds(10)), 70u);
}

TEST(RatePacer, RoundsDownBetweenItems) {
    RatePacer pacer(1000);

    EXPECT_EQ(pacer.scheduledCount(ns(500'000)), 0u);
    EXPECT_EQ(pacer.scheduledCount(ns(2'500'000)), 2u);
}

TEST(RatePacer, TheBoundaryIsExact) {
    RatePacer pacer(1000);

    EXPECT_EQ(pacer.scheduledCount(ns(999'999)), 0u);
    EXPECT_EQ(pacer.scheduledCount(ns(1'000'000)), 1u);
    EXPECT_EQ(pacer.scheduledCount(ns(1'999'999)), 1u);
    EXPECT_EQ(pacer.scheduledCount(ns(2'000'000)), 2u);
}

TEST(RatePacer, NeverDecreasesAsTimePasses) {
    RatePacer pacer(3);

    std::uint64_t previous = 0;
    for (std::int64_t t = 0; t <= 3'000'000'000; t += 99'991) {
        std::uint64_t current = pacer.scheduledCount(ns(t));
        ASSERT_GE(current, previous) << "at t=" << t;
        previous = current;
    }
    EXPECT_GE(pacer.scheduledCount(seconds(3)), previous);
    EXPECT_EQ(pacer.scheduledCount(seconds(3)), 9u);
}

TEST(RatePacer, NegativeElapsedIsTreatedAsZero) {
    RatePacer pacer(1000);

    EXPECT_EQ(pacer.scheduledCount(ns(-5'000'000)), 0u);
    EXPECT_EQ(pacer.scheduledCount(std::chrono::nanoseconds::min()), 0u);
}

TEST(RatePacer, ZeroRateMeansNothingIsEverDue) {
    RatePacer pacer(0);

    EXPECT_EQ(pacer.scheduledCount(seconds(1)), 0u);
    EXPECT_EQ(pacer.scheduledCount(seconds(100)), 0u);
}

TEST(RatePacer, ASingleNanosecondAtTheHighestRateIsOneItem) {
    RatePacer pacer(1'000'000'000);

    EXPECT_EQ(pacer.scheduledCount(ns(1)), 1u);
}

TEST(RatePacer, HugeRateOverLongTimeDoesNotOverflow) {
    RatePacer pacer(1'000'000'000);

    EXPECT_EQ(pacer.scheduledCount(seconds(100)), 100'000'000'000ULL);
    EXPECT_EQ(pacer.scheduledCount(seconds(100) + std::chrono::milliseconds(500)), 100'500'000'000ULL);
}
