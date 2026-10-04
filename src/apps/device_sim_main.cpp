#include "common/network/Codec.h"
#include "common/sensors/Device.h"
#include "common/config/DeviceSimArgs.h"
#include "common/sensors/FaultySensor.h"
#include "common/network/FrameReader.h"
#include "common/infra/IClock.h"
#include "common/core/Measurement.h"
#include "common/infra/RealClock.h"
#include "common/sensors/Sensor.h"
#include "common/core/SensorType.h"
#include "common/network/Socket.h"

#include <arpa/inet.h>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <optional>
#include <sys/socket.h>
#include <thread>
#include <vector>

namespace {

    constexpr int kMaxConnectAttempts = 5;
    constexpr std::chrono::seconds kRetryDelay(1);

    std::optional<Socket> connectWithBackoff(const char* host, std::uint16_t port) {
        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        ::inet_pton(AF_INET, host, &serverAddress.sin_addr);

        for (int attempt = 1; attempt <= kMaxConnectAttempts; ++attempt) {
            int socketFd = ::socket(AF_INET, SOCK_STREAM, 0);
            if (socketFd < 0) {
                std::cerr << "Failed to create socket\n";
                return std::nullopt;
            }

            Socket candidate(socketFd);
            if (::connect(candidate.fd(), reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == 0) {
                return candidate;
            }

            std::cerr << "Connect attempt " << attempt << " of " << kMaxConnectAttempts << " failed\n";
            if (attempt < kMaxConnectAttempts) {
                std::this_thread::sleep_for(kRetryDelay);
            }
        }
        return std::nullopt;
    }

}

int main(int argc, char* argv[]) {
    ignoreSigPipe();

    std::optional<DeviceSimConfig> config = parseDeviceSimArgs(argc, argv);
    if (!config) {
        std::cerr << "Usage: device_sim [--cycles N] [--period-ms N] [--fault TYPE] [--fault-after-ms N]\n";
        return 1;
    }

    std::optional<Socket> serverSocket = connectWithBackoff("127.0.0.1", 5000);
    if (!serverSocket.has_value()) {
        std::cerr << "Failed to connect after " << kMaxConnectAttempts << " attempts\n";
        return 1;
    }

    std::cout << "Connected to server!\n";

    auto temperatureSensor = std::make_unique<FaultySensor>(
        std::make_unique<Sensor>(SensorType::Temperature, temperatureSpec(), 42), 123);
    FaultySensor* faultySensorPtr = temperatureSensor.get();

    std::vector<std::unique_ptr<ISensor>> sensors;
    sensors.push_back(std::move(temperatureSensor));
    sensors.push_back(std::make_unique<Sensor>(SensorType::Pressure, pressureSpec(), 7));
    sensors.push_back(std::make_unique<Sensor>(SensorType::Voltage, voltageSpec(), 99));
    Device device(1, std::move(sensors));

    RealClock clock;
    std::chrono::steady_clock::time_point startTime = clock.now();
    std::chrono::steady_clock::time_point nextWakeTime = startTime;
    bool faultActivated = false;

    for (int cycle = 0; cycle < config->cycles; ++cycle) {
        if (config->fault && !faultActivated) {
            auto elapsed = clock.now() - startTime;
            if (elapsed >= config->fault->activateAfter) {
                faultySensorPtr->setFault(config->fault->type);
                faultActivated = true;
            }
        }

        std::vector<Measurement> measurements = device.sample(clock.now());
        for (const Measurement& measurement : measurements) {
            std::vector<std::uint8_t> frame = encodeFrame(encodeMeasurement(measurement));
            if (!serverSocket->sendAll(frame)) {
                std::cerr << "Failed to send measurement\n";
                return 1;
            }
        }

        nextWakeTime += std::chrono::milliseconds(config->periodMs);
        clock.sleepUntil(nextWakeTime);
    }

    std::cout << "Finished " << config->cycles << " cycles.\n";

    return 0;
}
