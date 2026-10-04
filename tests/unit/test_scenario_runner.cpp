#include <gtest/gtest.h>
#include "common/config/ScenarioParser.h"
#include "common/config/ScenarioRunner.h"

#include <chrono>
#include <optional>
#include <sstream>

namespace {

Scenario makeScenario(int cycles, int periodMs, std::optional<TimedFault> fault,
                      std::vector<Expectation> expectations = {}) {
    Scenario scenario;
    scenario.config.cycles = cycles;
    scenario.config.periodMs = periodMs;
    scenario.config.fault = fault;
    scenario.expectations = expectations;
    return scenario;
}

TimedFault faultAt(FaultType type, int afterMs) {
    return TimedFault{type, std::chrono::milliseconds(afterMs)};
}

}

TEST(ScenarioRunner, WithoutFaultEveryMeasurementIsCountedAndNoneIsCritical) {
    ScenarioOutcome outcome = runScenario(makeScenario(40, 100, std::nullopt));

    EXPECT_EQ(outcome.total, 120u);
    EXPECT_EQ(outcome.ok + outcome.warning + outcome.critical + outcome.invalid, outcome.total);
    EXPECT_EQ(outcome.critical, 0u);
    EXPECT_EQ(outcome.invalid, 0u);
}

TEST(ScenarioRunner, StuckFaultProducesExactlyTheCriticalsOfAFullWindow) {
    ScenarioOutcome outcome = runScenario(makeScenario(40, 100, faultAt(FaultType::Stuck, 1000)));

    EXPECT_EQ(outcome.total, 120u);
    EXPECT_EQ(outcome.critical, 21u);
}

TEST(ScenarioRunner, SpikeFaultIsCriticalFromTheFirstFaultyCycle) {
    ScenarioOutcome outcome = runScenario(makeScenario(40, 100, faultAt(FaultType::Spike, 1000)));

    EXPECT_EQ(outcome.critical, 30u);
}

TEST(ScenarioRunner, ExpectationsAreEvaluatedAgainstTheRun) {
    ScenarioOutcome outcome = runScenario(makeScenario(
        40, 100, faultAt(FaultType::Stuck, 1000),
        {Expectation{Metric::Critical, Comparison::AtLeast, 15},
         Expectation{Metric::Invalid, Comparison::Equal, 0},
         Expectation{Metric::Critical, Comparison::AtMost, 5}}));

    ASSERT_EQ(outcome.results.size(), 3u);
    EXPECT_TRUE(outcome.results[0].passed);
    EXPECT_TRUE(outcome.results[1].passed);
    EXPECT_FALSE(outcome.results[2].passed);
    EXPECT_EQ(outcome.results[2].actual, 21u);
}

TEST(ScenarioRunner, RunningTheSameScenarioTwiceGivesIdenticalCounts) {
    Scenario scenario = makeScenario(40, 100, faultAt(FaultType::Stuck, 1000));

    ScenarioOutcome first = runScenario(scenario);
    ScenarioOutcome second = runScenario(scenario);

    EXPECT_EQ(first.ok, second.ok);
    EXPECT_EQ(first.warning, second.warning);
    EXPECT_EQ(first.critical, second.critical);
    EXPECT_EQ(first.invalid, second.invalid);
    EXPECT_EQ(first.total, second.total);
}

TEST(ScenarioRunner, AParsedScenarioRunsEndToEnd) {
    std::istringstream input(
        "cycles 40\n"
        "period_ms 100\n"
        "at 1000 fault Stuck\n"
        "expect critical == 21\n"
        "expect invalid == 0\n");

    ScenarioParseResult parsed = parseScenario(input);
    ASSERT_TRUE(parsed.scenario.has_value());

    ScenarioOutcome outcome = runScenario(*parsed.scenario);

    ASSERT_EQ(outcome.results.size(), 2u);
    EXPECT_TRUE(outcome.results[0].passed);
    EXPECT_TRUE(outcome.results[1].passed);
}
