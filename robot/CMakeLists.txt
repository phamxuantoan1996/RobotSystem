cmake_minimum_required(VERSION 3.14)

#  jsoncpp library
find_package(jsoncpp REQUIRED)

add_library(robot_lib 
    application/orchestrator/Orchestrator.cpp
    application/controller/RobotController.cpp
    domain/value_objects/RobotTaskQueue.cpp
    domain/value_objects/MissionParser.cpp
)

target_link_libraries(robot_lib 
        PRIVATE common_interface
        PRIVATE JsonCpp::JsonCpp)

target_include_directories(robot_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/entities)
target_include_directories(robot_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/value_objects)
target_include_directories(robot_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/events)
target_include_directories(robot_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/application/orchestrator)
target_include_directories(robot_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/application/controller)


