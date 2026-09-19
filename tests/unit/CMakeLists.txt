add_executable(unit_tests test_common.cpp test_sensor.cpp test_device.cpp test_threshold_detector.cpp test_logger.cpp test_stats_collector.cpp test_manual_clock.cpp test_monitor_loop.cpp test_cli_args.cpp)
target_link_libraries(unit_tests PRIVATE rt_common GTest::gtest_main rt_project_warnings)

include(GoogleTest)
gtest_discover_tests(unit_tests)
