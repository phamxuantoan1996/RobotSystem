cmake_minimum_required(VERSION 3.14)

# Gom mã nguồn của phân hệ navigator thành một thư viện (Library)
add_library(navigator_lib 
    domain/value_objects/station.cpp
    domain/value_objects/location.cpp
    domain/value_objects/velocity.cpp
    drivers/seer/SeerNavigatorFrameCodec.cpp
    drivers/seer/SeerNavigatorCommandBuilder.cpp
)

# Chỉ đường dẫn để khi code bên ngoài #include "station.hpp" trình biên dịch tự tìm được
target_include_directories(navigator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/value_objects)
target_include_directories(navigator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/drivers/seer)
target_include_directories(navigator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/entities)
target_include_directories(navigator_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain/events)

# Nếu chế độ test được bật, đi sâu tiếp vào thư mục test của phân hệ này
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
