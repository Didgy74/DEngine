cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

project(dengine_desktop)

find_package(glfw3 REQUIRED)

add_subdirectory(../any any)

add_executable(dengine)
target_link_libraries(dengine
	PRIVATE
		dengine_any
		glfw)
target_sources(dengine
	PRIVATE
		"src/application.cpp")