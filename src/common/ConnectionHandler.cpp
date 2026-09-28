#include "common/ConnectionHandler.h"
#include "common/Codec.h"

#include <stdexcept>
#include <string>
#include <utility>

ConnectionHandler::ConnectionHandler(Socket socket, BoundedQueue<Measurement>& queue, Logger& logger)
    : socket_(std::move(socket)), queue_(queue), logger_(logger) {}

void ConnectionHandler::run() {
    while (true) {
        std::optional<std::vector<std::uint8_t>> chunk = socket_.receive();
        if (!chunk.has_value()) {
            logger_.log(LogLevel::Info, "Connection closed");
            return;
        }

        frameReader_.feed(*chunk);

        while (true) {
            std::optional<std::vector<std::uint8_t>> frame;
            try {
                frame = frameReader_.tryExtractFrame();
            } catch (const std::exception& e) {
                // A hostile length prefix leaves the reader's buffer in a state we can't
                // safely resynchronize from, so this connection (only) is dropped.
                logger_.log(LogLevel::Error, std::string("Closing connection after frame error: ") + e.what());
                return;
            }

            if (!frame.has_value()) {
                break;
            }

            std::optional<Measurement> measurement = decodeMeasurement(*frame);
            if (!measurement.has_value()) {
                logger_.log(LogLevel::Warning, "Discarding malformed measurement payload");
                continue;
            }

            queue_.push(*measurement);
        }
    }
}
