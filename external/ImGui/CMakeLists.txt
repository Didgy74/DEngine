cmake_minimum_required(VERSION 3.12)
project(ImGui)

set(CMAKE_CXX_STANDARD 17)

set(IMGUI_LIB_NAME ${PROJECT_NAME})
add_library(${IMGUI_LIB_NAME} STATIC)
target_compile_options(${IMGUI_LIB_NAME} PUBLIC /W4)

set(IMGUI_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(${IMGUI_LIB_NAME} PRIVATE ${IMGUI_INCLUDE_DIR})

file(GLOB_RECURSE IMGUI_SOURCE_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.inl"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

target_sources(${IMGUI_LIB_NAME} ${IMGUI_SOURCE_FILES})
target_sources(${IMGUI_LIB_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/examples/imgui_impl_sdl.cpp")

target_compile_definitions(${IMGUI_LIB_NAME} PRIVATE IMGUI_DISABLE_OBSOLETE_FUNCTIONS)



