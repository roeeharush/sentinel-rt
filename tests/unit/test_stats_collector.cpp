#include <gtest/gtest.h>
#include "common/StatsCollector.h"

TEST(StatsCollector, StartsAtZero) {
    StatsCollector stats;

    EXPECT_EQ(stats.totalCount(), 0u);
}

TEST(StatsCollector, RecordsEachStatusIndependently) {
    StatsCollector stats;

    stats.record(Status::Ok);
    stats.record(Status::Ok);
    stats.record(Status::Warning);
    stats.record(Status::Critical);
    stats.record(Status::Invalid);

    EXPECT_EQ(stats.okCount(), 2u);
    EXPECT_EQ(stats.warningCount(), 1u);
    EXPECT_EQ(stats.criticalCount(), 1u);
    EXPECT_EQ(stats.invalidCount(), 1u);
    EXPECT_EQ(stats.totalCount(), 5u);
}
