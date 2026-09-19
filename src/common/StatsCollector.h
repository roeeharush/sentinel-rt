#pragma once

#include "common/Status.h"
#include <cstdint>

class StatsCollector {
  public:
    void record(Status status);

    [[nodiscard]] std::uint64_t okCount() const;
    [[nodiscard]] std::uint64_t warningCount() const;
    [[nodiscard]] std::uint64_t criticalCount() const;
    [[nodiscard]] std::uint64_t invalidCount() const;
    [[nodiscard]] std::uint64_t totalCount() const;

private:
    std::uint64_t okCount_ = 0;
    std::uint64_t warningCount_ = 0;
    std::uint64_t criticalCount_ = 0;
    std::uint64_t invalidCount_ = 0;
};