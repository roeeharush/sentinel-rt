#pragma once

#include "common/benchmark/BenchmarkRunner.h"
#include <cstdint>
#include <optional>

struct BenchmarkArgs {
    BenchmarkConfig config;
    std::uint64_t warmupMs = 1000;
};

std::optional<BenchmarkArgs> parseBenchmarkArgs(int argc, char* argv[]);