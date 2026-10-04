#include <gtest/gtest.h>
#include "common/detection/SlidingWindow.h"

TEST(SlidingWindow, NotFullWindowNeverReportsStuck) {
    SlidingWindow window(6);
    window.push(55.0);
    window.push(55.0);
    window.push(55.0);

    EXPECT_FALSE(window.isStuck());
}

TEST(SlidingWindow, NotFullWindowNeverReportsDrifting) {
    SlidingWindow window(6);
    window.push(55.0);
    window.push(55.0);
    window.push(90.0);

    EXPECT_FALSE(window.isDrifting());
}

TEST(SlidingWindow, FullWindowOfIdenticalValuesIsStuck) {
    SlidingWindow window(6);
    for (int i = 0; i < 6; ++i) {
        window.push(55.0);
    }

    EXPECT_TRUE(window.isStuck());
}

TEST(SlidingWindow, FullWindowWithNaturalVarianceIsNotStuck) {
    SlidingWindow window(6);
    window.push(55.1);
    window.push(54.8);
    window.push(55.3);
    window.push(55.0);
    window.push(54.9);
    window.push(55.2);

    EXPECT_FALSE(window.isStuck());
}

TEST(SlidingWindow, PushBeyondCapacityNeverGrowsPastLimit) {
    SlidingWindow window(6);
    for (int i = 0; i < 10; ++i) {
        window.push(55.0);
    }

    EXPECT_EQ(window.size(), 6u);
}

TEST(SlidingWindow, PushingNewValueAfterFullEvictsOldestValue) {
    SlidingWindow window(6);
    for (int i = 0; i < 6; ++i) {
        window.push(55.0);
    }
    ASSERT_TRUE(window.isStuck());

    window.push(90.0);

    EXPECT_EQ(window.size(), 6u);
    EXPECT_FALSE(window.isStuck());
}

TEST(SlidingWindow, GradualIncreaseAcrossWindowIsDrifting) {
    SlidingWindow window(6);
    window.push(55.0);
    window.push(55.1);
    window.push(54.9);
    window.push(58.0);
    window.push(58.2);
    window.push(57.8);

    EXPECT_TRUE(window.isDrifting());
}

TEST(SlidingWindow, GradualDecreaseAcrossWindowIsDrifting) {
    SlidingWindow window(6);
    window.push(58.0);
    window.push(58.2);
    window.push(57.8);
    window.push(55.0);
    window.push(55.1);
    window.push(54.9);

    EXPECT_TRUE(window.isDrifting());
}

TEST(SlidingWindow, StableValuesAreNotDrifting) {
    SlidingWindow window(6);
    window.push(55.0);
    window.push(55.1);
    window.push(54.9);
    window.push(54.9);
    window.push(55.2);
    window.push(55.0);

    EXPECT_FALSE(window.isDrifting());
}
