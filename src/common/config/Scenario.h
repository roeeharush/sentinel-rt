#pragma once
#include "common/config/DeviceSimArgs.h"
#include <cstdint>
#include <vector>

enum class Metric {
    Ok,
    Warning,
    Critical,
    Invalid,
    Total
};

enum class Comparison {
    Equal,
    AtLeast,
    AtMost
};

struct Expectation {
    Metric metric;
    Comparison comparison;
    std::uint64_t value;
};

struct Scenario {
    DeviceSimConfig config;
    std::vector<Expectation> expectations;
};