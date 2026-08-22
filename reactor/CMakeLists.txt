cmake_minimum_required(VERSION 3.14)

#  jsoncpp library
find_package(jsoncpp REQUIRED)

add_library(reactor_lib 
    indicator/IndicatorReactor.cpp
)

target_link_libraries(reactor_lib 
        PRIVATE common_interface
        PRIVATE JsonCpp::JsonCpp)

target_include_directories(reactor_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/indicator)