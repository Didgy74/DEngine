cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

project(dengine)

add_subdirectory(../../all all)

add_library(dengine SHARED
    "src/application.cpp")
target_link_libraries(dengine
    PRIVATE
        dengine_all
        log
        android)