#include "common/config/ScenarioParser.h"
#include "common/config/ScenarioRunner.h"

#include <fstream>
#include <iostream>
#include <string>

namespace {

    constexpr int kExitAllPassed = 0;
    constexpr int kExitExpectationFailed = 1;
    constexpr int kExitBadInput = 2;

    const char* metricName(Metric metric) {
        switch (metric) {
            case Metric::Ok: return "ok";
            case Metric::Warning: return "warning";
            case Metric::Critical: return "critical";
            case Metric::Invalid: return "invalid";
            case Metric::Total: return "total";
        }
        return "?";
    }

    const char* comparisonSymbol(Comparison comparison) {
        switch (comparison) {
            case Comparison::Equal: return "==";
            case Comparison::AtLeast: return ">=";
            case Comparison::AtMost: return "<=";
        }
        return "?";
    }

    bool allPassed(const ScenarioOutcome& outcome) {
        for (const ExpectationResult& result : outcome.results) {
            if (!result.passed) {
                return false;
            }
        }
        return true;
    }

    void printOutcome(const ScenarioOutcome& outcome) {
        std::cout << "  ok=" << outcome.ok
                  << " warning=" << outcome.warning
                  << " critical=" << outcome.critical
                  << " invalid=" << outcome.invalid
                  << " total=" << outcome.total << "\n";

        for (const ExpectationResult& result : outcome.results) {
            std::cout << "  " << (result.passed ? "PASS" : "FAIL") << "  "
                      << metricName(result.expectation.metric) << " "
                      << comparisonSymbol(result.expectation.comparison) << " "
                      << result.expectation.value
                      << "  (actual " << result.actual << ")\n";
        }
    }

}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: scenario_runner <scenario-file>...\n";
        return kExitBadInput;
    }

    bool badInput = false;
    bool anyFailed = false;

    for (int i = 1; i < argc; ++i) {
        std::string path = argv[i];

        std::ifstream file(path);
        if (!file) {
            std::cerr << path << ": cannot open file\n";
            badInput = true;
            continue;
        }

        ScenarioParseResult parsed = parseScenario(file);
        if (!parsed.scenario) {
            std::cerr << path;
            if (parsed.errorLine > 0) {
                std::cerr << ":" << parsed.errorLine;
            }
            std::cerr << ": " << parsed.errorMessage << "\n";
            badInput = true;
            continue;
        }

        ScenarioOutcome outcome = runScenario(*parsed.scenario);
        bool passed = allPassed(outcome);

        std::cout << path << "\n";
        printOutcome(outcome);
        std::cout << "  RESULT: " << (passed ? "PASS" : "FAIL") << "\n";

        if (!passed) {
            anyFailed = true;
        }
    }

    if (badInput) {
        return kExitBadInput;
    }
    if (anyFailed) {
        return kExitExpectationFailed;
    }
    return kExitAllPassed;
}
