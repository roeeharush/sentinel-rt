#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

constexpr std::size_t kDefaultHistogramBuckets = 100000;

class LatencyHistogram {
public:
    explicit LatencyHistogram(std::size_t bucketCount = kDefaultHistogramBuckets);

    void record(std::chrono::nanoseconds latency);

    std::uint64_t count() const;
    std::uint64_t overflowCount() const;
    std::optional<std::uint64_t> percentileMicros(double percent) const;

private:
    std::vector<std::atomic<std::uint64_t>> buckets_;
    std::atomic<std::uint64_t> overflow_{0};
};
