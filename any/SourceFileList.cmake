# Header files
set(DENGINE_HEADER_INCLUDES
	GfxGuiDrawEngineImpl.hpp
	GuiPlaneComponentSystem.hpp)

# Editor
set(DENGINE_EDITOR_SOURCE_FILES

		src/DEngine/Editor/AppIntegration.cpp
		src/DEngine/Editor/ComponentWidgets.cpp
		src/DEngine/Editor/Editor.cpp
		src/DEngine/Editor/EditorUi.cpp
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
	src/DEngine/Gfx/Vk/RaiiHandles.cpp
	src/DEngine/Gfx/Vk/StagingBufferAlloc.cpp
	src/DEngine/Gfx/Vk/TextureManager.cpp
	src/DEngine/Gfx/Vk/ViewportManager.cpp
	src/DEngine/Gfx/Vk/Vk.cpp

	src/DEngine/Gfx/Vk/VmaImpl.cpp)

set(DENGINE_ANY_SOURCE_FILES

	src/main.cpp

	src/DEngine/Scene.cpp
	src/DEngine/Time.cpp
	src/DEngine/Physics2D.cpp

	src/DEngine/PlatformGuiGlue.cpp
	src/DEngine/GuiTextEngineImpl.cpp
	src/DEngine/GuiPlaneComponentSystem.cpp

	${DENGINE_ASSERT_SOURCE_FILES}
	${DENGINE_EDITOR_SOURCE_FILES}
	${DENGINE_GFX_SOURCE_FILES}
	${DENGINE_MATH_SOURCE_FILES}
)


# GUI Playground files
set(DENGINE_GUI_PLAYGROUND_SOURCE_FILES

	src/main_GuiPlayground.cpp

	src/DEngine/Time.cpp

	src/DEngine/PlatformGuiGlue.cpp
	src/DEngine/GuiTextEngineImpl.cpp

	${DENGINE_ASSERT_SOURCE_FILES}
	${DENGINE_GFX_SOURCE_FILES}
)