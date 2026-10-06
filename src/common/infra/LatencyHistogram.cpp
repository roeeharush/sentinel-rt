#include "common/infra/LatencyHistogram.h"
#include <cmath>

namespace {

    constexpr std::uint64_t kNanosPerMicro = 1000;

}

LatencyHistogram::LatencyHistogram(std::size_t bucketCount)
    : buckets_(bucketCount) {}

void LatencyHistogram::record(std::chrono::nanoseconds latency) {
    std::int64_t nanos = latency.count();
    if (nanos < 0) {
        nanos = 0;
    }

    std::uint64_t micros = (static_cast<std::uint64_t>(nanos) + kNanosPerMicro - 1) / kNanosPerMicro;

    if (micros >= buckets_.size()) {
        overflow_.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    buckets_[micros].fetch_add(1, std::memory_order_relaxed);
}

std::uint64_t LatencyHistogram::count() const {
    std::uint64_t total = overflow_.load(std::memory_order_relaxed);
    for (const std::atomic<std::uint64_t>& bucket : buckets_) {
        total += bucket.load(std::memory_order_relaxed);
    }
    return total;
}

std::uint64_t LatencyHistogram::overflowCount() const {
    return overflow_.load(std::memory_order_relaxed);
}

std::optional<std::uint64_t> LatencyHistogram::percentileMicros(double percent) const {
    if (!(percent > 0.0 && percent <= 100.0)) {
        return std::nullopt;
    }

    std::uint64_t total = count();
    if (total == 0) {
        return std::nullopt;
    }

    std::uint64_t rank = static_cast<std::uint64_t>(std::ceil(percent * static_cast<double>(total) / 100.0));

    std::uint64_t cumulative = 0;
    for (std::size_t i = 0; i < buckets_.size(); ++i) {
        cumulative += buckets_[i].load(std::memory_order_relaxed);
        if (cumulative >= rank) {
            return static_cast<std::uint64_t>(i);
        }
    }

    return std::nullopt;
}
