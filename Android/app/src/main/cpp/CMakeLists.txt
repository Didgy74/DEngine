
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

# ------------------------
# Make the main executable
# ------------------------
# START
    set(DENGINE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../..")

    file(GLOB_RECURSE DENGINE_SRC_FILES
            "${DENGINE_ROOT_DIR}/src/*.cpp")

    set(DENGINE_INCLUDE_DIRS "${DENGINE_ROOT_DIR}/include")

    add_library(main SHARED ${DENGINE_SRC_FILES})

    target_include_directories(main PUBLIC ${DENGINE_INCLUDE_DIRS})
#
# END
#

# -----------------------
# Link SDL and vulkan-headers
# -----------------------
# START
    set(VULKAN_INCLUDE_DIRS "${DENGINE_ROOT_DIR}/external/Vulkan-Headers/include")
    target_include_directories(main PUBLIC ${VULKAN_INCLUDE_DIRS})

    set(SDL2_INCLUDE_DIRS "${DENGINE_ROOT_DIR}/external/SDL2/include")
    target_include_directories(main PUBLIC ${SDL2_INCLUDE_DIRS})

    set(SDL2_LIBRARIES
            "${DENGINE_ROOT_DIR}/Android/app/src/main/jni/${CMAKE_ANDROID_ARCH_ABI}/libSDL2.so"
            "${DENGINE_ROOT_DIR}/Android/app/src/main/jni/${CMAKE_ANDROID_ARCH_ABI}/libhidapi.so")
    target_link_libraries(main PUBLIC ${SDL2_LIBRARIES})
#
# END
#


# ----------------------------
# Include and link ImGui stuff
# ----------------------------
# START
    set(IMGUI_ROOT_DIR "${DENGINE_ROOT_DIR}/external/ImGui")
    set(IMGUI_SRC_DIR "${IMGUI_ROOT_DIR}/src")
    set(IMGUI_INCLUDE_DIRS "${IMGUI_ROOT_DIR}/include")
    file(GLOB_RECURSE IMGUI_SOURCE_FILES "${IMGUI_SRC_DIR}/*.cpp")
    target_sources(main PRIVATE ${IMGUI_SOURCE_FILES})

    target_include_directories(main PRIVATE ${IMGUI_INCLUDE_DIRS})
#
# END
#