#include "common/infra/Logger.h"
#include <chrono>
#include <ctime>
#include <iomanip>

Logger::Logger(std::ostream& output) : output_(output) {}

namespace {

    const char* levelLabel(LogLevel level) {
        switch (level) {
            case LogLevel::Info:
                return "INFO";
            case LogLevel::Warning:
                return "WARNING";
            case LogLevel::Error:
                return "ERROR";
        }
        return "INFO";
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
    localtime_r(&nowTime, &localTime);

    std::lock_guard lock(mutex_);
    output_ << "[" << std::put_time(&localTime, "%H:%M:%S") << "] "<< levelLabel(level) << ": " << message << "\n";
}

