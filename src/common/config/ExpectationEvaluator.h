#pragma once

#include "common/config/Scenario.h"
#include "common/infra/StatsCollector.h"
#include <cstdint>
#include <vector>

struct ExpectationResult {
    Expectation expectation;
    std::uint64_t actual;
    bool passed;
};

std::vector<ExpectationResult> evaluateExpectations(const std::vector<Expectation>& expectations,const StatsCollector& stats);
