function(DEngineAny_LinkDependencies TARGET ENABLE_CMAKE_LOGGING)

	# FreeType
	if (${ENABLE_CMAKE_LOGGING})
		message(CHECK_START "DEngine: Searching for FreeType2 package.")
	endif()
	find_package(Freetype QUIET)
	if(NOT ${FREETYPE_FOUND})
		if (UNIX)
			message(FATAL_ERROR "DEngine: No system package FreeType2-dev found. Install this system package and try again.")
		endif()

		if (${ENABLE_CMAKE_LOGGING})
			message(CHECK_FAIL "No system FreeType2 package found. Building from source.")
		endif()

		set(SKIP_INSTALL_ALL OFF CACHE BOOL "" FORCE)
		set(FT_WITH_ZLIB OFF CACHE BOOL "" FORCE)
		set(FT_WITH_BZIP2 OFF CACHE BOOL "" FORCE)
		set(FT_WITH_PNG OFF CACHE BOOL "" FORCE)
		set(FT_WITH_HARFBUZZ OFF CACHE BOOL "" FORCE)
		set(FT_WITH_BROTLI OFF CACHE BOOL "" FORCE)
		set(CMAKE_DISABLE_FIND_PACKAGE_ZLIB TRUE)
		set(CMAKE_DISABLE_FIND_PACKAGE_BZip2 TRUE)
		set(CMAKE_DISABLE_FIND_PACKAGE_PNG TRUE)
		set(CMAKE_DISABLE_FIND_PACKAGE_HarfBuzz TRUE)
		set(CMAKE_DISABLE_FIND_PACKAGE_BrotliDec TRUE)

		add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/freetype" freetype)
	else()
		if (${ENABLE_CMAKE_LOGGING})
			message(CHECK_PASS "Found. Using system FreeType2 package.")
		endif()
	endif()
	if (NOT TARGET Freetype::Freetype)
		message(FATAL_ERROR "DEngine: CMake target 'Freetype::Freetype' was not found.")
	endif()
	target_link_libraries(${TARGET} PUBLIC Freetype::Freetype)



	# Texas
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/Texas" Texas)
	if (NOT TARGET Texas)
		message(FATAL_ERROR "DEngine: CMake target 'Texas' was not found.")
	endif()
	target_link_libraries(${TARGET} PUBLIC Texas)



	# Box2D
	set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
	set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)
	set(BOX2D_BUILD_DOCS OFF CACHE BOOL "" FORCE)
	add_subdirectory(external/box2d box2d)
	if (NOT TARGET box2d)
		message(FATAL_ERROR "DEngine: CMake target 'box2d' was not found.")
	endif()
	target_link_libraries(${TARGET} PUBLIC box2d)



	# Vulkan headers
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/Vulkan-Headers" Vulkan-Headers)
	if (NOT TARGET Vulkan-Headers)
		message(FATAL_ERROR "DEngine: CMake target 'Vulkan-Headers' was not found.")
	endif()
	target_link_libraries(${TARGET} PUBLIC Vulkan-Headers)


	# Tracy
	#add_subdirectory(external/Tracy Tracy)
	#target_link_libraries(${DENGINE_ANY_TARGET_NAME} PUBLIC TracyClient)
	target_sources(${TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/Tracy/TracyClient.cpp")
	target_include_directories(${TARGET} PUBLIC "external/Tracy")
	#target_compile_definitions(${TARGET} PUBLIC TRACY_ENABLE)

endfunction()