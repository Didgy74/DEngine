add_library(glfw INTERFACE)

set(glfw_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
target_include_directories(glfw INTERFACE ${glfw_INCLUDE_DIR})

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	if(${CMAKE_SIZEOF_VOID_P} MATCHES 8)
		set(glfw_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/MSVC x64/glfw3.lib;${CMAKE_CURRENT_LIST_DIR}/MSVC x64/glfw3dll.lib")
	else()
		set(glfw_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/MSVC x86/glfw3.lib;${CMAKE_CURRENT_LIST_DIR}/MSVC x86/glfw3dll.lib")
	endif()
else()
	message(FATAL_ERROR "Error. Haven't prepared any GLFW3 library files for this compiler.")
endif()

target_link_libraries(glfw INTERFACE ${glfw_LIBRARIES})

if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
	set(glfw_BINARIES "${CMAKE_CURRENT_LIST_DIR}/MSVC x64/glfw3.dll")
else()
	set(glfw_BINARIES "${CMAKE_CURRENT_LIST_DIR}/MSVC x86/glfw3.dll")
endif()