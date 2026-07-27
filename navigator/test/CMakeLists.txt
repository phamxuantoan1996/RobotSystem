cmake_minimum_required(VERSION 3.14)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)


# Tạo file thực thi bài test riêng cho phân hệ navigator
add_executable(navigator_tests 
    station_test.cpp
    location_test.cpp
    velocity_test.cpp
    SeerNavigatorFrameCodec_test.cpp
    SeerNavigatorBuilderCommand_test.cpp
)

# Đảm bảo file test tìm thấy file header .hpp của class Station
target_include_directories(navigator_tests PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../domain/value_objects
    ${CMAKE_CURRENT_SOURCE_DIR}/../drivers/seer
    ${JSONCPP_INCLUDE_DIRS}
)

# Link file test với Google Test và thư viện code tính năng vừa tạo ở trên
target_link_libraries(navigator_tests 
    PRIVATE 
        gtest 
        gtest_main 
        navigator_lib
        ${JSONCPP_LIBRARIES}
)

# Đăng ký test case với hệ thống ctest tổng
add_test(NAME NavigatorTests COMMAND navigator_tests)
