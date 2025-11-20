cmake_minimum_required(VERSION 4.1)
project(dengine_math)

if(NOT DEFINED DENGINE_MATH_ENABLE_ASSERT OR DENGINE_MATH_ENABLE_ASSERT STREQUAL "")
    message(FATAL_ERROR "Option DENGINE_MATH_ENABLE_ASSERT is not defined")
endif()

add_library(DEngine_Math STATIC)
if(NOT DEFINED DENGINE_ADDITIONAL_COMPILE_ARGS)
    message(FATAL_ERROR "When processing DEngine_Math, found no CMake variable for additional compile args")
endif()
target_compile_options(DEngine_Math PUBLIC ${DENGINE_ADDITIONAL_COMPILE_ARGS})
DEngine_SetCompilerSettings(DEngine_Math)

target_sources(DEngine_Math PRIVATE
    src/Common.cpp
    src/LinearTransform3D.cpp
    src/Trigonometric.cpp
    src/Vector.cpp)

target_sources(DEngine_Math PUBLIC
    FILE_SET dengine_math_cxx_modules TYPE CXX_MODULES
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
    FILES
        "src/Common.cppm"
        "src/Constant.cppm"
        "src/Trigonometric.cppm"
        "src/Vector.cppm")

target_include_directories(DEngine_Math PUBLIC "include")

set(DENGINE_MATH_INCLUDE_DIR_PREFIX "include/DEngine/Math")
target_sources(DEngine_Math PUBLIC
    "${DENGINE_MATH_INCLUDE_DIR_PREFIX}/Enums.hpp"
    "${DENGINE_MATH_INCLUDE_DIR_PREFIX}/LinearTransform2D.hpp"
    "${DENGINE_MATH_INCLUDE_DIR_PREFIX}/LinearTransform3D.hpp"
    "${DENGINE_MATH_INCLUDE_DIR_PREFIX}/Math.hpp"
    "${DENGINE_MATH_INCLUDE_DIR_PREFIX}/Matrix.hpp"
    "${DENGINE_MATH_INCLUDE_DIR_PREFIX}/Trait.hpp"
    "${DENGINE_MATH_INCLUDE_DIR_PREFIX}/UnitQuaternion.hpp"
)

if (NOT TARGET DEngine_Std)
    message(FATAL_ERROR "DEngine::Math unable to link to DEngine::Std")
endif()
target_link_libraries(DEngine_Math PUBLIC DEngine_Std)

if (DENGINE_MATH_ENABLE_ASSERT)
    if (NOT DENGINE_STD_ENABLE_ASSERT)
        message(
            WARNING
            "DEngine_Math asserts are enabled, but DEngine_Std asserts are not. Math asserts will not work")
    endif()
    target_compile_definitions(DEngine_Math PUBLIC DENGINE_MATH_ENABLE_ASSERT)
endif()