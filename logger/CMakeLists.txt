cmake_minimum_required(VERSION 3.14)

#  jsoncpp library
find_package(jsoncpp REQUIRED)

add_library(logger_lib 
    application/adapter/LogController.cpp
    drivers/file/FileLogWriter.cpp
)

target_link_libraries(logger_lib 
        PRIVATE common_interface
        PRIVATE JsonCpp::JsonCpp)

target_include_directories(logger_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/entities)
target_include_directories(logger_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/application/adapter)
target_include_directories(logger_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/drivers/file)
target_include_directories(logger_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/port)