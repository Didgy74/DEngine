function(LinkDependencies TARGET)

	# FreeType
	find_package(Freetype QUIET)
	if(NOT ${Freetype_FOUND})
		set(CMAKE_DISABLE_FIND_PACKAGE_ZLIB TRUE)
		set(CMAKE_DISABLE_FIND_PACKAGE_BZip2 TRUE)
		set(CMAKE_DISABLE_FIND_PACKAGE_PNG TRUE)
		set(CMAKE_DISABLE_FIND_PACKAGE_HarfBuzz TRUE)
		set(CMAKE_DISABLE_FIND_PACKAGE_BrotliDec TRUE)
		add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/freetype" freetype)
	endif()
	target_link_libraries(${TARGET} PUBLIC freetype)



	# Texas
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/Texas" Texas)
	target_link_libraries(${TARGET} PUBLIC Texas)



	# Box2D
	set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "Don't build box2d unit tests.")
	set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "Don't build box2d testbed.")
	set(BOX2D_BUILD_DOCS OFF CACHE BOOL "Don't build box2d documentation.")
	add_subdirectory(external/box2d box2d)
	target_link_libraries(${TARGET} PUBLIC box2d)



	# Vulkan headers
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/Vulkan-Headers" Vulkan-Headers)
	target_link_libraries(${TARGET} PUBLIC Vulkan-Headers)


	# Tracy
	#add_subdirectory(external/Tracy Tracy)
	#target_link_libraries(${DENGINE_ANY_TARGET_NAME} PUBLIC TracyClient)
	target_sources(${TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/Tracy/TracyClient.cpp")
	target_include_directories(${TARGET} PUBLIC "external/Tracy")
	#target_compile_definitions(${TARGET} PUBLIC TRACY_ENABLE)

endfunction()