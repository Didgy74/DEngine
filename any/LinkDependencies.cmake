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
	if (${ENABLE_CMAKE_LOGGING})
		message(CHECK_START "DEngine: Searching for FreeType2 package.")
	endif()
	find_package(Freetype QUIET)
	if(NOT ${FREETYPE_FOUND})
		if (${ENABLE_CMAKE_LOGGING})
			message(CHECK_FAIL "No system FreeType2 package found. Building from source.")
		endif()

		if (NOT TARGET freetype)
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

			add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/freetype")

			add_library(Freetype::Freetype ALIAS freetype)
		endif()

	else()
		if (${ENABLE_CMAKE_LOGGING})
			message(CHECK_PASS "Found. Using system FreeType2 package.")
		endif()
	endif()
	if (NOT TARGET Freetype::Freetype)
		message(FATAL_ERROR "DEngine: CMake target 'Freetype::Freetype' was not found.")
	endif()
	target_link_libraries(${TARGET} PUBLIC freetype)



	# Texas
	set(TEXAS_BUILD_TESTS OFF)
	FetchContent_Declare(
		texas
		GIT_REPOSITORY https://github.com/Didgy74/Texas.git
		GIT_TAG        development
	)
	FetchContent_MakeAvailable(texas)
	if (NOT TARGET Texas)
		message(FATAL_ERROR "DEngine: CMake target 'Texas' was not found.")
	endif()
	target_link_libraries(${TARGET} PUBLIC Texas)


	# Box2D
	if (NOT TARGET box2d)
		set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
		set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)
		set(BOX2D_BUILD_DOCS OFF CACHE BOOL "" FORCE)
		add_subdirectory(external/box2d)
	endif()
	if (NOT TARGET box2d)
		message(FATAL_ERROR "DEngine: CMake target 'box2d' was not found.")
	endif()
	target_link_libraries(${TARGET} PUBLIC box2d)



	# Vulkan headers
	if (NOT TARGET Vulkan-Headers)
		add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/Vulkan-Headers")
	endif()
	if (NOT TARGET Vulkan-Headers)
		message(FATAL_ERROR "DEngine: CMake target 'Vulkan-Headers' was not found.")
	endif()
	target_link_libraries(${TARGET} PUBLIC Vulkan-Headers)


	#add_compile_definitions(${TARGET} PUBLIC TRACY_ENABLE=1)

	include(FetchContent)
	FetchContent_Declare(
		tracy
		GIT_REPOSITORY https://www.github.com/wolfpld/tracy.git
		GIT_TAG v0.9.1
		FIND_PACKAGE_ARGS NAMES TracyClient
	)
	set(TRACY_CALLSTACK ON)
	FetchContent_MakeAvailable(tracy)
	target_link_libraries(${TARGET} PUBLIC TracyClient)
	target_compile_definitions(${TARGET} PUBLIC DENGINE_TRACY_LINKED)

	# Fetch fmt library from GitHub
	FetchContent_Declare(
		fmtlib
		GIT_REPOSITORY https://github.com/fmtlib/fmt.git
		GIT_TAG        7.1.3
	)
	# Make the fetched content available
	FetchContent_MakeAvailable(fmtlib)
	target_link_libraries(${TARGET} PUBLIC fmt::fmt)

endmacro()