#include "common/BoundedQueue.h"
#include "common/ConnectionHandler.h"
#include "common/Logger.h"
#include "common/Measurement.h"
#include "common/Socket.h"
#include "common/StatsCollector.h"
#include "common/ThresholdDetector.h"
#include "common/WorkerPool.h"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <utility>

namespace {

    constexpr std::size_t kQueueCapacity = 1024;
    constexpr std::size_t kWorkerCount = 4;
    constexpr int kListenBacklog = 10;

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

    WorkerPool<Measurement> pool(queue, logger, [&](Measurement measurement) {
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - measurement.timestamp);
        stats.recordLatency(latency);

        Status status = detector.evaluate(measurement);
        stats.record(status);
        if (status != Status::Ok) {
            logger.log(levelForStatus(status), "Measurement outside normal range");
        } else {
            logger.log(LogLevel::Info, "Measurement received");
        }
    }, kWorkerCount);

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
