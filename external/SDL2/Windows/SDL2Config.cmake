add_library(SDL2 INTERFACE)

set(SDL2_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../include")
target_include_directories(SDL2 INTERFACE ${SDL2_INCLUDE_DIR})

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	if(${CMAKE_SIZEOF_VOID_P} MATCHES 8)
		set(SDL2_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/MSVC x64/SDL2.lib;${CMAKE_CURRENT_LIST_DIR}/MSVC x64/SDL2main.lib")
	else()
		set(SDL2_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/MSVC x86/SDL2.lib;${CMAKE_CURRENT_LIST_DIR}/MSVC x86/SDL2main.lib")
	endif()
else()
	message(FATAL_ERROR "Error. Haven't prepared any SDL2 library files for this compiler.")
endif()

target_link_libraries(SDL2 INTERFACE ${SDL2_LIBRARIES})

if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
	set(SDL2_BINARIES "${CMAKE_CURRENT_LIST_DIR}/MSVC x64/SDL2.dll")
else()
	set(SDL2_BINARIES "${CMAKE_CURRENT_LIST_DIR}/MSVC x86/SDL2.dll")
endif()