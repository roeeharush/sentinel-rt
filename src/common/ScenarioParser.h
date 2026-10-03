#pragma once
#include "common/Scenario.h"
#include <istream>
#include <optional>
#include <string>

struct ScenarioParseResult {
    std::optional<Scenario> scenario;
    int errorLine = 0;
    std::string errorMessage;
};

ScenarioParseResult parseScenario(std::istream& input);