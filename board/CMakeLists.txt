cmake_minimum_required(VERSION 3.14)

#  jsoncpp library
find_package(jsoncpp REQUIRED)

add_library(board_lib 
    drivers/serial_transport/BoardSerialTransport.cpp
    domain/value_objects/BoardCommandBuilder.cpp
    domain/value_objects/BoardCommandQueue.cpp
    application/adapter/BoardController.cpp
)

target_link_libraries(board_lib 
        PRIVATE common_interface
        PRIVATE JsonCpp::JsonCpp)

target_include_directories(robot_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ports)
target_include_directories(robot_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/drivers/serial_transport)
target_include_directories(robot_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/entities)
target_include_directories(robot_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/events)
target_include_directories(robot_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/value_objects)
target_include_directories(robot_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/application/adapter)
