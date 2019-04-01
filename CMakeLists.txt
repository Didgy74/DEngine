cmake_minimum_required(VERSION 3.12)
project(DEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE include)

add_subdirectory(external)
target_link_libraries(${PROJECT_NAME} external::external)

# Add custom command to copy Data directory to build directory
# Remove Data directory in build directory if present
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		   COMMAND ${CMAKE_COMMAND} -E remove_directory
		   ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Data
		   )
# Copy directory from source folder to build folder
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${PROJECT_SOURCE_DIR}/Data
		   ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Data
		   )

# GLEW
find_package(GLEW 2.0 QUIET)
if (GLEW_FOUND)
	target_link_libraries(${PROJECT_NAME} GLEW::GLEW)
else()
	if(UNIX)
		message(FATAL_ERROR "Error. Building on UNIX without GLEW packages installed is not supported.")
	elseif(WIN32)
		set(GLEW_DIR lib/GLEW)
		find_package(GLEW CONFIG REQUIRED)
		target_include_directories(${PROJECT_NAME} PRIVATE ${GLEW_INCLUDE_DIRS})
		target_link_libraries(${PROJECT_NAME} ${GLEW_LIBRARIES})
	endif()
endif()

# OpenGL
find_package(OpenGL 4.3 REQUIRED)
target_link_libraries(${PROJECT_NAME} OpenGL::GL)

# GLFW
find_package(GLFW3 QUIET)
if(GLFW3_FOUND)
	target_include_directories(${PROJECT_NAME} PRIVATE ${GLFW3_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} ${GLFW3_LIBRARIES})
else()
	if (UNIX)
		message(FATAL_ERROR "Error. Building on UNIX without GLFW3 packages installed is not supported.")
	elseif(WIN32)
		set(GLFW3_DIR lib/GLFW3)
		find_package(GLFW3 CONFIG REQUIRED)
		target_include_directories(${PROJECT_NAME} PRIVATE ${GLFW3_INCLUDE_DIRS})
		target_link_libraries(${PROJECT_NAME} ${GLFW3_LIBRARIES})
	endif()
endif()
