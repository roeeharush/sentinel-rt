#pragma once

#include "common/core/Measurement.h"
#include <cstdint>
#include <optional>
#include <vector>

std::vector<std::uint8_t> encodeMeasurement(const Measurement& measurement);
std::optional<Measurement> decodeMeasurement(const std::vector<std::uint8_t>& bytes);