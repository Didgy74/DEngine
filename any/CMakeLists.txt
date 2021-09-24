cmake_minimum_required(VERSION 3.12)

project(dengine_any)

find_package(Threads REQUIRED)

file(GLOB_RECURSE DENGINE_HEADERS
	"*.hpp"
	"include")
file(READ "SourceFileList.txt" DENGINE_SOURCE_FILES)
string(REPLACE "\n" ";" DENGINE_SOURCE_FILES ${DENGINE_SOURCE_FILES})
set(DENGINE_ANY_TARGET_NAME dengine_any)

# Create main library
add_library(${DENGINE_ANY_TARGET_NAME} STATIC ${DENGINE_SOURCE_FILES})

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



set(DENGINE_DEFAULT_SETTING OFF)
option(DENGINE_ENABLE_ASSERT "Disabling will remove all asserts in engine" ${DENGINE_DEFAULT_SETTING})
if (${DENGINE_ENABLE_ASSERT})
	target_compile_definitions(dengine_any PUBLIC DENGINE_ENABLE_ASSERT)
endif()

option(DENGINE_APPLICATION_ENABLE_ASSERT "Asserts inside the application code" ${DENGINE_DEFAULT_SETTING})
if (${DENGINE_APPLICATION_ENABLE_ASSERT})
	target_compile_definitions(dengine_any PUBLIC DENGINE_APPLICATION_ENABLE_ASSERT)
endif()

option(DENGINE_CONTAINERS_ENABLE_ASSERT "Asserts inside DEngine containers" ${DENGINE_DEFAULT_SETTING})
if (${DENGINE_CONTAINERS_ENABLE_ASSERT})
	target_compile_definitions(dengine_any PUBLIC DENGINE_CONTAINERS_ENABLE_ASSERT)
endif()

option(DENGINE_GFX_ENABLE_ASSERT "Asserts inside the rendering code" ${DENGINE_DEFAULT_SETTING})
if (${DENGINE_GFX_ENABLE_ASSERT})
	target_compile_definitions(dengine_any PUBLIC DENGINE_GFX_ENABLE_ASSERT)
endif()

option(DENGINE_GUI_ENABLE_ASSERT "Asserts inside the GUI toolkit code" ${DENGINE_DEFAULT_SETTING})
if (${DENGINE_GFX_ENABLE_ASSERT})
	target_compile_definitions(dengine_any PUBLIC DENGINE_GUI_ENABLE_ASSERT)
endif()

option(DENGINE_MATH_ENABLE_ASSERT "Asserts inside DEngine math" ${DENGINE_DEFAULT_SETTING})
if (${DENGINE_MATH_ENABLE_ASSERT})
	target_compile_definitions(dengine_any PUBLIC DENGINE_MATH_ENABLE_ASSERT)
endif()