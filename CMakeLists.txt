cmake_minimum_required(VERSION 3.13)
project(VulkanProject1)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src)

target_include_directories(${PROJECT_NAME} PRIVATE include)

# SDL2
if (WIN32)
    set(SDL2_DIR lib/SDL2)
endif()
find_package(SDL2)
if (SDL2_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
else()
    find_package(SDL2 CONFIG REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
endif()

# GLEW
if (WIN32)
    set(GLEW_DIR lib/GLEW)
    find_package(GLEW CONFIG REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${GLEW_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${GLEW_LIBRARIES})
elseif(UNIX)
    find_package(GLEW 2.0)
    if (NOT GLEW_FOUND)
        message(WARNING "Download some packages for GLEW.")
    else()
        target_link_libraries(${PROJECT_NAME} GLEW::GLEW)
    endif()
endif()

# OpenGL
find_package(OpenGL 3.3 REQUIRED)
target_link_libraries(${PROJECT_NAME} OpenGL::GL)

# Vulkan
if (WIN32)
    find_package(Vulkan 1.1 REQUIRED)
    target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)
elseif(UNIX)
    find_package(Vulkan 1.1)
    if (NOT Vulkan_FOUND)
        message(WARNING "You need Vulkan dev packages")
    else()
        target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)
    endif()
endif()

