#pragma once

#include <cstdint>
#include <optional>
#include <vector>

class FrameReader {
public:
    void feed(const std::vector<std::uint8_t>& data);
    std::optional<std::vector<std::uint8_t>> tryExtractFrame();

private:
    std::vector<std::uint8_t> buffer_;
};