cmake_minimum_required(VERSION 3.13)
project(DEngine)

set(CMAKE_CXX_STANDARD 17)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

add_compile_definitions(DRENDERER_DEVELOPMENT)

file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.inl")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	target_link_libraries(${PROJECT_NAME} stdc++fs)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(external)
target_link_libraries(${PROJECT_NAME} external::external)

# Add custom command to copy Data directory to build directory
# Remove Data directory in build directory if present
#add_custom_command(
#	TARGET ${PROJECT_NAME} POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E remove_directory
#	$<TARGET_FILE_DIR:${PROJECT_NAME}>/data
#	)

# Copy directory from source folder to build folder
add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/data
	$<TARGET_FILE_DIR:${PROJECT_NAME}>/data
	)

# GLFW3 binaries
if(WIN32)
	set(GLFW3_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/external/GLFW3")
    if(MSVC)
		if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
			set(GLFW3_BINDIR ${GLFW3_FOLDER}/MSVC/x86/bin)
		elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
			set(GLFW3_BINDIR ${GLFW3_FOLDER}/MSVC/x64/bin)
		endif()
    elseif(MINGW)
		if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
			set(GLFW3_BINDIR ${GLFW3_FOLDER}/MinGW/x86/bin)	
		elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
			set(GLFW3_BINDIR ${GLFW3_FOLDER}/MinGW/x64/bin)
		endif()
	else()
		message(FATAL_ERROR "Platform not supported. Couldn't link GLEW.")
	endif()
	set(GLFW3_BINNAME glfw3.dll)
    set(GLFW3_BINARY "${GLFW3_BINDIR}/${GLFW3_BINNAME}")
	add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${GLFW3_BINARY}"
		"$<TARGET_FILE_DIR:${PROJECT_NAME}>/${GLFW3_BINNAME}"
		)
endif()

# GLEW binaries
if(WIN32)
	set(GLEW_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/external/GLEW")
    if(MSVC)
		if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
			set(GLEW_BINDIR ${GLEW_FOLDER}/MSVC/x86/bin)
		elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
			set(GLEW_BINDIR ${GLEW_FOLDER}/MSVC/x64/bin)
		endif()
    elseif(MINGW)
		if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
			set(GLEW_BINDIR ${GLEW_FOLDER}/MinGW/x86/bin)	
		elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
			set(GLEW_BINDIR ${GLEW_FOLDER}/MinGW/x64/bin)
		endif()
	else()
		message(FATAL_ERROR "Platform not supported. Couldn't link GLEW.")
	endif()
	set(GLEW_BINNAME glew32.dll)
    set(GLEW_BINARY "${GLEW_BINDIR}/${GLEW_BINNAME}")
	add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${GLEW_BINARY}"
		"$<TARGET_FILE_DIR:${PROJECT_NAME}>/${GLEW_BINNAME}"
		)
endif()