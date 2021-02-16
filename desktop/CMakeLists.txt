cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

project(dengine_desktop)

find_package(glfw3 REQUIRED)

add_subdirectory(../all all)

add_executable(dengine_desktop)
target_link_libraries(dengine_desktop
	PRIVATE
		dengine_all
		glfw)
target_sources(dengine_desktop
	PRIVATE
		"src/application.cpp")