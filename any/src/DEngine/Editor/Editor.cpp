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

		explicit EntityIdList(EditorImpl& editorImpl) :
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
			newEntityButton->colors.normal =  Settings::GetColor(Settings::Color::Button_Normal);
			newEntityButton->colors.toggled = Settings::GetColor(Settings::Color::Button_Active);
			newEntityButton->textMargin = Editor::Settings::defaultTextMargin;
			newEntityButton->activateFn = [this](
				Gui::Button& btn)
			{
				auto newId = this->editorImpl->scene->NewEntity();
				AddEntityToList(newId);
			};

			auto entityDeleteButton = new Gui::Button;
			topElementLayout->AddWidget(Std::Box{ entityDeleteButton });
			entityDeleteButton->text = "Delete";
			entityDeleteButton->colors.normal = Settings::GetColor(Settings::Color::Button_Normal);
			entityDeleteButton->colors.toggled = Settings::GetColor(Settings::Color::Button_Active);
			entityDeleteButton->textMargin = Editor::Settings::defaultTextMargin;
			entityDeleteButton->activateFn = [this](
				Gui::Button& btn)
			{
				if (!this->editorImpl->GetSelectedEntity().HasValue())
					return;

				auto selectedEntity = this->editorImpl->GetSelectedEntity().Value();

				RemoveEntityFromList(selectedEntity);

				this->editorImpl->UnselectEntity();

				this->editorImpl->scene->DeleteEntity(selectedEntity);
			};

			auto deselectButton = new Gui::Button;
			topElementLayout->AddWidget(Std::Box{ deselectButton });
			deselectButton->text = "Deselect";
			deselectButton->colors.normal = Settings::GetColor(Settings::Color::Button_Normal);
			deselectButton->colors.toggled = Settings::GetColor(Settings::Color::Button_Active);
			deselectButton->textMargin = Editor::Settings::defaultTextMargin;
			deselectButton->activateFn = [this](
				Gui::Button& btn)
			{
				if (!this->editorImpl->GetSelectedEntity().HasValue())
					return;

				this->editorImpl->UnselectEntity();
			};

			auto entityListScrollArea = new Gui::ScrollArea;
			this->AddWidget(Std::Box<Gui::Widget>{ entityListScrollArea });
			entityListScrollArea->scrollbarInactiveColor = Settings::GetColor(Settings::Color::Scrollbar_Normal);

			entitiesList = new Gui::LineList;
			entityListScrollArea->child = Std::Box<Gui::Widget>{ entitiesList };
			entitiesList->textMargin = Settings::defaultTextMargin;
			entitiesList->selectedLineChangedFn = [&editorImpl](
				Gui::LineList& widget, Gui::Context* ctx)
			{
				if (widget.selectedLine.HasValue())
				{
					auto const& lineText = widget.lines[widget.selectedLine.Value()];
					editorImpl.SelectEntity_MidDispatch((Entity)std::atoi(lineText.c_str()), *ctx);
				}
				else
					editorImpl.UnselectEntity();
			};

			auto entities = editorImpl.scene->GetEntities();
			for (uSize i = 0; i < entities.Size(); i += 1)
			{
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
			submenuLine.toggled = false;
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

			auto& submenuLine = editorImpl->viewMenuButton->submenu.lines[(int)FileMenuEnum::Components];
			submenuLine.toggled = false;
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

		void EntitySelected_MidDispatch(Entity id, Gui::Context& ctx)
		{
			outerLayout->ClearChildren();

			auto job = [this](Gui::Context& ctx) {
				moveWidget = new MoveWidget(*editorImpl);
				outerLayout->AddWidget(Std::Box{ moveWidget });

				transformWidget = new TransformWidget(*editorImpl);
				outerLayout->AddWidget(Std::Box{ transformWidget });

				spriteRendererWidget = new SpriteRenderer2DWidget(*editorImpl);
				outerLayout->AddWidget(Std::Box{ spriteRendererWidget });

				box2DWidget = new RigidbodyWidget(*editorImpl);
				outerLayout->AddWidget(Std::Box{ box2DWidget });
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
			Gui::MenuButton::Line line {};
			line.title = "Entities";
			line.toggled = true;
			line.togglable = true;
			line.callback = [&editorImpl](Gui::MenuButton::Line& line, Gui::Context* ctx) {
				if (line.toggled)
				{
					auto job = [&editorImpl](Gui::Context& ctx) {
						editorImpl.dockArea->AddWindow(
							"Entities",
							Settings::GetColor(Settings::Color::Window_Entities),
							Std::Box{ new EntityIdList(editorImpl) });
					};
					ctx->PushPostEventJob(job);
				}
				line.toggled = true;
			};
			menuButton->submenu.lines[(int)FileMenuEnum::Entities] = Std::Move(line);
		}
		{
			// Create button for Components window
			Gui::MenuButton::Line line {};
			line.title = "Components";
			line.toggled = true;
			line.togglable = true;
			line.callback = [&editorImpl](Gui::MenuButton::Line& line, Gui::Context* ctx) {
				if (line.toggled)
				{
					auto job = [&editorImpl](Gui::Context& ctx) {
						editorImpl.dockArea->AddWindow(
							"Components",
							Settings::GetColor(Settings::Color::Window_Components),
							Std::Box{ new ComponentList(editorImpl) });
					};
					ctx->PushPostEventJob(job);
				}
				line.toggled = true;
			};
			menuButton->submenu.lines[(int)FileMenuEnum::Components] = Std::Move(line);
		}
		{
			// Create button for adding viewport
			Gui::MenuButton::Line line {};
			line.title = "New viewport";
			line.toggled = false;
			line.togglable = false;
			line.callback = [&editorImpl](Gui::MenuButton::Line& line, Gui::Context* ctx) {

				auto job = [&editorImpl](Gui::Context& ctx) {
					editorImpl.dockArea->AddWindow(
						"Viewport",
						Settings::GetColor(Settings::Color::Window_Viewport),
						Std::Box{ new ViewportWidget(editorImpl)});
				};

				ctx->PushPostEventJob(job);
			};
			menuButton->submenu.lines[(int)FileMenuEnum::NewViewport] = Std::Move(line);
		}

		// Delta time counter at the top
		auto deltaText = new Gui::Text;
		stackLayout->AddWidget(Std::Box{ deltaText });
		editorImpl.test_fpsText = deltaText;
		deltaText->margin = Editor::Settings::defaultTextMargin;
		deltaText->text = "Child text";
		deltaText->expandX = false;

		auto playButton = new Gui::Button;
		stackLayout->AddWidget(Std::Box{ playButton });
		playButton->colors.normal = Settings::GetColor(Settings::Color::Button_Normal);
		playButton->colors.toggled = Settings::GetColor(Settings::Color::Button_Active);
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

	implData.appCtx->InsertEventForwarder(implData);

	auto outmostLayout = new Gui::StackLayout(Gui::StackLayout::Dir::Vertical);

	outmostLayout->AddWidget(CreateNavigationBar(implData));

	auto dockArea = new Gui::DockArea;
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


	auto& guiWinHandler = implData;
	auto ctx = new Gui::Context(
		Gui::Context::Create(
			guiWinHandler,
			implData.appCtx,
			implData.gfxCtx));
	implData.guiCtx = Std::Box{ ctx };



	auto const clearColor = Settings::GetColor(Settings::Color::Background);
	implData.guiCtx->AdoptWindow(
		(Gui::WindowID)createInfo.mainWindow,
		clearColor,
		{ createInfo.windowPos, { createInfo.windowExtent.width, createInfo.windowExtent.height } },
		createInfo.windowSafeAreaOffset,
		{ createInfo.windowSafeAreaExtent.width, createInfo.windowSafeAreaExtent.height },
		Std::Box{ outmostLayout });

	return newCtx;
}

void Editor::Context::ProcessEvents()
{
	auto& implData = GetImplData();

	if (implData.appCtx->TickCount() == 1)
		implData.InvalidateRendering();

	implData.FlushQueuedEventsToGui();

	for (auto viewportPtr : implData.viewportWidgets)
	{
		viewportPtr->Tick(Time::Delta());
	}
	if (implData.appCtx->TickCount() % 60 == 0)
	{
		std::string temp;
		//temp += std::to_string(1.f / Time::Delta()) + " - ";
		temp += std::to_string(Time::Delta());
		implData.test_fpsText->text = temp;
		implData.InvalidateRendering();
	}
	if (implData.appCtx->TickCount() % 10 == 0)
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
		Gui::Context::Render2_Params renderParams {
			.rectCollection = implData.guiRectCollection,
			.transientAlloc = implData.guiTransientAlloc,
			.vertices = implData.vertices,
			.indices = implData.indices,
			.drawCmds = implData.drawCmds,
			.windowUpdates = implData.windowUpdates };
		implData.guiCtx->Render2(renderParams);

		for (auto viewportPtr : implData.viewportWidgets)
		{
			if (viewportPtr->viewport->currentExtent == Gui::Extent{} && viewportPtr->viewport->newExtent != Gui::Extent{})
				viewportPtr->viewport->currentExtent = viewportPtr->viewport->newExtent;
		}
	}
}

Editor::Context::Context(Context&& other) noexcept :
	m_implData{other.m_implData }
{
	other.m_implData = nullptr;
}

Editor::Context::~Context()
{
	if (this->m_implData)
	{
		auto& implData = GetImplData();

		implData.appCtx->RemoveEventForwarder(implData);

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