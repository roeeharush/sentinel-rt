#pragma once

enum class FaultType {
    None,
    Spike,
    Drop,
    Stuck,
    Drift,
    Noise,
    Invalid
};