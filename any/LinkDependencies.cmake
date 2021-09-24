# FreeType
find_package(Freetype QUIET)
if(NOT ${Freetype_FOUND})
	set(CMAKE_DISABLE_FIND_PACKAGE_ZLIB TRUE)
	set(CMAKE_DISABLE_FIND_PACKAGE_BZip2 TRUE)
	set(CMAKE_DISABLE_FIND_PACKAGE_PNG TRUE)
	set(CMAKE_DISABLE_FIND_PACKAGE_HarfBuzz TRUE)
	set(CMAKE_DISABLE_FIND_PACKAGE_BrotliDec TRUE)
	add_subdirectory(external/freetype freetype)
endif()
target_link_libraries(${DENGINE_ANY_TARGET_NAME} PUBLIC freetype)



# Texas
add_subdirectory(external/Texas Texas)
target_link_libraries(${DENGINE_ANY_TARGET_NAME} PUBLIC Texas)



# Box2D
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "Don't build box2d unit tests.")
set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "Don't build box2d testbed.")
set(BOX2D_BUILD_DOCS OFF CACHE BOOL "Don't build box2d documentation.")
add_subdirectory(external/box2d box2d)
target_link_libraries(${DENGINE_ANY_TARGET_NAME} PUBLIC box2d)



# Vulkan headers
add_subdirectory(external/Vulkan-Headers Vulkan-Headers)
target_link_libraries(${DENGINE_ANY_TARGET_NAME} PUBLIC Vulkan-Headers)



# Tracy
add_subdirectory(external/Tracy Tracy)
target_link_libraries(${DENGINE_ANY_TARGET_NAME} PUBLIC TracyClient)