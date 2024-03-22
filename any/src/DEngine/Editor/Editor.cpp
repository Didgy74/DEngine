#include "Editor.hpp"
#include "EditorImpl.hpp"

#include "ViewportWidget.hpp"
#include "ComponentWidgets.hpp"

#include <DEngine/Gui/LineList.hpp>
#include <DEngine/Gui/ScrollArea.hpp>

#include <DEngine/Time.hpp>

#include <vector>
#include <string>
#include <fmt/format.h>

using namespace DEngine;
using namespace DEngine::Editor;

Std::Array<Math::Vec4, (int)Settings::Color::COUNT> Settings::colorArray = Settings::BuildColorArray();

Gui::TextManager& Editor::Context::GetTextManager() {
	auto& implData = GetImplData();
	return implData.guiCtx->GetTextManager();
}

namespace DEngine::Editor {
	enum class FileMenuEnum {
		Entities,
		Components,
		NewViewport,
		COUNT
	};

	class EntityIdList : public Gui::StackLayout
	{
	public:
		Gui::LineList* entitiesList = nullptr;
		EditorImpl* editorImpl = nullptr;

		explicit EntityIdList(EditorImpl& editorImpl) :
			editorImpl(&editorImpl)
		{
			DENGINE_IMPL_ASSERT(!editorImpl.entityIdList);
			editorImpl.entityIdList = this;

			direction = Direction::Vertical;

			auto topElementLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
			this->AddWidget(Std::Box{ topElementLayout });

			auto newEntityButton = new Gui::Button;
			topElementLayout->AddWidget(Std::Box{ newEntityButton });
			newEntityButton->text = "New";
			newEntityButton->colors.normal =  Settings::GetColor(Settings::Color::Button_Normal);
			newEntityButton->colors.toggled = Settings::GetColor(Settings::Color::Button_Active);
			newEntityButton->textMargin = Editor::Settings::defaultTextMargin;
			newEntityButton->activateFn = [this](Gui::Button& btn, Std::AnyRef customData){
                auto* appDataPtr = customData.Get<Editor::Context>();
                DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
                auto& appData = *appDataPtr;

                auto newId = appData.GetActiveScene().NewEntity();
				this->AddEntityToList(newId);
			};

			auto entityDeleteButton = new Gui::Button;
			topElementLayout->AddWidget(Std::Box{ entityDeleteButton });
			entityDeleteButton->text = "Delete";
			entityDeleteButton->colors.normal = Settings::GetColor(Settings::Color::Button_Normal);
			entityDeleteButton->colors.toggled = Settings::GetColor(Settings::Color::Button_Active);
			entityDeleteButton->textMargin = Editor::Settings::defaultTextMargin;
			entityDeleteButton->activateFn = [this](Gui::Button& btn, Std::AnyRef customData){
                auto* appDataPtr = customData.Get<Editor::Context>();
                DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
                auto& appData = *appDataPtr;

                auto selectedEntityOpt = appData.GetSelectedEntity();
				if (!selectedEntityOpt.Has())
					return;

				auto selectedEntity = selectedEntityOpt.Value();
				this->RemoveEntityFromList(selectedEntity);
				appData.UnselectEntity();
				appData.GetActiveScene().DeleteEntity(selectedEntity);
			};

			auto deselectButton = new Gui::Button;
			topElementLayout->AddWidget(Std::Box{ deselectButton });
			deselectButton->text = "Deselect";
			deselectButton->colors.normal = Settings::GetColor(Settings::Color::Button_Normal);
			deselectButton->colors.toggled = Settings::GetColor(Settings::Color::Button_Active);
			deselectButton->textMargin = Editor::Settings::defaultTextMargin;
			deselectButton->activateFn = [](Gui::Button& btn, Std::AnyRef customData) {
                auto* appDataPtr = customData.Get<Editor::Context>();
                DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
                auto& appData = *appDataPtr;

                auto selectedEntityOpt = appData.GetSelectedEntity();
                if (!selectedEntityOpt.Has())
                    return;

				appData.UnselectEntity();
			};

			auto entityListScrollArea = new Gui::ScrollArea;
			this->AddWidget(Std::Box{ entityListScrollArea });
			entityListScrollArea->scrollbarInactiveColor = Settings::GetColor(Settings::Color::Scrollbar_Normal);

			entitiesList = new Gui::LineList;
			entityListScrollArea->child = Std::Box{ entitiesList };
			entitiesList->selectedLineChangedFn =
				[&editorImpl](Gui::LineList& widget, Gui::Context* ctx) {
					if (widget.selectedLine.HasValue()) {
						auto const& lineText = widget.lines[widget.selectedLine.Value()];
						editorImpl.SelectEntity_MidDispatch((Entity)std::atoi(lineText.c_str()), *ctx);
					} else {
						editorImpl.UnselectEntity();
					}
				};

			auto entities = editorImpl.scene->GetEntities();
			for (uSize i = 0; i < entities.Size(); i += 1) {
				auto entityId = entities[i];
				AddEntityToList(entityId);
				if (editorImpl.GetSelectedEntity().HasValue() && entityId == editorImpl.GetSelectedEntity().Value())
				{
					entitiesList->selectedLine = i;
				}
			}
		}

		virtual ~EntityIdList() override
		{
			DENGINE_IMPL_ASSERT(editorImpl->entityIdList == this);
			editorImpl->entityIdList = nullptr;

			auto& submenuLine = editorImpl->viewMenuButton->submenu.lines[(int)FileMenuEnum::Entities];
			submenuLine.Get<Gui::MenuButton::Line>().toggled = false;
		}

		void AddEntityToList(Entity id) {
			entitiesList->lines.push_back(std::to_string((u64)id));
		}

		void RemoveEntityFromList(Entity id)
		{
			DENGINE_IMPL_ASSERT(entitiesList->selectedLine.HasValue());
			entitiesList->RemoveLine(entitiesList->selectedLine.Value());
		}

		void SelectEntity(Std::Opt<Entity> previousId, Entity newId)
		{
			// Find new entity and select it.
			Std::Opt<uSize> newLine;
			for (uSize i = 0; i < entitiesList->lines.size(); i += 1) {
				auto const id = std::stol(entitiesList->lines[i]);
				if ((Entity)id == newId)
				{
					newLine = i;
					break;
				}
			}
			DENGINE_IMPL_ASSERT(newLine.HasValue());
			entitiesList->selectedLine = newLine.Value();
		}

		void UnselectEntity()
		{
			entitiesList->selectedLine = Std::nullOpt;
		}
	};

	class ComponentList : public Gui::ScrollArea
	{
	public:
		EditorImpl* editorImpl = nullptr;

		Gui::StackLayout* outerLayout = nullptr;
		MoveWidget* moveWidget = nullptr;
		TransformWidget* transformWidget = nullptr;
		SpriteRenderer2DWidget* spriteRendererWidget = nullptr;
		RigidbodyWidget* box2DWidget = nullptr;

		explicit ComponentList(EditorImpl& inEditorImpl) :
			editorImpl(&inEditorImpl)
		{
			DENGINE_IMPL_ASSERT(!editorImpl->componentList);
			editorImpl->componentList = this;

			scrollbarInactiveColor = Settings::GetColor(Settings::Color::Scrollbar_Normal);

			outerLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Vertical);
			this->child = Std::Box<Gui::Widget>{ outerLayout };
			//outerLayout->expandNonDirection = true;

			if (editorImpl->GetSelectedEntity().HasValue()) {
				EntitySelected(editorImpl->GetSelectedEntity().Value());
			}
		}

		virtual ~ComponentList() override
		{
			DENGINE_IMPL_ASSERT(editorImpl->componentList == this);
			editorImpl->componentList = nullptr;

			auto& submenuLine = editorImpl->viewMenuButton->submenu.lines[(int)FileMenuEnum::Components];
			submenuLine.Get<Gui::MenuButton::Line>().toggled = false;
		}

		void EntitySelected(Entity id)
		{
			outerLayout->ClearChildren();

			box2DWidget = new RigidbodyWidget(*editorImpl);
			outerLayout->AddWidget(Std::Box{ box2DWidget });

			moveWidget = new MoveWidget(*editorImpl);
			outerLayout->AddWidget(Std::Box{ moveWidget });

			transformWidget = new TransformWidget(*editorImpl);
			outerLayout->AddWidget(Std::Box{ transformWidget });

			spriteRendererWidget = new SpriteRenderer2DWidget(*editorImpl);
			outerLayout->AddWidget(Std::Box{ spriteRendererWidget });
		}

		void EntitySelected_MidDispatch(Entity id, Gui::Context& ctx)
		{
			outerLayout->ClearChildren();

			auto job = [this](Gui::Context& ctx, Std::AnyRef customData) {
                auto* appDataPtr = customData.Get<Editor::Context>();
                DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
                auto& appData = *appDataPtr;

				box2DWidget = new RigidbodyWidget(appData.GetImplData());
				outerLayout->AddWidget(Std::Box{ box2DWidget });

				moveWidget = new MoveWidget(appData.GetImplData());
				outerLayout->AddWidget(Std::Box{ moveWidget });

				transformWidget = new TransformWidget(appData.GetImplData());
				outerLayout->AddWidget(Std::Box{ transformWidget });

				spriteRendererWidget = new SpriteRenderer2DWidget(appData.GetImplData());
				outerLayout->AddWidget(Std::Box{ spriteRendererWidget });
			};

			ctx.PushPostEventJob(job);
		}

		void Tick(Scene const& scene, Entity id)
		{
			if (auto ptr = scene.GetComponent<Transform>(id))
				transformWidget->Update(*ptr);
		}
	};
}

using namespace DEngine;

namespace DEngine::Editor
{
	[[nodiscard]] static Std::Box<Gui::Widget> CreateNavigationBar(
		EditorImpl& editorImpl,
		Gui::Context& guiCtx)
	{
		auto stackLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
		stackLayout->spacing = Editor::Settings::defaultTextMargin;

		auto menuButton = new Gui::MenuButton;
		editorImpl.viewMenuButton = menuButton;
		menuButton->colors.normal = Settings::GetColor(Settings::Color::Button_Normal);
		menuButton->spacing = Editor::Settings::defaultTextMargin;
		stackLayout->AddWidget(Std::Box{ menuButton });
		//menuButton->submenu.lines.resize((int)FileMenuEnum::COUNT);
		menuButton->title = "Menu";
		{
			// Create button for Entities window
			Gui::MenuButton::Line line {};
			line.title = "Entities";
			line.toggled = true;
			line.togglable = true;
			line.callback = [](
                Gui::MenuButton::Line& line,
                Gui::Context& ctx)
            {
                if (line.toggled) {
                    auto job = [](
                        Gui::Context& ctx,
                        Std::AnyRef customData)
                    {
                        auto* appDataPtr = customData.Get<Editor::Context>();
                        DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
                        auto& appData = *appDataPtr;

                        appData.GetImplData().dockArea->AddWindow(
                            "Entities",
                            Settings::GetColor(Settings::Color::Window_Entities),
                            Std::Box{ new EntityIdList(appData.GetImplData()) });
                    };
                    ctx.PushPostEventJob(job);
                }
                line.toggled = true;
            };
			//menuButton->submenu.lines[(int)FileMenuEnum::Entities] = Std::Move(line);
			menuButton->submenu.lines.emplace_back(Std::Move(line));
		}
		{
			// Create button for Components window
			Gui::MenuButton::Line line {};
			line.title = "Components";
			line.toggled = true;
			line.togglable = true;
			line.callback = [](
                Gui::MenuButton::Line& line,
                Gui::Context& ctx)
            {
                if (line.toggled) {
                    auto job = [](Gui::Context& ctx, Std::AnyRef customData) {
                        auto* appDataPtr = customData.Get<Editor::Context>();
                        DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
                        auto& appData = *appDataPtr;

                        appData.GetImplData().dockArea->AddWindow(
                            "Components",
                            Settings::GetColor(Settings::Color::Window_Components),
                            Std::Box{ new ComponentList(appData.GetImplData()) });
                    };
                    ctx.PushPostEventJob(job);
                }
                line.toggled = true;
            };
			//menuButton->submenu.lines[(int)FileMenuEnum::Components] = Std::Move(line);
			menuButton->submenu.lines.emplace_back(Std::Move(line));
		}
		{
			// Create button for adding viewport
			Gui::MenuButton::Line line {};
			line.title = "New viewport";
			line.toggled = false;
			line.togglable = false;
			line.callback = [](Gui::MenuButton::Line& line, Gui::Context& ctx)
            {
				auto job =
                    [](Gui::Context& ctx, Std::AnyRef customData) {
                        auto* appDataPtr = customData.Get<Editor::Context>();
                        DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
                        auto& appData = *appDataPtr;
                        appData.GetImplData().dockArea->AddWindow(
                            "Viewport",
                            Settings::GetColor(Settings::Color::Window_Viewport),
                            Std::Box{ new ViewportWidget(appData.GetImplData()) });
				    };
				ctx.PushPostEventJob(job);
			};
			//menuButton->submenu.lines[(int)FileMenuEnum::NewViewport] = Std::Move(line);
			menuButton->submenu.lines.emplace_back(Std::Move(line));
		}
		{
			auto* layout = new Gui::StackLayout(Gui::StackLayout::Dir::Horizontal);
			Gui::MenuButton::LineAny line = { .widget = Std::Box{ layout } };
			menuButton->submenu.lines.emplace_back(Std::Move(line));
			layout->spacing = Settings::defaultTextMargin;
			{
				auto* fontLabel = new Gui::Text();
				fontLabel->text = "Font";
				layout->AddWidget( Std::Box{ fontLabel });
			}
			{
				auto* incrementBtn = new Gui::Button();
				incrementBtn->text = "+";
				incrementBtn->textMargin = Settings::defaultTextMargin;
				incrementBtn->activateFn = [&guiCtx](Gui::Button& btn, Std::AnyRef customData) {
                    guiCtx.fontScale += 0.1f;
                };
				layout->AddWidget(Std::Box{ incrementBtn });
			}
			{
				auto* decrementBtn = new Gui::Button();
				decrementBtn->text = "-";
				decrementBtn->textMargin = Settings::defaultTextMargin;
				decrementBtn->activateFn = [&guiCtx](Gui::Button& btn, Std::AnyRef customData) {
                    guiCtx.fontScale -= 0.1f;
                };
				layout->AddWidget(Std::Box{ decrementBtn });
			}
			{
				auto* label = new Gui::Text();
				label->getText = [](Gui::Context const& ctx, auto const&, Gui::Text::TextPusher& textPusher) {
					fmt::format_to(textPusher.BackInserter(), "{:.1f}", ctx.fontScale);
				};
				layout->AddWidget(Std::Box{ label });
			}
		}
		{
			auto* layout = new Gui::StackLayout(Gui::StackLayout::Dir::Horizontal);
			Gui::MenuButton::LineAny line = { .widget = Std::Box{ layout } };
			menuButton->submenu.lines.emplace_back(Std::Move(line));
			layout->spacing = Settings::defaultTextMargin;
			{
				auto* fontLabel = new Gui::Text();
				fontLabel->text = "Min";
				layout->AddWidget( Std::Box{ fontLabel });
			}
			{
				auto* incrementBtn = new Gui::Button();
				incrementBtn->text = "+";
				incrementBtn->textMargin = Settings::defaultTextMargin;
				incrementBtn->activateFn = [&guiCtx](Gui::Button& btn, Std::AnyRef customData) {
					guiCtx.minimumHeightCm += 0.1f;
				};
				layout->AddWidget(Std::Box{ incrementBtn });
			}
			{
				auto* decrementBtn = new Gui::Button();
				decrementBtn->text = "-";
				decrementBtn->textMargin = Settings::defaultTextMargin;
				decrementBtn->activateFn = [&guiCtx](Gui::Button& btn, Std::AnyRef customData) {
					guiCtx.minimumHeightCm -= 0.1f;
					if (guiCtx.minimumHeightCm < Gui::Context::absoluteMinimumSize) {
						guiCtx.minimumHeightCm = Gui::Context::absoluteMinimumSize;
					}
				};
				layout->AddWidget(Std::Box{ decrementBtn });
			}
			{
				auto* label = new Gui::Text();
				label->getText = [](Gui::Context const& ctx, auto const&, Gui::Text::TextPusher& textPusher) {
					fmt::format_to(textPusher.BackInserter(), "{:.1f}", ctx.minimumHeightCm);
				};
				layout->AddWidget(Std::Box{ label });
			}
		}

		// Delta time counter at the top
		auto deltaTimeText = new Gui::Text;
		stackLayout->AddWidget(Std::Box{ deltaTimeText });
		editorImpl.test_fpsText = deltaTimeText;
		deltaTimeText->expandX = false;
		deltaTimeText->getText = [](
			Gui::Context const&,
			Std::ConstAnyRef const& customDataIn,
			Gui::Text::TextPusher& textPusher)
		{
			auto& appData = customDataIn.Get<Editor::Context>()->GetImplData();
			fmt::format_to(
				textPusher.BackInserter(),
				"{:.3f}ms / {} FPS",
				appData.deltaTime * 1000,
				(int)Math::Round(1.f / appData.deltaTime));
		};

		auto playButton = new Gui::Button;
		stackLayout->AddWidget(Std::Box{ playButton });
		playButton->colors.normal = Settings::GetColor(Settings::Color::Button_Normal);
		playButton->colors.toggled = Settings::GetColor(Settings::Color::Button_Active);
		playButton->text = "Play";
		playButton->textMargin = Editor::Settings::defaultTextMargin;
		playButton->type = Gui::Button::Type::Toggle;
		playButton->activateFn = [](Gui::Button& btn, Std::AnyRef customData){
            auto* appDataPtr = customData.Get<Editor::Context>();
            DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
            auto& appData = *appDataPtr;

			if (btn.GetToggled()) {
				appData.BeginSimulatingScene();
			} else {
				appData.StopSimulatingScene();
			}
		};

		auto gizmoBtnGroup = new Gui::ButtonGroup;
		editorImpl.gizmoTypeBtnGroup = gizmoBtnGroup;
		stackLayout->AddWidget(Std::Box{ gizmoBtnGroup });
		gizmoBtnGroup->colors.inactiveColor = Settings::GetColor(Settings::Color::Button_Normal);
		gizmoBtnGroup->colors.activeColor = Settings::GetColor(Settings::Color::Button_Active);
		gizmoBtnGroup->margin = Editor::Settings::defaultTextMargin;
		gizmoBtnGroup->AddButton("Translate");
		gizmoBtnGroup->AddButton("Rotate");
		gizmoBtnGroup->AddButton("Scale");

		return Std::Box{ stackLayout };
	}
}

Editor::Context Editor::Context::Create(
	CreateInfo const& createInfo)
{
	Context newCtx;

	newCtx.m_implData = new EditorImpl;
	EditorImpl& implData = *newCtx.m_implData;

	implData.appCtx = &createInfo.appCtx;
	implData.gfxCtx = &createInfo.gfxCtx;
	implData.scene = &createInfo.scene;

	auto& guiWinHandler = implData;
	auto ctx = new Gui::Context(
		Gui::Context::Create(
			guiWinHandler,
			implData.appCtx,
			implData.gfxCtx));
	//ctx->fontScale = 3.f;u
	implData.guiCtx = Std::Box{ ctx };

	auto outmostLayout = new Gui::StackLayout(Gui::StackLayout::Dir::Vertical);

	outmostLayout->AddWidget(CreateNavigationBar(implData, *implData.guiCtx));

	auto dockArea = new Gui::DockArea;
	implData.dockArea = dockArea;
	outmostLayout->AddWidget(Std::Box{ dockArea });

	dockArea->AddWindow(
		"Entities",
		Settings::GetColor(Settings::Color::Window_Entities),
		Std::Box{ new EntityIdList(implData) });

	dockArea->AddWindow(
		"Components",
		Settings::GetColor(Settings::Color::Window_Components),
		Std::Box{ new ComponentList(implData) });

	dockArea->AddWindow(
		"Viewport",
		Settings::GetColor(Settings::Color::Window_Viewport),
		Std::Box{ new ViewportWidget(implData) });


	Gui::Context::AdoptWindowInfo adoptInfo {};
	adoptInfo.id = (Gui::WindowID)createInfo.mainWindow;
	adoptInfo.contentScale = createInfo.windowContentScale;
	adoptInfo.dpiX = createInfo.windowDpiX;
	adoptInfo.dpiY = createInfo.windowDpiY;
	adoptInfo.clearColor = Settings::GetColor(Settings::Color::Background);
	adoptInfo.rect = { createInfo.windowPos, { createInfo.windowExtent.width, createInfo.windowExtent.height } };
	adoptInfo.visibleExtent = { createInfo.windowSafeAreaExtent.width, createInfo.windowSafeAreaExtent.height };
	adoptInfo.visibleOffset = createInfo.windowSafeAreaOffset;
	adoptInfo.widget = Std::Box{ outmostLayout };
	implData.guiCtx->AdoptWindow(Std::Move(adoptInfo));

	return newCtx;
}

void Editor::Context::ProcessEvents(float deltaTime) {
	auto& implData = GetImplData();



	if (implData.appCtx->TickCount() == 1)
		implData.InvalidateRendering();

	for (auto viewportPtr : implData.viewportWidgetPtrs) {
		viewportPtr->Tick(Time::Delta());
	}
	if (implData.appCtx->TickCount() % 60 == 0) {
		implData.deltaTime = deltaTime;
		implData.InvalidateRendering();
	}
	if (implData.appCtx->TickCount() % 10 == 0) {
		if (implData.componentList && implData.GetSelectedEntity().HasValue()) {
			implData.componentList->Tick(implData.GetActiveScene(), implData.GetSelectedEntity().Value());
			implData.InvalidateRendering();
		}
	}

	auto guiEventsHappened = FlushQueuedEventsToGui();

	if (implData.RenderIsInvalidated())
	//if (true)
	{

		struct Test : public Gui::Widget::AccessibilityInfoPusher {
			std::vector<char> text;
			std::vector<Gui::Widget::AccessibilityInfoElement> elements;
			int PushText(Std::Span<char const> in) override {
				int returnVal = (int)this->text.size();
				this->text.resize(this->text.size() + in.Size());
				std::memcpy(this->text.data() + returnVal, in.Data(), in.Size());
				return returnVal;
			}
			void PushElement(Gui::Widget::AccessibilityInfoElement const& in) override {
				this->elements.push_back(in);
			}
		};
		Test yo;

		implData.guiCtx->Event_Accessibility(
			Std::ConstAnyRef{ *this },
			implData.guiTransientAlloc,
			implData.guiRectCollection,
			yo);

		implData.appCtx->UpdateAccessibilityData(
			(App::WindowID)0,
			{
				(int)yo.elements.size(),
				[&](int i) {
					auto const& in = yo.elements[i];
					Application::AccessibilityUpdateElement out = {};
					out.posX = in.rect.position.x;
					out.posY = in.rect.position.y;
					out.width = (int)in.rect.extent.width;
					out.height = (int)in.rect.extent.height;
					out.textStart = in.textStart;
					out.textCount = in.textCount;
					out.isClickable = in.isClickable;
					return out;
				}
			},
			{ yo.text.data(), yo.text.size() });

		implData.guiRenderingInvalidated = false;

		for (auto viewportPtr : implData.viewportWidgetPtrs)
			viewportPtr->GetInternalViewport().wasRendered = false;

		implData.vertices.clear();
		implData.indices.clear();
		implData.drawCmds.clear();
		implData.windowUpdates.clear();
		implData.utfValues.clear();
		implData.textGlyphRects.clear();

		Gui::Context::Render2_Params renderParams {
			.rectCollection = implData.guiRectCollection,
			.transientAlloc = implData.guiTransientAlloc,
			.vertices = implData.vertices,
			.indices = implData.indices,
			.drawCmds = implData.drawCmds,
			.windowUpdates = implData.windowUpdates,
			.utfValues = implData.utfValues,
			.textGlyphRects = implData.textGlyphRects };
		implData.guiCtx->Render2(renderParams, Std::ConstAnyRef{ *this });


		// We want to see if our viewports were rendered, and update them if they were.
		for (auto viewportPtr : implData.viewportWidgetPtrs) {
			DENGINE_IMPL_ASSERT(viewportPtr);

			auto const& rectColl = implData.guiRectCollection;
			auto& viewport = viewportPtr->GetInternalViewport();

			auto entryOpt = rectColl.GetEntry(viewport);

			if (entryOpt.HasValue()) {
				// This means the viewport was rendered
				auto const& entry = entryOpt.Value();
				auto const& rectPair = rectColl.GetRect(entry);
				auto const visibleIntersection = Gui::Intersection(rectPair.widgetRect, rectPair.visibleRect);
				if (!visibleIntersection.IsNothing()) {
					auto const& currentRect = rectPair.widgetRect;
					viewport.wasRendered = true;
					viewport.currentlyResizing = viewport.newExtent != currentRect.extent;
					viewport.newExtent = currentRect.extent;

					if (viewport.currentExtent == Gui::Extent{} && viewport.newExtent != Gui::Extent{})
						viewport.currentExtent = viewport.newExtent;
				}
			}
		}
	}
}

Editor::Context::Context(Context&& other) noexcept :
	m_implData{ other.m_implData }
{
	other.m_implData = nullptr;
}

Editor::Context::~Context()
{
	if (this->m_implData)
	{
		auto& implData = GetImplData();
		delete &implData;
	}
}

Editor::DrawInfo Editor::Context::GetDrawInfo() const
{
	auto& implData = this->GetImplData();

	DrawInfo returnVal;
	
	returnVal.vertices = implData.vertices;
	returnVal.indices = implData.indices;
	returnVal.drawCmds = implData.drawCmds;
	returnVal.textGlyphRects = implData.textGlyphRects;
	returnVal.utfValues = implData.utfValues;

	returnVal.windowUpdates = implData.windowUpdates;

	for (auto viewportWidgetPtr : implData.viewportWidgetPtrs) {
		DENGINE_IMPL_ASSERT(viewportWidgetPtr);
		auto const& viewport = viewportWidgetPtr->GetInternalViewport();
		if (viewport.wasRendered && !viewport.currentExtent.IsNothing()) {
			Gfx::ViewportUpdate update = viewport.BuildViewportUpdate(
				returnVal.lineVertices,
				returnVal.lineDrawCmds);
			
			returnVal.viewportUpdates.push_back(update);
		}
	}

	return returnVal;
}

bool Editor::Context::IsSimulating() const
{
	auto& implData = GetImplData();
	return implData.tempScene.Get();
}

Scene& Editor::Context::GetActiveScene()
{
	auto& implData = this->GetImplData();
	return implData.GetActiveScene();
}

Std::Opt<Entity> const& Context::GetSelectedEntity() const {
    return GetImplData().GetSelectedEntity();
}

void Context::UnselectEntity() {
    return GetImplData().UnselectEntity();
}

void Context::BeginSimulatingScene() {
    GetImplData().BeginSimulating();
}

void Context::StopSimulatingScene() {
    GetImplData().StopSimulating();
}

GizmoType Context::GetCurrentGizmoType() const {
    return GetImplData().GetCurrentGizmoType();
}

void Context::SelectEntity(Entity id) {
    GetImplData().SelectEntity(id);
}

void Editor::EditorImpl::SelectEntity(Entity id)
{
	if (selectedEntity.HasValue() && selectedEntity.Value() == id)
		return;

	Std::Opt<Entity> prevEntity = selectedEntity;
	selectedEntity = id;

	// Update the entity list
	if (entityIdList)
		entityIdList->SelectEntity(prevEntity, id);

	// Update the component list
	if (componentList)
		componentList->EntitySelected(id);
}

void EditorImpl::SelectEntity_MidDispatch(Entity id, Gui::Context& ctx)
{
	if (selectedEntity.HasValue() && selectedEntity.Value() == id)
		return;

	Std::Opt<Entity> prevEntity = selectedEntity;
	selectedEntity = id;

	// Update the component list
	if (componentList)
		componentList->EntitySelected_MidDispatch(id, ctx);
}

void Editor::EditorImpl::UnselectEntity()
{
	// Update the entity list
	if (selectedEntity.HasValue() && entityIdList)
		entityIdList->UnselectEntity();

	// Clear the component list
	if (componentList)
		componentList->outerLayout->ClearChildren();

	selectedEntity = Std::nullOpt;
}

Editor::GizmoType Editor::EditorImpl::GetCurrentGizmoType() const
{
	DENGINE_IMPL_ASSERT(gizmoTypeBtnGroup->GetButtonCount() == (u8)GizmoType::COUNT);

	return (GizmoType)gizmoTypeBtnGroup->GetActiveButtonIndex();
}

Scene& Editor::EditorImpl::GetActiveScene()
{
	if (tempScene)
		return *tempScene;
	return *scene;
}

void Editor::EditorImpl::BeginSimulating()
{
	DENGINE_IMPL_ASSERT(!tempScene);

	Scene* copyScene = new Scene;
	tempScene = Std::Box{ copyScene };
	scene->Copy(*copyScene);
	copyScene->Begin();
}

void Editor::EditorImpl::StopSimulating()
{
	DENGINE_IMPL_ASSERT(tempScene);
	tempScene.Clear();
}

std::vector<Math::Vec3> Editor::BuildGizmoArrowMesh3D()
{
	auto const arrow = Gizmo::defaultArrow;

	f32 arrowShaftRadius = arrow.shaftDiameter / 2.f;
	f32 arrowCapRadius = arrow.capDiameter / 2.f;

	u32 subdivisions = 4;
	// We need atleast 2 subdivisons so we can atleast get a diamond
	DENGINE_IMPL_ASSERT(subdivisions > 1);
	u32 baseCircleTriangleCount = (subdivisions * 2);

	std::vector<Math::Vec3> vertices;
	for (u32 i = 0; i < baseCircleTriangleCount; i++) {
		f32 currentRadiansA = 2 * Math::pi / baseCircleTriangleCount * i;
		f32 currentRadiansB = 2 * Math::pi / baseCircleTriangleCount * ((i + 1) % baseCircleTriangleCount);
		
		{
			Math::Vec3 shaftBaseVertA = {};
			shaftBaseVertA.x = Math::Sin(currentRadiansA);
			shaftBaseVertA.x *= arrowShaftRadius;
			shaftBaseVertA.y = Math::Cos(currentRadiansA);
			shaftBaseVertA.y *= arrowShaftRadius;

			Math::Vec3 shaftBaseVertB = {};
			shaftBaseVertB.x = Math::Sin(currentRadiansB);
			shaftBaseVertB.x *= arrowShaftRadius;
			shaftBaseVertB.y = Math::Cos(currentRadiansB);
			shaftBaseVertB.y *= arrowShaftRadius;

			// Build the base circle triangle.
			// We use different winding on this base circle,
			// because this face faces away from the direction of the arrow.
			vertices.push_back(shaftBaseVertB);
			vertices.push_back({});
			vertices.push_back(shaftBaseVertA);
			
			// Build a wall from this base triangle
			Math::Vec3 shaftTopVertA = { shaftBaseVertA.x, shaftBaseVertA.y, shaftBaseVertA.z + arrow.shaftLength };
			Math::Vec3 shaftTopVertB = { shaftBaseVertB.x, shaftBaseVertB.y, shaftBaseVertB.z + arrow.shaftLength };
			vertices.push_back(shaftBaseVertA);
			vertices.push_back(shaftTopVertA);
			vertices.push_back(shaftBaseVertB);

			vertices.push_back(shaftBaseVertB);
			vertices.push_back(shaftTopVertA);
			vertices.push_back(shaftTopVertB);

			// Append walls to build base of cap
			Math::Vec3 capBaseVertA = {};
			capBaseVertA.x = Math::Sin(currentRadiansA);
			capBaseVertA.x *= arrowCapRadius;
			capBaseVertA.y = Math::Cos(currentRadiansA);
			capBaseVertA.y *= arrowCapRadius;
			capBaseVertA.z = arrow.shaftLength;

			Math::Vec3 capBaseVertB = {};
			capBaseVertB.x = Math::Sin(currentRadiansB);
			capBaseVertB.x *= arrowCapRadius;
			capBaseVertB.y = Math::Cos(currentRadiansB);
			capBaseVertB.y *= arrowCapRadius;
			capBaseVertB.z = arrow.shaftLength;

			vertices.push_back(shaftTopVertA);
			vertices.push_back(capBaseVertA);
			vertices.push_back(shaftTopVertB);

			vertices.push_back(shaftTopVertB);
			vertices.push_back(capBaseVertA);
			vertices.push_back(capBaseVertB);

			// Connect cap base to arrow head
			Math::Vec3 arrowHeadMidVert = {};
			arrowHeadMidVert.z = arrow.shaftLength + arrow.capLength;

			vertices.push_back(capBaseVertA);
			vertices.push_back(arrowHeadMidVert);
			vertices.push_back(capBaseVertB);
		}
	}

	return vertices;
}

std::vector<Math::Vec3> Editor::BuildGizmoTranslateArrowMesh2D()
{
	std::vector<Math::Vec3> vertices;

	// Make quad for the base
	constexpr auto arrow = Gizmo::defaultArrow;

	Math::Vec3 bleh = {};

	bleh.x = 0.f;
	bleh.y = arrow.shaftDiameter / 2.f;
	vertices.push_back(bleh);
	bleh.x = 0.f;
	bleh.y = -arrow.shaftDiameter / 2.f;
	vertices.push_back(bleh);
	bleh.x = arrow.shaftLength;
	bleh.y = -arrow.shaftDiameter / 2.f;
	vertices.push_back(bleh);

	bleh.x = 0.f;
	bleh.y = arrow.shaftDiameter / 2.f;
	vertices.push_back(bleh);
	bleh.x = arrow.shaftLength;
	bleh.y = -arrow.shaftDiameter / 2.f;
	vertices.push_back(bleh);
	bleh.x = arrow.shaftLength;
	bleh.y = arrow.shaftDiameter / 2.f;
	vertices.push_back(bleh);

	bleh.x = arrow.shaftLength;
	bleh.y = arrow.capDiameter / 2.f;
	vertices.push_back(bleh);
	bleh.x = arrow.shaftLength;
	bleh.y = -arrow.capDiameter / 2.f;
	vertices.push_back(bleh);
	bleh.x = arrow.shaftLength + arrow.capLength;
	bleh.y = 0.f;
	vertices.push_back(bleh);

	return vertices;
}

std::vector<Math::Vec3> Editor::BuildGizmoTorusMesh2D()
{
	std::vector<Math::Vec3> vertices;

	f32 const innerRadius = Gizmo::defaultRotateCircleInnerRadius;
	f32 const outerRadius = Gizmo::defaultRotateCircleOuterRadius;
	u32 const outerCount = 64;

	// Outer circle
	for (u32 i = 0; i < outerCount; i += 1)
	{
		f32 radianA = 2 * Math::pi / outerCount * i;
		f32 radianB = 2 * Math::pi / outerCount * (i + 1);

		Math::Vec3 temp = { Math::Cos(radianA), Math::Sin(radianA), 0.f };
		auto a = temp * (outerRadius + innerRadius);
		auto b = temp * (outerRadius - innerRadius);

		temp = { Math::Cos(radianB), Math::Sin(radianB), 0.f };
		auto c = temp * (outerRadius + innerRadius);

		vertices.push_back(a);
		vertices.push_back(b);
		vertices.push_back(c);

		auto d = temp * (outerRadius - innerRadius);
		vertices.push_back(b);
		vertices.push_back(c);
		vertices.push_back(d);
	}

	return vertices;
}

std::vector<Math::Vec3> Editor::BuildGizmoScaleArrowMesh2D()
{
	std::vector<Math::Vec3> vertices;

	// Make quad for the base
	constexpr auto arrow = Gizmo::defaultArrow;

	Math::Vec3 bleh = {};

	bleh.x = 0.f;
	bleh.y = arrow.shaftDiameter / 2.f;
	vertices.push_back(bleh);
	bleh.x = 0.f;
	bleh.y = -arrow.shaftDiameter / 2.f;
	vertices.push_back(bleh);
	bleh.x = arrow.shaftLength;
	bleh.y = -arrow.shaftDiameter / 2.f;
	vertices.push_back(bleh);

	bleh.x = 0.f;
	bleh.y = arrow.shaftDiameter / 2.f;
	vertices.push_back(bleh);
	bleh.x = arrow.shaftLength;
	bleh.y = -arrow.shaftDiameter / 2.f;
	vertices.push_back(bleh);
	bleh.x = arrow.shaftLength;
	bleh.y = arrow.shaftDiameter / 2.f;
	vertices.push_back(bleh);

	bleh.x = arrow.shaftLength;
	bleh.y = arrow.capLength / 2.f;
	vertices.push_back(bleh);
	bleh.x = arrow.shaftLength;
	bleh.y = -arrow.capLength / 2.f;
	vertices.push_back(bleh);
	bleh.x = arrow.shaftLength + arrow.capLength;
	bleh.y = -arrow.capLength / 2.f;
	vertices.push_back(bleh);

	bleh.x = arrow.shaftLength;
	bleh.y = arrow.capLength / 2.f;
	vertices.push_back(bleh);
	bleh.x = arrow.shaftLength + arrow.capLength;
	bleh.y = -arrow.capLength / 2.f;
	vertices.push_back(bleh);
	bleh.x = arrow.shaftLength + arrow.capLength;
	bleh.y = arrow.capLength / 2.f;
	vertices.push_back(bleh);

	return vertices;
}