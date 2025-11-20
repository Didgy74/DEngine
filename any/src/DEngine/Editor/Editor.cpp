#include "Editor.hpp"
#include "EditorImpl.hpp"

#include "ViewportGizmo.hpp"

#include "ViewportWidget.hpp"

#include <DEngine/Gui/StdWidgets/ButtonGroup.hpp>
#include <DEngine/Gui/StdWidgets/StackLayout.hpp>

#include <DEngine/Time.hpp>
#include <DEngine/PlatformGuiGlue.hpp>

#include <DEngine/GfxGuiDrawEngineImpl.hpp>

#include <format>
#include <vector>

import DEngine.Math.Common;

using namespace DEngine;
using namespace DEngine::Editor;

Std::Array<Math::Vec4, (int) Settings::Color::COUNT> Settings::colorArray = Settings::BuildColorArray();

GuiTextEngineImpl& Editor::Context::GetTextEngineImpl() {
	auto &implData = GetImplData();
	return implData.m_textEngine;
}

namespace DEngine::Editor {
	enum class FileMenuEnum {
		Entities,
		Components,
		NewViewport,
		COUNT
	};
}

using namespace DEngine;

namespace DEngine::Editor {

}

Editor::Context Editor::Context::Create(
	CreateInfo const &createInfo)
{
	Context newCtx;

	newCtx.m_implData = new EditorImpl;
	EditorImpl& implData = *newCtx.m_implData;

	implData.appCtx = &createInfo.appCtx;
	implData.gfxCtx = &createInfo.gfxCtx;
	implData.scene = &createInfo.scene;
	implData.m_mainWindowExtent = { createInfo.newWindowInfo.extent.width, createInfo.newWindowInfo.extent.height };

	auto& guiWinHandler = implData;
	//ctx->fontScale = 3.f;
	implData.guiCtx = Std::BoxAdopt(new Gui::Context(Gui::Context::Create(guiWinHandler)));
	auto& guiCtx = *implData.guiCtx;

	auto guiWindowAdoptInfo = PlatformGuiGlue::MakeGuiWindowAdoptInfo(
		createInfo.newWindowInfo,
		 Settings::GetColor(Settings::Color::Background),
		 SetupUiRoot(implData));
	implData.guiCtx->AdoptWindow(Std::Move(guiWindowAdoptInfo));

	return newCtx;
}

static void runAccessibilityStuff(Editor::Context& ctx, EditorImpl& implData) {
	struct TempPusher : public Gui::Widget::AccessibilityInfoPusher {
		std::vector<char> text;
		std::vector<std::pair<u64, Gui::Widget::AccessibilityInfoElement>> elements;

		virtual int PushText(Std::Span<char const> in) override {
			int returnVal = (int) this->text.size();
			this->text.resize(this->text.size() + in.Size());
			std::memcpy(this->text.data() + returnVal, in.Data(), in.Size());
			return returnVal;
		}

		[[nodiscard]] bool ContainsId(u64 id) const {
			for (auto const& item : this->elements) {
				if (item.first == id)
					return true;
			}
			return false;
		}

		virtual void PushElement(u64 id, Gui::Widget::AccessibilityInfoElement const &in) override {
			DENGINE_IMPL_ASSERT(!this->ContainsId(id));
			this->elements.emplace_back(id, in);
		}
	};
	TempPusher pusher;

	implData.guiCtx->Event_Accessibility(
		Std::ConstAnyRef{ctx},
		implData.guiTransientAlloc,
		implData.guiRectCollection,
		implData.m_textEngine,
		pusher);

	implData.appCtx->UpdateAccessibilityData(
		(Platform::WindowID)0,
		{
			(int)pusher.elements.size(),
			[&](auto index) {
				auto const& id = pusher.elements[index].first;
				auto const& item = pusher.elements[index].second;
				Platform::AccessibilityUpdateElement out = {};
				out.widgetId = id;
				out.posX = item.rect.position.x;
				out.posY = item.rect.position.y;
				out.width = item.rect.extent.width;
				out.height = item.rect.extent.height;
				out.textStart = item.textStart;
				out.textCount = item.textCount;
				out.isClickable = item.isClickable;
				return out;
			}},
		{ pusher.text.data(), pusher.text.size() });
}

void Editor::Context::ProcessEvents(float deltaTime) {
	auto &implData = GetImplData();

	if (implData.appCtx->TickCount() == 1)
		implData.InvalidateRendering();
	for (auto viewportPtr: implData.viewportWidgetPtrs) {
		viewportPtr->Tick(Time::Delta());
	}
	if (implData.appCtx->TickCount() % 60 == 0) {
		implData.deltaTime = deltaTime;
		implData.InvalidateRendering();
	}
	if (implData.appCtx->TickCount() % 10 == 0) {
		if (implData.componentList && implData.GetSelectedEntity().HasValue()) {
			implData.componentList->Tick(implData.GetActiveScene(),
										 implData.GetSelectedEntity().Value());
			implData.InvalidateRendering();
		}
	}

	auto guiEventsHappened = FlushQueuedEventsToGui();

	if (implData.RenderIsInvalidated()) {
		implData.guiRenderingInvalidated = false;

		//runAccessibilityStuff(*this, implData);

		for (auto viewportPtr: implData.viewportWidgetPtrs) {
			viewportPtr->GetInternalViewport().wasRendered = false;
		}

		implData.vertices.clear();
		implData.indices.clear();
		implData.drawCmds.clear();
		implData.windowUpdates.clear();
		implData.utfValues.clear();
		implData.textGlyphRects.clear();

		Gfx::NativeWindowUpdate nativeWindowUpdate = {};
		nativeWindowUpdate.clearColor = { 0.1f, 0.1f, 0.1f, 1.f };
		nativeWindowUpdate.drawCmdOffset = 0;
		nativeWindowUpdate.id = (Gfx::NativeWindowID)0;

		auto drawInfo = GfxGuiDrawEngineImpl{
			implData.vertices,
			implData.indices,
			implData.drawCmds,
			implData.utfValues,
			implData.textGlyphRects,
			implData.m_mainWindowExtent,
			{}, };

		implData.renderResultData.viewports.clear();

		EditorGuiAppData renderData { *this, implData.renderResultData, };

		Gui::Context::RenderWindowParams renderParams = {
			.appData = Std::ConstAnyRef{ renderData },
			.drawEngine = drawInfo,
			.rectCollection = implData.guiRectCollection,
			.textEngine = implData.m_textEngine,
			.transientAlloc = implData.guiTransientAlloc,
			.windowId = (Gui::WindowID)0, };
		implData.guiCtx->RenderWindow(renderParams);

		nativeWindowUpdate.drawCmdCount = implData.drawCmds.size();
		implData.windowUpdates.push_back(nativeWindowUpdate);

		// We want to see if our viewports were rendered, and update them if they were.
		for (auto viewportPtr: implData.viewportWidgetPtrs) {
			DENGINE_IMPL_ASSERT(viewportPtr);

			auto const& rectColl = implData.guiRectCollection;
			auto& viewport = viewportPtr->GetInternalViewport();

			auto entryOpt = rectColl.GetEntry(viewport);

			if (entryOpt.HasValue()) {
				// This means the viewport was rendered
				auto const& entry = entryOpt.Value();
				auto const& rectPair = rectColl.GetRect(entry);
				auto const visibleIntersection = Gui::Intersection(rectPair.widgetRect,
																   rectPair.visibleRect);
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

Editor::Context::Context(Context &&other) noexcept:
	m_implData{ other.m_implData }
{
	other.m_implData = nullptr;
}

Editor::Context::~Context() {
	if (this->m_implData) {
		auto& implData = GetImplData();
		delete &implData;
	}
}

Editor::Theme const& Editor::Context::GetTheme() const {
	return this->GetImplData().m_theme;
}

Editor::Theme& Editor::Context::GetTheme() {
	return this->GetImplData().m_theme;
}

Editor::DrawInfo Editor::Context::GetDrawInfo() const {
	auto &implData = this->GetImplData();

	DrawInfo returnVal;

	returnVal.vertices = implData.vertices;
	returnVal.indices = implData.indices;
	returnVal.drawCmds = implData.drawCmds;
	returnVal.textGlyphRects = implData.textGlyphRects;
	returnVal.utfValues = implData.utfValues;

	returnVal.windowUpdates = implData.windowUpdates;

	for (auto viewportWidgetPtr : implData.viewportWidgetPtrs) {
		DENGINE_IMPL_ASSERT(viewportWidgetPtr);
		auto const &viewport = viewportWidgetPtr->GetInternalViewport();
		if (viewport.wasRendered && !viewport.currentExtent.IsNothing()) {
			Gfx::ViewportUpdate update = viewport.BuildViewportUpdate(
					returnVal.lineVertices,
					returnVal.lineDrawCmds);

			returnVal.viewportUpdates.push_back(update);
		}
	}

	return returnVal;
}

bool Editor::Context::IsSimulating() const {
	auto &implData = GetImplData();
	return implData.tempScene.Get();
}

Scene &Editor::Context::GetActiveScene() {
	auto &implData = this->GetImplData();
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

void Editor::EditorImpl::SelectEntity(Entity id) {
	if (selectedEntity.HasValue() && selectedEntity.Value() == id) {
		return;
	}

	Std::Opt<Entity> prevEntity = selectedEntity;
	selectedEntity = id;

	// Update the entity list
	if (entityIdList != nullptr) {
		entityIdList->SelectEntity(prevEntity, id);
	}


	// Update the component list
	if (componentList != nullptr) {
		componentList->EntitySelected(id);
	}

}

void EditorImpl::SelectEntity_MidDispatch(Entity id, Gui::Widget::WidgetEvent_DeferredJobQueue& jobQueue) {
	if (selectedEntity.HasValue() && selectedEntity.Value() == id) {
		return;
	}

	Std::Opt<Entity> prevEntity = selectedEntity;
	selectedEntity = id;

	// Update the component list
	if (componentList != nullptr) {
		componentList->EntitySelected_MidDispatch(id, jobQueue);
	}
}

void Editor::EditorImpl::UnselectEntity() {
	// Update the entity list
	if (selectedEntity.HasValue() && entityIdList) {
		entityIdList->UnselectEntity();
	}


	// Clear the component list
	if (componentList) {
		componentList->outerLayout->ClearChildren();
	}

	selectedEntity = Std::nullOpt;
}

Editor::GizmoType Editor::EditorImpl::GetCurrentGizmoType() const {
	DENGINE_IMPL_ASSERT(gizmoTypeBtnGroup->GetButtonCount() == (u8) GizmoType::COUNT);

	return (GizmoType)gizmoTypeBtnGroup->GetActiveButtonIndex();
}

Scene &Editor::EditorImpl::GetActiveScene() {
	if (tempScene)
		return *tempScene;
	return *scene;
}

void Editor::EditorImpl::BeginSimulating() {
	DENGINE_IMPL_ASSERT(!tempScene);

	Scene *copyScene = new Scene;
	tempScene = Std::Box{copyScene};
	scene->Copy(*copyScene);
	copyScene->Begin();
}

void Editor::EditorImpl::StopSimulating() {
	DENGINE_IMPL_ASSERT(tempScene);
	tempScene.Clear();
}

std::vector<Math::Vec3> Editor::BuildGizmoArrowMesh3D() {
	auto const arrow = ViewportGizmo::defaultArrow;

	f32 arrowShaftRadius = arrow.shaftDiameter / 2.f;
	f32 arrowCapRadius = arrow.capDiameter / 2.f;

	u32 subdivisions = 4;
	// We need atleast 2 subdivisons so we can atleast get a diamond
	DENGINE_IMPL_ASSERT(subdivisions > 1);
	u32 baseCircleTriangleCount = (subdivisions * 2);

	std::vector<Math::Vec3> vertices;
	for (u32 i = 0; i < baseCircleTriangleCount; i++) {
		f32 currentRadiansA = 2 * Math::pi / baseCircleTriangleCount * i;
		f32 currentRadiansB =
			2 * Math::pi / baseCircleTriangleCount * ((i + 1) % baseCircleTriangleCount);

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
			Math::Vec3 shaftTopVertA = {shaftBaseVertA.x, shaftBaseVertA.y,
										shaftBaseVertA.z + arrow.shaftLength};
			Math::Vec3 shaftTopVertB = {shaftBaseVertB.x, shaftBaseVertB.y,
										shaftBaseVertB.z + arrow.shaftLength};
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

std::vector<Math::Vec3> Editor::BuildGizmoTranslateArrowMesh2D() {
	std::vector<Math::Vec3> vertices;

	// Make quad for the base
	constexpr auto arrow = ViewportGizmo::defaultArrow;

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

std::vector<Math::Vec3> Editor::BuildGizmoTorusMesh2D() {
	std::vector<Math::Vec3> vertices;

	f32 const innerRadius = ViewportGizmo::defaultRotateCircleInnerRadius;
	f32 const outerRadius = ViewportGizmo::defaultRotateCircleOuterRadius;
	u32 const outerCount = 64;

	// Outer circle
	for (u32 i = 0; i < outerCount; i += 1) {
		f32 radianA = 2 * Math::pi / outerCount * i;
		f32 radianB = 2 * Math::pi / outerCount * (i + 1);

		Math::Vec3 temp = {Math::Cos(radianA), Math::Sin(radianA), 0.f};
		auto a = temp * (outerRadius + innerRadius);
		auto b = temp * (outerRadius - innerRadius);

		temp = {Math::Cos(radianB), Math::Sin(radianB), 0.f};
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

std::vector<Math::Vec3> Editor::BuildGizmoScaleArrowMesh2D() {
	std::vector<Math::Vec3> vertices;

	// Make quad for the base
	constexpr auto arrow = ViewportGizmo::defaultArrow;

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