#pragma once

#include "Editor.hpp"

#include <DEngine/Gui/ButtonGroup.hpp>
#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/DockArea.hpp>
#include <DEngine/Gui/MenuButton.hpp>
#include <DEngine/Gui/Text.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Math/Vector.hpp>
#include <DEngine/Std/Containers/AnyRef.hpp>
#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/FnRef.hpp>
#include <DEngine/Std/Containers/Variant.hpp>
#include <DEngine/Std/Containers/FnScratchList.hpp>

#include <DEngine/Application.hpp>
#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Scene.hpp>

#include <vector>

namespace DEngine::Editor
{
	class EntityIdList;
	class ComponentList;
	class ViewportWidget;
    class Context;

	class EditorImpl : public Gui::WindowHandler
	{
	public:
        Editor::Context* parent = nullptr;
		App::Context* appCtx = nullptr;
		float deltaTime = 0.f;
		Std::Box<Gui::Context> guiCtx;
		Std::FrameAlloc guiTransientAlloc = Std::FrameAlloc::PreAllocate(1024 * 1024).Value();
		Gui::RectCollection guiRectCollection;

		std::vector<u32> guiQueuedTextInputData;
		Std::FnScratchList<Editor::Context&, Gui::Context&> queuedGuiEvents;
		template<class Callable>
		void PushQueuedGuiEvent(Callable&& in) requires (!Std::Trait::isRef<Callable>) {
			queuedGuiEvents.Push(Std::Move(in));
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

		[[nodiscard]] Editor::GizmoType GetCurrentGizmoType() const;
	private:
		Std::Opt<Entity> selectedEntity;
	};
}