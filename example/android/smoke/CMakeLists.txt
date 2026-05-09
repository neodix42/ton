cmake_minimum_required(VERSION 3.16)

project(tonlib_android_smoke LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(tonlibjson_smoke tonlibjson_smoke.cpp)
target_link_libraries(tonlibjson_smoke PRIVATE dl log)
