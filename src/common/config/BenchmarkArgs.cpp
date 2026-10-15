#include "common/config/BenchmarkArgs.h"

#include <charconv>
#include <cstdint>
#include <cstring>
#include <string>
#include <system_error>

namespace {

    constexpr std::uint64_t kMaxWorkers = 256;
    constexpr std::uint64_t kMaxDevices = 100000;
    constexpr std::uint64_t kMaxDurationMs = 3600000;
    constexpr std::uint64_t kMaxWarmupMs = 3600000;
    constexpr std::uint64_t kMaxRate = 10000000000ULL;

    std::optional<std::uint64_t> parseNumber(const char* text) {
        std::size_t length = std::strlen(text);
        if (length == 0) {
            return std::nullopt;
        }

        std::uint64_t value = 0;
        const char* end = text + length;
        std::from_chars_result result = std::from_chars(text, end, value);

        if (result.ec != std::errc() || result.ptr != end) {
            return std::nullopt;
        }

        return value;
    }

    std::optional<std::uint64_t> parseNumberInRange(const char* text, std::uint64_t min, std::uint64_t max) {
        std::optional<std::uint64_t> value = parseNumber(text);
        if (!value.has_value() || *value < min || *value > max) {
            return std::nullopt;
        }

        return value;
    }

    std::optional<BenchmarkMode> parseMode(const std::string& text) {
        if (text == "max") {
            return BenchmarkMode::Max;
        }
        if (text == "rate") {
            return BenchmarkMode::Rate;
        }
        if (text == "calibrate") {
            return BenchmarkMode::Calibrate;
        }

        return std::nullopt;
    }

    bool readNumber(const char* text, std::uint64_t min, std::uint64_t max, std::uint64_t& target) {
        std::optional<std::uint64_t> value = parseNumberInRange(text, min, max);
        if (!value.has_value()) {
            return false;
        }

        target = *value;
        return true;
    }
}

std::optional<BenchmarkArgs> parseBenchmarkArgs(int argc, char* argv[]) {
    BenchmarkArgs args;
    args.config.devices = 10;
    args.config.durationMs = 5000;
    bool rateGiven = false;

    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) {
            return std::nullopt;
        }

        std::string option = argv[i];
        const char* value = argv[i + 1];
        bool ok = false;

        if (option == "--mode") {
            std::optional<BenchmarkMode> mode = parseMode(value);
            if (mode.has_value()) {
                args.config.mode = *mode;
                ok = true;
            }
        }
        else if (option == "--workers") {
            ok = readNumber(value, 1, kMaxWorkers, args.config.workers);
        }
        else if (option == "--devices") {
            ok = readNumber(value, 1, kMaxDevices, args.config.devices);
        }
        else if (option == "--rate") {
            ok = readNumber(value, 1, kMaxRate, args.config.targetRate);
            rateGiven = ok;
        }
        else if (option == "--duration-ms") {
            ok = readNumber(value, 1, kMaxDurationMs, args.config.durationMs);
        }
        else if (option == "--warmup-ms") {
            ok = readNumber(value, 0, kMaxWarmupMs, args.warmupMs);
        }

        if (!ok) {
            return std::nullopt;
        }
    }

    if (args.config.mode == BenchmarkMode::Rate) {
        if (!rateGiven) {
            return std::nullopt;
        }
    }
    else if (rateGiven) {
        return std::nullopt;
    }

    return args;
}