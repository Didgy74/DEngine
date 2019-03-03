if (WIN32)
    if (MSVC)

        set(SDL2_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/MSVC/include)
        set(SDL2_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/lib)

        set(SDL2_LIBRARIES "${SDL2_LIBDIR}/SDL2.lib;${SDL2_LIBDIR}/SDL2main.lib")

        set(SDL2_BINDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/bin)
        set(SDL2_BINNAME SDL2.dll)

    elseif(MINGW)

        set(SDL2_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/MinGW/include)

        set(SDL2_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MinGW/lib)
        set(SDL2_LIBRARIES "-L${SDL2_LIBDIR} -lmingw32 -lSDL2main -lSDL2")

        set(SDL2_BINDIR ${CMAKE_CURRENT_LIST_DIR}/MinGW/bin)
        set(SDL2_BINNAME SDL2.dll)

    endif()

    set(SDL2_BINARY "${SDL2_BINDIR}/${SDL2_BINNAME}")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD        # Adds a post-build event to MyTest
            COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
            ${SDL2_BINARY}      # <--this is in-file
            ${CMAKE_BINARY_DIR}/${SDL2_BINNAME}) # <--this is out-file path

endif()


#string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)