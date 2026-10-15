#include "common/benchmark/BenchmarkResult.h"
#include "common/benchmark/BenchmarkRunner.h"
#include "common/config/BenchmarkArgs.h"
#include <optional>
#include <iostream>

#ifndef RT_BUILD_TYPE
#define RT_BUILD_TYPE "unknown"
#endif

namespace {

    constexpr int kExitOk = 0;
    constexpr int kExitBadInput = 2;

    constexpr const char* kUsage =
        "Usage: benchmark [--mode max|rate|calibrate] [--workers N] [--devices N]\n"
        "                 [--rate N] [--duration-ms N] [--warmup-ms N]\n";

}

int main(int argc, char* argv[]) {
    std::optional<BenchmarkArgs> args = parseBenchmarkArgs(argc, argv);
    if (!args.has_value()) {
        std::cerr << kUsage;
        return kExitBadInput;
    }

    if (args->warmupMs > 0) {
        BenchmarkConfig warmup = args->config;
        warmup.durationMs = args->warmupMs;
        runBenchmark(warmup);
    }
    BenchmarkResult result = runBenchmark(args->config);
    result.build = RT_BUILD_TYPE;

    std::cout << formatResult(result) << "\n";
    return kExitOk;
}