#pragma once

#include <cstdint>
#include <optional>
#include <vector>

std::vector<std::uint8_t> encodeFrame(const std::vector<std::uint8_t>& payload);

class FrameReader {
public:
    void feed(const std::vector<std::uint8_t>& data);
    std::optional<std::vector<std::uint8_t>> tryExtractFrame();

private:
    std::vector<std::uint8_t> buffer_;
};