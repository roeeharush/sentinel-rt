#include "common/StatsCollector.h"

void StatsCollector::record(Status status) {
  switch (status) {
    case Status::Ok:
      ++okCount_;
      break;
    case Status::Warning:
      ++warningCount_;
      break;
    case Status::Critical:
      ++criticalCount_;
      break;
    case Status::Invalid:
      ++invalidCount_;
      break;
  }
}

std::uint64_t StatsCollector::okCount() const {
  return okCount_;
}

std::uint64_t StatsCollector::warningCount() const {
  return warningCount_;
}

std::uint64_t StatsCollector::criticalCount() const {
  return criticalCount_;
}

std::uint64_t StatsCollector::invalidCount() const {
  return invalidCount_;
}

std::uint64_t StatsCollector::totalCount() const {
  return okCount_ + warningCount_ + criticalCount_ + invalidCount_;
}