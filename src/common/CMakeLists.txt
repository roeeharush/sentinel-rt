add_library(rt_common Sensor.cpp Device.cpp ThresholdDetector.cpp Logger.cpp StatsCollector.cpp RealClock.cpp ManualClock.cpp MonitorLoop.cpp CliArgs.cpp FrameReader.cpp)
target_include_directories(rt_common PUBLIC ${CMAKE_SOURCE_DIR}/src)
