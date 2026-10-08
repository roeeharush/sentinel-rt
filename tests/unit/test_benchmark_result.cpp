#include <gtest/gtest.h>
#include "common/benchmark/BenchmarkResult.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

BenchmarkResult sampleResult() {
    BenchmarkResult result;
    result.mode = BenchmarkMode::Rate;
    result.build = "release";
    result.workers = 4;
    result.devices = 10;
    result.targetRate = 5000;
    result.durationMs = 2000;
    result.sent = 10000;
    result.processed = 9990;
    result.throughputPerSec = 4995;
    result.maxLag = 12;
    result.queueAvg = 3;
    result.queueMax = 40;
    result.total = LatencySummary{120, 800, 1500, 9000, 0};
    result.service = LatencySummary{15, 22, 40, 70, 2};
    return result;
}

std::vector<std::string> tokensOf(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream stream(line);
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

std::map<std::string, std::string> fieldsOf(const std::string& line) {
    std::map<std::string, std::string> fields;
    for (const std::string& token : tokensOf(line)) {
        std::size_t equals = token.find('=');
        if (equals == std::string::npos) {
            continue;
        }
        fields[token.substr(0, equals)] = token.substr(equals + 1);
    }
    return fields;
}

std::string valueOf(const std::map<std::string, std::string>& fields, const std::string& key) {
    auto it = fields.find(key);
    return it == fields.end() ? "<missing key>" : it->second;
}

}

TEST(BenchmarkResult, FormatsEveryFieldInTheAgreedOrder) {
    std::string line = formatResult(sampleResult());

    EXPECT_EQ(line,
              "mode=rate build=release workers=4 devices=10 target_rate=5000 duration_ms=2000 "
              "sent=10000 processed=9990 throughput_per_s=4995 max_lag=12 queue_avg=3 queue_max=40 "
              "total_p50_us=120 total_p95_us=800 total_p99_us=1500 total_max_us=9000 total_overflow=0 "
              "service_p50_us=15 service_p95_us=22 service_p99_us=40 service_max_us=70 service_overflow=2");
}

TEST(BenchmarkResult, MissingPercentilesPrintAsNA) {
    BenchmarkResult result = sampleResult();
    result.total.p99Us = std::nullopt;
    result.total.maxUs = std::nullopt;
    result.total.overflow = 57;

    std::map<std::string, std::string> fields = fieldsOf(formatResult(result));

    EXPECT_EQ(valueOf(fields, "total_p99_us"), "NA");
    EXPECT_EQ(valueOf(fields, "total_max_us"), "NA");
    EXPECT_EQ(valueOf(fields, "total_overflow"), "57");
    EXPECT_EQ(valueOf(fields, "total_p50_us"), "120");
    EXPECT_EQ(valueOf(fields, "service_p99_us"), "40");
}

TEST(BenchmarkResult, AZeroPercentileIsNotConfusedWithAMissingOne) {
    BenchmarkResult result = sampleResult();
    result.service.p50Us = 0;

    std::map<std::string, std::string> fields = fieldsOf(formatResult(result));

    EXPECT_EQ(valueOf(fields, "service_p50_us"), "0");
}

TEST(BenchmarkResult, ZeroCountersAreStillPrinted) {
    BenchmarkResult result;
    result.build = "debug";

    std::map<std::string, std::string> fields = fieldsOf(formatResult(result));

    EXPECT_EQ(valueOf(fields, "workers"), "0");
    EXPECT_EQ(valueOf(fields, "sent"), "0");
    EXPECT_EQ(valueOf(fields, "total_overflow"), "0");
    EXPECT_EQ(valueOf(fields, "total_p50_us"), "NA");
}

TEST(BenchmarkResult, EveryTokenIsExactlyOneKeyEqualsValue) {
    std::string line = formatResult(sampleResult());

    ASSERT_FALSE(line.empty());
    EXPECT_NE(line.front(), ' ');
    EXPECT_NE(line.back(), ' ');
    EXPECT_EQ(line.find('\n'), std::string::npos);
    EXPECT_EQ(line.find("  "), std::string::npos);

    for (const std::string& token : tokensOf(line)) {
        std::size_t equals = token.find('=');
        ASSERT_NE(equals, std::string::npos) << "no '=' in: " << token;
        EXPECT_EQ(token.find('=', equals + 1), std::string::npos) << "second '=' in: " << token;
        EXPECT_GT(equals, 0u) << "empty key in: " << token;
        EXPECT_LT(equals + 1, token.size()) << "empty value in: " << token;
    }
}

TEST(BenchmarkResult, HasExactlyTheAgreedNumberOfDistinctFields) {
    std::string line = formatResult(sampleResult());

    EXPECT_EQ(tokensOf(line).size(), 22u);
    EXPECT_EQ(fieldsOf(line).size(), 22u);
}

TEST(BenchmarkResult, LargestValuesAreNotTruncated) {
    BenchmarkResult result = sampleResult();
    result.sent = std::numeric_limits<std::uint64_t>::max();
    result.total.maxUs = std::numeric_limits<std::uint64_t>::max();

    std::map<std::string, std::string> fields = fieldsOf(formatResult(result));

    EXPECT_EQ(valueOf(fields, "sent"), "18446744073709551615");
    EXPECT_EQ(valueOf(fields, "total_max_us"), "18446744073709551615");
}

TEST(BenchmarkResult, ModeIsPrintedByName) {
    BenchmarkResult result = sampleResult();

    result.mode = BenchmarkMode::Max;
    EXPECT_EQ(valueOf(fieldsOf(formatResult(result)), "mode"), "max");

    result.mode = BenchmarkMode::Rate;
    EXPECT_EQ(valueOf(fieldsOf(formatResult(result)), "mode"), "rate");

    result.mode = BenchmarkMode::Calibrate;
    EXPECT_EQ(valueOf(fieldsOf(formatResult(result)), "mode"), "calibrate");
}

TEST(BenchmarkResult, BuildIsPrintedAsGiven) {
    BenchmarkResult result = sampleResult();
    result.build = "debug";

    EXPECT_EQ(valueOf(fieldsOf(formatResult(result)), "build"), "debug");
}
