if (WIN32)

    if (MSVC)

        set(GLEW_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/MSVC/include)
        set(GLEW_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/lib)

        set(GLEW_LIBRARIES "${GLEW_LIBDIR}/glew32s.lib;${GLEW_LIBDIR}/glew32.lib")

        set(GLEW_BINDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/bin)
        set(GLEW_BINNAME glew32.dll)

    elseif(MINGW)

        set(GLEW_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/MinGW/include)

        set(GLEW_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MinGW/lib)
        set(GLEW_LIBRARIES "-L${GLEW_LIBDIR} -lglew32 -lglew32mx")

        set(GLEW_BINDIR ${CMAKE_CURRENT_LIST_DIR}/MinGW/bin)
        set(GLEW_BINNAME glew32.dll)

    endif()

    set(GLEW_BINARY "${GLEW_BINDIR}/${GLEW_BINNAME}")

    add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${GLEW_BINARY}"
		"$<TARGET_FILE_DIR:${PROJECT_NAME}>/${GLEW_BINNAME}"
		)

endif()


#string(STRIP "${GLEW_LIBRARIES}" GLEW_LIBRARIES)