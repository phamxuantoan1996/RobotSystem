cmake_minimum_required(VERSION 3.14)
project(CommonModule)

# Tạo một thư viện dạng INTERFACE (chỉ có header, không có file .cpp)
add_library(common_interface INTERFACE)

# Cấu hình đường dẫn include cho thư viện này
# CMAKE_CURRENT_SOURCE_DIR chính là thư mục common/
target_include_directories(common_interface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
