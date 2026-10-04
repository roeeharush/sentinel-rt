#pragma once

#include "common/pipeline/BoundedQueue.h"
#include "common/network/FrameReader.h"
#include "common/infra/Logger.h"
#include "common/core/Measurement.h"
#include "common/network/Socket.h"

class ConnectionHandler {
public:
    ConnectionHandler(Socket socket, BoundedQueue<Measurement>& queue, Logger& logger);

    void run();

private:
    Socket socket_;
    FrameReader frameReader_;
    BoundedQueue<Measurement>& queue_;
    Logger& logger_;
};
