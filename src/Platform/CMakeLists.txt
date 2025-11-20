cmake_minimum_required(VERSION 4.1)
if (NOT DENGINE_OS)
    message(FATAL_ERROR "DENGINE_OS not set")
endif()
project(dengine_platform LANGUAGES CXX)

set(DENGINE_PLATFORM_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if (DENGINE_OS STREQUAL "Windows")
    set(DENGINE_PLATFORM_OS_VALUE 0)
elseif (DENGINE_OS STREQUAL "Android")
    set(DENGINE_PLATFORM_OS_VALUE 2)
elseif (DENGINE_OS STREQUAL "macOS")
    set(DENGINE_PLATFORM_OS_VALUE 3)
else()
    message(FATAL_ERROR "DENGINE_OS set no unknown string")
endif()

if(NOT DEFINED DENGINE_PLATFORM_ENABLE_ASSERT OR DENGINE_PLATFORM_ENABLE_ASSERT STREQUAL "")
    message(FATAL_ERROR "Option DENGINE_PLATFORM_ENABLE_ASSERT is not defined")
endif()

add_library(DEngine_Platform STATIC)
if(NOT DEFINED DENGINE_ADDITIONAL_COMPILE_ARGS)
    message(FATAL_ERROR "When processing DEngine_Platform, found no CMake variable for additional compile args")
endif()
target_compile_options(DEngine_Platform PUBLIC ${DENGINE_ADDITIONAL_COMPILE_ARGS})
DEngine_SetCompilerSettings(DEngine_Platform)

set(DENGINE_PLATFORM_SOURCES
    "src/Platform.cpp")

set(DENGINE_PLATFORM_HEADERS_TEMP
    "PlatformAssert.hpp"
    "Platform.hpp")
set(DENGINE_PLATFORM_INCLUDE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/include/DEngine/Platform")
foreach(item IN LISTS DENGINE_PLATFORM_HEADERS_TEMP)
    list(APPEND DENGINE_PLATFORM_HEADERS "${DENGINE_PLATFORM_INCLUDE_ROOT}/${item}")
endforeach()

target_sources(DEngine_Platform PRIVATE ${DENGINE_PLATFORM_SOURCES})

function(apply_linking_and_stuff target)
    target_compile_definitions(${target} PUBLIC
        $<$<COMPILE_LANGUAGE:CXX>:DENGINE_PLATFORM_OS=${DENGINE_PLATFORM_OS_VALUE}>)

    target_include_directories(${target} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

    target_sources(${target} PRIVATE ${DENGINE_PLATFORM_HEADERS})

    if (NOT TARGET DEngine_Std)
        message(FATAL_ERROR "DEngine::Gui::StdWidgets was unable to link DEngine::Std")
    endif()
    target_link_libraries(${target} PUBLIC DEngine_Std)

    if (NOT TARGET DEngine_Math)
        message(FATAL_ERROR "DEngine::Gui::StdWidgets was unable to link DEngine::Math")
    endif()
    target_link_libraries(${target} PUBLIC DEngine_Math)

endfunction()

apply_linking_and_stuff(DEngine_Platform)

if (DENGINE_PLATFORM_ENABLE_ASSERT)
    if (NOT DENGINE_STD_ENABLE_ASSERT)
        message(
            WARNING
            "DEngine_Platform asserts are enabled, but DEngine_Std asserts are not. Platform asserts will not work.")
    endif()
    target_compile_definitions(DEngine_Platform PUBLIC DENGINE_APPLICATION_ENABLE_ASSERT)
endif()

if (DENGINE_OS STREQUAL "Windows")
    target_sources(DEngine_Platform
        PUBLIC
            "windows/PlatformImpl.cpp"
        PRIVATE
            "windows/Accessibility.cpp"
            "windows/win32_vk.cpp"
            "windows/InitGuid.cpp")

    if (NOT TARGET Vulkan-Headers)
        message(FATAL_ERROR "DEngine_Platform: CMake target 'Vulkan-Headers' was not found.")
    endif()
    target_link_libraries(DEngine_Platform PRIVATE Vulkan-Headers)
    target_link_libraries(DEngine_Platform PRIVATE xinput)

elseif (DENGINE_OS STREQUAL "macOS")
    target_sources(DEngine_Platform PRIVATE
        "${DENGINE_PLATFORM_SOURCE_DIR}/macos/PlatformImpl.cpp"
        "${DENGINE_PLATFORM_SOURCE_DIR}/macos/FileInputStream.cpp")

    # Embed the SDL gamecontroller DB as a string literal so we can feed it to
    # glfwUpdateGamepadMappings() at init. This makes Xbox/PlayStation/etc.
    # controllers light up as mapped gamepads on macOS where GLFW's built-in
    # mapping list is too sparse.
    file(READ
        "${DENGINE_PLATFORM_SOURCE_DIR}/macos/gamecontrollerdb.txt"
        GAMECONTROLLERDB_CONTENTS)
    configure_file(
        "${DENGINE_PLATFORM_SOURCE_DIR}/macos/gamecontrollerdb.hpp.in"
        "${CMAKE_CURRENT_BINARY_DIR}/macos_generated/gamecontrollerdb.hpp"
        @ONLY)
    target_include_directories(DEngine_Platform PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/macos_generated")

    if (${DENGINE_ENABLE_VERBOSE_CMAKE_OUTPUT})
        message(CHECK_START "DEngine_desktop: Searching for GLFW package.")
    endif()

    if (TARGET glfw)
        message(FATAL_ERROR "DEngine: CMake target 'glfw' was already found.")
    endif()
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    set(GLFW_VULKAN_STATIC OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 7b6aead9fb88b3623e3b3725ebb42670cbe4c579 # v3.4
        EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(glfw)
    if (NOT TARGET glfw)
        message(FATAL_ERROR "DEngine: CMake target 'glfw' was not found.")
    endif()

    target_link_libraries(DEngine_Platform PRIVATE glfw)

    if (NOT TARGET Vulkan-Headers)
        message(FATAL_ERROR "DEngine_Platform: CMake target 'Vulkan-Headers' was not found.")
    endif()
    target_link_libraries(DEngine_Platform PRIVATE Vulkan-Headers)

endif()