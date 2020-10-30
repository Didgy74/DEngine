
# -------------------
# Set DEngine options
# -------------------
# START
	option(DENGINE_ENABLE_ASSERT "Disabling will remove all asserts in engine" ON)
	if (${DENGINE_ENABLE_ASSERT})
		set(DENGINE_DEFINES ${DENGINE_DEFINES} DENGINE_ENABLE_ASSERT)
	endif()

	option(DENGINE_APPLICATION_ENABLE_ASSERT "Asserts inside the application code" ON)
	if (${DENGINE_APPLICATION_ENABLE_ASSERT})
		set(DENGINE_DEFINES ${DENGINE_DEFINES} DENGINE_APPLICATION_ENABLE_ASSERT)
	endif()

	option(DENGINE_CONTAINERS_ENABLE_ASSERT "Asserts inside DEngine containers" ON)	
	if (${DENGINE_CONTAINERS_ENABLE_ASSERT})
		set(DENGINE_DEFINES ${DENGINE_DEFINES} DENGINE_CONTAINERS_ENABLE_ASSERT)
	endif()

	option(DENGINE_GFX_ENABLE_ASSERT "Asserts inside the rendering code" ON)
	if (${DENGINE_GFX_ENABLE_ASSERT})
		set(DENGINE_DEFINES ${DENGINE_DEFINES} DENGINE_GFX_ENABLE_ASSERT)
	endif()

	option(DENGINE_GUI_ENABLE_ASSERT "Asserts inside the GUI toolkit code" ON)
	if (${DENGINE_GFX_ENABLE_ASSERT})
		set(DENGINE_DEFINES ${DENGINE_DEFINES} DENGINE_GUI_ENABLE_ASSERT)
	endif()

	option(DENGINE_MATH_ENABLE_ASSERT "Asserts inside DEngine math" ON)	
	if (${DENGINE_MATH_ENABLE_ASSERT})
		set(DENGINE_DEFINES ${DENGINE_DEFINES} DENGINE_MATH_ENABLE_ASSERT)
	endif()

#
# END
#


	set(DENGINE_INCLUDE_DIRS ${DENGINE_ROOT_DIR}/include)
	file(GLOB_RECURSE DENGINE_HEADER_FILES "${DENGINE_ROOT_DIR}/include/*.hpp")
	set(DENGINE_SRC_FILES 
		${DENGINE_HEADER_FILES}
		${DENGINE_ROOT_DIR}/src/main.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Assert.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Editor/Editor.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/FrameAllocator.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gui/Button.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gui/Context.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gui/DockArea.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gui/Image.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gui/LineEdit.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gui/LineList.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gui/MenuBar.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gui/ScrollArea.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gui/StackLayout.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gui/Text.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Physics2D.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Time.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Utility.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Application/Application.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Gfx.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/DeletionQueue.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/Draw.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/DynamicDispatch.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/Init.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/GuiResourceManager.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/NativeWindowManager.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/ObjectDataManager.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/QueueData.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/TextureManager.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/ViewportManager.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/Vk.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Gfx/Vk/vk_mem_alloc.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Math/Common.cpp
		${DENGINE_ROOT_DIR}/src/DEngine/Math/Vector.cpp
		)