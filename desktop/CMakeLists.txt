cmake_minimum_required(VERSION 3.18)

project(dengine_desktop)

find_package(glfw3 QUIET)
if(NOT ${glfw3_FOUND})
	set(GLFW_BUILD_DOCS OFF CACHE BOOL "Don't build GLFW documentation.")
	set(GLFW_INSTALL OFF CACHE BOOL "Don't build GLFW install target")

	add_subdirectory("external/glfw" glfw3)
endif()

add_subdirectory("../any" any)


add_executable(dengine)
target_sources(dengine
	PRIVATE
		"src/Application.cpp")
target_link_libraries(dengine
	PRIVATE
		dengine_any
		glfw)
set_target_properties(dengine PROPERTIES 
	CXX_STANDARD 20
	CXX_EXTENSIONS OFF)
#target_compile_options(dengine PUBLIC "-ftime-trace")