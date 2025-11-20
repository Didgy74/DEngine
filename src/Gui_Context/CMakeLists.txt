cmake_minimum_required(VERSION 4.1)
project(dengine_gui_context)

add_library(DEngine_Gui_Context STATIC)
if(NOT DEFINED DENGINE_ADDITIONAL_COMPILE_ARGS)
    message(FATAL_ERROR "When processing DEngine_Gui_Context, found no CMake variable for additional compile args")
endif()
target_compile_options(DEngine_Gui_Context PUBLIC ${DENGINE_ADDITIONAL_COMPILE_ARGS})
DEngine_SetCompilerSettings(DEngine_Gui_Context)

# We manually add all our headers to our target.
set(DENGINE_GUI_CONTEXT_HEADERS_TEMP
    "Context.hpp")
# Then we prepend the directory to these headers
set(DENGINE_GUI_CONTEXT_INCLUDE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/include/DEngine/Gui/Context")
foreach(item IN LISTS DENGINE_GUI_CONTEXT_HEADERS_TEMP)
    list(APPEND DENGINE_GUI_CONTEXT_HEADERS "${DENGINE_GUI_CONTEXT_INCLUDE_ROOT}/${item}")
endforeach()

target_sources(DEngine_Gui_Context PRIVATE
    src/Context.cpp)

function(apply_linking_and_stuff target)
    set_target_properties(${target} PROPERTIES UNITY_BUILD ON)

    target_include_directories(${target} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

    target_sources(${target} PRIVATE ${DENGINE_GUI_CONTEXT_HEADERS})

    if (NOT TARGET DEngine_Std)
        message(FATAL_ERROR "DEngine::Gui::Context was unable to link DEngine::Std")
    endif()
    target_link_libraries(${target} PUBLIC DEngine_Std)

    if (NOT TARGET DEngine_Math)
        message(FATAL_ERROR "DEngine::Gui::Context was unable to link DEngine::Math")
    endif()
    target_link_libraries(${target} PUBLIC DEngine_Math)

    if (NOT TARGET DEngine_Gui)
        message(FATAL_ERROR "DEngine::Gui::Context was unable to link DEngine::Gui")
    endif()
    target_link_libraries(${target} PUBLIC DEngine_Gui)
endfunction()

apply_linking_and_stuff(DEngine_Gui_Context)
