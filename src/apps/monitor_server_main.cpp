#include "common/pipeline/BoundedQueue.h"
#include "common/network/ConnectionHandler.h"
#include "common/infra/Logger.h"
#include "common/core/Measurement.h"
#include "common/network/Socket.h"
#include "common/detection/SlidingWindowDetector.h"
#include "common/infra/StatsCollector.h"
#include "common/detection/ThresholdDetector.h"
#include "common/detection/TimeoutMonitor.h"
#include "common/pipeline/WorkerPool.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <utility>
#include <vector>

namespace {

    constexpr std::size_t kQueueCapacity = 1024;
    constexpr std::size_t kWorkerCount = 4;
    constexpr int kListenBacklog = 10;
    constexpr std::size_t kWindowCapacity = 10;
    constexpr std::chrono::milliseconds kTimeoutThreshold(2000);
    constexpr std::chrono::milliseconds kTimeoutCheckInterval(500);

    LogLevel levelForStatus(Status status) {
        switch (status) {
            case Status::Warning:
                return LogLevel::Warning;
            case Status::Critical:
            case Status::Invalid:
                return LogLevel::Error;
            case Status::Ok:
                return LogLevel::Info;
        }
        return LogLevel::Info;
    }

    Status worse(Status a, Status b) {
        return static_cast<int>(b) > static_cast<int>(a) ? b : a;
    }

}

int main() {
    ignoreSigPipe();

    int listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    Socket listenSocket(listenFd);

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(5000);

    if (::bind(listenSocket.fd(), reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to bind\n";
        return 1;
    }

    std::cout << "Bound to port 5000\n";

    if (::listen(listenSocket.fd(), kListenBacklog) < 0) {
        std::cerr << "Failed to listen\n";
        return 1;
    }

    std::cout << "Listening on port 5000...\n";

    ThresholdDetector detector(defaultThresholds());
    Logger logger;
    StatsCollector stats(std::chrono::milliseconds(20));
    BoundedQueue<Measurement> queue(kQueueCapacity);
    SlidingWindowDetector windowDetector(kWindowCapacity);
    TimeoutMonitor timeoutMonitor(kTimeoutThreshold);

    WorkerPool<Measurement> pool(queue, logger, [&](Measurement measurement) {
        auto now = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(now - measurement.timestamp);
        stats.recordLatency(latency);

        timeoutMonitor.recordArrival(measurement.deviceId, now);

        Status thresholdStatus = detector.evaluate(measurement);
        Status windowStatus = windowDetector.evaluate(measurement);
        Status status = worse(thresholdStatus, windowStatus);

        stats.record(status);
        if (status != Status::Ok) {
            logger.log(levelForStatus(status), "Measurement outside normal range");
        } else {
            logger.log(LogLevel::Info, "Measurement received");
        }
    }, kWorkerCount);

    std::thread([&timeoutMonitor, &logger]() {
        while (true) {
            std::this_thread::sleep_for(kTimeoutCheckInterval);
            std::vector<std::uint32_t> timedOutDevices = timeoutMonitor.checkTimeouts(std::chrono::steady_clock::now());
            for (std::uint32_t deviceId : timedOutDevices) {
                logger.log(LogLevel::Error, "Device " + std::to_string(deviceId) + " timed out");
            }
        }
    }).detach();

    std::cout << "Waiting for clients to connect...\n";

    while (true) {
        int clientFd = ::accept(listenSocket.fd(), nullptr, nullptr);
        if (clientFd < 0) {
            logger.log(LogLevel::Error, "Failed to accept connection");
            continue;
        }

        Socket clientSocket(clientFd);
        logger.log(LogLevel::Info, "Client connected");

        std::thread([socket = std::move(clientSocket), &queue, &logger]() mutable {
            ConnectionHandler handler(std::move(socket), queue, logger);
            handler.run();
        }).detach();
    }
}
