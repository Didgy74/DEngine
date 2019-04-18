set(LIB_NAME GLEW)

add_library(${LIB_NAME} INTERFACE)
add_library(${LIB_NAME}::${LIB_NAME} ALIAS ${LIB_NAME})

find_package(GLEW 2.0 QUIET)
if (GLEW_FOUND)
	target_link_libraries(${PROJECT_NAME} INTERFACE GLEW::GLEW)
else()
	if(UNIX)

		message(FATAL_ERROR "Error. Building on UNIX without GLEW packages installed is not supported.")

	elseif(WIN32)

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

		# Link include directories
		set(GLEW_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/include")
		target_include_directories(
			${LIB_NAME}
			INTERFACE
			$<BUILD_INTERFACE:${GLEW_INCLUDE_DIRS}>
			$<INSTALL_INTERFACE:include>
			)
	
		# Link static libraries
		target_link_libraries(${LIB_NAME} INTERFACE ${GLEW_LIBRARIES})

		set(GLEW_BINNAME glew32.dll)
		set(GLEW_BINARY "${GLEW_BINDIR}/${GLEW_BINNAME}")

		#add_custom_command(
		#	TARGET ${LIB_NAME} POST_BUILD
		#	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		#	"${GLEW_BINARY}"
		#	"$<TARGET_FILE_DIR:${PROJECT_NAME}>/${GLEW_BINNAME}"
		#	)

	endif()

endif()