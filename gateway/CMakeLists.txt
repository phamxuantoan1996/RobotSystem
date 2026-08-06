cmake_minimum_required(VERSION 3.14)

#  jsoncpp library
find_package(jsoncpp REQUIRED)

add_library(gateway_lib
    drivers/rest/RestGateway.cpp
)

target_link_libraries(gateway_lib 
        PRIVATE common_interface
        PRIVATE JsonCpp::JsonCpp)

target_include_directories(gateway_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/events)
target_include_directories(gateway_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ports)
target_include_directories(gateway_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/drivers/rest)