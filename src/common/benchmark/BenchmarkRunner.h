#pragma once

#include "common/benchmark/BenchmarkResult.h"
#include <cstdint>

struct BenchmarkConfig {
    BenchmarkMode mode = BenchmarkMode::Max;
    std::uint64_t workers = 1;
    std::uint64_t devices = 1;
    std::uint64_t targetRate = 0;
    std::uint64_t durationMs = 1000;
};

BenchmarkResult runBenchmark(const BenchmarkConfig& config);
