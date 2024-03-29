
# Application
set(DENGINE_APPLICATION_SOURCE_FILES

		src/DEngine/Application/Application.cpp

		)


# Assert
set(DENGINE_ASSERT_SOURCE_FILES

		src/DEngine/Assert.cpp

		)


# Editor
set(DENGINE_EDITOR_SOURCE_FILES

		src/DEngine/Editor/AppIntegration.cpp
		src/DEngine/Editor/ComponentWidgets.cpp
		src/DEngine/Editor/Editor.cpp
		src/DEngine/Editor/Joystick.cpp
		src/DEngine/Editor/ViewportWidget.cpp

		)

# Gfx
set(DENGINE_GFX_SOURCE_FILES

	src/DEngine/Gfx/Gfx.cpp
	src/DEngine/Gfx/Vk/DeletionQueue.cpp
	src/DEngine/Gfx/Vk/Draw.cpp
	src/DEngine/Gfx/Vk/Draw_Gui.cpp
	src/DEngine/Gfx/Vk/DynamicDispatch.cpp
	src/DEngine/Gfx/Vk/Init.cpp
	src/DEngine/Gfx/Vk/GizmoManager.cpp
	src/DEngine/Gfx/Vk/GuiResourceManager.cpp
	src/DEngine/Gfx/Vk/NativeWindowManager.cpp
	src/DEngine/Gfx/Vk/ObjectDataManager.cpp
	src/DEngine/Gfx/Vk/QueueData.cpp
	src/DEngine/Gfx/Vk/StagingBufferAlloc.cpp
	src/DEngine/Gfx/Vk/TextureManager.cpp
	src/DEngine/Gfx/Vk/ViewportManager.cpp
	src/DEngine/Gfx/Vk/Vk.cpp
	src/DEngine/Gfx/Vk/vk_mem_alloc.cpp)

# Gui
set(DENGINE_GUI_SOURCE_FILES

		src/DEngine/Gui/Context.cpp
		src/DEngine/Gui/TextManager.cpp

		# Widgets
		src/DEngine/Gui/AnchorArea.cpp
		src/DEngine/Gui/Button.cpp
		src/DEngine/Gui/ButtonGroup.cpp
		src/DEngine/Gui/ButtonSizeBehavior.cpp
		src/DEngine/Gui/CollapsingHeader.cpp
		src/DEngine/Gui/DockArea/DockArea.cpp
		src/DEngine/Gui/DockArea/PointerBehavior.cpp
		src/DEngine/Gui/DockArea/Utilities.cpp
		src/DEngine/Gui/Dropdown.cpp
		src/DEngine/Gui/Grid.cpp
		src/DEngine/Gui/Image.cpp
		src/DEngine/Gui/LineEdit.cpp
		src/DEngine/Gui/LineIntEdit.cpp
		src/DEngine/Gui/LineFloatEdit.cpp
		src/DEngine/Gui/LineList.cpp
		src/DEngine/Gui/Menu.cpp
		src/DEngine/Gui/MenuButton.cpp
		src/DEngine/Gui/ScrollArea.cpp
		src/DEngine/Gui/StackLayout.cpp
		src/DEngine/Gui/Text.cpp

)



# Math
set(DENGINE_MATH_SOURCE_FILES

		src/DEngine/Math/Common.cpp
		src/DEngine/Math/Vector.cpp
		src/DEngine/Math/LinearTransform3D.cpp
)



# Std
set(DENGINE_STD_SOURCE_FILES

		src/DEngine/Std/Allocator.cpp
		src/DEngine/Std/BumpAllocator.cpp
		src/DEngine/Std/Std.cpp
		src/DEngine/Std/Utility.cpp

)

set(DENGINE_ANY_SOURCE_FILES

	src/main.cpp

	src/DEngine/Scene.cpp
	src/DEngine/Time.cpp
	src/DEngine/Physics2D.cpp

	${DENGINE_APPLICATION_SOURCE_FILES}
	${DENGINE_ASSERT_SOURCE_FILES}
	${DENGINE_EDITOR_SOURCE_FILES}
	${DENGINE_GFX_SOURCE_FILES}
	${DENGINE_GUI_SOURCE_FILES}
	${DENGINE_MATH_SOURCE_FILES}
	${DENGINE_STD_SOURCE_FILES}
)





# GUI Playground files
set(DENGINE_GUI_PLAYGROUND_SOURCE_FILES

	src/main_GuiPlayground.cpp

	src/DEngine/Scene.cpp
	src/DEngine/Time.cpp
	src/DEngine/Physics2D.cpp

	src/DEngine/Gui/Context.cpp
	src/DEngine/Gui/TextManager.cpp
	src/DEngine/Gui/Button.cpp
	src/DEngine/Gui/ButtonGroup.cpp
	src/DEngine/Gui/StackLayout.cpp
	src/DEngine/Gui/Text.cpp
	src/DEngine/Gui/LineEdit.cpp
	src/DEngine/Gui/LineList.cpp
	src/DEngine/Gui/CollapsingHeader.cpp
	src/DEngine/Gui/ScrollArea.cpp

	${DENGINE_APPLICATION_SOURCE_FILES}
	${DENGINE_ASSERT_SOURCE_FILES}
	${DENGINE_GFX_SOURCE_FILES}
	${DENGINE_MATH_SOURCE_FILES}
	${DENGINE_STD_SOURCE_FILES}
)