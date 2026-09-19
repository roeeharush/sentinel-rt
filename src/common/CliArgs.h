#pragma once

#include <optional>

struct AppConfig {
    int cycles;
    int periodMs;
};

std::optional<AppConfig> parseArgs(int argc, char* argv[]);