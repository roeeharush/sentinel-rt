#pragma once

#include "common/Device.h"
#include "common/ThresholdDetector.h"
#include "common/Logger.h"
#include "common/StatsCollector.h"
#include "common/IClock.h"
#include <chrono>

void runMonitoringLoop(Device& device, ThresholdDetector& detector, Logger& logger, StatsCollector& stats, IClock& clock,int cycleCount, std::chrono::milliseconds period);
