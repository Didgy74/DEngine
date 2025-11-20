# This root-located CMakeLists is for the desktop build of DEngine.
# The Android root is found under the 'android' subdirectory.

cmake_minimum_required(VERSION 4.1)
project(dengine_desktop LANGUAGES CXX)

option(DENGINE_USE_ASAN_OPTION "Enables the use of address sanitizer" FALSE)

#set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.deps-cache")

include(cmake/MakeDependenciesAvailable.cmake)
include(cmake/SetCompilerSettings.cmake)

set(DENGINE_ADDITIONAL_COMPILE_ARGS "")
if (DENGINE_USE_ASAN_OPTION)
	if (MSVC)
		message(VERBOSE "Adding address sanitizer compiles flags")
		list(APPEND DENGINE_ADDITIONAL_COMPILE_ARGS
			"/fsanitize=address"
			"/Zi"
			"/INCREMENTAL:NO")
	else()
		message(FATAL_ERROR "Address sanitizer support only enabled for MSVC")
	endif()
endif()

if (WIN32)
	set(DENGINE_OS "Windows")
elseif(APPLE)
	set(DENGINE_OS "macOS")
else()
	message(FATAL_ERROR "Unsupported OS")
endif()

DEngine_MakeDependenciesAvailable()

set(DENGINE_STD_ENABLE_ASSERT ON)
add_subdirectory(src/Std)
set(DENGINE_MATH_ENABLE_ASSERT ON)
add_subdirectory(src/Math)
set(DENGINE_PLATFORM_ENABLE_ASSERT ON)
add_subdirectory(src/Platform)
set(DENGINE_GUI_ENABLE_ASSERT ON)
add_subdirectory(src/Gui)
add_subdirectory(src/Gui_StdWidgets)
add_subdirectory(src/Gui_Context)

# Set the root dir, our other CMake files rely on it.
set(DENGINE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

option(DENGINE_ENABLE_VERBOSE_CMAKE_OUTPUT "Enable verbose CMake output for DEngine" ON)

# Create our main executable
add_executable(dengine_desktop WIN32)
target_compile_options(dengine_desktop PUBLIC ${DENGINE_ADDITIONAL_COMPILE_ARGS})

DEngine_SetCompilerSettings(dengine_desktop)

# Set the DEngine defines up
include(${DENGINE_ROOT_DIR}/cmake/SetDEngineDefines.cmake)
DEngine_SetDefines(dengine_desktop)

if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
	message(FATAL_ERROR
		"In-source directory build not supported.
		Create a build directory and build into that.")
endif()


# Integrate cross-platform DEngine lib
# This is mostly useful whenever we are switching to the gui playground
set(CROSS_PLAT_LIB_NAME dengine_any)
mark_as_advanced(CROSS_PLAT_LIB_NAME)
add_subdirectory(any)
if (NOT TARGET ${CROSS_PLAT_LIB_NAME})
	message(FATAL_ERROR "DEngine: CMake target '${CROSS_PLAT_LIB_NAME}' was not found.")
endif()
target_link_libraries(dengine_desktop
	PUBLIC
		${CROSS_PLAT_LIB_NAME}
		DEngine_Std
		DEngine_Math
		DEngine_Platform
		DEngine_Gui
		DEngine_Gui_StdWidgets
		DEngine_Gui_Context)


if(DENGINE_OS STREQUAL "Windows")

elseif (DENGINE_OS STREQUAL "macOS")


	if(NOT DEFINED ENV{VULKAN_SDK})
		message(FATAL_ERROR "Vulkan SDK not found. Configuring on macOS requires the environment variables VULKAN_SDK to be set.")
	endif()
	set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})

	find_library(VULKAN_LIB
		NAMES vulkan
		PATHS "${VULKAN_SDK_PATH}/macOS/lib"
		NO_DEFAULT_PATH
		REQUIRED)
	file(RELATIVE_PATH VULKAN_LIB_RELATIVE_PATH
		"${VULKAN_SDK_PATH}/macOS"
		"${VULKAN_LIB}")
	get_filename_component(VULKAN_LIB_RELATIVE_PATH_PARENT_DIR "${VULKAN_LIB_RELATIVE_PATH}" DIRECTORY)

	find_library(MOLTENVK_LIB
		NAMES libMoltenVK.dylib
		PATHS "${VULKAN_SDK_PATH}/macOS/lib"
		NO_DEFAULT_PATH
		REQUIRED)

	find_file(MOLTENVK_ICD_JSON
		NAMES MoltenVK_icd.json
		PATHS "${VULKAN_SDK_PATH}/macOS/share/vulkan/icd.d"
		NO_DEFAULT_PATH
		REQUIRED)

	file(RELATIVE_PATH ICD_RELATIVE_PATH
		"${VULKAN_SDK_PATH}/macOS"
		"${MOLTENVK_ICD_JSON}")

	add_custom_command(TARGET dengine_desktop POST_BUILD
		# Establish lib directory
		COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:dengine_desktop>/${VULKAN_LIB_RELATIVE_PATH_PARENT_DIR}"
		# Copy libvulkan.dylib
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${VULKAN_LIB}" "$<TARGET_FILE_DIR:dengine_desktop>/${VULKAN_LIB_RELATIVE_PATH_PARENT_DIR}"
		# Copy libMoltenVK.dylib
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MOLTENVK_LIB}" "$<TARGET_FILE_DIR:dengine_desktop>/${VULKAN_LIB_RELATIVE_PATH_PARENT_DIR}"
		# Establish directory for JSON
		COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:dengine_desktop>/${ICD_RELATIVE_DIR}"
		# Copy ICD Json
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MOLTENVK_ICD_JSON}" "$<TARGET_FILE_DIR:dengine_desktop>/${ICD_RELATIVE_PATH}"
		COMMENT "Copying MoltenVK files to executable dir")

else()
	message(FATAL_ERROR "Haven't maintained Linux build in a long time. Here there be dragons")
	target_sources(dengine_desktop
		PRIVATE
		"desktop/src/Application.cpp")

	if (${DENGINE_ENABLE_VERBOSE_CMAKE_OUTPUT})
		message(CHECK_START "DEngine_desktop: Searching for GLFW package.")
	endif()
	find_package(glfw3 QUIET)
	if(NOT ${glfw3_FOUND})
		set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
		set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
		set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
		set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
		set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
		set(GLFW_VULKAN_STATIC OFF CACHE BOOL "" FORCE)

		add_subdirectory("desktop/external/glfw")
	else()
		if (${DENGINE_ENABLE_VERBOSE_CMAKE_OUTPUT})
			message(CHECK_PASS "Found. Using system GLFW package.")
		endif()
	endif()
	if (NOT TARGET glfw)
		message(FATAL_ERROR "DEngine_desktop: CMake target 'glfw' was not found.")
	endif()
	target_link_libraries(${DENGINE_EXE_NAME}
		PRIVATE
		glfw)
endif()

set(DENGINE_PRECOMPILE_ASSET_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets")
include(${DENGINE_ROOT_DIR}/cmake/AddPrecompileAssetsCustomCommand.cmake)
dengine_setup_asset_prebuild_step(
	dengine_desktop
	OUTPUT_DIR "${DENGINE_PRECOMPILE_ASSET_OUTPUT_DIR}")
