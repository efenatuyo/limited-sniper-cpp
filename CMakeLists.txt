cmake_minimum_required(VERSION 3.12)
project(sniper)

set(CMAKE_CXX_STANDARD 11)

# Set the path to libcurl include directory
include_directories("D:/codes projekte/vcpkg/installed/x64-windows/include")

# Set the path to libcurl library directory
link_directories("D:/codes projekte/vcpkg/installed/x64-windows/lib")

# Add your source file
add_executable(sniper src/main.cpp)

# Link against libcurl
target_link_libraries(sniper libcurl)
target_link_libraries(sniper uv)
