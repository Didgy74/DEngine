cmake_minimum_required(VERSION 3.0)
project(Volk)

##
## TARGET
## create target and add include path
##
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(
    ${PROJECT_NAME}
    INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_sources(${LIB_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src/volk.c")