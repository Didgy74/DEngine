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
			this->AddLayout2(Std::Box<Gui::Layout>{ topElementLayout });

			Gui::Button* newEntityButton = new Gui::Button;
			topElementLayout->AddWidget2(Std::Box<Gui::Widget>{ newEntityButton });
			newEntityButton->textWidget.String_Set("New");
			newEntityButton->activatePfn = [this](
				Gui::Button& btn,
				Gui::Context& ctx,
				Gui::WindowID windowId,
				Gui::Rect widgetRect,
				Gui::Rect visibleRect)
			{
				Entity newId = this->ctxImpl->scene->NewEntity();
				AddEntityToList(newId);
			};

			Gui::Button* entityDeleteButton = new Gui::Button;
			topElementLayout->AddWidget2(Std::Box<Gui::Widget>{ entityDeleteButton });
			entityDeleteButton->textWidget.String_Set("Delete");
			entityDeleteButton->activatePfn = [this](
				Gui::Button& btn,
				Gui::Context& ctx,
				Gui::WindowID windowId,
				Gui::Rect widgetRect,
				Gui::Rect visibleRect)
			{
				if (!this->ctxImpl->selectedEntity.HasValue())
					return;

				Entity selectedEntity = this->ctxImpl->selectedEntity.Value();

				this->ctxImpl->scene->DeleteEntity(selectedEntity);

				this->ctxImpl->UnselectEntity();

				RemoveEntityFromList(selectedEntity);
			};
			
			Gui::ScrollArea* entityListScrollArea = new Gui::ScrollArea();
			this->AddLayout2(Std::Box<Gui::Layout>{ entityListScrollArea });

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
			entityListScrollArea->childType = Gui::ScrollArea::ChildType::Widget;
			entityListScrollArea->widget = Std::Box<Gui::Widget>{ entitiesList};

			for (uSize i = 0; i < ctxImpl->scene->entities.size(); i++)
			{
				Entity entityId = ctxImpl->scene->entities[i];

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
		SpriteRenderer2DWidget* spriteRendererWidget = nullptr;
		Rigidbody2DWidget* rbWidget = nullptr;
		CircleCollider2DWidget* circleColliderWidget = nullptr;
		BoxCollider2DWidget* boxColliderWidget = nullptr;

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
			this->AddLayout2(Std::Box<Gui::Layout>{ moveWidget });

			transformWidget = new TransformWidget(ctxImpl->scene, id);
			this->AddLayout2(Std::Box<Gui::Layout>{ transformWidget });

			spriteRendererWidget = new SpriteRenderer2DWidget(ctxImpl->scene, id);
			this->AddLayout2(Std::Box<Gui::Layout>{ spriteRendererWidget });

			rbWidget = new Rigidbody2DWidget(ctxImpl->scene, id);
			this->AddLayout2(Std::Box<Gui::Layout>{ rbWidget });

			circleColliderWidget = new CircleCollider2DWidget(ctxImpl->scene, id);
			this->AddLayout2(Std::Box<Gui::Layout>{ circleColliderWidget });

			boxColliderWidget = new BoxCollider2DWidget(ctxImpl->scene, id);
			this->AddLayout2(Std::Box<Gui::Layout>{ boxColliderWidget });
		}

		void Tick(Scene& scene, Entity id)
		{
			transformWidget->Tick(scene, id);
			rbWidget->Tick(scene, id);
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
	static Std::Box<Gui::Layout> CreateNavigationBar(
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
							newWindow.layout = Std::Box<Gui::Layout>{ entityIdList };

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
							newWindow.layout = Std::Box<Gui::Layout>{ componentList };

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
						newWindow.layout = Std::Box<Gui::Layout>{ viewport };

						implData.dockArea->topLevelNodes.emplace(implData.dockArea->topLevelNodes.begin(), Std::Move(newTop));
					});
			});

		// Delta time counter at the top
		Gui::Text* deltaText = new Gui::Text;
		menuBarA->stackLayout.AddWidget2(Std::Box<Gui::Widget>{ deltaText });
		implData.test_fpsText = deltaText;
		deltaText->String_Set("Child text");
		

		Gui::Button* playButton = new Gui::Button;
		menuBarA->stackLayout.AddWidget2(Std::Box<Gui::Widget>{ playButton });
		playButton->textWidget.String_Set("Play");
		playButton->type = Gui::Button::Type::Toggle;
		Scene& scene = *implData.scene;
		playButton->activatePfn = [&scene](
			Gui::Button& btn,
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect)
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

		return Std::Box<Gui::Layout>{ menuBarA };
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

		outmostLayout->AddLayout2(CreateNavigationBar(implData));

		Gui::DockArea* dockArea = new Gui::DockArea;
		implData.dockArea = dockArea;
		outmostLayout->AddLayout2(Std::Box<Gui::Layout>{ dockArea });

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
			newWindow.layout = Std::Box<Gui::Layout>{ entityIdList };

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
			newWindow.layout = Std::Box<Gui::Layout>{ viewport };

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
			newWindow.layout = Std::Box<Gui::Layout>{ scrollArea };

			ComponentList* componentList = new ComponentList(&implData);
			scrollArea->childType = Gui::ScrollArea::ChildType::Layout;
			scrollArea->layout = Std::Box<Gui::Layout>{ componentList };

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
		if (viewportWidgetPtr && viewportWidgetPtr->isVisible)
		{
			auto& viewportWidget = *viewportWidgetPtr;
			DENGINE_DETAIL_ASSERT(viewportWidget.currentExtent.width > 0 && viewportWidget.currentExtent.height > 0);
			DENGINE_DETAIL_ASSERT(viewportWidget.newExtent.width > 0 && viewportWidget.newExtent.height > 0);

			Gfx::ViewportUpdate update{};
			update.id = viewportWidget.viewportId;
			update.width = viewportWidget.currentExtent.width;
			update.height = viewportWidget.currentExtent.height;

			Math::Mat4 test = Math::Mat4::Identity();
			test.At(0, 0) = -1;
			//test.At(1, 1) = -1;
			test.At(2, 2) = -1;

			Math::Mat4 camMat = Math::LinTran3D::Rotate_Homo(viewportWidget.cam.rotation) * test;
			Math::LinTran3D::SetTranslation(camMat, viewportWidget.cam.position);

			Math::Mat4 test2 = Math::Mat4::Identity();
			//test2.At(0, 0) = -1;
			//test2.At(1, 1) = -1;
			//test2.At(2, 2) = -1;
			camMat = test2 * camMat;


			camMat = camMat.GetInverse().Value();
			f32 aspectRatio = (f32)viewportWidget.newExtent.width / viewportWidget.newExtent.height;

			camMat = Math::LinTran3D::Perspective_RH_ZO(viewportWidget.cam.verticalFov * Math::degToRad, aspectRatio, 0.1f, 100.f) * camMat;

			//camMat = test * camMat;

			update.transform = camMat;

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
