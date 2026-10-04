#pragma once

#include "common/config/ExpectationEvaluator.h"
#include "common/config/Scenario.h"
#include <cstdint>
#include <vector>

struct ScenarioOutcome {
    std::uint64_t ok;
    std::uint64_t warning;
    std::uint64_t critical;
    std::uint64_t invalid;
    std::uint64_t total;
    std::vector<ExpectationResult> results;
};

ScenarioOutcome runScenario(const Scenario& scenario);
