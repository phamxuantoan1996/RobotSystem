cmake_minimum_required(VERSION 3.14)

#  jsoncpp library
find_package(jsoncpp REQUIRED)

add_library(indicator_lib 
    application/adapter/IndicatorController.cpp
)

target_link_libraries(indicator_lib 
        PRIVATE common_interface
        PRIVATE JsonCpp::JsonCpp)

target_include_directories(indicator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/entities)
target_include_directories(indicator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/application/adapter)