#pragma once

#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/Text.hpp>
#include <DEngine/Gui/DockArea.hpp>
#include <DEngine/Gui/ButtonGroup.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Math/Vector.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Application.hpp>
#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Scene.hpp>

#include <vector>

namespace DEngine::Editor
{
	namespace impl
	{
		struct GuiEvent
		{
			enum class Type
			{
				CharEnterEvent,
				CharEvent,
				CharRemoveEvent,
				CursorClickEvent,
				CursorMoveEvent,
				TouchEvent,	
				WindowCloseEvent,
				WindowCursorEnterEvent,
				WindowMinimizeEvent,
				WindowMoveEvent,
				WindowResizeEvent,
			};
			Type type;

			union
			{
				Gui::CharEnterEvent charEnter;
				Gui::CharEvent charEvent;
				Gui::CharRemoveEvent charRemove;
				Gui::CursorClickEvent cursorClick;
				Gui::CursorMoveEvent cursorMove;
				Gui::TouchEvent touch;
				Gui::WindowCloseEvent windowClose;
				Gui::WindowCursorEnterEvent windowCursorEnter;
				Gui::WindowMinimizeEvent windowMinimize;
				Gui::WindowMoveEvent windowMove;
				Gui::WindowResizeEvent windowResize;
			};
		};
	}

	enum class GizmoType : u8 { Translate, Rotate, Scale, COUNT };
	class EntityIdList;
	class ComponentList;
	class InternalViewportWidget;

	class EditorImpl : public App::EventInterface, public Gui::WindowHandler
	{
	public:
		Std::Box<Gui::Context> guiCtx;

		// Override app-interface methods
		virtual void ButtonEvent(
			App::Button button,
			bool state) override;
		virtual void CharEnterEvent() override;
		virtual void CharEvent(u32 utfValue) override;
		virtual void CharRemoveEvent() override;
		virtual void CursorMove(
			Math::Vec2Int position,
			Math::Vec2Int positionDelta) override;
		virtual void TouchEvent(
			u8 id,
			App::TouchEventType type,
			Math::Vec2 position) override;
		virtual void WindowClose(
			App::WindowID window) override;
		virtual void WindowCursorEnter(
			App::WindowID window,
			bool entered) override;
		virtual void WindowMinimize(
			App::WindowID window,
			bool wasMinimized) override;
		virtual void WindowMove(
			App::WindowID window,
			Math::Vec2Int position) override;
		virtual void WindowResize(
			App::WindowID window,
			App::Extent extent,
			Math::Vec2Int visiblePos,
			App::Extent visibleExtent) override;
		
		
		std::vector<impl::GuiEvent> queuedGuiEvents;

		// Override window-handler methods
		virtual void CloseWindow(Gui::WindowID) override;
		virtual void SetCursorType(Gui::WindowID, Gui::CursorType) override;
		virtual void HideSoftInput() override;
		virtual void OpenSoftInput(Std::Str, Gui::SoftInputFilter) override;


		std::vector<Gfx::GuiVertex> vertices;
		std::vector<u32> indices;
		std::vector<Gfx::GuiDrawCmd> drawCmds;
		std::vector<Gfx::NativeWindowUpdate> windowUpdates;

		// App-specific stuff
		void InvalidateRendering() { guiRenderingInvalidated = true; }
		bool RenderIsInvalidated() const { return guiRenderingInvalidated; }
		bool guiRenderingInvalidated = true;
		Gfx::Context* gfxCtx = nullptr;

		Scene* scene = nullptr;
		Std::Box<Scene> tempScene;
		Scene& GetActiveScene();
		Scene const& GetActiveScene() const;
		void BeginSimulating();
		void StopSimulating();

		Gui::Text* test_fpsText = nullptr;
		
		EntityIdList* entityIdList = nullptr;
		ComponentList* componentList = nullptr;
		Gui::DockArea* dockArea = nullptr;
		Gui::ButtonGroup* gizmoTypeBtnGroup = nullptr;
		std::vector<InternalViewportWidget*> viewportWidgets;
		void SelectEntity(Entity id);
		void UnselectEntity();
		[[nodiscard]] Std::Opt<Entity> const& GetSelectedEntity() const { return selectedEntity; }

		[[nodiscard]] GizmoType GetCurrentGizmoType() const;
	private:
		Std::Opt<Entity> selectedEntity;
	};
}