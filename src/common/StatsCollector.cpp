#include "common/StatsCollector.h"

StatsCollector::StatsCollector(std::chrono::nanoseconds deadline)
    : deadline_(deadline) {}

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

void StatsCollector::recordLatency(std::chrono::nanoseconds latency) {
  std::int64_t desired = latency.count();
  std::int64_t current = maxLatencyNs_.load();
  while (desired > current && !maxLatencyNs_.compare_exchange_weak(current, desired)) {
  }
  if (latency > deadline_) {
    ++deadlineMissCount_;
  }
}

std::uint64_t StatsCollector::okCount() const {
  return okCount_.load();
}

std::uint64_t StatsCollector::warningCount() const {
  return warningCount_.load();
}

std::uint64_t StatsCollector::criticalCount() const {
  return criticalCount_.load();
}

std::uint64_t StatsCollector::invalidCount() const {
  return invalidCount_.load();
}

std::uint64_t StatsCollector::totalCount() const {
  return okCount_.load() + warningCount_.load() + criticalCount_.load() + invalidCount_.load();
}

std::chrono::nanoseconds StatsCollector::maxLatency() const {
  return std::chrono::nanoseconds(maxLatencyNs_.load());
}

std::uint64_t StatsCollector::deadlineMissCount() const {
  return deadlineMissCount_.load();
}