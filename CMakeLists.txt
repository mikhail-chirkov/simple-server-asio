cmake_minimum_required(VERSION 3.22.1)
project(simple-server-asio VERSION 0.1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")
find_package(asio CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)

add_subdirectory(src)