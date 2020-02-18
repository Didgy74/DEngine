cmake_minimum_required(VERSION 3.9)
project(DEngineEditor)

set(CMAKE_CXX_STANDARD 17)

# -------------------
# Set DEngine options
# -------------------
# START
	option(DENGINE_GFX_ENABLE_ASSERT "Asserts inside the rendering code" ON)

	option(DENGINE_CONTAINERS_ENABLE_ASSERT "Asserts inside DEngine containers" ON)
#
# END
#


# ------------------------
# Make the main executable
# ------------------------
# START
	file(GLOB_RECURSE DENGINE_SRC_FILES 
		"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" 
		"${CMAKE_CURRENT_SOURCE_DIR}/src/*.h" 
		"${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

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
	set(TEXAS_ENABLE_EVERYTHING ON)
	#set(TEXAS_ENABLE_KTX_READ OFF)
	#set(TEXAS_ENABLE_PNG_READ ON)
	set(TEXAS_ENABLE_VULKAN_TOOLS ON)
	#add_subdirectory("Texas")
	#target_link_libraries(${PROJECT_NAME} PUBLIC Texas)
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
	file(GLOB_RECURSE IMGUI_SOURCE_FILES "${IMGUI_SRC_DIR}/*.cpp" "${IMGUI_SRC_DIR}/*.h" "${IMGUI_SRC_DIR}/*.hpp")
	target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_SOURCE_FILES})

	target_include_directories(${PROJECT_NAME} PRIVATE ${IMGUI_INCLUDE_DIR})
#
# END
#


# -----------------------
# Link external libraries
# -----------------------
# START
	if(WIN32)
		set(SDL2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2/Windows")
	endif()
	find_package(SDL2 REQUIRED)
	target_link_libraries(${PROJECT_NAME} PUBLIC SDL2)

	find_package(Vulkan REQUIRED)
	target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
	#target_link_libraries(${DENGINE_TARGET_NAME} PRIVATE Vulkan::Vulkan)
#
# END
#


# SDL2 binaries
if(WIN32)
	set(SDL2_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2/Windows")
    if(MSVC)
		if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
			set(SDL2_BINDIR "${SDL2_FOLDER}/MSVC x86")
		elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
			set(SDL2_BINDIR "${SDL2_FOLDER}/MSVC x64")
		endif()
    elseif(MINGW)
		if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
			set(SDL2_BINDIR "${SDL2_FOLDER}/MinGW/x86/bin")	
		elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
			set(SDL2_BINDIR "${SDL2_FOLDER}/MinGW/x64/bin")
		endif()
	else()
		message(FATAL_ERROR "Platform not supported. Couldn't link GLEW.")
	endif()
	set(SDL2_BINNAME "SDL2.dll")
    set(SDL2_BINARY "${SDL2_BINDIR}/${SDL2_BINNAME}")
	add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${SDL2_BINARY}"
		"$<TARGET_FILE_DIR:${PROJECT_NAME}>/${SDL2_BINNAME}"
		)
endif()