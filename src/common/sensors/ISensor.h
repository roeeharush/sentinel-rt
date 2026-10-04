#pragma once
#include "common/core/SensorType.h"

class ISensor{
  public:
    virtual SensorType type() const = 0;
    virtual double read() = 0;
    virtual ~ISensor() = default;
};