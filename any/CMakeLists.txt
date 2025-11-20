cmake_minimum_required(VERSION 4.1)
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
include(SourceFileList.cmake)

# Create main library
add_library(dengine_any STATIC ${DENGINE_ANY_SOURCE_FILES})
target_include_directories(dengine_any PUBLIC "include")
if(NOT DEFINED DENGINE_ADDITIONAL_COMPILE_ARGS)
	message(FATAL_ERROR "When processing dengine_any, found no CMake variable for additional compile args")
endif()
target_compile_options(dengine_any PUBLIC ${DENGINE_ADDITIONAL_COMPILE_ARGS})

if (NOT TARGET DEngine_Std)
	message(FATAL_ERROR "dengine_any unable to link DEngine::Std")
endif()
target_link_libraries(dengine_any PRIVATE DEngine_Std)
if (NOT TARGET DEngine_Math)
	message(FATAL_ERROR "dengine_any unable to link DEngine::Math")
endif()
target_link_libraries(dengine_any PRIVATE DEngine_Math)
if (NOT TARGET DEngine_Platform)
	message(FATAL_ERROR "dengine_any unable to link DEngine::Platform")
endif()
target_link_libraries(dengine_any PRIVATE DEngine_Platform)
if (NOT TARGET DEngine_Gui)
	message(FATAL_ERROR "dengine_any unable to link DEngine::Gui")
endif()
target_link_libraries(dengine_any PRIVATE DEngine_Gui)
if (NOT TARGET DEngine_Gui_StdWidgets)
	message(FATAL_ERROR "dengine_any unable to link DEngine::Gui::StdWidgets")
endif()
target_link_libraries(dengine_any PRIVATE DEngine_Gui_StdWidgets)
if (NOT TARGET DEngine_Gui_Context)
	message(FATAL_ERROR "dengine_any unable to link DEngine::Gui::Context")
endif()
target_link_libraries(dengine_any PRIVATE DEngine_Gui_Context)

# Set the DEngine defines up
include(${DENGINE_ROOT_DIR}/cmake/SetDEngineDefines.cmake)
DEngine_SetDefines(dengine_any)

# Set compiler settings
include(${DENGINE_ROOT_DIR}/cmake/SetCompilerSettings.cmake)
DEngine_SetCompilerSettings(dengine_any)

# Run script for linking dependencies
include(LinkDependencies.cmake)
DEngineAny_LinkDependencies(dengine_any ${DENGINE_ENABLE_VERBOSE_CMAKE_OUTPUT})

# Setup the assert settings for the different modules.
set(DENGINE_DEFAULT_ASSERT_SETTING ON)
include(SetupAssertDefines.cmake)
DEngineAny_SetupAssertDefines(dengine_any ${DENGINE_DEFAULT_ASSERT_SETTING})

# Rendering multi-threading switch
if (${DENGINE_GFX_ENABLE_DEDICATED_THREAD})
	target_compile_definitions(dengine_any PUBLIC DENGINE_GFX_ENABLE_DEDICATED_THREAD)
endif()

# Create GUI playground lib
if (FALSE)
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

	target_link_libraries(gui_playground PRIVATE DEngine_Std)
	target_link_libraries(gui_playground PRIVATE DEngine_Math)
	target_link_libraries(gui_playground PRIVATE DEngine_Platform)
	target_link_libraries(gui_playground PRIVATE DEngine_Gui)
	target_link_libraries(gui_playground PRIVATE DEngine_Gui_StdWidgets)
	target_link_libraries(gui_playground PRIVATE DEngine_Gui_Context)
	#target_link_libraries(gui_playground PRIVATE DEngine_Gui_StdWidgets_Test)
endif()
