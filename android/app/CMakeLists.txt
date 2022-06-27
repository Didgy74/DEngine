cmake_minimum_required(VERSION 3.12)

set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

option(DENGINE_ENABLE_VERBOSE_CMAKE_OUTPUT "Enable verbose CMake output for DEngine" ON)

project(dengine)

add_subdirectory("../../any" "any")

add_library(dengine SHARED
	"src/BackendData.hpp"
    "src/application.cpp"
	"src/FileInputStream.cpp")
target_compile_features(dengine PRIVATE cxx_std_20)
set_target_properties(dengine PROPERTIES
		CXX_STANDARD 20
		CXX_EXTENSIONS OFF)
target_link_libraries(dengine
    PRIVATE
        dengine_any
        log
        android)