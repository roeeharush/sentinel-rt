#include <gtest/gtest.h>
#include "common/infra/Logger.h"
#include <sstream>

TEST(Logger, WritesLevelAndMessage) {
    std::ostringstream captured;
    Logger logger(captured);

    logger.log(LogLevel::Info, "Device connected");

    EXPECT_NE(captured.str().find("INFO: Device connected"), std::string::npos);
}

TEST(Logger, DifferentLevelsProduceDifferentLabels) {
    std::ostringstream captured;
    Logger logger(captured);

    logger.log(LogLevel::Warning, "Temperature rising");

    EXPECT_NE(captured.str().find("WARNING: Temperature rising"), std::string::npos);
}
