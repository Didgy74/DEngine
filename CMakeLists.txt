cmake_minimum_required(VERSION 3.9)
project(dengine)

set(CMAKE_CXX_STANDARD 17)

set(DENGINE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
include(Common.cmake)

# ------------------------
# Make the main executable
# ------------------------
# START

	add_executable(dengine ${DENGINE_SRC_FILES})
	target_include_directories(dengine PRIVATE ${DENGINE_INCLUDE_DIRS})
	target_compile_definitions(dengine PRIVATE ${DENGINE_DEFINES})
	target_link_libraries(dengine PRIVATE ${CMAKE_DL_LIBS})
	if(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
		target_compile_options(dengine PRIVATE -Og)
	endif()

	# Remove data folder from target binary directory
	add_custom_command(
		TARGET dengine POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E remove_directory
		$<TARGET_FILE_DIR:dengine>/data/)

	# Copy data directory from source folder to build folder
	add_custom_command(
		TARGET dengine POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/data/data
		$<TARGET_FILE_DIR:dengine>/data/)
#
# END
#

	set(VULKAN_INCLUDE_DIRS ${DENGINE_ROOT_DIR}/external/Vulkan-Headers/include)
	target_include_directories(dengine PRIVATE ${VULKAN_INCLUDE_DIRS})

# ----------
# Link Texas
# ----------
# START
	add_subdirectory(external/Texas Texas EXCLUDE_FROM_ALL)
	target_link_libraries(dengine PRIVATE Texas)
#
# END
#

add_subdirectory(external/freetype2 freetype2 EXCLUDE_FROM_ALL)
target_link_libraries(dengine PRIVATE freetype)

#add_subdirectory(external/harfbuzz harfbuzz EXCLUDE_FROM_ALL)
#target_link_libraries(dengine PRIVATE harfbuzz)

# -----------------------
# Link external libraries
# -----------------------
# START
	add_library(dengine_application STATIC ${DENGINE_ROOT_DIR}/src/DEngine/Application/Application_Desktop.cpp)
	target_link_libraries(dengine PRIVATE dengine_application)
	target_include_directories(dengine_application PRIVATE ${DENGINE_INCLUDE_DIRS})
	target_compile_definitions(dengine_application PRIVATE ${DENGINE_DEFINES})
	target_include_directories(dengine_application PRIVATE ${VULKAN_INCLUDE_DIRS})
	if(UNIX)
		find_package(glfw3 REQUIRED)
	elseif(WIN32)
		set(GLFW3_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw3)
		# GLFW3
		set(GLFW_BUILD_EXAMPLES OFF)
		set(GLFW_BUILD_TESTS OFF)
		set(GLFW_BUILD_DOCS OFF)
		set(GLFW_INSTALL OFF)
		add_subdirectory(${GLFW3_DIR} glfw EXCLUDE_FROM_ALL)
		if(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
			target_compile_options(glfw PRIVATE -Wno-everything)
	endif()
	endif()

	
	target_link_libraries(dengine_application PRIVATE glfw)

#
# END
#
