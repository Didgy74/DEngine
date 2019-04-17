if(WIN32)

    if(MSVC)

        if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
		
			set(GLEW_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/x86/lib)
			set(GLEW_BINDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/x86/bin)
			
		elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
		
			set(GLEW_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/x64/lib)
			set(GLEW_BINDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/x64/bin)
			
		endif()
        
		set(GLEW_LIBRARIES "${GLEW_LIBDIR}/glew32s.lib;${GLEW_LIBDIR}/glew32.lib")
		
    elseif(MINGW)
		
		if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
		
			set(GLEW_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MinGW/x86/lib)
			set(GLEW_BINDIR ${CMAKE_CURRENT_LIST_DIR}/MinGW/x86/bin)
			
		elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
		
			set(GLEW_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MinGW/x64/lib)
			set(GLEW_BINDIR ${CMAKE_CURRENT_LIST_DIR}/MinGW/x64/bin)
			
		endif() 

		set(GLEW_LIBRARIES "-L${GLEW_LIBDIR} -lglew32 -lglew32mx")

    endif()

	set(GLEW_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/include")
	
	set(GLEW_BINNAME glew32.dll)
    set(GLEW_BINARY "${GLEW_BINDIR}/${GLEW_BINNAME}")

    add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${GLEW_BINARY}"
		"$<TARGET_FILE_DIR:${PROJECT_NAME}>/${GLEW_BINNAME}"
		)

endif()


#string(STRIP "${GLEW_LIBRARIES}" GLEW_LIBRARIES)