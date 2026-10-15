#include <gtest/gtest.h>
#include "common/config/BenchmarkArgs.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {

std::optional<BenchmarkArgs> parse(std::vector<std::string> args) {
    std::string program = "benchmark";
    std::vector<char*> argv;
    argv.push_back(program.data());
    for (std::string& arg : args) {
        argv.push_back(arg.data());
    }
    return parseBenchmarkArgs(static_cast<int>(argv.size()), argv.data());
}

}

TEST(BenchmarkArgs, NoArgumentsGiveTheDefaults) {
    std::optional<BenchmarkArgs> args = parse({});

    ASSERT_TRUE(args.has_value());
    EXPECT_EQ(args->config.mode, BenchmarkMode::Max);
    EXPECT_EQ(args->config.workers, 1u);
    EXPECT_EQ(args->config.devices, 10u);
    EXPECT_EQ(args->config.targetRate, 0u);
    EXPECT_EQ(args->config.durationMs, 5000u);
    EXPECT_EQ(args->warmupMs, 1000u);
}

TEST(BenchmarkArgs, EveryOptionIsParsed) {
    std::optional<BenchmarkArgs> args = parse({"--mode", "rate", "--workers", "4", "--devices", "100",
                                               "--rate", "5000", "--duration-ms", "2000", "--warmup-ms", "500"});

    ASSERT_TRUE(args.has_value());
    EXPECT_EQ(args->config.mode, BenchmarkMode::Rate);
    EXPECT_EQ(args->config.workers, 4u);
    EXPECT_EQ(args->config.devices, 100u);
    EXPECT_EQ(args->config.targetRate, 5000u);
    EXPECT_EQ(args->config.durationMs, 2000u);
    EXPECT_EQ(args->warmupMs, 500u);
}

TEST(BenchmarkArgs, AllThreeModeNamesAreRecognised) {
    std::optional<BenchmarkArgs> max = parse({"--mode", "max"});
    std::optional<BenchmarkArgs> rate = parse({"--mode", "rate", "--rate", "100"});
    std::optional<BenchmarkArgs> calibrate = parse({"--mode", "calibrate"});

    ASSERT_TRUE(max.has_value());
    ASSERT_TRUE(rate.has_value());
    ASSERT_TRUE(calibrate.has_value());
    EXPECT_EQ(max->config.mode, BenchmarkMode::Max);
    EXPECT_EQ(rate->config.mode, BenchmarkMode::Rate);
    EXPECT_EQ(calibrate->config.mode, BenchmarkMode::Calibrate);
}

TEST(BenchmarkArgs, ZeroWarmupMeansNoWarmupAndIsAllowed) {
    std::optional<BenchmarkArgs> args = parse({"--warmup-ms", "0"});

    ASSERT_TRUE(args.has_value());
    EXPECT_EQ(args->warmupMs, 0u);
}

TEST(BenchmarkArgs, RateModeNeedsARate) {
    ASSERT_TRUE(parse({"--mode", "rate", "--rate", "100"}).has_value());

    EXPECT_FALSE(parse({"--mode", "rate"}).has_value());
}

TEST(BenchmarkArgs, ARateIsOnlyAllowedInRateMode) {
    ASSERT_TRUE(parse({"--mode", "rate", "--rate", "100"}).has_value());

    EXPECT_FALSE(parse({"--mode", "max", "--rate", "100"}).has_value());
    EXPECT_FALSE(parse({"--mode", "calibrate", "--rate", "100"}).has_value());
    EXPECT_FALSE(parse({"--rate", "100"}).has_value());
}

TEST(BenchmarkArgs, WorkersMustBeBetweenOneAndTwoHundredFiftySix) {
    ASSERT_TRUE(parse({"--workers", "1"}).has_value());
    ASSERT_TRUE(parse({"--workers", "256"}).has_value());

    EXPECT_FALSE(parse({"--workers", "0"}).has_value());
    EXPECT_FALSE(parse({"--workers", "257"}).has_value());
}

TEST(BenchmarkArgs, DevicesMustBeBetweenOneAndOneHundredThousand) {
    ASSERT_TRUE(parse({"--devices", "1"}).has_value());
    ASSERT_TRUE(parse({"--devices", "100000"}).has_value());

    EXPECT_FALSE(parse({"--devices", "0"}).has_value());
    EXPECT_FALSE(parse({"--devices", "100001"}).has_value());
}

TEST(BenchmarkArgs, DurationMustBeBetweenOneMillisecondAndOneHour) {
    ASSERT_TRUE(parse({"--duration-ms", "1"}).has_value());
    ASSERT_TRUE(parse({"--duration-ms", "3600000"}).has_value());

    EXPECT_FALSE(parse({"--duration-ms", "0"}).has_value());
    EXPECT_FALSE(parse({"--duration-ms", "3600001"}).has_value());
}

TEST(BenchmarkArgs, WarmupCannotExceedOneHour) {
    ASSERT_TRUE(parse({"--warmup-ms", "3600000"}).has_value());

    EXPECT_FALSE(parse({"--warmup-ms", "3600001"}).has_value());
}

TEST(BenchmarkArgs, RateMustBeBetweenOneAndTenBillion) {
    ASSERT_TRUE(parse({"--mode", "rate", "--rate", "1"}).has_value());
    ASSERT_TRUE(parse({"--mode", "rate", "--rate", "10000000000"}).has_value());

    EXPECT_FALSE(parse({"--mode", "rate", "--rate", "0"}).has_value());
    EXPECT_FALSE(parse({"--mode", "rate", "--rate", "10000000001"}).has_value());
}

TEST(BenchmarkArgs, UnknownModeIsRejected) {
    ASSERT_TRUE(parse({"--mode", "max"}).has_value());

    EXPECT_FALSE(parse({"--mode", "fast"}).has_value());
    EXPECT_FALSE(parse({"--mode", "Max"}).has_value());
}

TEST(BenchmarkArgs, UnknownOptionIsRejected) {
    ASSERT_TRUE(parse({"--workers", "2"}).has_value());

    EXPECT_FALSE(parse({"--bogus", "1"}).has_value());
    EXPECT_FALSE(parse({"--workers", "2", "--bogus", "1"}).has_value());
    EXPECT_FALSE(parse({"stray"}).has_value());
}

TEST(BenchmarkArgs, AnOptionWithoutAValueIsRejected) {
    ASSERT_TRUE(parse({"--workers", "2"}).has_value());

    EXPECT_FALSE(parse({"--workers"}).has_value());
    EXPECT_FALSE(parse({"--mode"}).has_value());
    EXPECT_FALSE(parse({"--devices", "5", "--duration-ms"}).has_value());
}

TEST(BenchmarkArgs, NonNumericOrNegativeNumbersAreRejected) {
    ASSERT_TRUE(parse({"--workers", "4"}).has_value());

    EXPECT_FALSE(parse({"--workers", "abc"}).has_value());
    EXPECT_FALSE(parse({"--workers", "4x"}).has_value());
    EXPECT_FALSE(parse({"--workers", "-3"}).has_value());
    EXPECT_FALSE(parse({"--workers", "2.5"}).has_value());
    EXPECT_FALSE(parse({"--workers", ""}).has_value());
}

TEST(BenchmarkArgs, NumbersTooLargeForTheirTypeAreRejected) {
    ASSERT_TRUE(parse({"--duration-ms", "1000"}).has_value());
    ASSERT_TRUE(parse({"--warmup-ms", "1000"}).has_value());

    EXPECT_FALSE(parse({"--duration-ms", "99999999999999999999999"}).has_value());
    EXPECT_FALSE(parse({"--warmup-ms", "99999999999999999999999"}).has_value());
}
