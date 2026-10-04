#pragma once

#include "common/sensors/Device.h"
#include "common/infra/IClock.h"
#include <chrono>
#include "common/pipeline/BoundedQueue.h"
#include "common/core/Measurement.h"


void runMonitoringLoop(Device& device, BoundedQueue<Measurement>& queue, IClock& clock, int cycleCount, std::chrono::milliseconds period, bool blockWhenFull);