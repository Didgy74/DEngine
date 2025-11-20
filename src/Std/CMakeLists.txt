cmake_minimum_required(VERSION 4.1)
project(dengine_std)

if(NOT DEFINED DENGINE_STD_ENABLE_ASSERT OR DENGINE_STD_ENABLE_ASSERT STREQUAL "")
    message(FATAL_ERROR "Option DENGINE_STD_ENABLE_ASSERT is not defined")
endif()

add_library(DEngine_Std STATIC)
if(NOT DEFINED DENGINE_ADDITIONAL_COMPILE_ARGS)
    message(FATAL_ERROR "When processing DEngine_Std, found no CMake variable for additional compile args")
endif()
target_compile_options(DEngine_Std PUBLIC ${DENGINE_ADDITIONAL_COMPILE_ARGS})

target_sources(DEngine_Std PRIVATE
    src/Allocator.cpp
    src/Assert.cpp
    src/BumpAllocator.cpp
    src/Common.cpp
    src/Std.cpp
    src/Utility.cpp)

DEngine_SetCompilerSettings(DEngine_Std)

target_include_directories(DEngine_Std PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(DENGINE_STD_INCLUDE_DIR_PREFIX "include/DEngine/Std")

target_sources(DEngine_Std PUBLIC
    FILE_SET dengine_std_cxx_modules TYPE CXX_MODULES
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
    FILES
        "src/Allocator.cppm"
        "src/BumpAllocator.cppm"
        "src/Common.cppm"
        "src/FixedWidthTypes.cppm"
        "src/Range.cppm"
        "src/Trait.cppm"
        "src/Utility.cppm"
        "src/Containers/AnyRef.cppm"
        "src/Containers/Array.cppm"
        "src/Containers/Box.cppm"
        "src/Containers/Opt.cppm"
        "src/Containers/RangeFnRef.cppm"
        "src/Containers/Span.cppm"
        "src/Containers/StackVec.cppm"
        "src/Containers/Vec.cppm")

find_package(Threads REQUIRED)
target_link_libraries(DEngine_Std PUBLIC Threads::Threads)

set(NATVIS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Containers/Span.natvis
)

# Add natvis files to your target
target_sources(DEngine_Std PRIVATE "${NATVIS_FILES}")

# Register them for Visual Studio debugger
set_target_properties(DEngine_Std PROPERTIES
    VS_DEBUGGER_VISUALIZER_FILES "${NATVIS_FILES}"
)

if (DENGINE_STD_ENABLE_ASSERT)
    target_compile_definitions(DEngine_Std PUBLIC
        $<$<COMPILE_LANGUAGE:CXX>:DENGINE_ENABLE_ASSERT=TRUE>)
endif()
