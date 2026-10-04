#include "common/config/ExpectationEvaluator.h"

namespace {

    std::uint64_t metricValue(const StatsCollector& stats, Metric metric) {
        switch (metric) {
            case Metric::Ok:
                return stats.okCount();
            case Metric::Warning:
                return stats.warningCount();
            case Metric::Critical:
                return stats.criticalCount();
            case Metric::Invalid:
                return stats.invalidCount();
            case Metric::Total:
                return stats.totalCount();
        }
        return 0;
    }

    bool satisfies(std::uint64_t actual, Comparison comparison, std::uint64_t expected) {
        switch (comparison) {
            case Comparison::Equal:
                return actual == expected;
            case Comparison::AtLeast:
                return actual >= expected;
            case Comparison::AtMost:
                return actual <= expected;
        }
        return false;
    }

}

std::vector<ExpectationResult> evaluateExpectations(const std::vector<Expectation>& expectations,const StatsCollector& stats) {
    std::vector<ExpectationResult> results;
    results.reserve(expectations.size());

    for (const Expectation& expectation : expectations) {
        std::uint64_t actual = metricValue(stats, expectation.metric);
        bool passed = satisfies(actual, expectation.comparison, expectation.value);
        results.push_back(ExpectationResult{expectation, actual, passed});
    }

    return results;
}
