cmake_minimum_required(VERSION 3.10)

set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "Don't build box2d unit tests.")
set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "Don't build box2d testbed.")
set(BOX2D_BUILD_DOCS OFF CACHE BOOL "Don't build box2d documentation.")

project(dengine_any)

find_package(Threads REQUIRED)

add_subdirectory(external/box2d box2d)
add_subdirectory(external/Texas Texas)
add_subdirectory(external/freetype freetype)
add_subdirectory(external/Vulkan-Headers Vulkan-Headers)

file(GLOB_RECURSE DENGINE_HEADERS
	"*.hpp"
	"include")

add_library(dengine_any STATIC
		"src/main.cpp"
		"src/DEngine/Assert.cpp"
		"src/DEngine/Editor/Editor.cpp"
		"src/DEngine/Editor/ComponentWidgets.cpp"
		"src/DEngine/Editor/ViewportWidget.cpp"
		"src/DEngine/FrameAllocator.cpp"
		"src/DEngine/Gui/Button.cpp"
		"src/DEngine/Gui/CollapsingHeader.cpp"
		"src/DEngine/Gui/Context.cpp"
		"src/DEngine/Gui/DockArea.cpp"
		"src/DEngine/Gui/Dropdown.cpp"
		"src/DEngine/Gui/Image.cpp"
		"src/DEngine/Gui/LineEdit.cpp"
		"src/DEngine/Gui/LineFloatEdit.cpp"
		"src/DEngine/Gui/LineList.cpp"
		"src/DEngine/Gui/MenuBar.cpp"
		"src/DEngine/Gui/ScrollArea.cpp"
		"src/DEngine/Gui/StackLayout.cpp"
		"src/DEngine/Gui/Text.cpp"
		"src/DEngine/Physics2D.cpp"
		"src/DEngine/Time.cpp"
		"src/DEngine/Utility.cpp"
		"src/DEngine/Application/Application.cpp"
		"src/DEngine/Gfx/Gfx.cpp"
		"src/DEngine/Gfx/Vk/DeletionQueue.cpp"
		"src/DEngine/Gfx/Vk/Draw.cpp"
		"src/DEngine/Gfx/Vk/DynamicDispatch.cpp"
		"src/DEngine/Gfx/Vk/Init.cpp"
		"src/DEngine/Gfx/Vk/GuiResourceManager.cpp"
		"src/DEngine/Gfx/Vk/NativeWindowManager.cpp"
		"src/DEngine/Gfx/Vk/ObjectDataManager.cpp"
		"src/DEngine/Gfx/Vk/QueueData.cpp"
		"src/DEngine/Gfx/Vk/TextureManager.cpp"
		"src/DEngine/Gfx/Vk/ViewportManager.cpp"
		"src/DEngine/Gfx/Vk/Vk.cpp"
		"src/DEngine/Gfx/Vk/vk_mem_alloc.cpp"
		"src/DEngine/Math/Common.cpp"
		"src/DEngine/Math/Vector.cpp")

target_include_directories(dengine_any
	PUBLIC
		"include")
target_link_libraries(dengine_any
	PUBLIC
		Threads::Threads
		Vulkan-Headers
		freetype
		Texas
		box2d
		${CMAKE_DL_LIBS})
target_sources(dengine_any
	PUBLIC
		${DENGINE_HEADERS})


option(DENGINE_ENABLE_ASSERT "Disabling will remove all asserts in engine" ON)
if (${DENGINE_ENABLE_ASSERT})
	target_compile_definitions(dengine_any PUBLIC DENGINE_ENABLE_ASSERT)
endif()

option(DENGINE_APPLICATION_ENABLE_ASSERT "Asserts inside the application code" ON)
if (${DENGINE_APPLICATION_ENABLE_ASSERT})
	target_compile_definitions(dengine_any PUBLIC DENGINE_APPLICATION_ENABLE_ASSERT)
endif()

option(DENGINE_CONTAINERS_ENABLE_ASSERT "Asserts inside DEngine containers" ON)	
if (${DENGINE_CONTAINERS_ENABLE_ASSERT})
	target_compile_definitions(dengine_any PUBLIC DENGINE_CONTAINERS_ENABLE_ASSERT)
endif()

option(DENGINE_GFX_ENABLE_ASSERT "Asserts inside the rendering code" ON)
if (${DENGINE_GFX_ENABLE_ASSERT})
	target_compile_definitions(dengine_any PUBLIC DENGINE_GFX_ENABLE_ASSERT)
endif()

option(DENGINE_GUI_ENABLE_ASSERT "Asserts inside the GUI toolkit code" ON)
if (${DENGINE_GFX_ENABLE_ASSERT})
	target_compile_definitions(dengine_any PUBLIC DENGINE_GUI_ENABLE_ASSERT)
endif()

option(DENGINE_MATH_ENABLE_ASSERT "Asserts inside DEngine math" ON)	
if (${DENGINE_MATH_ENABLE_ASSERT})
	target_compile_definitions(dengine_any PUBLIC DENGINE_MATH_ENABLE_ASSERT)
endif()