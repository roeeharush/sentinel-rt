#include "common/network/FrameReader.h"
#include <stdexcept>

namespace {

    constexpr std::uint32_t kMaxFrameSize = 65536;

    std::uint32_t decodeBigEndianLength(const std::vector<std::uint8_t>& buffer) {
        return (static_cast<std::uint32_t>(buffer[0]) << 24) |
               (static_cast<std::uint32_t>(buffer[1]) << 16) |
               (static_cast<std::uint32_t>(buffer[2]) << 8) |
               (static_cast<std::uint32_t>(buffer[3]));
    }

}

void FrameReader::feed(const std::vector<std::uint8_t>& data) {
    buffer_.insert(buffer_.end(), data.begin(), data.end());
}

std::optional<std::vector<std::uint8_t>> FrameReader::tryExtractFrame() {
    if (buffer_.size() < 4) {
        return std::nullopt;
    }

    std::uint32_t length = decodeBigEndianLength(buffer_);

    if (length > kMaxFrameSize) {
        throw std::runtime_error("frame length exceeds maximum allowed size");
    }

    if (buffer_.size() < 4 + length) {
        return std::nullopt;
    }

    std::vector<std::uint8_t> frame(buffer_.begin() + 4, buffer_.begin() + 4 + length);
    buffer_.erase(buffer_.begin(), buffer_.begin() + 4 + length);
    return frame;
}

std::vector<std::uint8_t> encodeFrame(const std::vector<std::uint8_t>& payload) {
    auto length = static_cast<std::uint32_t>(payload.size());

    std::vector<std::uint8_t> frame;
    frame.reserve(4 + payload.size());

    frame.push_back(static_cast<std::uint8_t>((length >> 24) & 0xFF));
    frame.push_back(static_cast<std::uint8_t>((length >> 16) & 0xFF));
    frame.push_back(static_cast<std::uint8_t>((length >> 8) & 0xFF));
    frame.push_back(static_cast<std::uint8_t>(length & 0xFF));

    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}