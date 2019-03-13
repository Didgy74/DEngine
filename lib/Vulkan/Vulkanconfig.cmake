if (MSVC)
    set(Vulkan_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/MSVC/include)
    set(Vulkan_LIBDIR ${CMAKE_CURRENT_LIST_DIR}/MSVC/lib)

    set(Vulkan_LIBRARIES "${Vulkan_LIBDIR}/vulkan-1.lib")

elseif(MINGW)

	set(Vulkan_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/MSVC/include)
	#message(WARNING "Error. Compiling on MinGW without Vulkan SDK currently does not work.")
endif()