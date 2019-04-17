if(WIN32)

    if(MSVC)

		if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
		
			set(GLFW3_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/x86/lib)
			set(GLFW3_BINDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/x86/bin)
			
		elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
		
			set(GLFW3_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/x64/lib)
			set(GLFW3_BINDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/x64/bin)
			
		endif()
        
		set(GLFW3_LIBRARIES "${GLFW3_LIBDIR}/glfw3.lib;${GLFW3_LIBDIR}/glfw3dll.lib")
		
    elseif(MINGW)
		
		if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
	
			set(GLFW3_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MinGW/x86/lib)
			set(GLFW3_BINDIR ${CMAKE_CURRENT_LIST_DIR}/MinGW/x86/bin)
			
		elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
		
			set(GLFW3_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MinGW/x64/lib)
			set(GLFW3_BINDIR ${CMAKE_CURRENT_LIST_DIR}/MinGW/x64/bin)
			
		endif()
        
		set(GLFW3_LIBRARIES "-L${GLFW3_LIBDIR} -lglfw3 -lglfw3dll")

    endif()

	set(GLFW3_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include)
	
	set(GLFW3_BINNAME glfw3.dll)
    set(GLFW3_BINARY "${GLFW3_BINDIR}/${GLFW3_BINNAME}")

    add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${GLFW3_BINARY}"
		"$<TARGET_FILE_DIR:${PROJECT_NAME}>/${GLFW3_BINNAME}"
		)

endif()