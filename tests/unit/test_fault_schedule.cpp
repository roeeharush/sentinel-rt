#include <gtest/gtest.h>
#include "common/sensors/FaultSchedule.h"

#include <chrono>

namespace {

std::chrono::milliseconds ms(int value) {
    return std::chrono::milliseconds(value);
}

FaultSchedule scheduleFor(FaultType type, int afterMs) {
    return FaultSchedule(TimedFault{type, ms(afterMs)});
}

}

TEST(FaultSchedule, NeverFiresWhenThereIsNoFault) {
    FaultSchedule schedule(std::nullopt);

    EXPECT_FALSE(schedule.due(ms(0)).has_value());
    EXPECT_FALSE(schedule.due(ms(10000)).has_value());
}

TEST(FaultSchedule, DoesNotFireBeforeTheActivationTime) {
    FaultSchedule schedule = scheduleFor(FaultType::Stuck, 1000);

    EXPECT_FALSE(schedule.due(ms(0)).has_value());
    EXPECT_FALSE(schedule.due(ms(999)).has_value());
}

TEST(FaultSchedule, FiresWhenTheActivationTimeIsReached) {
    FaultSchedule schedule = scheduleFor(FaultType::Stuck, 1000);

    std::optional<FaultType> fault = schedule.due(ms(1000));

    ASSERT_TRUE(fault.has_value());
    EXPECT_EQ(*fault, FaultType::Stuck);
}

TEST(FaultSchedule, FiresOnlyOnce) {
    FaultSchedule schedule = scheduleFor(FaultType::Stuck, 1000);

    ASSERT_TRUE(schedule.due(ms(1000)).has_value());
    EXPECT_FALSE(schedule.due(ms(1100)).has_value());
    EXPECT_FALSE(schedule.due(ms(5000)).has_value());
}

TEST(FaultSchedule, FiresLateWhenTheFirstCheckIsAfterTheActivationTime) {
    FaultSchedule schedule = scheduleFor(FaultType::Spike, 1000);

    std::optional<FaultType> fault = schedule.due(ms(2500));

    ASSERT_TRUE(fault.has_value());
    EXPECT_EQ(*fault, FaultType::Spike);
}

TEST(FaultSchedule, FiresImmediatelyWhenTheDelayIsZero) {
    FaultSchedule schedule = scheduleFor(FaultType::Drift, 0);

    std::optional<FaultType> fault = schedule.due(ms(0));

    ASSERT_TRUE(fault.has_value());
    EXPECT_EQ(*fault, FaultType::Drift);
}
