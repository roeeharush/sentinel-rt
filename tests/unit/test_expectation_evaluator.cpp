#include <gtest/gtest.h>
#include "common/config/ExpectationEvaluator.h"

namespace {

void fill(StatsCollector& stats, int ok, int warning, int critical, int invalid) {
    for (int i = 0; i < ok; ++i) stats.record(Status::Ok);
    for (int i = 0; i < warning; ++i) stats.record(Status::Warning);
    for (int i = 0; i < critical; ++i) stats.record(Status::Critical);
    for (int i = 0; i < invalid; ++i) stats.record(Status::Invalid);
}

Expectation expect(Metric metric, Comparison comparison, std::uint64_t value) {
    return Expectation{metric, comparison, value};
}

}

TEST(ExpectationEvaluator, EmptyExpectationListGivesEmptyResults) {
    StatsCollector stats;

    EXPECT_TRUE(evaluateExpectations({}, stats).empty());
}

TEST(ExpectationEvaluator, AtLeastPassesWhenActualIsGreaterOrEqual) {
    StatsCollector stats;
    fill(stats, 0, 0, 21, 0);

    std::vector<ExpectationResult> results = evaluateExpectations(
        {expect(Metric::Critical, Comparison::AtLeast, 15),
         expect(Metric::Critical, Comparison::AtLeast, 21)},
        stats);

    ASSERT_EQ(results.size(), 2u);
    EXPECT_TRUE(results[0].passed);
    EXPECT_EQ(results[0].actual, 21u);
    EXPECT_TRUE(results[1].passed);
}

TEST(ExpectationEvaluator, AtLeastFailsWhenActualIsLower) {
    StatsCollector stats;
    fill(stats, 0, 0, 3, 0);

    std::vector<ExpectationResult> results = evaluateExpectations(
        {expect(Metric::Critical, Comparison::AtLeast, 15)}, stats);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_FALSE(results[0].passed);
    EXPECT_EQ(results[0].actual, 3u);
}

TEST(ExpectationEvaluator, AtMostPassesAtTheBoundaryAndFailsAboveIt) {
    StatsCollector stats;
    fill(stats, 0, 10, 0, 0);

    std::vector<ExpectationResult> results = evaluateExpectations(
        {expect(Metric::Warning, Comparison::AtMost, 10),
         expect(Metric::Warning, Comparison::AtMost, 9)},
        stats);

    ASSERT_EQ(results.size(), 2u);
    EXPECT_TRUE(results[0].passed);
    EXPECT_FALSE(results[1].passed);
}

TEST(ExpectationEvaluator, EqualPassesOnlyOnExactMatch) {
    StatsCollector stats;
    fill(stats, 0, 0, 0, 2);

    std::vector<ExpectationResult> results = evaluateExpectations(
        {expect(Metric::Invalid, Comparison::Equal, 2),
         expect(Metric::Invalid, Comparison::Equal, 0)},
        stats);

    ASSERT_EQ(results.size(), 2u);
    EXPECT_TRUE(results[0].passed);
    EXPECT_FALSE(results[1].passed);
}

TEST(ExpectationEvaluator, EachMetricReadsItsOwnCounter) {
    StatsCollector stats;
    fill(stats, 1, 2, 3, 4);

    std::vector<ExpectationResult> results = evaluateExpectations(
        {expect(Metric::Ok, Comparison::Equal, 1),
         expect(Metric::Warning, Comparison::Equal, 2),
         expect(Metric::Critical, Comparison::Equal, 3),
         expect(Metric::Invalid, Comparison::Equal, 4),
         expect(Metric::Total, Comparison::Equal, 10)},
        stats);

    ASSERT_EQ(results.size(), 5u);
    for (const ExpectationResult& result : results) {
        EXPECT_TRUE(result.passed);
    }
}

TEST(ExpectationEvaluator, ResultsKeepTheOrderOfTheExpectations) {
    StatsCollector stats;
    fill(stats, 0, 0, 1, 0);

    std::vector<ExpectationResult> results = evaluateExpectations(
        {expect(Metric::Invalid, Comparison::Equal, 0),
         expect(Metric::Critical, Comparison::AtLeast, 1)},
        stats);

    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].expectation.metric, Metric::Invalid);
    EXPECT_EQ(results[1].expectation.metric, Metric::Critical);
}
