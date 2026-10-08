#include "common/benchmark/RatePacer.h"

namespace {

    constexpr std::uint64_t kNanosPerSecond = 1'000'000'000ULL;

}

RatePacer::RatePacer(std::uint64_t itemsPerSecond)
    : itemsPerSecond_(itemsPerSecond) {}

std::uint64_t RatePacer::scheduledCount(std::chrono::nanoseconds elapsed) const {
    std::int64_t nanos = elapsed.count();
    if (nanos < 0) {
        nanos = 0;
    }

    std::uint64_t total = static_cast<std::uint64_t>(nanos);

    // Split into whole seconds and a remainder below one second, so the
    // multiplication by the rate never produces a huge intermediate value.
    std::uint64_t wholeSeconds = total / kNanosPerSecond;
    std::uint64_t remainderNanos = total % kNanosPerSecond;

    return wholeSeconds * itemsPerSecond_ + remainderNanos * itemsPerSecond_ / kNanosPerSecond;
}
