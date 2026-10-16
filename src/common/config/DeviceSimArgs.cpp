#include "common/config/DeviceSimArgs.h"
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

std::optional<FaultType> parseFaultType(const std::string& text) {
    if (text == "Spike") return FaultType::Spike;
    if (text == "Drop") return FaultType::Drop;
    if (text == "Stuck") return FaultType::Stuck;
    if (text == "Drift") return FaultType::Drift;
    if (text == "Noise") return FaultType::Noise;
    if (text == "Invalid") return FaultType::Invalid;
    return std::nullopt;
}

std::optional<DeviceSimConfig> parseDeviceSimArgs(int argc, char* argv[]) {
    DeviceSimConfig config{10, 100, std::nullopt};

    std::optional<FaultType> faultType;
    std::optional<int> faultAfterMs;

    int i = 1;
    while (i < argc) {
        std::string arg = argv[i];

        if (arg == "--cycles" && i + 1 < argc) {
            std::optional<int> value = parsePositiveInt(argv[i + 1]);
            if (!value) return std::nullopt;
            config.cycles = *value;
            i += 2;
        }
        else if (arg == "--period-ms" && i + 1 < argc) {
            std::optional<int> value = parsePositiveInt(argv[i + 1]);
            if (!value) return std::nullopt;
            config.periodMs = *value;
            i += 2;
        }
        else if (arg == "--device-id" && i + 1 < argc) {
            std::optional<int> value = parsePositiveInt(argv[i + 1]);
            if (!value) return std::nullopt;
            config.deviceId = static_cast<std::uint32_t>(*value);
            i += 2;
        }
        else if (arg == "--fault" && i + 1 < argc) {
            faultType = parseFaultType(argv[i + 1]);
            if (!faultType) return std::nullopt;
            i += 2;
        }
        else if (arg == "--fault-after-ms" && i + 1 < argc) {
            faultAfterMs = parsePositiveInt(argv[i + 1]);
            if (!faultAfterMs) return std::nullopt;
            i += 2;
        }
        else {
            return std::nullopt;
        }
    }

    if (faultAfterMs && !faultType) {
        return std::nullopt;
    }

    if (faultType) {
        int delay = faultAfterMs.value_or(0);
        config.fault = TimedFault{*faultType, std::chrono::milliseconds(delay)};
    }

    return config;
}
