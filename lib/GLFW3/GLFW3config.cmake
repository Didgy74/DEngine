if (WIN32)

    if (MSVC)

        set(GLFW3_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/MSVC/include)
        set(GLFW3_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/lib)

        set(GLFW3_LIBRARIES "${GLFW3_LIBDIR}/glfw3.lib;${GLFW3_LIBDIR}/glfw3dll.lib")

        set(GLFW3_BINDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/bin)
        set(GLFW3_BINNAME glfw3.dll)

    elseif(MINGW)

        set(GLFW3_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/MinGW/include)

        set(GLFW3_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MinGW/lib)
        set(GLFW3_LIBRARIES "-L${GLFW3_LIBDIR} -lglfw3 -lglfw3dll")

        set(GLFW3_BINDIR ${CMAKE_CURRENT_LIST_DIR}/MinGW/bin)
        set(GLFW3_BINNAME glfw3.dll)

    endif()

    set(GLFW3_BINARY "${GLFW3_BINDIR}/${GLFW3_BINNAME}")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD        # Adds a post-build event to MyTest
            COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
            ${GLFW3_BINARY}      # <--this is in-file
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${GLFW3_BINNAME}) # <--this is out-file path

endif()


#string(STRIP "${GLFW_LIBRARIES}" GLFW_LIBRARIES)