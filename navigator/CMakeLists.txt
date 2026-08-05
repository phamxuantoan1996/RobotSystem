cmake_minimum_required(VERSION 3.14)

#  jsoncpp library
find_package(jsoncpp REQUIRED)

# Gom mã nguồn của phân hệ navigator thành một thư viện (Library)
add_library(navigator_lib 
    domain/value_objects/station.cpp
    domain/value_objects/location.cpp
    domain/value_objects/velocity.cpp
    drivers/seer/SeerNavigatorFrameCodec.cpp
    drivers/seer/SeerNavigatorCommandBuilder.cpp
    drivers/seer/SeerNavigatorStateMapper.cpp
    drivers/seer/SeerNavigatorConnectionReal.cpp
    drivers/seer/SeerNavigatorDriverReal.cpp
    application/adapter/NavigatorController.cpp
    application/services/NavigatorReconnectService.cpp
    application/use_cases/GoToStationStep.cpp
)

# LIÊN KẾT ĐẾN COMMON:
target_link_libraries(navigator_lib 
        PRIVATE common_interface
        PRIVATE JsonCpp::JsonCpp)

# Chỉ đường dẫn để khi code bên ngoài #include "station.hpp" trình biên dịch tự tìm được
target_include_directories(navigator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/value_objects)
target_include_directories(navigator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/drivers/seer)
target_include_directories(navigator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/entities)
target_include_directories(navigator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/events)
target_include_directories(navigator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ports)
target_include_directories(navigator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/application/adapter)
target_include_directories(navigator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/application/services)
target_include_directories(navigator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/application/use_cases)

# Nếu chế độ test được bật, đi sâu tiếp vào thư mục test của phân hệ này
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
