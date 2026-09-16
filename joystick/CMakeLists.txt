cmake_minimum_required(VERSION 3.14)

#  jsoncpp library
find_package(jsoncpp REQUIRED)

add_library(joystick_lib 
    application/adapter/JoyStickController.cpp
    drivers/JoyStickPLC/JoyStickPLC.cpp
)

target_link_libraries(joystick_lib 
        PRIVATE common_interface
        PRIVATE JsonCpp::JsonCpp)

target_include_directories(joystick_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/entities)
target_include_directories(joystick_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/events)
target_include_directories(joystick_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ports)
target_include_directories(joystick_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/application/adapter)
target_include_directories(joystick_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/drivers/JoyStickPLC)

