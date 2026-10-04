#include <gtest/gtest.h>
#include "common/sensors/FaultySensor.h"
#include <cmath>
#include <memory>

namespace {

    class StubSensor : public ISensor {
    public:
        explicit StubSensor(double value) : value_(value) {}

        SensorType type() const override { return SensorType::Temperature; }
        double read() override { ++readCount_; return value_; }

        void setValue(double value) { value_ = value; }
        int readCount() const { return readCount_; }

    private:
        double value_;
        int readCount_ = 0;
    };

}

TEST(FaultySensor, NoFaultReturnsInnerValueUnchanged) {
    auto stub = std::make_unique<StubSensor>(55.0);
    FaultySensor sensor(std::move(stub), 1);

    EXPECT_DOUBLE_EQ(sensor.read(), 55.0);
}

TEST(FaultySensor, SpikeMultipliesInnerValue) {
    auto stub = std::make_unique<StubSensor>(55.0);
    FaultySensor sensor(std::move(stub), 1);
    sensor.setFault(FaultType::Spike);

    EXPECT_DOUBLE_EQ(sensor.read(), 275.0);
}

TEST(FaultySensor, DropReducesInnerValue) {
    auto stub = std::make_unique<StubSensor>(55.0);
    FaultySensor sensor(std::move(stub), 1);
    sensor.setFault(FaultType::Drop);

    EXPECT_DOUBLE_EQ(sensor.read(), 5.5);
}

TEST(FaultySensor, StuckReturnsFirstValueRepeatedly) {
    auto stub = std::make_unique<StubSensor>(55.0);
    StubSensor* rawStub = stub.get();
    FaultySensor sensor(std::move(stub), 1);
    sensor.setFault(FaultType::Stuck);

    double first = sensor.read();
    rawStub->setValue(999.0);
    double second = sensor.read();

    EXPECT_DOUBLE_EQ(first, 55.0);
    EXPECT_DOUBLE_EQ(second, 55.0);
    EXPECT_EQ(rawStub->readCount(), 2);
}

TEST(FaultySensor, DriftAccumulatesOverSuccessiveReads) {
    auto stub = std::make_unique<StubSensor>(55.0);
    FaultySensor sensor(std::move(stub), 1);
    sensor.setFault(FaultType::Drift);

    EXPECT_DOUBLE_EQ(sensor.read(), 56.0);
    EXPECT_DOUBLE_EQ(sensor.read(), 57.0);
    EXPECT_DOUBLE_EQ(sensor.read(), 58.0);
}

TEST(FaultySensor, NoiseVariesAroundInnerValue) {
    auto stub = std::make_unique<StubSensor>(55.0);
    FaultySensor sensor(std::move(stub), 1);
    sensor.setFault(FaultType::Noise);

    double first = sensor.read();
    double second = sensor.read();

    EXPECT_NE(first, second);
    EXPECT_GT(first, 22.0);
    EXPECT_LT(first, 88.0);
}

TEST(FaultySensor, InvalidReturnsNaN) {
    auto stub = std::make_unique<StubSensor>(55.0);
    FaultySensor sensor(std::move(stub), 1);
    sensor.setFault(FaultType::Invalid);

    EXPECT_TRUE(std::isnan(sensor.read()));
}