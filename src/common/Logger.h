#pragma once

#include <iostream>
#include <mutex>
#include <string>

enum class LogLevel {
    Info,
    Warning,
    Error
};

class Logger {
public:
    explicit Logger(std::ostream& output = std::cout);

    void log(LogLevel level, const std::string& message);

private:
    std::ostream& output_;
    std::mutex mutex_;
};