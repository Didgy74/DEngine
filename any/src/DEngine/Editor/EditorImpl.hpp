#pragma once

#include "Editor.hpp"

#include <DEngine/Gui/Context/Context.hpp>
#include <DEngine/Gui/StdWidgets/ButtonGroup.hpp>
#include <DEngine/Gui/StdWidgets/DockArea.hpp>
#include <DEngine/Gui/StdWidgets/Text.hpp>

#include <DEngine/Std/Containers/FnScratchList.hpp>

#include <DEngine/Platform/Platform.hpp>
#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Scene.hpp>

#include "ComponentWidgets.hpp"

#include <vector>

namespace DEngine::Editor {
	class EntityIdList;
	class ComponentList;
	class ViewportWidget;
    class Context;

	class EditorImpl : public Gui::Context::WindowHandler
	{
	public:
        Editor::Context* parent = nullptr;
		Platform::Context* appCtx = nullptr;
		float deltaTime = 0.f;
		Std::Box<Gui::Context> guiCtx;
		Std::FrameAlloc guiTransientAlloc = Std::FrameAlloc::PreAllocate(1024 * 1024).Value();
		Gui::RectCollection guiRectCollection;

		// TODO: This is a temporary measure while we refactor stuff
		Gui::Extent m_mainWindowExtent;

		GuiTextEngineImpl m_textEngine;

		Editor::Theme m_theme;

		std::vector<u32> guiQueuedTextInputData;
		Std::FnScratchList<Editor::Context&, Gui::Context&> queuedGuiEvents;
		template<class Callable>
		void PushQueuedGuiEvent(Callable&& in) requires (!Std::Trait::isRef<Callable>) {
			queuedGuiEvents.Push(Std::Move(in));
		}

		// Override window-handler methods
		void CloseWindow(Gui::WindowID) override;
		void SetCursorType(Gui::WindowID, Gui::CursorType) override;
		void HideSoftInput() override;
		void OpenSoftInput(
			Gui::WindowID windowId,
			Std::Span<char const> inputText,
			u64 selectionStart,
			u64 selectionCount,
			Gui::TextInputType inputFilter) override;
		void UpdateTextInputConnection(
			Gui::WindowID windowId,
			u64 selIndex,
			u64 selCount,
			Std::Span<u32 const> inputText) override;
		void UpdateTextInputConnectionSelection(
			Gui::WindowID windowId,
			u64 selIndex,
			u64 selCount) override;

		std::vector<Gfx::GuiVertex> vertices;
		std::vector<u32> indices;
		std::vector<Gfx::GuiDrawCmd> drawCmds;
		std::vector<Gfx::NativeWindowUpdate> windowUpdates;
		std::vector<u32> utfValues;
		std::vector<Gfx::GlyphRect> textGlyphRects;

		// Per-viewport data emitted by InternalViewportWidget::Render2 during the GUI render
		// pass and consumed afterwards by BuildViewportUpdate. Persisted here (rather than as a
		// render-pass local) so it survives until GetDrawInfo runs.
		EditorGuiAppData_Render_Temp renderResultData;

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
		//Gui::MenuButton* viewMenuButton = nullptr;
		Gui::DockArea* dockArea = nullptr;
		Gui::ButtonGroup* gizmoTypeBtnGroup = nullptr;
		std::vector<ViewportWidget*> viewportWidgetPtrs;
		void SelectEntity(Entity);
		void SelectEntity_MidDispatch(Entity, Gui::Widget::WidgetEvent_DeferredJobQueue&);
		void UnselectEntity();
		[[nodiscard]] Std::Opt<Entity> const& GetSelectedEntity() const { return selectedEntity; }

		[[nodiscard]] Editor::GizmoType GetCurrentGizmoType() const;
	private:
		Std::Opt<Entity> selectedEntity;
	};

	[[nodiscard]] Std::Box<Gui::Widget> SetupUiRoot(EditorImpl& editorImpl);
}

namespace DEngine::Editor {
	class EntityIdList : public Gui::StackLayout {
	public:
		Gui::LineList* entitiesList = nullptr;
		EditorImpl* editorImpl = nullptr;

		explicit EntityIdList(EditorImpl &editorImpl) :
			editorImpl(&editorImpl)
		{
			DENGINE_IMPL_ASSERT(!editorImpl.entityIdList);
			editorImpl.entityIdList = this;

			direction = Direction::Vertical;
			this->getThemeFn = Editor::WidgetThemeAdapters::stackLayout;

			auto topElementLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
			topElementLayout->getThemeFn = Editor::WidgetThemeAdapters::stackLayout;
			this->AddWidget(Std::BoxAdopt( topElementLayout ));

			auto newEntityButton = new Gui::Button;
			topElementLayout->AddWidget(Std::BoxAdopt(newEntityButton));
			newEntityButton->text = "New";
			newEntityButton->colors.normal = Settings::GetColor(Settings::Color::Button_Normal);
			newEntityButton->colors.toggled = Settings::GetColor(Settings::Color::Button_Active);
			newEntityButton->getThemeFn = Editor::WidgetThemeAdapters::button;
			newEntityButton->activateFn = [this](Gui::Button::ActivateParams const& in) {
				auto* appDataPtr = in.appData.Get<Editor::Context>();
				DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
				auto& appData = *appDataPtr;
				auto newId = appData.GetActiveScene().NewEntity();
				this->AddEntityToList(newId);
			};

			auto entityDeleteButton = new Gui::Button;
			topElementLayout->AddWidget(Std::Box{entityDeleteButton});
			entityDeleteButton->text = "Delete";
			entityDeleteButton->colors.normal = Settings::GetColor(Settings::Color::Button_Normal);
			entityDeleteButton->colors.toggled = Settings::GetColor(Settings::Color::Button_Active);
			entityDeleteButton->getThemeFn = Editor::WidgetThemeAdapters::button;
			entityDeleteButton->activateFn = [this](Gui::Button::ActivateParams const& in) {
				auto* appDataPtr = in.appData.Get<Editor::EditorGuiAppData>();
				DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
				auto& editorCtx = appDataPtr->EditorCtx();

				auto selectedEntityOpt = editorCtx.GetSelectedEntity();
				if (!selectedEntityOpt.Has()) {
					return;
				}

				auto selectedEntity = selectedEntityOpt.Value();
				this->RemoveEntityFromList(selectedEntity);
				editorCtx.UnselectEntity();
				editorCtx.GetActiveScene().DeleteEntity(selectedEntity);
			};

			auto deselectButton = new Gui::Button;
			topElementLayout->AddWidget(Std::BoxAdopt(deselectButton));
			deselectButton->text = "Deselect";
			deselectButton->colors.normal = Settings::GetColor(Settings::Color::Button_Normal);
			deselectButton->colors.toggled = Settings::GetColor(Settings::Color::Button_Active);
			deselectButton->getThemeFn = Editor::WidgetThemeAdapters::button;
			deselectButton->activateFn = [](Gui::Button::ActivateParams const& in) {
				auto* appDataPtr = in.appData.Get<Editor::EditorGuiAppData>();
				DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
				auto& editorCtx = appDataPtr->EditorCtx();
				auto selectedEntityOpt = editorCtx.GetSelectedEntity();
				if (!selectedEntityOpt.Has()) {
					return;
				}

				editorCtx.UnselectEntity();
			};

			auto *entityListScrollArea = new Gui::ScrollArea;
			this->AddWidget(Std::BoxAdopt(entityListScrollArea));
			entityListScrollArea->scrollbarInactiveColor = Editor::Settings::GetColor(Settings::Color::Scrollbar_Normal);
			entityListScrollArea->getThemeFn = [](Gui::ScrollArea::GetThemeParamsT const&) {
				return Gui::ScrollArea::Theme { .scrollbarPadding = Editor::Theme::defaultScrollbarPadding, };
			};

			entitiesList = new Gui::LineList;
			entityListScrollArea->child = Std::BoxAdopt(entitiesList);
			entitiesList->getThemeFn = Editor::WidgetThemeAdapters::lineList;
			entitiesList->selectedLineChangedFn =
				[&editorImpl](Gui::LineList &widget, Gui::Widget::WidgetEvent_DeferredJobQueue& jobQueue) {
					if (widget.selectedLine.HasValue()) {
						auto const &lineText = widget.lines[widget.selectedLine.Value()];
						editorImpl.SelectEntity_MidDispatch((Entity)std::atoi(lineText.c_str()), jobQueue);
					} else {
						editorImpl.UnselectEntity();
					}
				};

			auto entities = editorImpl.scene->GetEntities();
			for (uSize i = 0; i < entities.Size(); i += 1) {
				auto entityId = entities[i];
				AddEntityToList(entityId);
				if (editorImpl.GetSelectedEntity().HasValue() &&
					entityId == editorImpl.GetSelectedEntity().Value()) {
					entitiesList->selectedLine = i;
				}
			}
		}

		virtual ~EntityIdList() override {
			DENGINE_IMPL_ASSERT(editorImpl->entityIdList == this);
			editorImpl->entityIdList = nullptr;
			/*
			auto &submenuLine = editorImpl->viewMenuButton->submenu.lines[(int) FileMenuEnum::Entities];
			submenuLine.Get<Gui::MenuButton::Line>().toggled = false;
			*/
		}

		void AddEntityToList(Entity id) {
			entitiesList->lines.push_back(std::to_string((u64) id));
		}

		void RemoveEntityFromList(Entity id) {
			DENGINE_IMPL_ASSERT(entitiesList->selectedLine.HasValue());
			entitiesList->RemoveLine(entitiesList->selectedLine.Value());
		}

		void SelectEntity(Std::Opt<Entity> previousId, Entity newId) {
			// Find new entity and select it.
			Std::Opt<uSize> newLine;
			for (uSize i = 0; i < entitiesList->lines.size(); i += 1) {
				auto const id = std::stol(entitiesList->lines[i]);
				if ((Entity) id == newId) {
					newLine = i;
					break;
				}
			}
			DENGINE_IMPL_ASSERT(newLine.HasValue());
			entitiesList->selectedLine = newLine.Value();
		}

		void UnselectEntity() {
			entitiesList->selectedLine = Std::nullOpt;
		}
	};

	class ComponentList : public Gui::ScrollArea {
	public:
		EditorImpl* editorImpl = nullptr;

		Gui::StackLayout* outerLayout = nullptr;
		MoveWidget* moveWidget = nullptr;
		TransformWidget* transformWidget = nullptr;
		SpriteRenderer2DWidget* spriteRendererWidget = nullptr;
		RigidbodyWidget* box2DWidget = nullptr;

		explicit ComponentList(EditorImpl &inEditorImpl) :
			editorImpl(&inEditorImpl)
		{
			DENGINE_IMPL_ASSERT(!editorImpl->componentList);
			editorImpl->componentList = this;

			this->getThemeFn = Editor::WidgetThemeAdapters::scrollArea;
			scrollbarInactiveColor = Settings::GetColor(Settings::Color::Scrollbar_Normal);

			outerLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Vertical);
			outerLayout->getThemeFn = Editor::WidgetThemeAdapters::stackLayout;
			this->child = Std::BoxAdopt(outerLayout);

			if (editorImpl->GetSelectedEntity().HasValue()) {
				EntitySelected(editorImpl->GetSelectedEntity().Value());
			}
		}

		virtual ~ComponentList() override {
			DENGINE_IMPL_ASSERT(editorImpl->componentList == this);
			editorImpl->componentList = nullptr;

			/*
			auto &submenuLine = editorImpl->viewMenuButton->submenu.lines[(int) FileMenuEnum::Components];
			submenuLine.Get<Gui::MenuButton::Line>().toggled = false;
			*/
		}

		void EntitySelected(Entity id) {
			outerLayout->ClearChildren();

			box2DWidget = new RigidbodyWidget(*editorImpl);
			outerLayout->AddWidget(Std::Box{box2DWidget});

			moveWidget = new MoveWidget(*editorImpl);
			outerLayout->AddWidget(Std::Box{moveWidget});

			transformWidget = new TransformWidget(*editorImpl);
			outerLayout->AddWidget(Std::Box{transformWidget});

			spriteRendererWidget = new SpriteRenderer2DWidget(*editorImpl);
			outerLayout->AddWidget(Std::Box{spriteRendererWidget});
		}

		void EntitySelected_MidDispatch(Entity id, Gui::Widget::WidgetEvent_DeferredJobQueue& jobQueue) {
			outerLayout->ClearChildren();

			auto job = [this](Std::AnyRef customData){

				auto* appDataPtr = customData.Get<Editor::EditorGuiAppData>();
				DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
				auto& editorCtx = appDataPtr->EditorCtx();

				box2DWidget = new RigidbodyWidget(editorCtx.GetImplData());
				outerLayout->AddWidget(Std::BoxAdopt(box2DWidget));

				moveWidget = new MoveWidget(editorCtx.GetImplData());
				outerLayout->AddWidget(Std::BoxAdopt(moveWidget));

				transformWidget = new TransformWidget(editorCtx.GetImplData());
				outerLayout->AddWidget(Std::BoxAdopt(transformWidget));

				spriteRendererWidget = new SpriteRenderer2DWidget(editorCtx.GetImplData());
				outerLayout->AddWidget(Std::Box(spriteRendererWidget));
			};

			jobQueue.PushPostEventJob(Std::Move(job));
		}

		void Tick(Scene const &scene, Entity id) {
			if (auto const* ptr = scene.GetComponent<Transform>(id)) {
				transformWidget->Update(*ptr);
			}
		}
	};
}