#pragma once

#include <cstdint>
#include <optional>
#include <string>

enum class BenchmarkMode {
    Max,
    Rate,
    Calibrate
};

struct LatencySummary {
    std::optional<std::uint64_t> p50Us;
    std::optional<std::uint64_t> p95Us;
    std::optional<std::uint64_t> p99Us;
    std::optional<std::uint64_t> maxUs;
    std::uint64_t overflow = 0;
};

struct BenchmarkResult {
    BenchmarkMode mode = BenchmarkMode::Max;
    std::string build;
    std::uint64_t workers = 0;
    std::uint64_t devices = 0;
    std::uint64_t targetRate = 0;
    std::uint64_t durationMs = 0;

    std::uint64_t sent = 0;
    std::uint64_t processed = 0;
    std::uint64_t throughputPerSec = 0;
    std::uint64_t maxLag = 0;
    std::uint64_t queueAvg = 0;
    std::uint64_t queueMax = 0;

    LatencySummary total;
    LatencySummary service;
};

std::string formatResult(const BenchmarkResult& result);
