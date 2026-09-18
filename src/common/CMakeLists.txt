add_library(rt_common Sensor.cpp Device.cpp ThresholdDetector.cpp Logger.cpp)
target_include_directories(rt_common PUBLIC ${CMAKE_SOURCE_DIR}/src)
