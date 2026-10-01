#include "common/Codec.h"
#include "common/FrameReader.h"
#include "common/Measurement.h"
#include "common/SensorType.h"
#include "common/Socket.h"

#include <arpa/inet.h>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <netinet/in.h>
#include <optional>
#include <sys/socket.h>
#include <thread>

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

int main() {
    ignoreSigPipe();

    std::optional<Socket> serverSocket = connectWithBackoff("127.0.0.1", 5000);
    if (!serverSocket.has_value()) {
        std::cerr << "Failed to connect after " << kMaxConnectAttempts << " attempts\n";
        return 1;
    }

    std::cout << "Connected to server!\n";

    Measurement measurement;
    measurement.deviceId = 1;
    measurement.sensorType = SensorType::Temperature;
    measurement.timestamp = std::chrono::steady_clock::now();
    measurement.value = 73.5;

    std::vector<std::uint8_t> frame = encodeFrame(encodeMeasurement(measurement));

    if (!serverSocket->sendAll(frame)) {
        std::cerr << "Failed to send measurement\n";
        return 1;
    }

    std::cout << "Measurement sent\n";

    return 0;
}
