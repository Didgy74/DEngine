cmake_minimum_required(VERSION 3.18)
project(dengine)

set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
	
option(DENGINE_ENABLE_VERBOSE_CMAKE_OUTPUT "Enable verbose CMake output for DEngine" ON)

set(DENGINE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")



add_library(dengine SHARED
	"src/BackendData.hpp"
    "src/application.cpp"
	"src/FileInputStream.cpp"
	"src/HandleCustomEvent.hpp"
	"src/HandleCustomEvent.cpp"
	"src/HandleInputEvent.hpp"
	"src/HandleInputEvent.cpp")
target_link_libraries(dengine
    PRIVATE
        log
        android)

# Set the DEngine defines up
include(${DENGINE_ROOT_DIR}/cmake/SetDEngineDefines.cmake)
set(DENGINE_OS ${DENGINE_OS_ANDROID} CACHE STRING "" FORCE)
DEngine_SetDefines(dengine)

include(${DENGINE_ROOT_DIR}/cmake/SetCompilerSettings.cmake)
DEngine_SetCompilerSettings(dengine)

add_subdirectory("${DENGINE_ROOT_DIR}/any" "any")
target_link_libraries(dengine PUBLIC dengine_any)