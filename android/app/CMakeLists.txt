cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_STANDARD 17)
set(DENGINE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
include(${DENGINE_ROOT_DIR}/Common.cmake)



# Main engine library
add_library(dengine SHARED ${DENGINE_SRC_FILES})
target_include_directories(dengine PRIVATE ${DENGINE_INCLUDE_DIRS})
target_compile_definitions(dengine PRIVATE ${DENGINE_DEFINES})
target_link_libraries(dengine PRIVATE ${CMAKE_DL_LIBS})

# ----------
# Link Texas
# ----------
# START
	add_subdirectory(${DENGINE_ROOT_DIR}/external/Texas Texas EXCLUDE_FROM_ALL)
	target_link_libraries(dengine PRIVATE Texas)
#
# END
#

add_subdirectory(${DENGINE_ROOT_DIR}/external/freetype2 freetype EXCLUDE_FROM_ALL)
target_link_libraries(dengine PRIVATE freetype)

# Vulkan
	set(VULKAN_INCLUDE_DIRS ${DENGINE_ROOT_DIR}/external/Vulkan-Headers/include)
	target_include_directories(dengine PRIVATE ${VULKAN_INCLUDE_DIRS})

# Application backend library
	add_library(dengine_application STATIC "${DENGINE_ROOT_DIR}/src/DEngine/Application/Application_Android.cpp")
	target_include_directories(dengine_application PRIVATE ${DENGINE_INCLUDE_DIRS})
	target_compile_definitions(dengine_application PRIVATE ${DENGINE_DEFINES})
	target_include_directories(dengine_application PRIVATE ${VULKAN_INCLUDE_DIRS})
	target_link_libraries(dengine PRIVATE dengine_application)
# Add the Android-centric stuff
	add_library(
		app-glue
		STATIC
		${DENGINE_ROOT_DIR}/external/android_native_app_glue/android_native_app_glue.c)
	target_link_libraries(dengine_application PRIVATE app-glue)
	target_include_directories(dengine_application PRIVATE ${DENGINE_ROOT_DIR}/external/android_native_app_glue/)

	# Adding this flag is really important, do not remove it.
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
	target_link_libraries(dengine_application PRIVATE log android)