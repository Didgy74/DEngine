cmake_minimum_required(VERSION 4.1)
project(dengine)

set(DENGINE_OS "Android")

set(CMAKE_CXX_STANDARD 23)

option(DENGINE_ENABLE_VERBOSE_CMAKE_OUTPUT "Enable verbose CMake output for DEngine" ON)
set(DENGINE_PRECOMPILE_ASSET_OUTPUT_DIR
    ""
    CACHE PATH "Optional asset output directory (must be explicitly set)"
)
if(NOT DEFINED DENGINE_PRECOMPILE_ASSET_OUTPUT_DIR OR DENGINE_PRECOMPILE_ASSET_OUTPUT_DIR STREQUAL "")
    message(FATAL_ERROR "DENGINE_PRECOMPILE_ASSET_OUTPUT_DIR was not set")
endif()

set(DENGINE_ADDITIONAL_COMPILE_ARGS "")

set(DENGINE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")

add_library(dengine SHARED
	src/BackendData.hpp
	src/application.cpp
	src/FileInputStream.cpp
	src/HandleCustomEvent.hpp
	src/HandleCustomEvent.cpp
	src/HandleInputEvent.hpp
	src/HandleInputEvent.cpp)
target_link_libraries(dengine PRIVATE
	log
	android)

# Set the DEngine defines up
include(${DENGINE_ROOT_DIR}/cmake/SetDEngineDefines.cmake)
DEngine_SetDefines(dengine)

include(${DENGINE_ROOT_DIR}/cmake/SetCompilerSettings.cmake)
DEngine_SetCompilerSettings(dengine)

set(DENGINE_STD_ENABLE_ASSERT ON)
add_subdirectory(${DENGINE_ROOT_DIR}/src/Std DEngine_Std_Dir)
set(DENGINE_MATH_ENABLE_ASSERT ON)
add_subdirectory(${DENGINE_ROOT_DIR}/src/Math DEngine_Math_Dir)
set(DENGINE_PLATFORM_ENABLE_ASSERT ON)
add_subdirectory(${DENGINE_ROOT_DIR}/src/Platform DEngine_Platform_Dir)
set(DENGINE_GUI_ENABLE_ASSERT ON)
add_subdirectory(${DENGINE_ROOT_DIR}/src/Gui DEngine_Gui_Dir)
add_subdirectory(${DENGINE_ROOT_DIR}/src/Gui_StdWidgets DEngine_StdWidgets_Dir)
add_subdirectory(${DENGINE_ROOT_DIR}/src/Gui_Context DEngine_Gui_Context_Dir)

include(${DENGINE_ROOT_DIR}/cmake/MakeDependenciesAvailable.cmake)
DEngine_MakeDependenciesAvailable()
add_subdirectory("${DENGINE_ROOT_DIR}/any" DEngine_any)

target_link_libraries(dengine PUBLIC DEngine_Std)
target_link_libraries(dengine PUBLIC DEngine_Math)
target_link_libraries(dengine PUBLIC DEngine_Platform)

target_link_libraries(dengine PUBLIC dengine_any)
#target_link_libraries(dengine PUBLIC gui_playground)

include(${DENGINE_ROOT_DIR}/cmake/AddPrecompileAssetsCustomCommand.cmake)
dengine_setup_asset_prebuild_step(
    dengine
    OUTPUT_DIR ${DENGINE_PRECOMPILE_ASSET_OUTPUT_DIR})
