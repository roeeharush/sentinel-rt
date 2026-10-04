#include <gtest/gtest.h>
#include "common/infra/ManualClock.h"

TEST(ManualClock, StartsAtGivenTime) {
    std::chrono::steady_clock::time_point start{std::chrono::milliseconds(1000)};
    ManualClock clock(start);

    EXPECT_EQ(clock.now(), start);
}

TEST(ManualClock, SleepUntilJumpsToExactTime) {
    std::chrono::steady_clock::time_point start{std::chrono::milliseconds(1000)};
    ManualClock clock(start);

    std::chrono::steady_clock::time_point wakeTime{std::chrono::milliseconds(5000)};
    clock.sleepUntil(wakeTime);

    EXPECT_EQ(clock.now(), wakeTime);
}
