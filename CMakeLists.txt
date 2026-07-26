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

# 3. Gọi phân hệ navigator vào hệ thống build chung
add_subdirectory(navigator)
# (Sau này có thêm test cho robot, map... thì cũng add_subdirectory vào đây)

# 4. App chính chạy file main.cpp của bro
add_executable(RobotSystemApp main.cpp)

# 5. Link app chính với thư viện của phân hệ navigator để main.cpp gọi được class Station
target_link_libraries(RobotSystemApp PRIVATE navigator_lib)

target_include_directories(RobotSystemApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

