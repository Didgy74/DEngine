#pragma once

#include <DEngine/Gui/ButtonGroup.hpp>
#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/DockArea.hpp>
#include <DEngine/Gui/MenuButton.hpp>
#include <DEngine/Gui/Text.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Math/Vector.hpp>
#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/FnRef.hpp>
#include <DEngine/Std/Containers/Variant.hpp>

#include <DEngine/Application.hpp>
#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Scene.hpp>

#include <vector>

namespace DEngine::Editor
{
	enum class GizmoType : u8 { Translate, Rotate, Scale, COUNT };
	class EntityIdList;
	class ComponentList;
	class ViewportWidget;

	class EditorImpl : public App::EventForwarder, public Gui::WindowHandler
	{
	public:
		App::Context* appCtx = nullptr;
		Std::Box<Gui::Context> guiCtx;
		Std::FrameAlloc guiTransientAlloc = Std::FrameAlloc::PreAllocate(1024 * 1024).Value();
		Gui::RectCollection guiRectCollection;

		void FlushQueuedEventsToGui();

		// Override app-interface methods
		virtual void ButtonEvent(
			App::WindowID windowId,
			App::Button button,
			bool state) override;
		virtual void CursorMove(
			App::Context& appCtx,
			App::WindowID windowId,
			Math::Vec2Int position,
			Math::Vec2Int positionDelta) override;

		virtual void TextInputEvent(
			App::Context& ctx,
			App::WindowID windowId,
			uSize oldIndex,
			uSize oldCount,
			Std::Span<u32 const> newString) override;
		virtual void EndTextInputSessionEvent(
			App::Context& ctx,
			App::WindowID windowId) override;

		virtual void TouchEvent(
			App::WindowID windowId,
			u8 id,
			App::TouchEventType type,
			Math::Vec2 position) override;
		virtual bool WindowCloseSignal(
			App::Context& appCtx,
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
			App::Context& appCtx,
			App::WindowID window,
			App::Extent extent,
			Math::Vec2UInt visibleOffset,
			App::Extent visibleExtent) override;

		std::vector<Std::FnRef<void(EditorImpl&, Gui::Context&)>> queuedGuiEvents;
		Std::FrameAlloc queuedGuiEvents_InnerBuffer = Std::FrameAlloc::PreAllocate(1024).Get();
		std::vector<u32> guiQueuedTextInputData;
		template<class Callable>
		void PushQueuedGuiEvent(Callable const& in) {
			auto* temp = (Callable*)queuedGuiEvents_InnerBuffer.Alloc(
				sizeof(Callable),
				alignof(Callable));
			new(temp) Callable(in);
			queuedGuiEvents.push_back({ *temp });
		}


		// Override window-handler methods
		virtual void CloseWindow(Gui::WindowID) override;
		virtual void SetCursorType(Gui::WindowID, Gui::CursorType) override;
		virtual void HideSoftInput() override;
		virtual void OpenSoftInput(Std::Span<char const>, Gui::SoftInputFilter) override;

		std::vector<Gfx::GuiVertex> vertices;
		std::vector<u32> indices;
		std::vector<Gfx::GuiDrawCmd> drawCmds;
		std::vector<Gfx::NativeWindowUpdate> windowUpdates;
		std::vector<u32> utfValues;
		std::vector<Gfx::GlyphRect> textGlyphRects;

		// App-specific stuff
		void InvalidateRendering() { guiRenderingInvalidated = true; }
		bool RenderIsInvalidated() const { return guiRenderingInvalidated; }
		bool guiRenderingInvalidated = true;
		Gfx::Context* gfxCtx = nullptr;

		Scene* scene = nullptr;
		Std::Box<Scene> tempScene;
		Scene& GetActiveScene();
		void BeginSimulating();
		void StopSimulating();

		Gui::Text* test_fpsText = nullptr;

		EntityIdList* entityIdList = nullptr;
		ComponentList* componentList = nullptr;
		Gui::MenuButton* viewMenuButton = nullptr;
		Gui::DockArea* dockArea = nullptr;
		Gui::ButtonGroup* gizmoTypeBtnGroup = nullptr;
		std::vector<ViewportWidget*> viewportWidgetPtrs;
		void SelectEntity(Entity id);
		void SelectEntity_MidDispatch(Entity id, Gui::Context& ctx);
		void UnselectEntity();
		[[nodiscard]] Std::Opt<Entity> const& GetSelectedEntity() const { return selectedEntity; }

		[[nodiscard]] GizmoType GetCurrentGizmoType() const;
	private:
		Std::Opt<Entity> selectedEntity;
	};
}