#include <gtest/gtest.h>
#include "common/DeviceSimArgs.h"

TEST(DeviceSimArgs, UsesDefaultsWhenNoArgumentsGiven) {
    const char* argv[] = {"device_sim"};
    int argc = 1;

    std::optional<DeviceSimConfig> config = parseDeviceSimArgs(argc, const_cast<char**>(argv));

    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->cycles, 10);
    EXPECT_EQ(config->periodMs, 100);
    EXPECT_FALSE(config->fault.has_value());
}

TEST(DeviceSimArgs, ParsesCyclesAndPeriodFlags) {
    const char* argv[] = {"device_sim", "--cycles", "50", "--period-ms", "200"};
    int argc = 5;

    std::optional<DeviceSimConfig> config = parseDeviceSimArgs(argc, const_cast<char**>(argv));

    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->cycles, 50);
    EXPECT_EQ(config->periodMs, 200);
}

TEST(DeviceSimArgs, RejectsNonNumericCycles) {
    const char* argv[] = {"device_sim", "--cycles", "abc"};
    int argc = 3;

    EXPECT_FALSE(parseDeviceSimArgs(argc, const_cast<char**>(argv)).has_value());
}

TEST(DeviceSimArgs, RejectsNegativeCycles) {
    const char* argv[] = {"device_sim", "--cycles", "-5"};
    int argc = 3;

    EXPECT_FALSE(parseDeviceSimArgs(argc, const_cast<char**>(argv)).has_value());
}

TEST(DeviceSimArgs, RejectsUnknownFlag) {
    const char* argv[] = {"device_sim", "--bogus", "1"};
    int argc = 3;

    EXPECT_FALSE(parseDeviceSimArgs(argc, const_cast<char**>(argv)).has_value());
}

TEST(DeviceSimArgs, RejectsFlagMissingValue) {
    const char* argv[] = {"device_sim", "--fault"};
    int argc = 2;

    EXPECT_FALSE(parseDeviceSimArgs(argc, const_cast<char**>(argv)).has_value());
}

TEST(DeviceSimArgs, FaultFlagActivatesImmediatelyByDefault) {
    const char* argv[] = {"device_sim", "--fault", "Stuck"};
    int argc = 3;

    std::optional<DeviceSimConfig> config = parseDeviceSimArgs(argc, const_cast<char**>(argv));

    ASSERT_TRUE(config.has_value());
    ASSERT_TRUE(config->fault.has_value());
    EXPECT_EQ(config->fault->type, FaultType::Stuck);
    EXPECT_EQ(config->fault->activateAfter, std::chrono::milliseconds(0));
}

TEST(DeviceSimArgs, FaultWithExplicitDelayIsParsedCorrectly) {
    const char* argv[] = {"device_sim", "--fault", "Drift", "--fault-after-ms", "500"};
    int argc = 5;

    std::optional<DeviceSimConfig> config = parseDeviceSimArgs(argc, const_cast<char**>(argv));

    ASSERT_TRUE(config.has_value());
    ASSERT_TRUE(config->fault.has_value());
    EXPECT_EQ(config->fault->type, FaultType::Drift);
    EXPECT_EQ(config->fault->activateAfter, std::chrono::milliseconds(500));
}

TEST(DeviceSimArgs, RejectsUnknownFaultName) {
    const char* argv[] = {"device_sim", "--fault", "Explosion"};
    int argc = 3;

    EXPECT_FALSE(parseDeviceSimArgs(argc, const_cast<char**>(argv)).has_value());
}

TEST(DeviceSimArgs, RejectsFaultAfterMsWithoutFault) {
    const char* argv[] = {"device_sim", "--fault-after-ms", "500"};
    int argc = 3;

    EXPECT_FALSE(parseDeviceSimArgs(argc, const_cast<char**>(argv)).has_value());
}

TEST(DeviceSimArgs, RejectsNonNumericFaultAfterMs) {
    const char* argv[] = {"device_sim", "--fault", "Stuck", "--fault-after-ms", "soon"};
    int argc = 5;

    EXPECT_FALSE(parseDeviceSimArgs(argc, const_cast<char**>(argv)).has_value());
}
