#include "common/benchmark/BenchmarkResult.h"

namespace {

    const char* modeName(BenchmarkMode mode) {
        switch (mode) {
            case BenchmarkMode::Max:
                return "max";
            case BenchmarkMode::Rate:
                return "rate";
            case BenchmarkMode::Calibrate:
                return "calibrate";
        }
        return "unknown";
    }

    void appendField(std::string& line, const std::string& key, const std::string& value) {
        if (!line.empty()) {
            line += ' ';
        }
        line += key;
        line += '=';
        line += value;
    }

    void appendNumber(std::string& line, const std::string& key, std::uint64_t value) {
        appendField(line, key, std::to_string(value));
    }

    void appendOptional(std::string& line, const std::string& key, std::optional<std::uint64_t> value) {
        appendField(line, key, value.has_value() ? std::to_string(*value) : "NA");
    }

    void appendLatency(std::string& line, const std::string& prefix, const LatencySummary& summary) {
        appendOptional(line, prefix + "_p50_us", summary.p50Us);
        appendOptional(line, prefix + "_p95_us", summary.p95Us);
        appendOptional(line, prefix + "_p99_us", summary.p99Us);
        appendOptional(line, prefix + "_max_us", summary.maxUs);
        appendNumber(line, prefix + "_overflow", summary.overflow);
    }

}

std::string formatResult(const BenchmarkResult& result) {
    std::string line;

    appendField(line, "mode", modeName(result.mode));
    appendField(line, "build", result.build);
    appendNumber(line, "workers", result.workers);
    appendNumber(line, "devices", result.devices);
    appendNumber(line, "target_rate", result.targetRate);
    appendNumber(line, "duration_ms", result.durationMs);

    appendNumber(line, "sent", result.sent);
    appendNumber(line, "processed", result.processed);
    appendNumber(line, "throughput_per_s", result.throughputPerSec);
    appendNumber(line, "max_lag", result.maxLag);
    appendNumber(line, "queue_avg", result.queueAvg);
    appendNumber(line, "queue_max", result.queueMax);

    appendLatency(line, "total", result.total);
    appendLatency(line, "service", result.service);

    return line;
}
