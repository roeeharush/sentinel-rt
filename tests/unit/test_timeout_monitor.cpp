#include <gtest/gtest.h>
#include "common/detection/TimeoutMonitor.h"

TEST(TimeoutMonitor, NoDevicesReportedWhenNoneRecorded) {
    TimeoutMonitor monitor(std::chrono::milliseconds(100));
    auto now = std::chrono::steady_clock::now();

    EXPECT_TRUE(monitor.checkTimeouts(now).empty());
}

TEST(TimeoutMonitor, RecentArrivalIsNotTimedOut) {
    TimeoutMonitor monitor(std::chrono::milliseconds(100));
    auto t0 = std::chrono::steady_clock::now();

    monitor.recordArrival(1, t0);
    auto result = monitor.checkTimeouts(t0 + std::chrono::milliseconds(50));

    EXPECT_TRUE(result.empty());
}

TEST(TimeoutMonitor, SilentDeviceIsReportedOnceThresholdExceeded) {
    TimeoutMonitor monitor(std::chrono::milliseconds(100));
    auto t0 = std::chrono::steady_clock::now();

    monitor.recordArrival(1, t0);
    auto result = monitor.checkTimeouts(t0 + std::chrono::milliseconds(150));

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0], 1u);
}

TEST(TimeoutMonitor, TimedOutDeviceIsNotReportedAgainOnNextCheck) {
    TimeoutMonitor monitor(std::chrono::milliseconds(100));
    auto t0 = std::chrono::steady_clock::now();

    monitor.recordArrival(1, t0);
    monitor.checkTimeouts(t0 + std::chrono::milliseconds(150));
    auto secondResult = monitor.checkTimeouts(t0 + std::chrono::milliseconds(200));

    EXPECT_TRUE(secondResult.empty());
}

TEST(TimeoutMonitor, RecoveredDeviceCanBeReportedAsTimedOutAgainLater) {
    TimeoutMonitor monitor(std::chrono::milliseconds(100));
    auto t0 = std::chrono::steady_clock::now();

    monitor.recordArrival(1, t0);
    monitor.checkTimeouts(t0 + std::chrono::milliseconds(150));

    monitor.recordArrival(1, t0 + std::chrono::milliseconds(160));
    auto afterRecovery = monitor.checkTimeouts(t0 + std::chrono::milliseconds(170));
    EXPECT_TRUE(afterRecovery.empty());

    auto afterSecondSilence = monitor.checkTimeouts(t0 + std::chrono::milliseconds(300));
    ASSERT_EQ(afterSecondSilence.size(), 1u);
    EXPECT_EQ(afterSecondSilence[0], 1u);
}

TEST(TimeoutMonitor, MultipleDevicesAreTrackedIndependently) {
    TimeoutMonitor monitor(std::chrono::milliseconds(100));
    auto t0 = std::chrono::steady_clock::now();

    monitor.recordArrival(1, t0);
    monitor.recordArrival(2, t0 + std::chrono::milliseconds(140));

    auto result = monitor.checkTimeouts(t0 + std::chrono::milliseconds(150));

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0], 1u);
}
