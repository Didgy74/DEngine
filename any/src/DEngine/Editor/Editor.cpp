#include "Editor.hpp"
#include "EditorImpl.hpp"

#include "ViewportWidget.hpp"
#include "ComponentWidgets.hpp"

#include <DEngine/Gui/LineList.hpp>
#include <DEngine/Gui/ScrollArea.hpp>

#include <DEngine/Time.hpp>

#include <vector>
#include <string>

using namespace DEngine;
using namespace DEngine::Editor;

Std::Array<Math::Vec4, (int)Settings::Color::COUNT> Settings::colorArray = Settings::BuildColorArray();

namespace DEngine::Editor
{
	enum class FileMenuEnum
	{
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

		EntityIdList(EditorImpl& editorImpl) :
			editorImpl(&editorImpl)
		{
			DENGINE_IMPL_ASSERT(!editorImpl.entityIdList);
			editorImpl.entityIdList = this;

			direction = Direction::Vertical;

			auto topElementLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
			topElementLayout->spacing = 15;
			this->AddWidget(Std::Box{ topElementLayout });

			auto newEntityButton = new Gui::Button;
			topElementLayout->AddWidget(Std::Box{ newEntityButton });
			newEntityButton->text = "New";
			newEntityButton->normalColor = Settings::GetColor(Settings::Color::Button_Normal);
			newEntityButton->toggledColor = Settings::GetColor(Settings::Color::Button_Active);
			newEntityButton->textMargin = Editor::Settings::defaultTextMargin;
			newEntityButton->activateFn = [this](
				Gui::Button& btn)
			{
				Entity newId = this->editorImpl->scene->NewEntity();
				AddEntityToList(newId);
			};

			Gui::Button* entityDeleteButton = new Gui::Button;
			topElementLayout->AddWidget(Std::Box{ entityDeleteButton });
			entityDeleteButton->text = "Delete";
			entityDeleteButton->normalColor = Settings::GetColor(Settings::Color::Button_Normal);
			entityDeleteButton->toggledColor = Settings::GetColor(Settings::Color::Button_Active);
			entityDeleteButton->textMargin = Editor::Settings::defaultTextMargin;
			entityDeleteButton->activateFn = [this](
				Gui::Button& btn)
			{
				if (!this->editorImpl->GetSelectedEntity().HasValue())
					return;

				Entity selectedEntity = this->editorImpl->GetSelectedEntity().Value();

				RemoveEntityFromList(selectedEntity);

				this->editorImpl->UnselectEntity();

				this->editorImpl->scene->DeleteEntity(selectedEntity);
			};

			auto deselectButton = new Gui::Button;
			topElementLayout->AddWidget(Std::Box{ deselectButton });
			deselectButton->text = "Deselect";
			deselectButton->normalColor = Settings::GetColor(Settings::Color::Button_Normal);
			deselectButton->toggledColor = Settings::GetColor(Settings::Color::Button_Active);
			deselectButton->textMargin = Editor::Settings::defaultTextMargin;
			deselectButton->activateFn = [this](
				Gui::Button& btn)
			{
				if (!this->editorImpl->GetSelectedEntity().HasValue())
					return;

				this->editorImpl->UnselectEntity();
			};

			Gui::ScrollArea* entityListScrollArea = new Gui::ScrollArea();
			this->AddWidget(Std::Box<Gui::Widget>{ entityListScrollArea });
			entityListScrollArea->scrollbarInactiveColor = Settings::GetColor(Settings::Color::Scrollbar_Normal);

			entitiesList = new Gui::LineList();
			entityListScrollArea->widget = Std::Box<Gui::Widget>{ entitiesList };
			entitiesList->textMargin = Settings::defaultTextMargin;
			entitiesList->selectedLineChangedFn = [this](
				Gui::LineList& widget)
			{
				if (widget.selectedLine.HasValue())
				{
					auto lineText = widget.lines[widget.selectedLine.Value()].c_str();
					this->editorImpl->SelectEntity((Entity)std::atoi(lineText));
				}
				else
					this->editorImpl->UnselectEntity();
			};

			auto entities = editorImpl.scene->GetEntities();
			for (uSize i = 0; i < entities.Size(); i += 1)
			{
				Entity entityId = entities[i];
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

			auto& submenuLine = editorImpl->viewMenuButton->submenu.lines[(int)FileMenuEnum::Entities].data;
			DENGINE_IMPL_ASSERT(submenuLine.IsA<Gui::MenuButton::LineButton>());
			auto& lineButton = submenuLine.Get<Gui::MenuButton::LineButton>();
			lineButton.toggled = false;
		}

		void AddEntityToList(Entity id)
		{
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
			for (uSize i = 0; i < entitiesList->lines.size(); i += 1)
			{
				long id = std::stol(entitiesList->lines[i]);
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

		ComponentList(EditorImpl& inEditorImpl) :
			editorImpl(&inEditorImpl)
		{
			DENGINE_IMPL_ASSERT(!editorImpl->componentList);
			editorImpl->componentList = this;

			scrollbarInactiveColor = Settings::GetColor(Settings::Color::Scrollbar_Normal);

			outerLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Vertical);
			this->widget = Std::Box<Gui::Widget>{ outerLayout };
			outerLayout->padding = 5;
			outerLayout->spacing = 10;
			//outerLayout->expandNonDirection = true;

			if (editorImpl->GetSelectedEntity().HasValue())
			{
				EntitySelected(editorImpl->GetSelectedEntity().Value());
			}
		}

		virtual ~ComponentList() override
		{
			DENGINE_IMPL_ASSERT(editorImpl->componentList == this);
			editorImpl->componentList = nullptr;

			auto& submenuLine = editorImpl->viewMenuButton->submenu.lines[(int)FileMenuEnum::Components].data;
			DENGINE_IMPL_ASSERT(submenuLine.IsA<Gui::MenuButton::LineButton>());
			auto& lineButton = submenuLine.Get<Gui::MenuButton::LineButton>();
			lineButton.toggled = false;
		}

		void EntitySelected(Entity id)
		{
			outerLayout->ClearChildren();

			moveWidget = new MoveWidget(*editorImpl);
			outerLayout->AddWidget(Std::Box{ moveWidget });

			transformWidget = new TransformWidget(*editorImpl);
			outerLayout->AddWidget(Std::Box{ transformWidget });

			spriteRendererWidget = new SpriteRenderer2DWidget(*editorImpl);
			outerLayout->AddWidget(Std::Box{ spriteRendererWidget });

			box2DWidget = new RigidbodyWidget(*editorImpl);
			outerLayout->AddWidget(Std::Box{ box2DWidget });
		}

		void Tick(Scene const& scene, Entity id)
		{
			if (auto ptr = scene.GetComponent<Transform>(id))
				transformWidget->Update(*ptr);
		}
	};
}

namespace DEngine::Editor::detail
{
	struct Gfx_App_Connection : public Gfx::WsiInterface
	{
		App::WindowID appWindowID = {};

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
	[[nodiscard]] static Std::Box<Gui::Widget> CreateNavigationBar(
		EditorImpl& editorImpl)
	{
		auto stackLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
		stackLayout->spacing = Editor::Settings::defaultTextMargin;

		auto menuButton = new Gui::MenuButton;
		editorImpl.viewMenuButton = menuButton;
		menuButton->colors.normal = Settings::GetColor(Settings::Color::Button_Normal);
		menuButton->spacing = Editor::Settings::defaultTextMargin;
		menuButton->margin = Editor::Settings::defaultTextMargin;
		stackLayout->AddWidget(Std::Box{ menuButton });
		menuButton->submenu.lines.resize((int)FileMenuEnum::COUNT);
		menuButton->title = "Menu";
		{
			// Create button for Entities window
			Gui::MenuButton::LineButton btn = {};
			btn.toggled = true;
			btn.togglable = true;
			btn.callback = [&editorImpl](Gui::MenuButton::LineButton& btn) {
				if (btn.toggled)
				{
					editorImpl.dockArea->AddWindow(
						"Entities",
						Settings::GetColor(Settings::Color::Window_Entities),
						Std::Box{ new EntityIdList(editorImpl) });
				}
				btn.toggled = true;
			};
			Gui::MenuButton::Line line = { Std::Move(btn) };
			line.title = "Entities";
			menuButton->submenu.lines[(int)FileMenuEnum::Entities] = Std::Move(line);
		}
		{
			// Create button for Components window
			Gui::MenuButton::LineButton btn = {};
			btn.toggled = true;
			btn.togglable = true;
			btn.callback = [&editorImpl](Gui::MenuButton::LineButton& btn) {
				if (btn.toggled)
				{
					editorImpl.dockArea->AddWindow(
						"Components",
						Settings::GetColor(Settings::Color::Window_Components),
						Std::Box{ new ComponentList(editorImpl) });
				}
				btn.toggled = true;
			};
			Gui::MenuButton::Line line = { Std::Move(btn) };
			line.title = "Components";
			menuButton->submenu.lines[(int)FileMenuEnum::Components] = Std::Move(line);
		}
		{
			// Create button for adding viewport
			Gui::MenuButton::LineButton btn = {};
			btn.toggled = false;
			btn.togglable = false;
			btn.callback = [&editorImpl](Gui::MenuButton::LineButton& btn) {
				editorImpl.dockArea->AddWindow(
					"Viewport",
					Settings::GetColor(Settings::Color::Window_Viewport),
					Std::Box{ new ViewportWidget(editorImpl) });
			};
			Gui::MenuButton::Line line = { Std::Move(btn) };
			line.title = "New viewport";
			menuButton->submenu.lines[(int)FileMenuEnum::NewViewport] = Std::Move(line);
		}

		// Delta time counter at the top
		Gui::Text* deltaText = new Gui::Text;
		stackLayout->AddWidget(Std::Box{ deltaText });
		editorImpl.test_fpsText = deltaText;
		deltaText->margin = Editor::Settings::defaultTextMargin;
		deltaText->String_Set("Child text");

		auto playButton = new Gui::Button;
		stackLayout->AddWidget(Std::Box{ playButton });
		playButton->normalColor = Settings::GetColor(Settings::Color::Button_Normal);
		playButton->toggledColor = Settings::GetColor(Settings::Color::Button_Active);
		playButton->text = "Play";
		playButton->textMargin = Editor::Settings::defaultTextMargin;
		playButton->type = Gui::Button::Type::Toggle;
		playButton->activateFn = [&editorImpl](Gui::Button& btn)
		{
			if (btn.GetToggled())
			{
				editorImpl.BeginSimulating();
			}
			else
			{
				editorImpl.StopSimulating();
			}
		};

		Gui::ButtonGroup* gizmoBtnGroup = new Gui::ButtonGroup;
		editorImpl.gizmoTypeBtnGroup = gizmoBtnGroup;
		stackLayout->AddWidget(Std::Box{ gizmoBtnGroup });
		gizmoBtnGroup->inactiveColor = Settings::GetColor(Settings::Color::Button_Normal);
		gizmoBtnGroup->activeColor = Settings::GetColor(Settings::Color::Button_Active);
		gizmoBtnGroup->margin = Editor::Settings::defaultTextMargin;
		gizmoBtnGroup->AddButton("Translate");
		gizmoBtnGroup->AddButton("Rotate");
		gizmoBtnGroup->AddButton("Scale");

		return Std::Box{ stackLayout };
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
	
	implData.gfxCtx = gfxCtx;
	implData.scene = scene;
	App::InsertEventInterface(implData);

	auto outmostLayout = new Gui::StackLayout(Gui::StackLayout::Dir::Vertical);

	outmostLayout->AddWidget(CreateNavigationBar(implData));

	Gui::DockArea* dockArea = new Gui::DockArea;
	implData.dockArea = dockArea;
	outmostLayout->AddWidget(Std::Box{ dockArea });
	dockArea->tabTextMargin = Editor::Settings::defaultTextMargin;

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
	
	Gui::WindowHandler& guiWinHandler = implData;
	implData.guiCtx = Std::Box{ new Gui::Context(Gui::Context::Create(guiWinHandler, gfxCtx)) };

	auto const clearColor = Settings::GetColor(Settings::Color::Background);
	auto windowExtent = App::GetWindowExtent(mainWindow);
	auto windowPos = App::GetWindowPosition(mainWindow);
	auto visibleExtent = App::GetWindowVisibleExtent(mainWindow);
	auto visibleOffset = App::GetWindowVisibleOffset(mainWindow);
	implData.guiCtx->AdoptWindow(
		(Gui::WindowID)mainWindow,
		clearColor,
		{ windowPos, { windowExtent.width, windowExtent.height } },
		{ windowPos + visibleOffset, { visibleExtent.width, visibleExtent.height } },
		Std::Box{ outmostLayout });

	return newCtx;
}

void Editor::EditorImpl::FlushQueuedEvents()
{
	for (auto const& event : queuedGuiEvents)
	{
		using EventT = Std::Trait::RemoveCVRef<decltype(event)>;

		switch (event.GetIndex())
		{
			case EventT::indexOf<Gui::CharEnterEvent>:
				guiCtx->PushEvent(event.Get<Gui::CharEnterEvent>());
				break;
			case EventT::indexOf<Gui::CharEvent>:
				guiCtx->PushEvent(event.Get<Gui::CharEvent>());
				break;
			case EventT::indexOf<Gui::CharRemoveEvent>:
				guiCtx->PushEvent(event.Get<Gui::CharRemoveEvent>());
				break;
			case EventT::indexOf<Gui::CursorClickEvent>:
				guiCtx->PushEvent(event.Get<Gui::CursorClickEvent>());
				break;
			case EventT::indexOf<Gui::CursorMoveEvent>:
				guiCtx->PushEvent(event.Get<Gui::CursorMoveEvent>());
				break;
			case EventT::indexOf<Gui::TouchPressEvent>:
				guiCtx->PushEvent(event.Get<Gui::TouchPressEvent>());
				break;
			case EventT::indexOf<Gui::TouchMoveEvent>:
				guiCtx->PushEvent(event.Get<Gui::TouchMoveEvent>());
				break;
			case EventT::indexOf<Gui::WindowCloseEvent>:
				guiCtx->PushEvent(event.Get<Gui::WindowCloseEvent>());
				break;
			case EventT::indexOf<Gui::WindowCursorEnterEvent>:
				guiCtx->PushEvent(event.Get<Gui::WindowCursorEnterEvent>());
				break;
			case EventT::indexOf<Gui::WindowMinimizeEvent>:
				guiCtx->PushEvent(event.Get<Gui::WindowMinimizeEvent>());
				break;
			case EventT::indexOf<Gui::WindowMoveEvent>:
				guiCtx->PushEvent(event.Get<Gui::WindowMoveEvent>());
				break;
			case EventT::indexOf<Gui::WindowResizeEvent>:
				guiCtx->PushEvent(event.Get<Gui::WindowResizeEvent>());
				break;

			default:
				DENGINE_IMPL_UNREACHABLE();
				break;
		}
		InvalidateRendering();
	}
	queuedGuiEvents.clear();
}

void Editor::Context::ProcessEvents()
{
	EditorImpl& implData = this->ImplData();

	if (App::TickCount() == 1)
		implData.InvalidateRendering();
	
	implData.FlushQueuedEvents();

	for (auto viewportPtr : implData.viewportWidgets)
	{
		viewportPtr->Tick(Time::Delta());
	}
	if (App::TickCount() % 60 == 0)
	{
		std::string temp;
		//temp += std::to_string(1.f / Time::Delta()) + " - ";
		temp += std::to_string(Time::Delta());
		implData.test_fpsText->String_Set(temp.c_str());
		implData.InvalidateRendering();
	}
	if (App::TickCount() % 10 == 0)
	{
		if (implData.componentList && implData.GetSelectedEntity().HasValue())
		{
			implData.componentList->Tick(implData.GetActiveScene(), implData.GetSelectedEntity().Value());
			implData.InvalidateRendering();
		}
	}

	if (implData.RenderIsInvalidated())
	{
		implData.guiRenderingInvalidated = false;

		for (auto viewportPtr : implData.viewportWidgets)
		{
			viewportPtr->viewport->isVisible = false;
		}

		implData.vertices.clear();
		implData.indices.clear();
		implData.drawCmds.clear();
		implData.windowUpdates.clear();
		implData.guiCtx->Render(
			implData.vertices,
			implData.indices,
			implData.drawCmds,
			implData.windowUpdates);

		for (auto viewportPtr : implData.viewportWidgets)
		{
			if (viewportPtr->viewport->currentExtent == Gui::Extent{} && viewportPtr->viewport->newExtent != Gui::Extent{})
				viewportPtr->viewport->currentExtent = viewportPtr->viewport->newExtent;
		}
	}
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
	
	returnVal.vertices = implData.vertices;
	returnVal.indices = implData.indices;
	returnVal.drawCmds = implData.drawCmds;

	returnVal.windowUpdates = implData.windowUpdates;

	for (auto viewportWidgetPtr : implData.viewportWidgets)
	{
		DENGINE_IMPL_ASSERT(viewportWidgetPtr);
		auto& viewportWidget = *viewportWidgetPtr;
		if (viewportWidgetPtr->viewport->isVisible)
		{
			DENGINE_IMPL_ASSERT(viewportWidget.viewport->currentExtent.width > 0 && viewportWidget.viewport->currentExtent.height > 0);
			DENGINE_IMPL_ASSERT(viewportWidget.viewport->newExtent.width > 0 && viewportWidget.viewport->newExtent.height > 0);

			Gfx::ViewportUpdate update = viewportWidget.viewport->GetViewportUpdate(
				*this,
				returnVal.lineVertices,
				returnVal.lineDrawCmds);
			
			returnVal.viewportUpdates.push_back(update);
		}
	}

	return returnVal;
}

bool Editor::Context::IsSimulating() const
{
	EditorImpl& implData = this->ImplData();
	return implData.tempScene;
}

Scene& Editor::Context::GetActiveScene() const
{
	EditorImpl& implData = this->ImplData();
	return implData.GetActiveScene();
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

Scene const& Editor::EditorImpl::GetActiveScene() const
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
	for (u32 i = 0; i < baseCircleTriangleCount; i++)
	{
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