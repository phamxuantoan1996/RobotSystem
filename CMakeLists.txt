cmake_minimum_required(VERSION 3.16)

project(RobotESA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Xuất compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Cấu hình flags cho từng build type (CMake tự động thêm -g cho Debug)
# Bạn có thể tùy chỉnh thêm nếu cần
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# 1. Bật tính năng kiểm thử của CMake
include(CTest)
enable_testing()

# 2. Nhúng Google Test từ Internet (CI/CD và máy local tự tải về xài, rất tiện)
# include(FetchContent)
# FetchContent_Declare(
#   googletest
#   URL https://github.com
# )
# FetchContent_MakeAvailable(googletest)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)
add_subdirectory(common)

# 3. Gọi cac phân hệ vào hệ thống build chung
add_subdirectory(navigator)

add_subdirectory(robot)

add_subdirectory(gateway)

add_subdirectory(board)

add_subdirectory(lift)

# 4. App chính chạy file main.cpp của bro
add_executable(RobotSystemApp main.cpp)

# 5. Link app chính với thư viện của phân hệ navigator để main.cpp gọi được class Station
target_link_libraries(RobotSystemApp PRIVATE robot_lib lift_lib navigator_lib gateway_lib board_lib common_interface)

target_include_directories(RobotSystemApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

