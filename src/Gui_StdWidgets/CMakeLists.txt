cmake_minimum_required(VERSION 4.1)
project(dengine_gui_stdwidgets)

add_library(DEngine_Gui_StdWidgets STATIC)
if(NOT DEFINED DENGINE_ADDITIONAL_COMPILE_ARGS)
    message(FATAL_ERROR "When processing DEngine_Gui_StdWidgets, found no CMake variable for additional compile args")
endif()
target_compile_options(DEngine_Gui_StdWidgets PUBLIC ${DENGINE_ADDITIONAL_COMPILE_ARGS})
DEngine_SetCompilerSettings(DEngine_Gui_StdWidgets)

# We manually add all our headers to our target.
set(DENGINE_GUI_STDWIDGETS_HEADERS_TEMP
    "Utilities/LineEditUtils.hpp"
    "DockArea.hpp"
    "Button.hpp"
    "CollapsingHeader.hpp"
    "LineEdit.hpp"
    "LineIntEdit.hpp"
    "LineFloatEdit.hpp"
    "Slider.hpp"
    "Text.hpp")
# Then we prepend the directory to these headers
set(DENGINE_GUI_STDWIDGETS_INCLUDE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/include/DEngine/Gui/StdWidgets")
foreach(item IN LISTS DENGINE_GUI_STDWIDGETS_HEADERS_TEMP)
    list(APPEND DENGINE_GUI_STDWIDGETS_HEADERS "${DENGINE_GUI_STDWIDGETS_INCLUDE_ROOT}/${item}")
endforeach()

target_sources(DEngine_Gui_StdWidgets PRIVATE
    src/DockArea/DockArea.cpp
    src/DockArea/PointerBehavior.cpp
    src/DockArea/Utilities.cpp
    src/AnchorArea.cpp
    src/Button.cpp
    src/ButtonUtils.cpp
    src/ButtonGroup.cpp
    src/CollapsingHeader.cpp
    src/Dropdown.cpp
    src/Grid.cpp
    src/LineEditUtils.cpp
    src/LineEdit.cpp
    src/LineFloatEdit.cpp
    src/LineIntEdit.cpp
    src/LineList.cpp
    src/Menu.cpp
    src/MenuButton.cpp
    src/ScrollArea.cpp
    src/Slider.cpp
    src/StackLayout.cpp
    src/Text.cpp)

set(DENGINE_GUI_STDWIDGETS_UNITY_EXCLUDES
    src/ScrollArea.cpp
)
set_source_files_properties(
    ${DENGINE_GUI_STDWIDGETS_UNITY_EXCLUDES}
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
)

function(apply_linking_and_stuff target)
    set_target_properties(${target} PROPERTIES UNITY_BUILD ON)

    target_include_directories(${target} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

    target_sources(${target} PRIVATE ${DENGINE_GUI_STDWIDGETS_HEADERS})

    if (NOT TARGET DEngine_Std)
        message(FATAL_ERROR "DEngine::Gui::StdWidgets was unable to link DEngine::Std")
    endif()
    target_link_libraries(${target} PUBLIC DEngine_Std)

    if (NOT TARGET DEngine_Math)
        message(FATAL_ERROR "DEngine::Gui::StdWidgets was unable to link DEngine::Math")
    endif()
    target_link_libraries(${target} PUBLIC DEngine_Math)

    if (NOT TARGET DEngine_Gui)
        message(FATAL_ERROR "DEngine::Gui::StdWidgets was unable to link DEngine::Gui")
    endif()
    target_link_libraries(${target} PUBLIC DEngine_Gui)
endfunction()

apply_linking_and_stuff(DEngine_Gui_StdWidgets)

add_library(DEngine_Gui_StdWidgets_Test STATIC
    src/Button.cpp
    src/StackLayout.cpp
)
apply_linking_and_stuff(DEngine_Gui_StdWidgets_Test)
