#include "ViewportWidget.hpp"

#include "Editor.hpp"
#include "EditorImpl.hpp"
#include "ComponentWidgets.hpp"

#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/DockArea.hpp>
#include <DEngine/Gui/Image.hpp>
#include <DEngine/Gui/LineEdit.hpp>
#include <DEngine/Gui/LineList.hpp>
#include <DEngine/Gui/MenuBar.hpp>
#include <DEngine/Gui/ScrollArea.hpp>
#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Text.hpp>
#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Application.hpp>
#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Time.hpp>

#include <DEngine/Physics.hpp>

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Utility.hpp>

#include <DEngine/Math/Constant.hpp>
#include <DEngine/Math/LinearTransform3D.hpp>

#include <vector>
#include <string>
#include <functional>

namespace DEngine::Editor
{
	class EntityIdList : public Gui::StackLayout
	{
	public:
		Gui::LineList* entitiesList = nullptr;
		EditorImpl* ctxImpl = nullptr;

		EntityIdList(EditorImpl* ctxImpl) :
			ctxImpl(ctxImpl)
		{
			DENGINE_DETAIL_ASSERT(!ctxImpl->entityIdList);
			ctxImpl->entityIdList = this;

			this->direction = Direction::Vertical;

			Gui::StackLayout* topElementLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
			this->AddWidget(Std::Box<Gui::Widget>{ topElementLayout });

			Gui::Button* newEntityButton = new Gui::Button;
			topElementLayout->AddWidget(Std::Box<Gui::Widget>{ newEntityButton });
			newEntityButton->text = "New";
			newEntityButton->activatePfn = [this](
				Gui::Button& btn,
				Gui::Context* guiCtx)
			{
				Entity newId = this->ctxImpl->scene->NewEntity();
				AddEntityToList(newId);
			};

			Gui::Button* entityDeleteButton = new Gui::Button;
			topElementLayout->AddWidget(Std::Box<Gui::Widget>{ entityDeleteButton });
			entityDeleteButton->text = "Delete";
			entityDeleteButton->activatePfn = [this](
				Gui::Button& btn,
				Gui::Context* guiCtx)
			{
				if (!this->ctxImpl->selectedEntity.HasValue())
					return;

				Entity selectedEntity = this->ctxImpl->selectedEntity.Value();

				this->ctxImpl->scene->DeleteEntity(selectedEntity);

				this->ctxImpl->UnselectEntity();

				RemoveEntityFromList(selectedEntity);
			};
			
			Gui::ScrollArea* entityListScrollArea = new Gui::ScrollArea();
			this->AddWidget(Std::Box<Gui::Widget>{ entityListScrollArea });

			entitiesList = new Gui::LineList();
			entitiesList->SetCanSelect(true);
			entitiesList->selectedLineChangedCallback = [this](Gui::LineList& widget)
			{
				Std::Opt<uSize> selectedLineOpt = widget.GetSelectedLine();
				if (selectedLineOpt.HasValue())
				{
					uSize selectedLine = selectedLineOpt.Value();
					std::string_view lineText = widget.GetLine(selectedLine);
					this->ctxImpl->SelectEntity((Entity)std::atoi(lineText.data()));
				}
			};
			entityListScrollArea->widget = Std::Box<Gui::Widget>{ entitiesList};

			for (Entity entityId : ctxImpl->scene->GetEntities())
			{
				AddEntityToList(entityId);
			}
		}

		virtual ~EntityIdList()
		{
			DENGINE_DETAIL_ASSERT(ctxImpl->entityIdList == this);
			ctxImpl->entityIdList = nullptr;
		}

		void AddEntityToList(Entity id)
		{
			entitiesList->AddLine(std::to_string((u64)id));
		}

		void RemoveEntityFromList(Entity id)
		{
			Std::Opt<uSize> selectedLineOpt = entitiesList->GetSelectedLine();
			if (selectedLineOpt.HasValue())
			{
				uSize selectedLine = selectedLineOpt.Value();
				entitiesList->RemoveLine(selectedLine);
			}
		}

		void SelectEntity(Std::Opt<Entity> previousId, Entity newId)
		{
			if (previousId.HasValue())
			{
				UnselectEntity(previousId.Value());
			}
		}

		void UnselectEntity(Entity id)
		{
		}
	};

	class ComponentList : public Gui::StackLayout
	{
	public:
		EditorImpl* ctxImpl = nullptr;

		MoveWidget* moveWidget = nullptr;
		TransformWidget* transformWidget = nullptr;
		TransformWidget2* transformWidget2 = nullptr;
		SpriteRenderer2DWidget* spriteRendererWidget = nullptr;

		ComponentList(EditorImpl* ctxImpl) :
			ctxImpl(ctxImpl)
		{
			DENGINE_DETAIL_ASSERT(!ctxImpl->componentList);
			ctxImpl->componentList = this;

			direction = Direction::Vertical;
			spacing = 10;
			this->expandNonDirection = true;

			if (ctxImpl->selectedEntity.HasValue())
			{
				EntitySelected(ctxImpl->selectedEntity.Value());
			}
		}

		virtual ~ComponentList()
		{
			DENGINE_DETAIL_ASSERT(ctxImpl->componentList == this);
			ctxImpl->componentList = nullptr;
		}

		void EntitySelected(Entity id)
		{
			this->ClearChildren();
			
			moveWidget = new MoveWidget(ctxImpl->scene, id);
			this->AddWidget(Std::Box<Gui::Widget>{ moveWidget });

			transformWidget = new TransformWidget(ctxImpl->scene, id);
			this->AddWidget(Std::Box<Gui::Widget>{ transformWidget });

			transformWidget2 = new TransformWidget2(*ctxImpl);
			this->AddWidget(Std::Box<Gui::Widget>{ transformWidget2 });

			spriteRendererWidget = new SpriteRenderer2DWidget(ctxImpl->scene, id);
			this->AddWidget(Std::Box<Gui::Widget>{ spriteRendererWidget });
		}

		void Tick(Scene& scene, Entity id)
		{
			transformWidget->Tick(scene, id);
			
			if (auto ptr = scene.GetComponent<Transform>(id))
				transformWidget2->Update(*ptr);
		}
	};
}

namespace DEngine::Editor::detail
{
	struct Gfx_App_Connection : public Gfx::WsiInterface
	{
		App::WindowID appWindowID{};

		// Return type is VkResult
		//
		// Argument #1: VkInstance - The Vulkan instance handle
		// Argument #2: VkAllocationCallbacks const* - Allocation callbacks for surface creation.
		// Argument #3: VkSurfaceKHR* - The output surface handle
		virtual i32 CreateVkSurface(uSize vkInstance, void const* allocCallbacks, u64& outSurface) override
		{
			auto resultOpt = App::CreateVkSurface(appWindowID, vkInstance, nullptr);
			if (resultOpt.HasValue())
			{
				outSurface = resultOpt.Value();
				return 0; // 0 is VK_RESULT_SUCCESS
			}
			else
				return -1;
		}
	};
}

using namespace DEngine;

namespace DEngine::Editor
{
	static Std::Box<Gui::Widget> CreateNavigationBar(
		EditorImpl& implData)
	{
		// Menu button
		Gui::MenuBar* menuBarA = new Gui::MenuBar(Gui::MenuBar::Direction::Horizontal);

		menuBarA->stackLayout.spacing = 25;
		
		menuBarA->AddSubmenuButton(
			"Submenu",
			[&implData](
				Gui::MenuBar& newMenuBar)
			{
				newMenuBar.stackLayout.color = { 0.25f, 0.f, 0.25f, 1.f };
				newMenuBar.stackLayout.spacing = 10;
				newMenuBar.stackLayout.padding = 10;

				
				newMenuBar.AddToggleMenuButton(
					"Entities",
					implData.entityIdList != nullptr,
					[&implData](
						bool activated)
					{
						if (activated)
						{
							Gui::DockArea::TopLevelNode newTop{};
							newTop.rect = { { 0, 0 }, { 400, 400 } };
							Gui::DockArea::Node* topNode = new Gui::DockArea::Node;
							newTop.node = Std::Box{ topNode };
							topNode->type = Gui::DockArea::Node::Type::Window;
							topNode->windows.push_back(Gui::DockArea::Window{});
							auto& newWindow = topNode->windows.back();
							newWindow.title = "Entities";
							newWindow.titleBarColor = { 0.5f, 0.5f, 0.f, 1.f };
							EntityIdList* entityIdList = new EntityIdList(&implData);
							newWindow.widget = Std::Box<Gui::Widget>{ entityIdList };

							implData.dockArea->topLevelNodes.emplace(implData.dockArea->topLevelNodes.begin(), Std::Move(newTop));
						}
					});

				newMenuBar.AddToggleMenuButton(
					"Components",
					implData.componentList != nullptr,
					[&implData](
						bool activated)
					{
						if (activated)
						{
							Gui::DockArea::TopLevelNode newTop{};
							newTop.rect = { { 250, 250 }, { 400, 400 } };
							Gui::DockArea::Node* topNode = new Gui::DockArea::Node;
							newTop.node = Std::Box{ topNode };
							topNode->type = Gui::DockArea::Node::Type::Window;
							topNode->windows.push_back(Gui::DockArea::Window{});
							auto& newWindow = topNode->windows.back();
							newWindow.title = "Components";
							newWindow.titleBarColor = { 0.f, 0.5f, 0.5f, 1.f };
							ComponentList* componentList = new ComponentList(&implData);
							newWindow.widget = Std::Box<Gui::Widget>{ componentList };

							implData.dockArea->topLevelNodes.emplace(implData.dockArea->topLevelNodes.begin(), Std::Move(newTop));
						}
					});


				newMenuBar.AddMenuButton(
					"New viewport",
					[&implData]()
					{
						Gui::DockArea::TopLevelNode newTop{};
						newTop.rect = { { 150, 150 }, { 400, 400 } };
						Gui::DockArea::Node* topNode = new Gui::DockArea::Node;
						newTop.node = Std::Box{ topNode };
						topNode->type = Gui::DockArea::Node::Type::Window;
						topNode->windows.push_back(Gui::DockArea::Window{});
						auto& newWindow = topNode->windows.back();
						newWindow.title = "Viewport";
						newWindow.titleBarColor = { 0.5f, 0.f, 0.5f, 1.f };
						ViewportWidget* viewport = new ViewportWidget(implData, *implData.gfxCtx);
						newWindow.widget = Std::Box<Gui::Widget>{ viewport };

						implData.dockArea->topLevelNodes.emplace(implData.dockArea->topLevelNodes.begin(), Std::Move(newTop));
					});
			});

		// Delta time counter at the top
		Gui::Text* deltaText = new Gui::Text;
		menuBarA->stackLayout.AddWidget(Std::Box<Gui::Widget>{ deltaText });
		implData.test_fpsText = deltaText;
		deltaText->String_Set("Child text");
		

		Gui::Button* playButton = new Gui::Button;
		menuBarA->stackLayout.AddWidget(Std::Box<Gui::Widget>{ playButton });
		playButton->text = "Play";
		playButton->type = Gui::Button::Type::Toggle;
		Scene& scene = *implData.scene;
		playButton->activatePfn = [&scene](
			Gui::Button& btn,
			Gui::Context* guiCtx)
		{
			if (btn.GetToggled())
			{
				DENGINE_DETAIL_ASSERT(!scene.play);
				scene.play = true;
			}
			else
			{
				DENGINE_DETAIL_ASSERT(scene.play);
				scene.play = false;
			}
		};

		return Std::Box<Gui::Widget>{ menuBarA };
	}
}

Editor::Context Editor::Context::Create(
	App::WindowID mainWindow,
	Scene* scene,
	Gfx::Context* gfxCtx)
{
	Context newCtx;

	newCtx.implData = new EditorImpl;
	EditorImpl& implData = *newCtx.implData;
	implData.guiCtx = Std::Box{ new DEngine::Gui::Context(DEngine::Gui::Context::Create(implData, gfxCtx)) };
	implData.gfxCtx = gfxCtx;
	implData.scene = scene;
	App::InsertEventInterface(implData);

	{
		Gui::StackLayout* outmostLayout = implData.guiCtx->outerLayout;

		outmostLayout->AddWidget(CreateNavigationBar(implData));

		Gui::DockArea* dockArea = new Gui::DockArea;
		implData.dockArea = dockArea;
		outmostLayout->AddWidget(Std::Box<Gui::Widget>{ dockArea });

		{
			Gui::DockArea::TopLevelNode newTop{};
			newTop.rect = { { 0, 0 }, { 400, 400 } };
			Gui::DockArea::Node* topNode = new Gui::DockArea::Node;
			newTop.node = Std::Box{ topNode };
			topNode->type = Gui::DockArea::Node::Type::Window;
			topNode->windows.push_back(Gui::DockArea::Window{});
			auto& newWindow = topNode->windows.back();
			newWindow.title = "Entities";
			newWindow.titleBarColor = { 0.5f, 0.5f, 0.f, 1.f };
			EntityIdList* entityIdList = new EntityIdList(&implData);
			newWindow.widget = Std::Box<Gui::Widget>{ entityIdList };

			dockArea->topLevelNodes.emplace_back(Std::Move(newTop));
		}

		{
			Gui::DockArea::TopLevelNode newTop{};
			newTop.rect = { { 150, 150 }, { 400, 400 } };
			Gui::DockArea::Node* topNode = new Gui::DockArea::Node;
			newTop.node = Std::Box{ topNode };
			topNode->type = Gui::DockArea::Node::Type::Window;
			topNode->windows.push_back(Gui::DockArea::Window{});
			auto& newWindow = topNode->windows.back();
			newWindow.title = "Viewport";
			newWindow.titleBarColor = { 0.5f, 0.f, 0.5f, 1.f };
			ViewportWidget* viewport = new ViewportWidget(implData, *gfxCtx);
			newWindow.widget = Std::Box<Gui::Widget>{ viewport };

			dockArea->topLevelNodes.emplace_back(Std::Move(newTop));
		}

		{
			Gui::DockArea::TopLevelNode newTop{};
			newTop.rect = { { 250, 250 }, { 400, 400 } };
			Gui::DockArea::Node* topNode = new Gui::DockArea::Node;
			newTop.node = Std::Box{ topNode };
			topNode->type = Gui::DockArea::Node::Type::Window;
			topNode->windows.push_back(Gui::DockArea::Window{});
			auto& newWindow = topNode->windows.back();
			newWindow.title = "Components";
			newWindow.titleBarColor = { 0.f, 0.5f, 0.5f, 1.f };

			Gui::ScrollArea* scrollArea = new Gui::ScrollArea;
			newWindow.widget = Std::Box<Gui::Widget>{ scrollArea };

			ComponentList* componentList = new ComponentList(&implData);
			scrollArea->widget = Std::Box<Gui::Widget>{ componentList };

			dockArea->topLevelNodes.emplace_back(Std::Move(newTop));
		}
	}
	
	return newCtx;
}

void Editor::Context::ProcessEvents()
{
	EditorImpl& implData = this->ImplData();

	if (App::TickCount() % 60 == 0)
		implData.test_fpsText->String_Set(std::to_string(Time::Delta()).c_str());

	for (impl::GuiEvent const& event : implData.queuedGuiEvents)
	{
		switch (event.type)
		{
		case impl::GuiEvent::Type::CharEnterEvent: implData.guiCtx->PushEvent(event.charEnter); break;
		case impl::GuiEvent::Type::CharEvent: implData.guiCtx->PushEvent(event.charEvent); break;
		case impl::GuiEvent::Type::CharRemoveEvent: implData.guiCtx->PushEvent(event.charRemove); break;
		case impl::GuiEvent::Type::CursorClickEvent: implData.guiCtx->PushEvent(event.cursorClick); break;
		case impl::GuiEvent::Type::CursorMoveEvent: implData.guiCtx->PushEvent(event.cursorMove); break;
		case impl::GuiEvent::Type::TouchEvent: implData.guiCtx->PushEvent(event.touch); break;
		case impl::GuiEvent::Type::WindowCloseEvent: implData.guiCtx->PushEvent(event.windowClose); break;
		case impl::GuiEvent::Type::WindowCursorEnterEvent: implData.guiCtx->PushEvent(event.windowCursorEnter); break;
		case impl::GuiEvent::Type::WindowMinimizeEvent: implData.guiCtx->PushEvent(event.windowMinimize); break;
		case impl::GuiEvent::Type::WindowMoveEvent: implData.guiCtx->PushEvent(event.windowMove); break;
		case impl::GuiEvent::Type::WindowResizeEvent: implData.guiCtx->PushEvent(event.windowResize); break;
		}
	}
	implData.queuedGuiEvents.clear();

	for (auto viewportPtr : implData.viewportWidgets)
	{
		viewportPtr->TickTest(Time::Delta());
	}
	if (App::TickCount() % 10 == 0)
	{
		if (implData.componentList && implData.selectedEntity.HasValue())
			implData.componentList->Tick(*implData.scene, implData.selectedEntity.Value());
	}


	//implData.guiCtx->Tick();
	implData.guiCtx->Render();
}

Editor::Context::Context(Context&& other) noexcept :
	implData{ other.implData }
{
	other.implData = nullptr;
}

Editor::Context::~Context()
{
	if (this->implData)
	{
		EditorImpl& implData = this->ImplData();
		App::RemoveEventInterface(implData);
		delete &implData;
	}
}

Editor::DrawInfo Editor::Context::GetDrawInfo() const
{
	EditorImpl& implData = this->ImplData();

	DrawInfo returnVal;
	
	returnVal.vertices = implData.guiCtx->vertices;
	returnVal.indices = implData.guiCtx->indices;
	returnVal.drawCmds = implData.guiCtx->drawCmds;

	returnVal.windowUpdates = implData.guiCtx->windowUpdates;

	for (auto viewportWidgetPtr : implData.viewportWidgets)
	{
		DENGINE_DETAIL_ASSERT(viewportWidgetPtr);
		auto& viewportWidget = *viewportWidgetPtr;
		if (viewportWidgetPtr->isVisible)
		{
			DENGINE_DETAIL_ASSERT(viewportWidget.currentExtent.width > 0 && viewportWidget.currentExtent.height > 0);
			DENGINE_DETAIL_ASSERT(viewportWidget.newExtent.width > 0 && viewportWidget.newExtent.height > 0);

			Gfx::ViewportUpdate update{};
			update.id = viewportWidget.viewportId;
			update.width = viewportWidget.currentExtent.width;
			update.height = viewportWidget.currentExtent.height;

			f32 aspectRatio = (f32)viewportWidget.newExtent.width / (f32)viewportWidget.newExtent.height;
			Math::Mat4 projMat = viewportWidget.GetProjectionMatrix(aspectRatio);

			update.transform = projMat;

			if (implData.selectedEntity.HasValue())
			{
				Entity selected = implData.selectedEntity.Value();
				// Find Transform component of this entity
				Transform const* transformPtr = implData.scene->GetComponent<Transform>(selected);
				if (transformPtr != nullptr)
				{
					Transform const& transform = *transformPtr;

					update.gizmo = Gfx::ViewportUpdate::Gizmo{};
					Gfx::ViewportUpdate::Gizmo& gizmo = update.gizmo.Value();

					gizmo.position = transform.position;

					Math::Mat4 worldTransform = Math::Mat4::Identity();
					Math::LinTran3D::SetTranslation(worldTransform, { transform.position.x, transform.position.y, 0.f });
					u32 smallestViewportExtent = Math::Min(viewportWidget.newExtent.width, viewportWidget.newExtent.height);
					f32 scale = Gizmo::ComputeScale(
						worldTransform,
						smallestViewportExtent * Gizmo::defaultGizmoSizeRelative,
						projMat,
						viewportWidget.newExtent);
					gizmo.scale = scale;

					gizmo.quadScale = gizmo.scale * Gizmo::defaultPlaneScaleRelative;
					gizmo.quadOffset = gizmo.scale * Gizmo::defaultPlaneOffsetRelative;
				}
			}
			returnVal.viewportUpdates.push_back(update);
		}
	}

	return returnVal;
}

void Editor::EditorImpl::SelectEntity(Entity id)
{
	if (selectedEntity.HasValue() && selectedEntity.Value() == id)
		return;

	// Update the entity list
	if (entityIdList)
		entityIdList->SelectEntity(selectedEntity, id);

	// Update the component list
	if (componentList)
		componentList->EntitySelected(id);

	selectedEntity = id;
}

void Editor::EditorImpl::UnselectEntity()
{
	// Update the entity list
	if (selectedEntity.HasValue() && entityIdList)
		entityIdList->UnselectEntity(selectedEntity.Value());

	// Clear the component list
	if (componentList)
		componentList->ClearChildren();

	selectedEntity = Std::nullOpt;
}

void Editor::EditorImpl::ButtonEvent(
	App::Button button,
	bool state)
{
	if (button == App::Button::LeftMouse || button == App::Button::RightMouse)
	{
		impl::GuiEvent event{};
		event.type = impl::GuiEvent::Type::CursorClickEvent;
		if (button == App::Button::LeftMouse)
			event.cursorClick.button = Gui::CursorButton::Left;
		else if (button == App::Button::RightMouse)
			event.cursorClick.button = Gui::CursorButton::Right;
		event.cursorClick.clicked = state;
		queuedGuiEvents.push_back(event);
	}
}

void DEngine::Editor::EditorImpl::CharEnterEvent()
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::CharEnterEvent;
	event.charEnter = {};
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::CharEvent(
	u32 value)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::CharEvent;
	event.charEvent.utfValue = value;
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::CharRemoveEvent()
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::CharRemoveEvent;
	event.charRemove = {};
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::CursorMove(
	Math::Vec2Int position,
	Math::Vec2Int positionDelta)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::CursorMoveEvent;
	event.cursorMove.position = position;
	event.cursorMove.positionDelta = positionDelta;
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::TouchEvent(
	u8 id,
	App::TouchEventType type,
	Math::Vec2 position)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::TouchEvent;
	event.touch.id = id;
	event.touch.position = position;
	if (type == App::TouchEventType::Down)
		event.touch.type = Gui::TouchEventType::Down;
	else if (type == App::TouchEventType::Moved)
		event.touch.type = Gui::TouchEventType::Moved;
	else if (type == App::TouchEventType::Up)
		event.touch.type = Gui::TouchEventType::Up;
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::WindowClose(App::WindowID windowId)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::WindowCloseEvent;
	event.windowClose.windowId = (Gui::WindowID)windowId;
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::WindowCursorEnter(
	App::WindowID window,
	bool entered)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::WindowCursorEnterEvent;
	event.windowCursorEnter.windowId = (Gui::WindowID)window;
	event.windowCursorEnter.entered = entered;
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::WindowMinimize(
	App::WindowID window,
	bool wasMinimized)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::WindowMinimizeEvent;
	event.windowMinimize.windowId = (Gui::WindowID)window;
	event.windowMinimize.wasMinimized = wasMinimized;
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::WindowMove(
	App::WindowID window,
	Math::Vec2Int position)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::WindowMoveEvent;
	event.windowMove.windowId = (Gui::WindowID)window;
	event.windowMove.position = position;
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::WindowResize(
	App::WindowID window,
	App::Extent newExtent,
	Math::Vec2Int visiblePos,
	App::Extent visibleSize)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::WindowResizeEvent;
	event.windowResize.windowId = (Gui::WindowID)window;
	event.windowResize.extent = Gui::Extent{ newExtent.width, newExtent.height };
	event.windowResize.visibleRect = Gui::Rect{ visiblePos, Gui::Extent{ newExtent.width, newExtent.height } };
	queuedGuiEvents.push_back(event);
}

void Editor::EditorImpl::CloseWindow(Gui::WindowID id)
{
	App::DestroyWindow((App::WindowID)id);
}

void Editor::EditorImpl::SetCursorType(Gui::WindowID id, Gui::CursorType cursorType)
{
	App::CursorType appCursorType{};
	switch (cursorType)
	{
	case Gui::CursorType::Arrow:
		appCursorType = App::CursorType::Arrow;
		break;
	case Gui::CursorType::HorizontalResize:
		appCursorType = App::CursorType::HorizontalResize;
		break;
	case Gui::CursorType::VerticalResize:
		appCursorType = App::CursorType::VerticalResize;
		break;
	default:
		break;
	}
	App::SetCursor((App::WindowID)id, appCursorType);
}

void Editor::EditorImpl::HideSoftInput()
{
	App::HideSoftInput();
}

void Editor::EditorImpl::OpenSoftInput(
	std::string_view currentText, 
	Gui::SoftInputFilter inputFilter)
{
	App::SoftInputFilter filter{};
	switch (inputFilter)
	{
	case Gui::SoftInputFilter::Float:
		filter = App::SoftInputFilter::Float;
		break;
	case Gui::SoftInputFilter::Integer:
		filter = App::SoftInputFilter::Integer;
		break;
	case Gui::SoftInputFilter::UnsignedInteger:
		filter = App::SoftInputFilter::UnsignedInteger;
		break;
	}
	App::OpenSoftInput(currentText, filter);
}

std::vector<Math::Vec3> Editor::BuildGizmoArrowMesh()
{
	Gizmo::Arrow arrow = Editor::Gizmo::defaultArrow;

	f32 arrowShaftRadius = arrow.shaftDiameter / 2.f;
	f32 arrowCapRadius = arrow.capSize / 2.f;

	u32 subdivisions = 4;
	// We need atleast 2 subdivisons so we can atleast get a diamond
	DENGINE_DETAIL_ASSERT(subdivisions > 1);
	u32 baseCircleTriangleCount = (subdivisions * 2);

	std::vector<Math::Vec3> vertices;
	for (u32 i = 0; i < baseCircleTriangleCount; i++)
	{
		f32 currentRadiansA = 2 * Math::pi / baseCircleTriangleCount * i;
		f32 currentRadiansB = 2 * Math::pi / baseCircleTriangleCount * ((i + 1) % baseCircleTriangleCount);
		
		{
			Math::Vec3 shaftBaseVertA{};
			shaftBaseVertA.x = Math::Sin(currentRadiansA);
			shaftBaseVertA.x *= arrowShaftRadius;
			shaftBaseVertA.y = Math::Cos(currentRadiansA);
			shaftBaseVertA.y *= arrowShaftRadius;

			Math::Vec3 shaftBaseVertB{};
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
			Math::Vec3 capBaseVertA{};
			capBaseVertA.x = Math::Sin(currentRadiansA);
			capBaseVertA.x *= arrowCapRadius;
			capBaseVertA.y = Math::Cos(currentRadiansA);
			capBaseVertA.y *= arrowCapRadius;
			capBaseVertA.z = arrow.shaftLength;

			Math::Vec3 capBaseVertB{};
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
			Math::Vec3 arrowHeadMidVert{};
			arrowHeadMidVert.z = arrow.shaftLength + arrow.capLength;

			vertices.push_back(capBaseVertA);
			vertices.push_back(arrowHeadMidVert);
			vertices.push_back(capBaseVertB);
		}
	}

	return vertices;
}