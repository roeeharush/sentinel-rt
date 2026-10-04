#include <gtest/gtest.h>
#include "common/config/ScenarioParser.h"
#include <sstream>

TEST(ScenarioParser, RejectsEmptyInput) {
    std::istringstream input("");

    ScenarioParseResult result = parseScenario(input);

    EXPECT_FALSE(result.scenario.has_value());
    EXPECT_EQ(result.errorLine, 0);
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST(ScenarioParser, ParsesSingleExpectation) {
    std::istringstream input("expect critical >= 15\n");

    ScenarioParseResult result = parseScenario(input);

    ASSERT_TRUE(result.scenario.has_value());
    ASSERT_EQ(result.scenario->expectations.size(), 1u);
    EXPECT_EQ(result.scenario->expectations[0].metric, Metric::Critical);
    EXPECT_EQ(result.scenario->expectations[0].comparison, Comparison::AtLeast);
    EXPECT_EQ(result.scenario->expectations[0].value, 15u);
}

TEST(ScenarioParser, ParsesSettings) {
    std::istringstream input("cycles 40\nperiod_ms 250\nexpect ok >= 1\n");

    ScenarioParseResult result = parseScenario(input);

    ASSERT_TRUE(result.scenario.has_value());
    EXPECT_EQ(result.scenario->config.cycles, 40);
    EXPECT_EQ(result.scenario->config.periodMs, 250);
}

TEST(ScenarioParser, UsesCliDefaultsWhenSettingsAreOmitted) {
    std::istringstream input("expect ok >= 1\n");

    ScenarioParseResult result = parseScenario(input);

    ASSERT_TRUE(result.scenario.has_value());
    EXPECT_EQ(result.scenario->config.cycles, 10);
    EXPECT_EQ(result.scenario->config.periodMs, 100);
}

TEST(ScenarioParser, ParsesFaultEvent) {
    std::istringstream input("at 1000 fault Stuck\nexpect critical >= 1\n");

    ScenarioParseResult result = parseScenario(input);

    ASSERT_TRUE(result.scenario.has_value());
    ASSERT_TRUE(result.scenario->config.fault.has_value());
    EXPECT_EQ(result.scenario->config.fault->type, FaultType::Stuck);
    EXPECT_EQ(result.scenario->config.fault->activateAfter.count(), 1000);
}

TEST(ScenarioParser, HasNoFaultWhenNoEventGiven) {
    std::istringstream input("expect ok >= 1\n");

    ScenarioParseResult result = parseScenario(input);

    ASSERT_TRUE(result.scenario.has_value());
    EXPECT_FALSE(result.scenario->config.fault.has_value());
}

TEST(ScenarioParser, RejectsSecondFaultEvent) {
    std::istringstream input("at 1000 fault Stuck\nat 2000 fault Spike\nexpect ok >= 1\n");

    ScenarioParseResult result = parseScenario(input);

    EXPECT_FALSE(result.scenario.has_value());
    EXPECT_EQ(result.errorLine, 2);
}

TEST(ScenarioParser, RejectsUnknownFaultType) {
    std::istringstream input("at 1000 fault Banana\nexpect ok >= 1\n");

    ScenarioParseResult result = parseScenario(input);

    EXPECT_FALSE(result.scenario.has_value());
    EXPECT_EQ(result.errorLine, 1);
    EXPECT_NE(result.errorMessage.find("Banana"), std::string::npos);
}

TEST(ScenarioParser, RejectsUnknownMetricAndReportsItsLine) {
    std::istringstream input("# a comment\n\nexpect critcal >= 15\n");

    ScenarioParseResult result = parseScenario(input);

    EXPECT_FALSE(result.scenario.has_value());
    EXPECT_EQ(result.errorLine, 3);
    EXPECT_NE(result.errorMessage.find("critcal"), std::string::npos);
}

TEST(ScenarioParser, SkipsBlankLinesAndComments) {
    std::istringstream input("# a comment\n\n   \nexpect ok >= 1\n");

    ScenarioParseResult result = parseScenario(input);

    ASSERT_TRUE(result.scenario.has_value());
    EXPECT_EQ(result.scenario->expectations.size(), 1u);
}

TEST(ScenarioParser, RejectsNegativeValue) {
    std::istringstream input("expect critical >= -5\n");

    ScenarioParseResult result = parseScenario(input);

    EXPECT_FALSE(result.scenario.has_value());
    EXPECT_EQ(result.errorLine, 1);
}

TEST(ScenarioParser, RejectsTrailingText) {
    std::istringstream input("expect critical >= 15 extra\n");

    ScenarioParseResult result = parseScenario(input);

    EXPECT_FALSE(result.scenario.has_value());
    EXPECT_EQ(result.errorLine, 1);
}

TEST(ScenarioParser, RejectsUnknownKeyword) {
    std::istringstream input("bogus 1\nexpect ok >= 1\n");

    ScenarioParseResult result = parseScenario(input);

    EXPECT_FALSE(result.scenario.has_value());
    EXPECT_EQ(result.errorLine, 1);
}

TEST(ScenarioParser, RejectsDuplicateSetting) {
    std::istringstream input("cycles 10\ncycles 20\nexpect ok >= 1\n");

    ScenarioParseResult result = parseScenario(input);

    EXPECT_FALSE(result.scenario.has_value());
    EXPECT_EQ(result.errorLine, 2);
}

TEST(ScenarioParser, RejectsFileWithOnlySettings) {
    std::istringstream input("cycles 10\nperiod_ms 100\n");

    ScenarioParseResult result = parseScenario(input);

    EXPECT_FALSE(result.scenario.has_value());
    EXPECT_EQ(result.errorLine, 0);
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST(ScenarioParser, ParsesFullScenario) {
    std::istringstream input(
        "# stuck temperature sensor\n"
        "cycles 40\n"
        "period_ms 100\n"
        "\n"
        "at 1000 fault Stuck\n"
        "\n"
        "expect critical >= 15\n"
        "expect invalid == 0\n");

    ScenarioParseResult result = parseScenario(input);

    ASSERT_TRUE(result.scenario.has_value());
    EXPECT_EQ(result.scenario->config.cycles, 40);
    EXPECT_EQ(result.scenario->config.periodMs, 100);
    ASSERT_TRUE(result.scenario->config.fault.has_value());
    EXPECT_EQ(result.scenario->config.fault->type, FaultType::Stuck);
    EXPECT_EQ(result.scenario->config.fault->activateAfter.count(), 1000);
    ASSERT_EQ(result.scenario->expectations.size(), 2u);
    EXPECT_EQ(result.scenario->expectations[1].metric, Metric::Invalid);
    EXPECT_EQ(result.scenario->expectations[1].comparison, Comparison::Equal);
    EXPECT_EQ(result.scenario->expectations[1].value, 0u);
}