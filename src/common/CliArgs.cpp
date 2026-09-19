#include "common/CliArgs.h"
#include <string>

namespace {

    std::optional<int> parsePositiveInt(const char* text) {
        try {
            int value = std::stoi(text);
            if (value <= 0) {
                return std::nullopt;
            }
            return value;
        } catch (...) {
            return std::nullopt;
        }
    }

}

std::optional<AppConfig> parseArgs(int argc, char* argv[]) {
    AppConfig config{10, 100};

    int i = 1;
    while (i < argc) {
        std::string arg = argv[i];

        if (arg == "--cycles" && i + 1 < argc) {
            std::optional<int> value = parsePositiveInt(argv[i + 1]);
            if (!value) {
                return std::nullopt;
            }
            config.cycles = *value;
            i += 2;
        } else if (arg == "--period-ms" && i + 1 < argc) {
            std::optional<int> value = parsePositiveInt(argv[i + 1]);
            if (!value) {
                return std::nullopt;
            }
            config.periodMs = *value;
            i += 2;
        } else {
            return std::nullopt;
        }
    }

    return config;
}