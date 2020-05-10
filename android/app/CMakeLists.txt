cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_STANDARD 17)

set(DENGINE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(DENGINE_INCLUDE_DIRS "${DENGINE_ROOT_DIR}/include")
set(DENGINE_SRC_FILES
		"${DENGINE_ROOT_DIR}/src/main.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Assert.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Editor.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Time.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Utility.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Application/Application.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Gfx.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/DeletionQueue.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/Draw.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/DynamicDispatch.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/Init.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/ObjectDataManager.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/QueueData.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/TextureManager.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/ViewportManager.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/Vk.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/vk_mem_alloc.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Math/Common.cpp"
		"${DENGINE_ROOT_DIR}/src/DEngine/Math/Vector.cpp"
		)

# Main engine library
	add_library(dengine SHARED ${DENGINE_SRC_FILES})
	target_include_directories(dengine PRIVATE ${DENGINE_INCLUDE_DIRS})
	file(GLOB_RECURSE test "${DENGINE_INCLUDE_DIRS}/*.hpp")
	target_sources(dengine PUBLIC ${test})

	target_link_libraries(dengine PRIVATE ${CMAKE_DL_LIBS})

# ----------
# Link Texas
# ----------
# START
	add_subdirectory("${DENGINE_ROOT_DIR}/external/Texas" binary_dir)
	target_link_libraries(dengine PRIVATE Texas)
#
# END
#

# Vulkan
	set(VULKAN_INCLUDE_DIRS "${DENGINE_ROOT_DIR}/external/Vulkan-Headers/include")
	target_include_directories(dengine PRIVATE ${VULKAN_INCLUDE_DIRS})

# Application backend library
	add_library(dengine_application STATIC "${DENGINE_ROOT_DIR}/src/DEngine/Application/Application_Android.cpp")
	target_link_libraries(dengine PRIVATE dengine_application)
	target_include_directories(dengine_application PRIVATE ${DENGINE_INCLUDE_DIRS})
	target_include_directories(dengine_application PRIVATE ${VULKAN_INCLUDE_DIRS})
# Add the Android-centric stuff
	set(APP_GLUE_DIR "${DENGINE_ROOT_DIR}/external/android_native_app_glue")
	add_library(android_native_app_glue STATIC "${APP_GLUE_DIR}/android_native_app_glue.c")
	target_link_libraries(dengine_application PRIVATE android_native_app_glue)
	target_include_directories(dengine_application PRIVATE ${APP_GLUE_DIR})
	#target_compile_definitions(android_native_app_glue PRIVATE "NDEBUG")

	# Adding this flag is really important, do not remove it.
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
	target_link_libraries(dengine_application PRIVATE log android)

# ----------------------------
# Include and link ImGui stuff
# ----------------------------
# START
	set(IMGUI_ROOT_DIR "${DENGINE_ROOT_DIR}/external/ImGui")
	set(IMGUI_SRC_DIR "${IMGUI_ROOT_DIR}/src")
	set(IMGUI_INCLUDE_DIRS "${IMGUI_ROOT_DIR}/include")
	file(GLOB_RECURSE IMGUI_SOURCE_FILES "${IMGUI_SRC_DIR}/*.cpp" )
	add_library(IMGUI_LIB STATIC ${IMGUI_SOURCE_FILES})
	target_include_directories(IMGUI_LIB PUBLIC ${IMGUI_INCLUDE_DIRS})
	target_include_directories(IMGUI_LIB PRIVATE ${VULKAN_INCLUDE_DIRS})

	target_link_libraries(dengine PRIVATE IMGUI_LIB)
#
# END
#

