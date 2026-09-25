#include "common/CliArgs.h"
#include <string>
#include <thread>

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

    int defaultWorkerCount() {
        unsigned int detected = std::thread::hardware_concurrency();
        return detected > 0 ? static_cast<int>(detected) : 4;
    }

    std::optional<bool> parseBool(const char* text) {
        std::string value = text;
        if (value == "true") {
            return true;
        }
        if (value == "false") {
            return false;
        }
        return std::nullopt;
    }
}

std::optional<AppConfig> parseArgs(int argc, char* argv[]) {
    AppConfig config{10, 100, defaultWorkerCount(), 100, true};

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
        }
        else if (arg == "--period-ms" && i + 1 < argc) {
            std::optional<int> value = parsePositiveInt(argv[i + 1]);
            if (!value) {
                return std::nullopt;
            }
            config.periodMs = *value;
            i += 2;
        }
        else if (arg == "--workers" && i + 1 < argc) {
            std::optional<int> value = parsePositiveInt(argv[i + 1]);
            if (!value) {
                return std::nullopt;
            }
            config.workers = *value;
            i += 2;
        }
        else if (arg == "--queue-capacity" && i + 1 < argc) {
            std::optional<int> value = parsePositiveInt(argv[i + 1]);
            if (!value) {
                return std::nullopt;
            }
            config.queueCapacity = *value;
            i += 2;
        }
        else if (arg == "--block-when-full" && i + 1 < argc) {
            std::optional<bool> value = parseBool(argv[i + 1]);
            if (!value) {
                return std::nullopt;
            }
            config.blockWhenFull = *value;
            i += 2;
        }
        else {
            return std::nullopt;
        }
    }
    return config;
}