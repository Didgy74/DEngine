macro(DEngineAny_LinkDependencies TARGET ENABLE_CMAKE_LOGGING)

	include(FetchContent)

	# Posix threads
	find_package(Threads REQUIRED)
	target_link_libraries(${TARGET}
		PUBLIC
		Threads::Threads)

	# Dynamic lib loading
	target_link_libraries(${TARGET}
		PUBLIC
		${CMAKE_DL_LIBS})

	# FreeType
	if (NOT TARGET freetype)
		message(FATAL_ERROR "DEngine: CMake target 'freetype' was not found.")
	endif()
	target_link_libraries(${TARGET} PRIVATE freetype)

	# Texas
	if (NOT TARGET Texas)
		message(FATAL_ERROR "DEngine: CMake target 'Texas' was not found.")
	endif()
	target_link_libraries(${TARGET} PRIVATE Texas)

	# Box2D
	if (NOT TARGET box2d)
		message(FATAL_ERROR "DEngine: CMake target 'box2d' was not found.")
	endif()
	target_link_libraries(${TARGET} PRIVATE box2d)

	# Vulkan headers
	if (NOT TARGET Vulkan-Headers)
		message(FATAL_ERROR "DEngine: CMake target 'Vulkan-Headers' was not found.")
	endif()
	target_link_libraries(${TARGET} PUBLIC Vulkan-Headers)

	if (NOT TARGET VulkanMemoryAllocator)
		message(FATAL_ERROR "DEngine: CMake target 'VulkanMemoryAllocator' was not found.")
	endif()
	target_link_libraries(${TARGET} PRIVATE VulkanMemoryAllocator)
	target_compile_definitions(${TARGET}
		PRIVATE
			$<$<COMPILE_LANGUAGE:CXX>:VMA_STATIC_VULKAN_FUNCTIONS=0>
			$<$<COMPILE_LANGUAGE:CXX>:VMA_DYNAMIC_VULKAN_FUNCTIONS=0>
			$<$<COMPILE_LANGUAGE:CXX>:VMA_VULKAN_VERSION=1001000>)


	#add_compile_definitions(${TARGET} PUBLIC TRACY_ENABLE=1)

	#include(FetchContent)
	#FetchContent_Declare(
	#	tracy
	#	GIT_REPOSITORY https://www.github.com/wolfpld/tracy.git
	#	GIT_TAG v0.9.1
	#	FIND_PACKAGE_ARGS NAMES TracyClient
	#)
	#set(TRACY_CALLSTACK ON)
	#FetchContent_MakeAvailable(tracy)
	#target_link_libraries(${TARGET} PUBLIC TracyClient)
	#target_compile_definitions(${TARGET} PUBLIC DENGINE_TRACY_LINKED)


	#target_link_libraries(${TARGET} PUBLIC fmt::fmt)

endmacro()