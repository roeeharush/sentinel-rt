#pragma once

#include <chrono>
#include <cstdint>

class RatePacer {
public:
    explicit RatePacer(std::uint64_t itemsPerSecond);

    std::uint64_t scheduledCount(std::chrono::nanoseconds elapsed) const;

private:
    std::uint64_t itemsPerSecond_;
};
