#pragma once

#include "common/BoundedQueue.h"
#include "common/FrameReader.h"
#include "common/Logger.h"
#include "common/Measurement.h"
#include "common/Socket.h"

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
