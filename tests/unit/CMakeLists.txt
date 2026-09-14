add_executable(unit_tests test_common.cpp)
target_link_libraries(unit_tests PRIVATE rt_common GTest::gtest_main rt_project_warnings)

include(GoogleTest)
gtest_discover_tests(unit_tests)
