
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

set(DENGINE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../..")

file(GLOB_RECURSE DENGINE_SRC_FILES
        "${DENGINE_ROOT_DIR}/src/*.cpp"
        "${DENGINE_ROOT_DIR}/src/*.hpp"
        "${DENGINE_ROOT_DIR}/include/*.hpp")

set(DENGINE_INCLUDE_DIR "${DENGINE_ROOT_DIR}/include")

set(LIB_NAME "main")
add_library(${LIB_NAME} SHARED ${DENGINE_SRC_FILES})

target_include_directories(${LIB_NAME} PUBLIC ${DENGINE_INCLUDE_DIR})

set(VULKAN_INCLUDE_DIR "C:/VulkanSDK/1.2.131.1/Include")
target_include_directories(${LIB_NAME} PUBLIC ${VULKAN_INCLUDE_DIR})

set(SDL2_INCLUDE_DIR "${DENGINE_ROOT_DIR}/external/SDL2/include")
target_include_directories(${LIB_NAME} PUBLIC ${SDL2_INCLUDE_DIR})

set(SDL2_LIBRARIES
        "${DENGINE_ROOT_DIR}/Android/app/src/main/jni/${CMAKE_ANDROID_ARCH_ABI}/libSDL2.so"
        "${DENGINE_ROOT_DIR}/Android/app/src/main/jni/${CMAKE_ANDROID_ARCH_ABI}/libhidapi.so")
target_link_libraries(${LIB_NAME} PUBLIC ${SDL2_LIBRARIES})

# ----------------------------
# Include and link ImGui stuff
# ----------------------------
# START
set(IMGUI_ROOT_DIR "${DENGINE_ROOT_DIR}/external/ImGui")
set(IMGUI_SRC_DIR "${IMGUI_ROOT_DIR}/src")
set(IMGUI_INCLUDE_DIR "${IMGUI_ROOT_DIR}/include")
file(GLOB_RECURSE IMGUI_SOURCE_FILES "${IMGUI_SRC_DIR}/*.cpp" "${IMGUI_SRC_DIR}/*.h" "${IMGUI_SRC_DIR}/*.hpp")
target_sources(${LIB_NAME} PRIVATE ${IMGUI_SOURCE_FILES})

target_include_directories(${LIB_NAME} PRIVATE ${IMGUI_INCLUDE_DIR})
#
# END
#