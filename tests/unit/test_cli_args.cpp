#include <gtest/gtest.h>
#include "common/CliArgs.h"

TEST(CliArgs, UsesDefaultsWhenNoArgumentsGiven) {
    const char* argv[] = {"monitor_app"};
    int argc = 1;

    std::optional<AppConfig> config = parseArgs(argc, const_cast<char**>(argv));

    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->cycles, 10);
    EXPECT_EQ(config->periodMs, 100);
}

TEST(CliArgs, ParsesValidFlags) {
    const char* argv[] = {"monitor_app", "--cycles", "50", "--period-ms", "200"};
    int argc = 5;

    std::optional<AppConfig> config = parseArgs(argc, const_cast<char**>(argv));

    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->cycles, 50);
    EXPECT_EQ(config->periodMs, 200);
}

TEST(CliArgs, RejectsNonNumericValue) {
    const char* argv[] = {"monitor_app", "--cycles", "abc"};
    int argc = 3;

    EXPECT_FALSE(parseArgs(argc, const_cast<char**>(argv)).has_value());
}

TEST(CliArgs, RejectsNegativeValue) {
    const char* argv[] = {"monitor_app", "--cycles", "-5"};
    int argc = 3;

    EXPECT_FALSE(parseArgs(argc, const_cast<char**>(argv)).has_value());
}

TEST(CliArgs, RejectsUnknownFlag) {
    const char* argv[] = {"monitor_app", "--bogus", "1"};
    int argc = 3;

    EXPECT_FALSE(parseArgs(argc, const_cast<char**>(argv)).has_value());
}

TEST(CliArgs, RejectsFlagMissingValue) {
    const char* argv[] = {"monitor_app", "--cycles"};
    int argc = 2;

    EXPECT_FALSE(parseArgs(argc, const_cast<char**>(argv)).has_value());
}
