cmake_minimum_required(VERSION 3.9)
project(DEngineEditor)

set(CMAKE_CXX_STANDARD 17)

# -------------------
# Set DEngine options
# -------------------
# START
	option(DENGINE_GFX_ENABLE_ASSERT "Asserts inside the rendering code" OFF)

	option(DENGINE_CONTAINERS_ENABLE_ASSERT "Asserts inside DEngine containers" OFF)
#
# END
#


# ------------------------
# Make the main executable
# ------------------------
# START
	file(GLOB_RECURSE DENGINE_SRC_FILES 
		"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

	add_executable(${PROJECT_NAME} ${DENGINE_SRC_FILES})

	# Remove data folder from target binary directory
	add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E remove_directory
		"$<TARGET_FILE_DIR:${PROJECT_NAME}>/data")

	# Copy data directory from source folder to build folder
	add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${PROJECT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:${PROJECT_NAME}>/data")

	target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

	target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})

	if (MSVC)
		target_compile_options(${PROJECT_NAME} PRIVATE /WX)
	endif()

	
#
# END
#

# ----------------------
# Handle DEngine options
# ----------------------
# START
	# Handles the renderer's assert option
	if(DENGINE_GFX_ENABLE_ASSERT)
		target_compile_definitions(${PROJECT_NAME} PRIVATE "DENGINE_GFX_ENABLE_ASSERT")
	endif()

	# Handles the DEngine container's assert option
	if(DENGINE_CONTAINERS_ENABLE_ASSERT)
		target_compile_definitions(${PROJECT_NAME} PRIVATE "DENGINE_CONTAINERS_ENABLE_ASSERT")
	endif()
#
# END
#


# ----------
# Link Texas
# ----------
# START
	#add_subdirectory("Texas")
	#target_link_libraries(${PROJECT_NAME} PUBLIC Texas)
#
# END
#

# -----------------------
# Link external libraries
# -----------------------
# START
	if(WIN32)
		set(glfw3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw3")
	endif()
	# GLFW3
	find_package(glfw3 REQUIRED)
	target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

	find_package(Vulkan REQUIRED)
	target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
#
# END
#

# ----------------------------
# Include and link ImGui stuff
# ----------------------------
# START
	set(IMGUI_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/ImGui")
	set(IMGUI_SRC_DIR "${IMGUI_ROOT_DIR}/src")
	set(IMGUI_INCLUDE_DIR "${IMGUI_ROOT_DIR}/include")
	file(GLOB_RECURSE IMGUI_SOURCE_FILES "${IMGUI_SRC_DIR}/*.cpp" )
	add_library(IMGUI_LIB ${IMGUI_SOURCE_FILES})
	target_include_directories(IMGUI_LIB PUBLIC ${IMGUI_INCLUDE_DIR})
	target_link_libraries(IMGUI_LIB PUBLIC glfw)
	target_include_directories(IMGUI_LIB PRIVATE ${Vulkan_INCLUDE_DIRS})
	if (MSVC)
		target_compile_options(IMGUI_LIB PRIVATE /w)
	endif()

	target_link_libraries(${PROJECT_NAME} PRIVATE IMGUI_LIB)
#
# END
#


# GLFW3 binaries
if(WIN32)
	set(GLFW3_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/external/GLFW3")
    if(MSVC)
		if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
			set(GLFW3_BINDIR "${GLFW3_FOLDER}/MSVC x86")
		elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
			set(GLFW3_BINDIR "${GLFW3_FOLDER}/MSVC x64")
		endif()
	else()
		message(FATAL_ERROR "Platform not supported. Couldn't link GLFW.")
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