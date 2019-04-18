set(LIB_NAME GLFW3)

add_library(${LIB_NAME} INTERFACE)
add_library(${LIB_NAME}::${LIB_NAME} ALIAS ${LIB_NAME})

# GLFW
find_package(GLFW3 QUIET)
if(GLFW3_FOUND)
	target_include_directories(${LIB_NAME} INTERFACE ${GLFW3_INCLUDE_DIRS})
	target_link_libraries(${LIB_NAME} INTERFACE ${GLFW3_LIBRARIES})
else()
	if (UNIX)
		message(FATAL_ERROR "Error. Building on UNIX without GLFW3 packages installed is not supported.")
	elseif(WIN32)
		
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

		# Link include directories
		set(GLFW3_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/include")
		target_include_directories(
			${LIB_NAME}
			INTERFACE
			$<BUILD_INTERFACE:${GLFW3_INCLUDE_DIRS}>
			$<INSTALL_INTERFACE:include>
			)

		# Link static libraries
		target_link_libraries(${LIB_NAME} INTERFACE ${GLFW3_LIBRARIES})
	
		set(GLFW3_BINNAME glfw3.dll)
		set(GLFW_BINARY "${GLFW3_BINDIR}/${GLFW3_BINNAME}")

	endif()
endif()