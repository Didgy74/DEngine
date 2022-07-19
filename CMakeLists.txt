cmake_minimum_required(VERSION 3.18)

option(DENGINE_ENABLE_VERBOSE_CMAKE_OUTPUT "Enable verbose CMake output for DEngine" ON)

project(dengine_desktop)

set(DENGINE_EXECUTABLE_NAME ${CMAKE_PROJECT_NAME})

add_executable(${DENGINE_EXECUTABLE_NAME})
target_sources(${DENGINE_EXECUTABLE_NAME}
		PRIVATE
		"desktop/src/Application.cpp")

target_compile_features(${DENGINE_EXECUTABLE_NAME} PRIVATE cxx_std_20)
#target_compile_options(${DENGINE_EXECUTABLE_NAME} PUBLIC "-ftime-trace")

if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
	message(FATAL_ERROR "In-source directory build not supported. Create a build directory and build into that.")
endif()

# Add rule to automatically copy assets folder to binary output
if (NOT "${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
	add_custom_command(
			TARGET "${CMAKE_PROJECT_NAME}" POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E
			copy_directory "${CMAKE_SOURCE_DIR}/data" "${CMAKE_BINARY_DIR}")
endif()


# Integrate cross-platform DEngine lib
add_subdirectory("any" any)
if (NOT TARGET dengine_any)
	message(FATAL_ERROR "DEngine_desktop: CMake target 'dengine_any' was not found.")
endif()
target_link_libraries(${DENGINE_EXECUTABLE_NAME}
		PRIVATE
		dengine_any)


# Integrate GLFW
if (${DENGINE_ENABLE_VERBOSE_CMAKE_OUTPUT})
	message(CHECK_START "DEngine_desktop: Searching for GLFW package.")
endif()
find_package(glfw3 QUIET)
if(NOT ${glfw3_FOUND})
	if (${DENGINE_ENABLE_VERBOSE_CMAKE_OUTPUT})
		message(CHECK_FAIL "No system GLFW package found. Building from source.")
	endif()

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
target_link_libraries(${DENGINE_EXECUTABLE_NAME}
		PRIVATE
		glfw)

add_subdirectory(any/external/Tracy TracyLib)
target_link_libraries(${DENGINE_EXECUTABLE_NAME} PUBLIC TracyClient)