cmake_minimum_required(VERSION 3.12)

project(dengine_any)

find_package(Threads REQUIRED)

file(GLOB_RECURSE DENGINE_HEADERS
	"*.hpp"
	"include")
include(SourceFileList.cmake)

set(DENGINE_ANY_TARGET_NAME dengine_any)


# Create main library
add_library(${DENGINE_ANY_TARGET_NAME} STATIC ${DENGINE_ANY_SOURCE_FILES})

target_include_directories(${DENGINE_ANY_TARGET_NAME}
	PUBLIC
		"include")
target_link_libraries(${DENGINE_ANY_TARGET_NAME}
	PUBLIC
		Threads::Threads
		${CMAKE_DL_LIBS})
target_sources(${DENGINE_ANY_TARGET_NAME}
	PUBLIC
		${DENGINE_HEADERS})
target_compile_features(${DENGINE_ANY_TARGET_NAME} PRIVATE cxx_std_20)
set_target_properties(${DENGINE_ANY_TARGET_NAME} PROPERTIES
	CXX_STANDARD 20
	CXX_EXTENSIONS OFF)
#target_compile_options(dengine_any PUBLIC "-ftime-trace")

# Run script for linking dependencies
include(LinkDependencies.cmake)
LinkDependencies(${DENGINE_ANY_TARGET_NAME})


set(DENGINE_DEFAULT_ASSERT_SETTING ON)
include(SetupAssertDefines.cmake)
SetupAssertDefines(${DENGINE_ANY_TARGET_NAME} ${DENGINE_DEFAULT_ASSERT_SETTING})