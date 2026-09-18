#pragma once

#include "common/Status.h"
#include <cstdint>

class StatsCollector {
  public:
    void record(Status status);

    std::uint64_t okCount() const;
    std::uint64_t warningCount() const;
    std::uint64_t criticalCount() const;
    std::uint64_t invalidCount() const;
    std::uint64_t totalCount() const;

private:
    std::uint64_t okCount_ = 0;
    std::uint64_t warningCount_ = 0;
    std::uint64_t criticalCount_ = 0;
    std::uint64_t invalidCount_ = 0;
};