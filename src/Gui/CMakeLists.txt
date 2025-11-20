cmake_minimum_required(VERSION 4.1)
project(dengine_gui)

option(DENGINE_GUI_BUILD_TESTS "Whether to configure and build tests" OFF)

if(NOT DEFINED DENGINE_GUI_ENABLE_ASSERT OR DENGINE_GUI_ENABLE_ASSERT STREQUAL "")
    message(FATAL_ERROR "Option DENGINE_GUI_ENABLE_ASSERT is not defined")
endif()

add_library(DEngine_Gui STATIC)
if(NOT DEFINED DENGINE_ADDITIONAL_COMPILE_ARGS)
    message(FATAL_ERROR "When processing DEngine_Gui, found no CMake variable for additional compile args")
endif()
target_compile_options(DEngine_Gui PUBLIC ${DENGINE_ADDITIONAL_COMPILE_ARGS})
DEngine_SetCompilerSettings(DEngine_Gui)

target_sources(DEngine_Gui PRIVATE
    #src/ButtonSizeBehavior.cpp
    src/DebugLog.cpp
    src/Gui.cpp
    src/RectCollection.cpp
    src/Utility.cpp)

target_sources(DEngine_Gui PUBLIC
    FILE_SET dengine_gui_cxx_modules TYPE CXX_MODULES
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
    FILES
        "src/AllocRef.cppm"
        "src/DrawEngine.cppm"
        "src/Events.cppm"
        "src/FontFaceSizeId.cppm"
        "src/SizeHint.cppm"
        "src/TextEngine.cppm"
        "src/TextInputType.cppm"
        "src/Utility.cppm"
        "src/WindowID.cppm"
        "src/WindowInsets.cppm")

set(INCLUDE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/include/DEngine/Gui")
target_sources(DEngine_Gui PRIVATE
    ${INCLUDE_ROOT}/impl/Assert.hpp
)

target_include_directories(DEngine_Gui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (NOT TARGET DEngine_Std)
    message(FATAL_ERROR "DEngine::Gui was unable to link DEngine::Std")
endif()
target_link_libraries(DEngine_Gui PUBLIC DEngine_Std)

if (NOT TARGET DEngine_Math)
    message(FATAL_ERROR "DEngine::Gui was unable to link DEngine::Math")
endif()
target_link_libraries(DEngine_Gui PUBLIC DEngine_Math)

if (DENGINE_GUI_ENABLE_ASSERT)
    if (NOT DENGINE_STD_ENABLE_ASSERT)
        message(
            WARNING
            "DEngine_Gui asserts are enabled, but DEngine_St asserts are not. Gui asserts will not work")
    endif()
    target_compile_definitions(DEngine_Gui PUBLIC DENGINE_GUI_ENABLE_ASSERT)
endif()

if (DENGINE_GUI_BUILD_TESTS)
    enable_testing()

    add_executable(DEngine_Gui_TestsAll
        tests/tests.cpp
    )

    target_link_libraries(DEngine_Gui_TestsAll PUBLIC DEngine_Gui)

    add_test(NAME DEngine_Gui_TestsAll COMMAND DEngine_Gui_TestsAlw)
endif()