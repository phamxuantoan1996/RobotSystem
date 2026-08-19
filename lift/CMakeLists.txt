cmake_minimum_required(VERSION 3.14)

#  jsoncpp library
find_package(jsoncpp REQUIRED)

add_library(lift_lib 
    domain/value_objects/LiftTarget.cpp
    application/adapter/LiftController.cpp
    application/use_cases/LiftMoveStep.cpp
)

target_link_libraries(lift_lib 
        PRIVATE common_interface
        PRIVATE JsonCpp::JsonCpp)

target_include_directories(lift_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/entities)
target_include_directories(lift_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/events)
target_include_directories(lift_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/value_objects)
target_include_directories(lift_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/application/adapter)
target_include_directories(lift_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/application/use_cases)

