add_executable(monitor_app main.cpp)
target_link_libraries(monitor_app PRIVATE rt_common rt_project_warnings)

add_executable(monitor_server monitor_server_main.cpp)
target_link_libraries(monitor_server PRIVATE rt_common rt_project_warnings)

add_executable(device_sim device_sim_main.cpp)
target_link_libraries(device_sim PRIVATE rt_common rt_project_warnings)

add_executable(scenario_runner scenario_runner_main.cpp)
target_link_libraries(scenario_runner PRIVATE rt_common rt_project_warnings)

file(GLOB SCENARIO_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/scenarios/*.scenario)
foreach(SCENARIO_FILE ${SCENARIO_FILES})
    get_filename_component(SCENARIO_NAME ${SCENARIO_FILE} NAME_WE)
    add_test(NAME scenario_${SCENARIO_NAME} COMMAND scenario_runner ${SCENARIO_FILE})
endforeach()
