cmake_minimum_required(VERSION 3.14)

#  jsoncpp library
find_package(jsoncpp REQUIRED)

set(MYSQL_INCLUDE_DIRS "/usr/include" CACHE PATH "" FORCE)
set(MYSQL_LIBRARIES "/usr/lib/x86_64-linux-gnu/libz.so" CACHE FILEPATH "" FORCE) 
set(HIREDIS_INCLUDE_DIR "/usr/include" CACHE PATH "" FORCE)
set(HIREDIS_LIBRARY "/usr/lib/x86_64-linux-gnu/libz.so" CACHE FILEPATH "" FORCE)
find_package(Drogon REQUIRED)

add_library(gateway_lib
    drivers/rest/RestGateway.cpp
    application/adapter/GatewayController.cpp
)

target_link_libraries(gateway_lib 
        PRIVATE common_interface
        PRIVATE JsonCpp::JsonCpp
        PRIVATE Drogon::Drogon)

target_include_directories(gateway_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/events)
target_include_directories(gateway_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/entities)
target_include_directories(gateway_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ports)
target_include_directories(gateway_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/drivers/rest)
target_include_directories(gateway_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/application/adapter)