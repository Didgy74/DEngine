if (WIN32)

    if (MSVC)

        set(GLFW_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/MSVC/include)
        set(GLFW_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/lib)

        set(GLFW_LIBRARIES "${GLFW_LIBDIR}/glfw3.lib;${GLFW_LIBDIR}/glfw3dll.lib")

        set(GLFW_BINDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/bin)
        set(GLFW_BINNAME glfw3.dll)

    elseif(MINGW)

        set(GLFW_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/MinGW/include)

        set(GLFW_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MinGW/lib)
        set(GLFW_LIBRARIES "-L${GLFW_LIBDIR} -lglfw3 -lglfw3dll")

        set(GLFW_BINDIR ${CMAKE_CURRENT_LIST_DIR}/MinGW/bin)
        set(GLFW_BINNAME glfw3.dll)

    endif()

    set(GLFW_BINARY "${GLFW_BINDIR}/${GLFW_BINNAME}")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD        # Adds a post-build event to MyTest
            COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
            ${GLFW_BINARY}      # <--this is in-file
            ${CMAKE_BINARY_DIR}/${GLFW_BINNAME}) # <--this is out-file path

endif()


#string(STRIP "${GLFW_LIBRARIES}" GLFW_LIBRARIES)