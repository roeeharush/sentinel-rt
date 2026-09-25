#pragma once

#include <optional>

struct AppConfig {
    int cycles;
    int periodMs;
    int workers;
    int queueCapacity;
    bool blockWhenFull;
};

std::optional<AppConfig> parseArgs(int argc, char* argv[]);