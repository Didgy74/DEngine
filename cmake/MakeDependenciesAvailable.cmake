macro(DEngine_MakeDependenciesAvailable)
    include(FetchContent)

    # FreeType
    if (TARGET freetype)
        message(FATAL_ERROR "DEngine: CMake target 'freetype' was alrea found.")
    endif()
    # These must be set before FetchContent_MakeAvailable so that FreeType's
    # CMakeLists picks them up while it configures; setting them afterwards is a
    # no-op.
    set(SKIP_INSTALL_ALL OFF CACHE BOOL "" FORCE)
    set(FT_WITH_ZLIB FALSE CACHE BOOL "" FORCE)
    set(FT_WITH_BZIP2 OFF CACHE BOOL "" FORCE)
    set(FT_WITH_PNG OFF CACHE BOOL "" FORCE)
    set(FT_WITH_HARFBUZZ OFF CACHE BOOL "" FORCE)
    set(FT_WITH_BROTLI OFF CACHE BOOL "" FORCE)
    set(CMAKE_DISABLE_FIND_PACKAGE_ZLIB TRUE)
    set(CMAKE_DISABLE_FIND_PACKAGE_BZip2 TRUE)
    set(CMAKE_DISABLE_FIND_PACKAGE_PNG TRUE)
    set(CMAKE_DISABLE_FIND_PACKAGE_HarfBuzz TRUE)
    set(CMAKE_DISABLE_FIND_PACKAGE_BrotliDec TRUE)
    FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG        master
        EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(freetype)
    if (NOT TARGET freetype)
        message(FATAL_ERROR "DEngine: CMake target 'freetype' was not found.")
    endif()
    mark_as_advanced(freetype)

    # Texas
    if (TARGET Texas)
        message(FATAL_ERROR "DEngine: CMake target 'Texas' was already found.")
    endif()
    set(TEXAS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        texas
        GIT_REPOSITORY https://github.com/Didgy74/Texas.git
        GIT_TAG        development
        EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(texas)
    if (NOT TARGET Texas)
        message(FATAL_ERROR "DEngine: CMake target 'Texas' was not found.")
    endif()

    # Box2D
    if (TARGET box2d)
        message(FATAL_ERROR "DEngine: CMake target 'box2d' was already found.")
    endif()
    set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)
    set(BOX2D_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        box2d
        GIT_REPOSITORY https://github.com/erincatto/box2d.git
        GIT_TAG v2.4.2
        EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(box2d)
    if (NOT TARGET box2d)
        message(FATAL_ERROR "DEngine: CMake target 'box2d' was not found.")
    endif()
    mark_as_advanced(box2d)

    if (TARGET Vulkan-Headers)
        message(FATAL_ERROR "DEngine: CMake target 'Vulkan-Headers' was already found.")
    endif()
    FetchContent_Declare(
        vulkan-headers
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
        GIT_TAG main
        EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(vulkan-headers)
    if (NOT TARGET Vulkan-Headers)
        message(FATAL_ERROR "DEngine: CMake target 'Vulkan-Headers' was not found.")
    endif()

    if (TARGET VulkanMemoryAllocator)
        message(FATAL_ERROR "DEngine: CMake target 'VulkanMemoryAllocator' was already found.")
    endif()
    FetchContent_Declare(
        vmalib
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG 1d8f600fd424278486eade7ed3e877c99f0846b1 # v3.3.0
        EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(vmalib)
    if (NOT TARGET VulkanMemoryAllocator)
        message(FATAL_ERROR "DEngine: CMake target 'VulkanMemoryAllocator' was not found.")
    endif()


endmacro()
