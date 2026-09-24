#pragma once

#include "common/Device.h"
#include "common/IClock.h"
#include <chrono>
#include "common/BoundedQueue.h"
#include "common/Measurement.h"


void runMonitoringLoop(Device& device, BoundedQueue<Measurement>& queue, IClock& clock, int cycleCount, std::chrono::milliseconds period);
