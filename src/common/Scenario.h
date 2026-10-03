#pragma once
#include "common/DeviceSimArgs.h"
#include <cstdint>
#include <vector>

enum class Metric {
    Ok,
    Warning,
    Critical,
    Invalid,
    Total,
    DeadlineMisses
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