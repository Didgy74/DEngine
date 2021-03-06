#include "ViewportWidget.hpp"

#include "Editor.hpp"
#include "EditorImpl.hpp"
#include "ComponentWidgets.hpp"

#include <DEngine/Gui/ButtonGroup.hpp>
#include <DEngine/Gui/DockArea.hpp>
#include <DEngine/Gui/LineList.hpp>
#include <DEngine/Gui/ScrollArea.hpp>

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

namespace DEngine::Editor
{
	class EntityIdList : public Gui::StackLayout
	{
	public:
		Gui::LineList* entitiesList = nullptr;
		EditorImpl* editorImpl = nullptr;

		EntityIdList(EditorImpl* editorImpl) :
			editorImpl(editorImpl)
		{
			DENGINE_DETAIL_ASSERT(!editorImpl->entityIdList);
			editorImpl->entityIdList = this;

			this->direction = Direction::Vertical;

			Gui::StackLayout* topElementLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
			this->AddWidget(Std::Box<Gui::Widget>{ topElementLayout });

			Gui::Button* newEntityButton = new Gui::Button;
			topElementLayout->AddWidget(Std::Box<Gui::Widget>{ newEntityButton });
			newEntityButton->text = "New";
			newEntityButton->activatePfn = [this](
				Gui::Button& btn)
			{
				Entity newId = this->editorImpl->scene->NewEntity();
				AddEntityToList(newId);
			};

			Gui::Button* entityDeleteButton = new Gui::Button;
			topElementLayout->AddWidget(Std::Box<Gui::Widget>{ entityDeleteButton });
			entityDeleteButton->text = "Delete";
			entityDeleteButton->activatePfn = [this](
				Gui::Button& btn)
			{
				if (!this->editorImpl->GetSelectedEntity().HasValue())
					return;

				Entity selectedEntity = this->editorImpl->GetSelectedEntity().Value();

				RemoveEntityFromList(selectedEntity);

				this->editorImpl->UnselectEntity();

				this->editorImpl->scene->DeleteEntity(selectedEntity);
			};
			
			Gui::ScrollArea* entityListScrollArea = new Gui::ScrollArea();
			this->AddWidget(Std::Box<Gui::Widget>{ entityListScrollArea });

			entitiesList = new Gui::LineList();
			entityListScrollArea->widget = Std::Box<Gui::Widget>{ entitiesList };
			entitiesList->canSelect = true;
			entitiesList->selectedLineChangedCallback = [this](
				Gui::LineList& widget)
			{
				if (widget.selectedLine.HasValue())
				{
					auto lineText = widget.lines[widget.selectedLine.Value()].c_str();
					this->editorImpl->SelectEntity((Entity)std::atoi(lineText));
				}
			};

			auto entities = editorImpl->scene->GetEntities();
			for (uSize i = 0; i < entities.Size(); i += 1)
			{
				Entity entityId = entities[i];
				AddEntityToList(entityId);
				if (editorImpl->GetSelectedEntity().HasValue() && entityId == editorImpl->GetSelectedEntity().Value())
				{
					entitiesList->selectedLine = i;
				}
			}
		}

		virtual ~EntityIdList()
		{
			DENGINE_DETAIL_ASSERT(editorImpl->entityIdList == this);
			editorImpl->entityIdList = nullptr;
		}

		void AddEntityToList(Entity id)
		{
			entitiesList->lines.push_back(std::to_string((u64)id));
		}

		void RemoveEntityFromList(Entity id)
		{
			DENGINE_DETAIL_ASSERT(entitiesList->selectedLine.HasValue());
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
			DENGINE_DETAIL_ASSERT(newLine.HasValue());
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

		ComponentList(EditorImpl* inEditorImpl) :
			editorImpl(inEditorImpl)
		{
			DENGINE_DETAIL_ASSERT(!editorImpl->componentList);
			editorImpl->componentList = this;

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

		virtual ~ComponentList()
		{
			DENGINE_DETAIL_ASSERT(editorImpl->componentList == this);
			editorImpl->componentList = nullptr;
		}

		void EntitySelected(Entity id)
		{
			outerLayout->ClearChildren();
			
			moveWidget = new MoveWidget(editorImpl->scene, id);
			outerLayout->AddWidget(Std::Box<Gui::Widget>{ moveWidget });

			transformWidget = new TransformWidget(*editorImpl);
			outerLayout->AddWidget(Std::Box<Gui::Widget>{ transformWidget });

			spriteRendererWidget = new SpriteRenderer2DWidget(*editorImpl);
			outerLayout->AddWidget(Std::Box<Gui::Widget>{ spriteRendererWidget });

			box2DWidget = new RigidbodyWidget(*editorImpl);
			outerLayout->AddWidget(Std::Box<Gui::Widget>{ box2DWidget });
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
		EditorImpl& editorImpl)
	{
		// Menu button
		Gui::MenuBar* menuBarA = new Gui::MenuBar(Gui::MenuBar::Direction::Horizontal);

		menuBarA->stackLayout.spacing = 25;
		
		menuBarA->AddSubmenuButton(
			"Submenu",
			[&editorImpl](
				Gui::MenuBar& newMenuBar)
			{
				newMenuBar.stackLayout.color = { 0.25f, 0.f, 0.25f, 1.f };
				newMenuBar.stackLayout.spacing = 10;
				newMenuBar.stackLayout.padding = 10;

				
				newMenuBar.AddToggleMenuButton(
					"Entities",
					editorImpl.entityIdList != nullptr,
					[&editorImpl](
						bool activated)
					{
						if (activated)
						{
							editorImpl.InvalidateRendering();
							editorImpl.dockArea->AddWindow(
								"Entities",
								{ 0.5f, 0.5f, 0.f, 1.f },
								Std::Box<Gui::Widget>{ new EntityIdList(&editorImpl) });
						}
					});

				newMenuBar.AddToggleMenuButton(
					"Components",
					editorImpl.componentList != nullptr,
					[&editorImpl](
						bool activated)
					{
						if (activated)
						{
							editorImpl.InvalidateRendering();
							editorImpl.dockArea->AddWindow(
								"Components",
								{ 0.f, 0.5f, 0.5f, 1.f },
								Std::Box<Gui::Widget>{ new ComponentList(&editorImpl) });
						}
					});

				newMenuBar.AddMenuButton(
					"New viewport",
					[&editorImpl]()
					{
						editorImpl.InvalidateRendering();
						editorImpl.dockArea->AddWindow(
							"Viewport",
							{ 0.5f, 0.f, 0.5f, 1.f },
							Std::Box<Gui::Widget>{ new ViewportWidget(editorImpl, *editorImpl.gfxCtx) });
					});
			});

		// Delta time counter at the top
		Gui::Text* deltaText = new Gui::Text;
		menuBarA->stackLayout.AddWidget(Std::Box<Gui::Widget>{ deltaText });
		editorImpl.test_fpsText = deltaText;
		deltaText->String_Set("Child text");
		

		Gui::Button* playButton = new Gui::Button;
		menuBarA->stackLayout.AddWidget(Std::Box<Gui::Widget>{ playButton });
		playButton->text = "Play";
		playButton->type = Gui::Button::Type::Toggle;
		playButton->activatePfn = [&editorImpl](
			Gui::Button& btn)
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
		menuBarA->stackLayout.AddWidget(Std::Box<Gui::Widget>{ gizmoBtnGroup });
		gizmoBtnGroup->AddButton("Translate");
		gizmoBtnGroup->AddButton("Rotate");
		gizmoBtnGroup->AddButton("Scale");

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
	implData.guiCtx = Std::Box{ new Gui::Context(Gui::Context::Create(implData, gfxCtx)) };
	implData.gfxCtx = gfxCtx;
	implData.scene = scene;
	App::InsertEventInterface(implData);

	Gui::StackLayout* outmostLayout = implData.guiCtx->outerLayout;

	outmostLayout->AddWidget(CreateNavigationBar(implData));

	Gui::DockArea* dockArea = new Gui::DockArea;
	implData.dockArea = dockArea;
	outmostLayout->AddWidget(Std::Box<Gui::Widget>{ dockArea });

	implData.dockArea->AddWindow(
		"Entities",
		{ 0.5f, 0.5f, 0.f, 1.f },
		Std::Box<Gui::Widget>{ new EntityIdList(&implData) });

	implData.dockArea->AddWindow(
		"Components",
		{ 0.f, 0.5f, 0.5f, 1.f },
		Std::Box<Gui::Widget>{ new ComponentList(&implData) });
		
	implData.dockArea->AddWindow(
		"Viewport",
		{ 0.5f, 0.f, 0.5f, 1.f },
		Std::Box<Gui::Widget>{ new ViewportWidget(implData, *implData.gfxCtx) });
	
	return newCtx;
}

void Editor::Context::ProcessEvents()
{
	EditorImpl& implData = this->ImplData();

	if (App::TickCount() == 1)
		implData.InvalidateRendering();
	
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
		implData.InvalidateRendering();
	}
	implData.queuedGuiEvents.clear();

	for (auto viewportPtr : implData.viewportWidgets)
	{
		viewportPtr->TickTest(Time::Delta());
	}
	if (App::TickCount() % 60 == 0)
	{
		implData.test_fpsText->String_Set(std::to_string(Time::Delta()).c_str());
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
			viewportPtr->isVisible = false;
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
			if (viewportPtr->currentExtent == Gui::Extent{} && viewportPtr->newExtent != Gui::Extent{})
				viewportPtr->currentExtent = viewportPtr->newExtent;
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
		DENGINE_DETAIL_ASSERT(viewportWidgetPtr);
		auto& viewportWidget = *viewportWidgetPtr;
		if (viewportWidgetPtr->isVisible)
		{
			DENGINE_DETAIL_ASSERT(viewportWidget.currentExtent.width > 0 && viewportWidget.currentExtent.height > 0);
			DENGINE_DETAIL_ASSERT(viewportWidget.newExtent.width > 0 && viewportWidget.newExtent.height > 0);

			Gfx::ViewportUpdate update = viewportWidget.GetViewportUpdate(
				*this,
				returnVal.lineVertices,
				returnVal.lineDrawCmds);
			
			returnVal.viewportUpdates.push_back(update);
		}
	}

	return returnVal;
}

bool Editor::Context::CurrentlySimulating() const
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
	DENGINE_DETAIL_ASSERT(gizmoTypeBtnGroup->GetButtonCount() == (u8)GizmoType::COUNT);

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
	DENGINE_DETAIL_ASSERT(!tempScene);

	Scene* copyScene = new Scene;
	tempScene = Std::Box{ copyScene };
	scene->Copy(*copyScene);
	copyScene->Begin();
}

void Editor::EditorImpl::StopSimulating()
{
	DENGINE_DETAIL_ASSERT(tempScene);

	tempScene.Clear();
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

std::vector<Math::Vec3> Editor::BuildGizmoTorusMesh()
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
		Math::Vec3 a = temp * (outerRadius + innerRadius);
		Math::Vec3 b = temp * (outerRadius - innerRadius);

		temp = { Math::Cos(radianB), Math::Sin(radianB), 0.f };
		Math::Vec3 c = temp * (outerRadius + innerRadius);

		vertices.push_back(a);
		vertices.push_back(b);
		vertices.push_back(c);

		Math::Vec3 d = temp * (outerRadius - innerRadius);
		vertices.push_back(b);
		vertices.push_back(c);
		vertices.push_back(d);
	}

	return vertices;
}
