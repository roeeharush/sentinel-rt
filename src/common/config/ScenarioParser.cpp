#include "common/config/ScenarioParser.h"
#include <charconv>
#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>

namespace {

    ScenarioParseResult makeError(int line, const std::string& message) {
        ScenarioParseResult result;
        result.errorLine = line;
        result.errorMessage = message;
        return result;
    }

    std::optional<Metric> parseMetric(const std::string& text) {
        if (text == "ok") return Metric::Ok;
        if (text == "warning") return Metric::Warning;
        if (text == "critical") return Metric::Critical;
        if (text == "invalid") return Metric::Invalid;
        if (text == "total") return Metric::Total;
        return std::nullopt;
    }

    std::optional<Comparison> parseComparison(const std::string& text) {
        if (text == "==") return Comparison::Equal;
        if (text == ">=") return Comparison::AtLeast;
        if (text == "<=") return Comparison::AtMost;
        return std::nullopt;
    }

    std::optional<std::uint64_t> parseCount(const std::string& text) {
        const char* begin = text.data();
        const char* end = text.data() + text.size();

        std::uint64_t value = 0;
        auto result = std::from_chars(begin, end, value);
        if (result.ec != std::errc{} || result.ptr != end) {
            return std::nullopt;
        }
        return value;
    }

    std::optional<int> parseNonNegativeInt(const std::string& text) {
        std::optional<std::uint64_t> count = parseCount(text);
        if (!count || *count > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
            return std::nullopt;
        }
        return static_cast<int>(*count);
    }

    std::optional<int> parsePositiveInt(const std::string& text) {
        std::optional<int> value = parseNonNegativeInt(text);
        if (!value || *value == 0) {
            return std::nullopt;
        }
        return value;
    }

}

ScenarioParseResult parseScenario(std::istream& input) {
    Scenario scenario;
    scenario.config.cycles = 10;
    scenario.config.periodMs = 100;
    bool cyclesSet = false;
    bool periodSet = false;
    std::string line;
    int lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;

        std::istringstream words(line);
        std::string keyword;
        if (!(words >> keyword) || keyword[0] == '#') {
            continue;
        }

        if (keyword == "expect") {
            std::string metricWord;
            std::string comparisonWord;
            std::string valueWord;
            if (!(words >> metricWord >> comparisonWord >> valueWord)) {
                return makeError(lineNumber, "expect needs a metric, a comparison and a value");
            }

            std::optional<Metric> metric = parseMetric(metricWord);
            if (!metric) {
                return makeError(lineNumber, "unknown metric '" + metricWord + "'");
            }

            std::optional<Comparison> comparison = parseComparison(comparisonWord);
            if (!comparison) {
                return makeError(lineNumber, "unknown comparison '" + comparisonWord + "'");
            }

            std::optional<std::uint64_t> value = parseCount(valueWord);
            if (!value) {
                return makeError(lineNumber, "invalid value '" + valueWord + "'");
            }

            scenario.expectations.push_back(Expectation{*metric, *comparison, *value});
        } else if (keyword == "cycles" || keyword == "period_ms") {
            bool& alreadySet = (keyword == "cycles") ? cyclesSet : periodSet;
            if (alreadySet) {
                return makeError(lineNumber, keyword + " is set more than once");
            }
            alreadySet = true;

            std::string valueWord;
            if (!(words >> valueWord)) {
                return makeError(lineNumber, keyword + " needs a value");
            }

            std::optional<int> value = parsePositiveInt(valueWord);
            if (!value) {
                return makeError(lineNumber, "invalid value '" + valueWord + "'");
            }

            if (keyword == "cycles") {
                scenario.config.cycles = *value;
            } else {
                scenario.config.periodMs = *value;
            }
        } else if (keyword == "at") {
            std::string timeWord;
            std::string eventWord;
            std::string typeWord;
            if (!(words >> timeWord >> eventWord >> typeWord)) {
                return makeError(lineNumber, "at needs a time, 'fault' and a fault type");
            }

            if (eventWord != "fault") {
                return makeError(lineNumber, "unknown event '" + eventWord + "'");
            }

            if (scenario.config.fault) {
                return makeError(lineNumber, "only one fault per scenario is supported");
            }

            std::optional<int> timeMs = parseNonNegativeInt(timeWord);
            if (!timeMs) {
                return makeError(lineNumber, "invalid time '" + timeWord + "'");
            }

            std::optional<FaultType> faultType = parseFaultType(typeWord);
            if (!faultType) {
                return makeError(lineNumber, "unknown fault type '" + typeWord + "'");
            }

            scenario.config.fault = TimedFault{*faultType, std::chrono::milliseconds(*timeMs)};
        } else {
            return makeError(lineNumber, "unknown keyword '" + keyword + "'");
        }

        std::string extra;
        if (words >> extra) {
            return makeError(lineNumber, "unexpected text: '" + extra + "'");
        }
    }

    if (scenario.expectations.empty()) {
        return makeError(0, "scenario contains no expectations");
    }

    ScenarioParseResult result;
    result.scenario = scenario;
    return result;
}