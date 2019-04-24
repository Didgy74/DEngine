project(volk-cpp)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)

set(LIB_NAME ${PROJECT_NAME})
add_library(${LIB_NAME} INTERFACE)
add_library(${LIB_NAME}::${LIB_NAME} ALIAS ${LIB_NAME})

set(INCLUDE_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(
    ${LIB_NAME}
    INTERFACE
    $<BUILD_INTERFACE:${INCLUDE_FOLDER}>
    $<INSTALL_INTERFACE:include>
)

target_sources(${LIB_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src/volk.cpp")