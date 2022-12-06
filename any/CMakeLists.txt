cmake_minimum_required(VERSION 3.20)
project(dengine_any)

# We need to have a root folder setting before including this
option(DENGINE_ROOT_DIR "Path to the project root directory" OFF)
if(${DENGINE_ROOT_DIR} EQUAL OFF)
	message(FATAL_ERROR "CMake variable 'DENGINE_ROOT_DIR' has not been set.")
endif()

option(DENGINE_OS "Must be one of the OS values" "")
if(NOT DEFINED DENGINE_OS OR ${DENGINE_OS} STREQUAL "")
	message(FATAL_ERROR "CMake variable 'DENGINE_OS' has not been set.")
endif()

option(DENGINE_GFX_ENABLE_DEDICATED_THREAD "Enables a dedicated thread to handle the renderer." ON)
set(DENGINE_GFX_ENABLE_DEDICATED_THREAD ON)

set(DENGINE_ANY_TARGET_NAME dengine_any)
file(GLOB_RECURSE DENGINE_HEADERS "*.hpp" "include")
include(SourceFileList.cmake)

# Create main library
add_library(${DENGINE_ANY_TARGET_NAME} STATIC ${DENGINE_ANY_SOURCE_FILES})
target_include_directories(${DENGINE_ANY_TARGET_NAME} PUBLIC "include")
target_sources(${DENGINE_ANY_TARGET_NAME} PUBLIC ${DENGINE_HEADERS})

# Set the DEngine defines up
include(${DENGINE_ROOT_DIR}/cmake/SetDEngineDefines.cmake)
DEngine_SetDefines(${DENGINE_ANY_TARGET_NAME})

# Set compiler settings
include(${DENGINE_ROOT_DIR}/cmake/SetCompilerSettings.cmake)
DEngine_SetCompilerSettings(${DENGINE_ANY_TARGET_NAME})

# Run script for linking dependencies
include(LinkDependencies.cmake)
DEngineAny_LinkDependencies(${DENGINE_ANY_TARGET_NAME} ${DENGINE_ENABLE_VERBOSE_CMAKE_OUTPUT})

# Setup the assert settings for the different modules.
set(DENGINE_DEFAULT_ASSERT_SETTING ON)
include(SetupAssertDefines.cmake)
DEngineAny_SetupAssertDefines(${DENGINE_ANY_TARGET_NAME} ${DENGINE_DEFAULT_ASSERT_SETTING})

# Rendering multi-threading switch
if (${DENGINE_GFX_ENABLE_DEDICATED_THREAD})
	target_compile_definitions(${DENGINE_ANY_TARGET_NAME} PUBLIC DENGINE_GFX_ENABLE_DEDICATED_THREAD)
endif()



# Create GUI playground lib
add_library(gui_playground STATIC ${DENGINE_GUI_PLAYGROUND_SOURCE_FILES})
target_include_directories(gui_playground PUBLIC "include")
target_sources(gui_playground PUBLIC ${DENGINE_HEADERS})
# Set compiler settings
DEngine_SetCompilerSettings(gui_playground)
# Run script for linking dependencies
DEngineAny_LinkDependencies(gui_playground ${DENGINE_ENABLE_VERBOSE_CMAKE_OUTPUT})
# Setup the assert settings for the different modules.
DEngineAny_SetupAssertDefines(gui_playground ${DENGINE_DEFAULT_ASSERT_SETTING})
# Rendering multi-threading switch
if (${DENGINE_GFX_ENABLE_DEDICATED_THREAD})
	target_compile_definitions(gui_playground PUBLIC DENGINE_GFX_ENABLE_DEDICATED_THREAD)
endif()