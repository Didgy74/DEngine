#include <DEngine/Gui/DockArea.hpp>

#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include "Impl.hpp"

#include <new>

using namespace DEngine;
using namespace DEngine::Gui;


constexpr float gizmoOffsetFactor = 0.25f;

void Gui::impl::DA_PushLayerToFront(DockArea& dockArea, uSize indexToPush)
{
	auto& layers = DockArea::Impl::GetLayers(dockArea);

	DENGINE_IMPL_GUI_ASSERT(indexToPush < layers.size());
	if (indexToPush == 0)
		return;

	// First remove the element and store it in a temporary
	auto temp = Std::Move(layers[indexToPush]);
	layers.erase(layers.begin() + indexToPush);

	// Insert it at front
	layers.insert(layers.begin(), Std::Move(temp));
}

int Gui::impl::FindLayerIndex(
	DockArea const& dockArea,
	DA_Node const* targetWindow,
	AllocRef transientAlloc)
{
	for (auto const& layerIt : DA_BuildLayerItPair(dockArea)) {
		for (auto const& nodeIt : DA_BuildNodeItPair(layerIt.node, transientAlloc))
		{
			if (targetWindow == &nodeIt.node)
				return layerIt.layerIndex;
		}
	}
	DENGINE_IMPL_GUI_UNREACHABLE();
	return 0;
}

auto Gui::impl::DA_ToInnerLayoutGizmo(DA_OuterLayoutGizmo in) noexcept -> DA_InnerDockingGizmo
{
	switch (in)
	{
		case DA_OuterLayoutGizmo::Top: return DA_InnerDockingGizmo::Top;
		case DA_OuterLayoutGizmo::Bottom: return DA_InnerDockingGizmo::Bottom;
		case DA_OuterLayoutGizmo::Left: return DA_InnerDockingGizmo::Left;
		case DA_OuterLayoutGizmo::Right: return DA_InnerDockingGizmo::Right;
		default:
			DENGINE_IMPL_UNREACHABLE();
			return {};
	}
}

Rect Gui::impl::DA_BuildOuterLayoutGizmoRect(
	Rect layerRect,
	DA_OuterLayoutGizmo in,
	Extent const& gizmoSize) noexcept
{
	Rect returnVal = {};
	returnVal.position = layerRect.position;
	returnVal.extent = gizmoSize;
	auto offsetX = (int)Math::Floor((f32)gizmoSize.width * gizmoOffsetFactor);
	auto offsetY = (int)Math::Floor((f32)gizmoSize.height * gizmoOffsetFactor);

	switch (in) {
		case DA_OuterLayoutGizmo::Top:
			returnVal.position.x += (int)layerRect.extent.width / 2 - (int)gizmoSize.width / 2;
			returnVal.position.y += offsetY;
			break;
		case DA_OuterLayoutGizmo::Bottom:
			returnVal.position.x += (int)layerRect.extent.width / 2 - (int)gizmoSize.width / 2;
			returnVal.position.y += (int)layerRect.extent.height - (int)gizmoSize.height;
			returnVal.position.y -= offsetY;
			break;
		case DA_OuterLayoutGizmo::Left:
			returnVal.position.x += offsetX;
			returnVal.position.y += (int)layerRect.extent.height / 2 - (int)gizmoSize.height / 2;
			break;
		case DA_OuterLayoutGizmo::Right:
			returnVal.position.x += (int)layerRect.extent.width - (int)gizmoSize.width;
			returnVal.position.x -= offsetX;
			returnVal.position.y += (int)layerRect.extent.height / 2 - (int)gizmoSize.height / 2;
			break;
		default:
			DENGINE_IMPL_UNREACHABLE();
			break;
	}
	return returnVal;
}

namespace DEngine::Gui::impl
{
	Std::Array<Rect, 2> DA_BuildSplitNodeChildRects(
		Rect nodeRect,
		f32 splitOffset,
		DA_SplitNode::Direction dir) noexcept
	{
		bool const isHoriz = dir == DA_SplitNode::Direction::Horizontal;
		u32 const nodeExtent = isHoriz ? nodeRect.extent.width : nodeRect.extent.height;

		Std::Array<Rect, 2> returnVal = {};

		Rect childRect = nodeRect;

		u32& childExtent = isHoriz ? childRect.extent.width : childRect.extent.height;
		i32& childPos = isHoriz ? childRect.position.x : childRect.position.y;

		childExtent = u32(nodeExtent * splitOffset);
		returnVal[0] = childRect;

		childPos += childExtent;
		childExtent = nodeExtent - childExtent;
		returnVal[1] = childRect;

		return returnVal;
	}
}

Std::Array<Rect, 2> Gui::impl::DA_SplitNode::BuildChildRects(Rect const& nodeRect) const {
	return DA_BuildSplitNodeChildRects(nodeRect, split, dir);
}

auto Gui::impl::DA_CheckHitOuterLayoutGizmo(
	Rect const& nodeRect,
	Extent const& gizmoSize,
	Math::Vec2 point) noexcept -> Std::Opt<DA_OuterLayoutGizmo>
{
	Std::Opt<DA_OuterLayoutGizmo> gizmoHit;
	for (int i = 0; i < (int)DA_OuterLayoutGizmo::COUNT; i += 1) {
		auto gizmo = (DA_OuterLayoutGizmo)i;
		Rect gizmoRect = DA_BuildOuterLayoutGizmoRect(nodeRect, gizmo, gizmoSize);
		if (gizmoRect.PointIsInside(point)) {
			gizmoHit = gizmo;
			break;
		}
	}
	return gizmoHit;
}

Rect Gui::impl::DA_BuildDeleteGizmoRect(
	Rect widgetRect,
	Extent const& gizmoSize)
{
	Rect returnVal = widgetRect;
	returnVal.extent = gizmoSize;
	returnVal.position.x += (i32)widgetRect.extent.width / 2 - (i32)returnVal.extent.width / 2;
	returnVal.position.y += (i32)widgetRect.extent.height - (i32)returnVal.extent.height * 2;
	returnVal.position.y -= (int)Math::Floor((f32)gizmoSize.height * gizmoOffsetFactor) * 2;
	return returnVal;
}

bool Gui::impl::DA_CheckHitDeleteGizmo(
	Rect widgetRect,
	Extent const& gizmoSize,
	Math::Vec2 point) noexcept
{
	return DA_BuildDeleteGizmoRect(widgetRect, gizmoSize).PointIsInside(point);
}

Rect Gui::impl::DA_BuildInnerDockingGizmoRect(
	Rect const& nodeRect,
	DA_InnerDockingGizmo in,
	Extent const& gizmoSize) noexcept
{
	// We initialize it with the top-left corner of the center gizmo
	Rect returnVal = {};
	returnVal.extent = gizmoSize;
	returnVal.position = nodeRect.position;
	returnVal.position.x += (i32)nodeRect.extent.width / 2 - (i32)returnVal.extent.width / 2;
	returnVal.position.y += (i32)nodeRect.extent.height / 2 - (i32)returnVal.extent.height / 2;

	auto offsetX = (int)Math::Floor((f32)gizmoSize.width * gizmoOffsetFactor);
	auto offsetY = (int)Math::Floor((f32)gizmoSize.height * gizmoOffsetFactor);

	switch (in)
	{
		case DA_InnerDockingGizmo::Center:
			// Do nothing
			break;
		case DA_InnerDockingGizmo::Top:
			returnVal.position.y -= (i32)returnVal.extent.height;
			returnVal.position.y -= offsetY;
			break;
		case DA_InnerDockingGizmo::Bottom:
			returnVal.position.y += (i32)returnVal.extent.height;
			returnVal.position.y += offsetY;
			break;
		case DA_InnerDockingGizmo::Left:
			returnVal.position.x -= (i32)returnVal.extent.width;
			returnVal.position.x -= offsetX;
			break;
		case DA_InnerDockingGizmo::Right:
			returnVal.position.x += (i32)returnVal.extent.width;
			returnVal.position.x += offsetX;
			break;
		default:
			DENGINE_IMPL_UNREACHABLE();
			break;
	}
	return returnVal;
}

auto Gui::impl::DA_CheckHitInnerDockingGizmo(
	Rect const& nodeRect,
	Extent const& gizmoSize,
	Math::Vec2 const& point) noexcept
		-> Std::Opt<DA_InnerDockingGizmo>
{
	Std::Opt<DA_InnerDockingGizmo> gizmoHit;
	for (int i = {}; i < (int)DA_InnerDockingGizmo::COUNT; i += 1) {
		auto gizmo = (DA_InnerDockingGizmo)i;
		Rect gizmoRect = DA_BuildInnerDockingGizmoRect(nodeRect, gizmo, gizmoSize);
		if (gizmoRect.PointIsInside(point)) {
			gizmoHit = gizmo;
			break;
		}
	}
	return gizmoHit;
}

auto Gui::impl::DA_Layer_CheckHitInnerDockingGizmo(
	DA_Node& rootNode,
	Rect const& layerRect,
	Extent const& gizmoSize,
	Math::Vec2 point,
	AllocRef transientAlloc)
		-> DA_Layer_CheckHitInnerDockingGizmo_ReturnT
{
	for (auto const& nodeIt : DA_BuildNodeItPair(rootNode, layerRect, transientAlloc))
	{
		if (nodeIt.node.GetNodeType() != NodeType::Window)
			continue;
		auto& node = static_cast<DA_WindowNode&>(nodeIt.node);

		auto& nodeRect = nodeIt.rect;
		auto hitInnerGizmoOpt = DA_CheckHitInnerDockingGizmo(nodeRect, gizmoSize, point);
		if (hitInnerGizmoOpt.HasValue()) {
			DA_Layer_CheckHitInnerDockingGizmo_ReturnT returnValue = {};
			returnValue.node = &node;
			returnValue.gizmo = hitInnerGizmoOpt.Value();
			return returnValue;
		}
	}
	return {};
}

Rect Gui::impl::DA_BuildDockingHighlightRect(
	Rect nodeRect,
	DA_InnerDockingGizmo gizmo)
{
	Rect returnVal = nodeRect;
	switch (gizmo) {
		case DA_InnerDockingGizmo::Top:
			returnVal.extent.height = nodeRect.extent.height / 2;
			break;
		case DA_InnerDockingGizmo::Left:
			returnVal.extent.width = nodeRect.extent.width / 2;
			break;
		case DA_InnerDockingGizmo::Bottom:
			returnVal.extent.height = nodeRect.extent.height / 2;
			returnVal.position.y += (i32)nodeRect.extent.height / 2;
			break;
		case DA_InnerDockingGizmo::Right:
			returnVal.extent.width = nodeRect.extent.width / 2;
			returnVal.position.x += (i32)nodeRect.extent.width / 2;
			break;
		case DA_InnerDockingGizmo::Center:
			break;
		default:
			DENGINE_IMPL_UNREACHABLE();
			break;
	}
	return returnVal;
}

Rect Gui::impl::DA_BuildDockingHighlightRect(
	Rect nodeRect,
	DA_OuterLayoutGizmo gizmo)
{
	return DA_BuildDockingHighlightRect(
		nodeRect,
		DA_ToInnerLayoutGizmo(gizmo));
}

Rect Gui::impl::DA_BuildSplitNodeResizeHandleRect(
	Rect nodeRect,
	u32 thickness,
	u32 length,
	f32 splitOffset,
	DA_SplitNode::Direction dir) noexcept
{
	bool const isHoriz = dir == DA_SplitNode::Direction::Horizontal;

	Rect returnVal = nodeRect;
	returnVal.extent.width = isHoriz ? thickness : length;
	returnVal.extent.height = isHoriz ? length : thickness;

	if (isHoriz) {
		returnVal.position.x += (i32)Math::Round((f32)nodeRect.extent.width * splitOffset);
		returnVal.position.x -= (i32)returnVal.extent.width / 2;
		returnVal.position.y += (i32)nodeRect.extent.height / 2;
		returnVal.position.y -= (i32)returnVal.extent.height / 2;
	} else {
		returnVal.position.x += (i32)nodeRect.extent.width / 2;
		returnVal.position.x -= (i32)returnVal.extent.width / 2;
		returnVal.position.y += (i32)Math::Round((f32)nodeRect.extent.height * splitOffset);
		returnVal.position.y -= (i32)returnVal.extent.height / 2;
	}

	return returnVal;
}

auto Gui::impl::DA_BuildLayerItPair(DockArea& dockArea) noexcept -> DA_LayerItPair
{
	DA_LayerItPair returnValue {
		.dockArea = dockArea,};
	returnValue.startIndex = 0;
	returnValue.endIndex = DockArea::Impl::GetLayers(dockArea).size();
	return returnValue;
}

auto Gui::impl::DA_BuildLayerItPair(DockArea const& dockArea) noexcept -> DA_LayerConstItPair
{
	DA_LayerConstItPair returnValue = {
		.dockArea = dockArea, };
	returnValue.startIndex = 0;
	returnValue.endIndex = DockArea::Impl::GetLayers(dockArea).size();
	return returnValue;
}

Rect Gui::impl::DA_BuildLayerRect(
	Std::Span<DA_Layer const> layers,
	uSize layerIndex,
	Rect const& widgetRect)
{
	DENGINE_IMPL_GUI_ASSERT(layerIndex < layers.Size());

	// If we're on the rear-most layer, we want it to have
	// the full widget size.
	Rect returnValue = widgetRect;
	if (layers.Size() > 1 && layerIndex < layers.Size() - 1)
	{
		returnValue = layers[layerIndex].rect;
		returnValue.position += widgetRect.position;
	}

	// Clamp position to widgetRects size
	returnValue.position.x = Math::Min(
		returnValue.position.x,
		widgetRect.Right() - (i32)returnValue.extent.width);
	returnValue.position.x = Math::Max(
		returnValue.position.x,
		widgetRect.Left());

	returnValue.position.y = Math::Min(
		returnValue.position.y,
		widgetRect.Bottom() - (i32)returnValue.extent.height);
	returnValue.position.y = Math::Max(
		returnValue.position.y,
		widgetRect.Top());

	return returnValue;
}

auto Gui::impl::DA_FindParentsPtrToNode(
	DockArea& dockArea,
	DA_Node const& targetNode,
	AllocRef transientAlloc) -> DA_Node**
{
	for (auto const& layerIt : DA_BuildLayerItPair(dockArea)) {
		for (auto const& nodeIt : DA_BuildNodeItPair(layerIt.ppParentPtrToNode, transientAlloc)) {
			if (&nodeIt.node == &targetNode)
				return nodeIt.parentPtrToNode;
		}
	}
	return nullptr;
}

namespace DEngine::Gui::impl
{
	void DA_DockNode_HandleSplitNodeDocking_Inner(
		DA_Node** targetNodeParentPtr,
		DA_Node* nodeToDock,
		DA_InnerDockingGizmo gizmo)
	{
		DENGINE_IMPL_GUI_ASSERT(gizmo != DA_InnerDockingGizmo::Center);

		auto* existingNode = *targetNodeParentPtr;
		auto* newSplit = new DA_SplitNode;
		(*targetNodeParentPtr) = newSplit;

		// Figure out if our split should be horizontal or vertical.
		if (gizmo == DA_InnerDockingGizmo::Left || gizmo == DA_InnerDockingGizmo::Right)
			newSplit->dir = DA_SplitNode::Direction::Horizontal;
		else
			newSplit->dir = DA_SplitNode::Direction::Vertical;

		auto const dockInA =
			gizmo == DA_InnerDockingGizmo::Left ||
			gizmo == DA_InnerDockingGizmo::Top;
		if (dockInA)
		{
			newSplit->a = nodeToDock;
			newSplit->b = existingNode;
		}
		else
		{
			newSplit->a = existingNode;
			newSplit->b = nodeToDock;
		}
	}

	void DA_DockNode_HandleSplitNodeCase_OuterLayout(
		DockArea& dockArea,
		DA_WindowNode& nodeToDockInto,
		DA_OuterLayoutGizmo gizmo)
	{
		auto& layers = DA_GetLayers(dockArea);

		auto* nodeToDock = layers.front().root;
		layers.front().root = nullptr;
		layers.erase(layers.begin());

		auto** ppTargetNodeParentPtr = &layers.back().root;

		DA_DockNode_HandleSplitNodeDocking_Inner(
			ppTargetNodeParentPtr,
			nodeToDock,
			DA_ToInnerLayoutGizmo(gizmo));
	}

	void DA_DockNode_HandleSplitNodeCase(
		DockArea& dockArea,
		DA_WindowNode& nodeToDockInto,
		DA_InnerDockingGizmo gizmo,
		AllocRef transientAlloc)
	{
		DENGINE_IMPL_GUI_ASSERT(gizmo != DA_InnerDockingGizmo::Center);

		auto& layers = DA_GetLayers(dockArea);
		auto* nodeToDock = layers.front().root;
		layers.front().root = nullptr;
		layers.erase(layers.begin());

		auto** ppTargetNodeParentPtr = DA_FindParentsPtrToNode(
			dockArea,
			nodeToDockInto,
			transientAlloc);
		DENGINE_IMPL_GUI_ASSERT(ppTargetNodeParentPtr);

		DA_DockNode_HandleSplitNodeDocking_Inner(
			ppTargetNodeParentPtr,
			nodeToDock,
			gizmo);
	}
}

void Gui::impl::DockNode(
	DockArea& dockArea,
	DockingJob const& dockingJob,
	AllocRef transientAlloc)
{
	auto& layers = DA_GetLayers(dockArea);

	if (dockingJob.dockIntoOuter)
	{
		DA_DockNode_HandleSplitNodeCase_OuterLayout(
			dockArea,
			*dockingJob.targetNode,
			dockingJob.outerGizmo);
	}
	else
	{
		// Search for the target node that we wanted to dock in
		DENGINE_IMPL_GUI_ASSERT(dockingJob.targetNode);
		auto& targetNode = *dockingJob.targetNode;

		if (dockingJob.innerGizmo != impl::DA_InnerDockingGizmo::Center)
		{
			DA_DockNode_HandleSplitNodeCase(
				dockArea,
				*dockingJob.targetNode,
				dockingJob.innerGizmo,
				transientAlloc);
		}
		else // Docking is in the center.
		{
			auto currFrontLayer = Std::Move(layers[0]);
			layers.erase(layers.begin());

			// Iterate over all the window nodes of the front layer and move the tabs into the target window node.
			for (auto const& nodeIt : DA_BuildNodeItPair(*currFrontLayer.root, transientAlloc))
			{
				if (nodeIt.node.GetNodeType() != NodeType::Window)
					continue;
				auto& oldWindowNode = static_cast<DA_WindowNode&>(nodeIt.node);
				for (auto& tab : oldWindowNode.tabs)
					targetNode.tabs.emplace_back(Std::Move(tab));
				oldWindowNode.tabs.clear();
			}
			delete currFrontLayer.root;
			currFrontLayer.root = nullptr;
		}
	}
}

auto Gui::impl::DA_WindowNode_BuildPrimaryRects(
	Rect const& nodeRect,
	u32 totalTabHeight) -> DA_WindowNodePrimaryRects
{
	DA_WindowNodePrimaryRects returnValue = {};
	returnValue.titlebarRect = nodeRect;
	returnValue.titlebarRect.extent.height = totalTabHeight;

	returnValue.contentRect = nodeRect;
	returnValue.contentRect.position.y += (i32)returnValue.titlebarRect.extent.height;

	returnValue.contentRect.extent.height = (u32)Math::Max(
		0,
		(i32)returnValue.contentRect.extent.height - (i32)returnValue.titlebarRect.extent.height);

	return returnValue;
}

Rect Gui::impl::DA_Layer_FindWindowNodeRect(
	DA_Node const& rootNode,
	Rect const& layerRect,
	DA_WindowNode const* targetNode,
	AllocRef transientAlloc)
{
	for (auto const& nodeIt : DA_BuildNodeItPair(
		rootNode,
		layerRect,
		transientAlloc))
	{
		if (nodeIt.node.GetNodeType() != NodeType::Window)
			continue;
		if (&nodeIt.node == targetNode)
			return nodeIt.rect;
	}
	DENGINE_IMPL_GUI_UNREACHABLE();
	return {};
}

Rect Gui::impl::DA_FindNodeRect(
	DockArea const& dockArea,
	Rect const& widgetRect,
	DA_Node const* targetNode,
	AllocRef transientAlloc)
{
	auto const& layers = DA_GetLayers(dockArea);
	for (auto const& layerIt : DA_BuildLayerItPair(dockArea))
	{
		auto const layerRect = DA_BuildLayerRect(
			{ layers.data(), layers.size() },
			layerIt.layerIndex,
			widgetRect);
		for (auto const& nodeIt : DA_BuildNodeItPair(
			layerIt.node,
			layerRect,
			transientAlloc))
		{
			if (&nodeIt.node == targetNode)
				return nodeIt.rect;
		}
	}

	DENGINE_IMPL_GUI_UNREACHABLE();
	return {};
}

auto Gui::impl::CheckHitWindowNode(
	DA_Node& rootNode,
	Rect const& layerRect,
	Math::Vec2 point,
	AllocRef transientAlloc)
		-> CheckHitWindowNode_ReturnT
{
	// Check if we are hitting any window nodes on this layer
	for (auto const& nodeIt : DA_BuildNodeItPair(rootNode, layerRect, transientAlloc))
	{
		if (nodeIt.node.GetNodeType() != NodeType::Window)
			continue;
		auto& node = static_cast<DA_WindowNode&>(nodeIt.node);
		if (nodeIt.rect.PointIsInside(point))
			return { &node, nodeIt.rect };
	}

	return {};
}

Std::Vec<Rect, AllocRef> Gui::impl::DA_WindowNode_BuildTabRects(
	Rect const& titlebarRect,
	Std::Span<DA_WindowTab const> tabs,
	FontFaceSizeId fontSizeId,
	int horiMarginAmount,
	TextManager& textManager,
	AllocRef transientAlloc)
{
	int const tabCount = (int) tabs.Size();

	// First gather all the desired widths.
	auto desiredWidths = Std::NewVec<int>(transientAlloc);
	desiredWidths.Resize(tabCount);
	for (int i = 0 ; i < tabCount ; i += 1) {
		auto const& text = tabs[i].title;
		auto& desiredWidth = desiredWidths[i];

		auto outerExtent = textManager.GetOuterExtent(
			{ text.data(), text.size() },
			fontSizeId);
		desiredWidth = (int)outerExtent.width + 2 * horiMarginAmount;
	}

	// Then figure out the sum of the desired widths.
	int sumDesired = 0;
	for (int i = 0 ; i < tabCount ; i += 1)
		sumDesired += (int)desiredWidths[i];

	auto const sizeRatio = (f32) titlebarRect.extent.width / (f32) sumDesired;

	auto rects = Std::NewVec<Rect>(transientAlloc);
	rects.Resize(tabCount);
	int remainingWidth = (int)titlebarRect.extent.width;
	int horizontalPosOffset = 0;
	for (int i = 0 ; i < tabCount ; i += 1) {
		// Set the width
		int width = 0;
		if (sizeRatio >= 1.0f) {
			width = (int)desiredWidths[i];
		}
		else {
			if (i == (tabCount - 1))
				width = remainingWidth;
			else
				width = (int)Math::Round((f32)desiredWidths[i] * sizeRatio);
		}

		auto& rect = rects[i];
		rect.extent.width = (u32)width;
		rect.extent.height = titlebarRect.extent.height;
		rect.position = titlebarRect.position;
		rect.position.x += horizontalPosOffset;

		remainingWidth -= width;
		horizontalPosOffset += width;
	}

	return rects;
}

Std::Opt<uSize> Gui::impl::DA_CheckHitTab(
	Rect const& titlebarRect,
	Std::Span<DA_WindowTab const> tabs,
	FontFaceSizeId fontSizeId,
	int horiMarginAmount,
	TextManager& textManager,
	Math::Vec2 point,
	AllocRef const& transientAlloc)
{
	auto tabRects = DA_WindowNode_BuildTabRects(
		titlebarRect,
		tabs,
		fontSizeId,
		horiMarginAmount,
		textManager,
		transientAlloc);
	// Then check if we hit any of these rects.
	for (int i = 0; i < (int)tabs.Size(); i += 1) {
		if (tabRects[i].PointIsInside(point))
			return i;
	}

	return Std::nullOpt;
}

namespace DEngine::Gui::impl {
	void Render_WindowNode_Child(
		DA_WindowTab const& tab,
		Rect const& contentRect,
		Rect const& visibleRect,
		Widget::Render_Params const& renderParams)
	{
		auto& drawInfo = renderParams.drawInfo;

		auto const visibleIntersection = Intersection(contentRect, visibleRect);
		if (!visibleIntersection.IsNothing()) {
			f32 contentBackgroundDarkenFactor = 0.5f;
			auto contentBackgroundColor = tab.color;
			contentBackgroundColor *= contentBackgroundDarkenFactor;
			contentBackgroundColor.w = 1.f;
			drawInfo.PushFilledQuad(contentRect, contentBackgroundColor);

			if (tab.widget) {
				auto scissor = DrawInfo::ScopedScissor(drawInfo, visibleIntersection);
				auto const& child = *tab.widget;
				child.Render2(
					renderParams,
					contentRect,
					visibleIntersection);
			}
		}
	}

	void Render_WindowNode_TitlebarBackground(
		Math::Vec4 titlebarColor,
		Rect const& titlebarRect,
		int radius,
		DrawInfo& drawInfo)
	{
		f32 titlebarDarkenFactor = 1.f;
		titlebarColor *= titlebarDarkenFactor;
		titlebarColor.w = 1.f;
		drawInfo.PushFilledQuad(titlebarRect, titlebarColor, { radius, 0, 0, radius });
	}
}

void Gui::impl::Render_WindowNode(Render_WindowNode_Params const& params)
{
	auto& textManager = params.widgetRenderParams.textManager;
	auto& transientAlloc = params.widgetRenderParams.transientAlloc;
	auto& drawInfo = params.widgetRenderParams.drawInfo;
	auto const& tabs = params.windowNode.tabs;
	auto const& activeTabIndex = params.windowNode.activeTabIndex;
	auto const& nodeRect = params.nodeRect;
	auto const& visibleRect = params.visibleRect;
	auto tabTotalHeight = params.tabTotalHeight;
	auto fontSizeId = params.fontSizeId;
	auto tabMarginAmount = params.textMarginAmount;
	auto isBackLayer = params.isBackLayer;

	auto [titlebarRect, contentRect] =
		impl::DA_WindowNode_BuildPrimaryRects(nodeRect, tabTotalHeight);

	DENGINE_IMPL_GUI_ASSERT(!tabs.empty());
	auto& activeTab = tabs[activeTabIndex];

	Render_WindowNode_Child(activeTab, contentRect, visibleRect, params.widgetRenderParams);

	// Render the titlebar background
	// Calculate radius
	auto minDimension = Math::Min(titlebarRect.extent.width, titlebarRect.extent.height);
	auto radius = (int)Math::Floor((f32)minDimension * 0.25f);
	auto titlebarRadius = isBackLayer ? 0 : radius;
	Render_WindowNode_TitlebarBackground(activeTab.color, titlebarRect, titlebarRadius, drawInfo);

	// Now we start rendering the tabs of the titlebar.
	auto tabRects = DA_WindowNode_BuildTabRects(
		titlebarRect,
		{ tabs.data(), tabs.size() },
		fontSizeId,
		tabMarginAmount,
		textManager,
		transientAlloc);

	// Render each tab
	for (int i = 0; i < (int)tabs.size(); i += 1) {
		auto const& tab = tabs[i];
		auto color = tab.color;
		auto tabRect = tabRects[i];
		Math::Vec4 textColor = { 1.f, 1.f, 1.f, 1.f };
		if (i != activeTabIndex) {
			textColor *= 0.7f;
			textColor.w = 1.f;
			color.w = 0.8f;
		} else if (i == activeTabIndex) {
			color = { 1, 1, 1, 0.1f };
		}
		// Render the background of this tab.
		drawInfo.PushFilledQuad(tabRect, color, { radius, 0, 0, radius });

		auto glyphRects = Std::NewVec<Rect>(transientAlloc);
		glyphRects.Resize(tab.title.size());
		auto textExtent = textManager.GetOuterExtent(
			{ tab.title.data(), tab.title.size() },
			fontSizeId,
			TextHeightType::Alphas,
			glyphRects.ToSpan());
		Rect textRect = {};
		textRect.position = tabRect.position + Extent::CenteringOffset(tabRect.extent, textExtent);
		textRect.extent = textExtent;

		auto scissor = DrawInfo::ScopedScissor(drawInfo, textRect, tabRect);

		drawInfo.PushText(
			(u64)fontSizeId,
			{ tab.title.data(), tab.title.size() },
			glyphRects.Data(),
			textRect.position,
			textColor);
	}
}

Std::Opt<Rect> Gui::impl::FindSplitNodeRect(
	DA_Node const& rootNode,
	Rect const& layerRect,
	DA_SplitNode const* targetNode,
	AllocRef transientAlloc)
{
	for (auto const& nodeIt : DA_BuildNodeItPair(
		rootNode,
		layerRect,
		transientAlloc))
	{
		if (nodeIt.node.GetNodeType() != impl::NodeType::Split)
			continue;
		if (&nodeIt.node != targetNode)
			continue;
		return nodeIt.rect;
	}
	return Std::nullOpt;
}

auto Gui::impl::DA_Layer_CheckHitResizeHandle(
	u32 resizeHandleThickness,
	u32 resizeHandleLength,
	DA_Node& rootNode,
	Rect const& layerRect,
	AllocRef transientAlloc,
	Math::Vec2 pos)
		-> DA_SplitNode*
{
	for (auto const& nodeIt : DA_BuildNodeItPair(
		rootNode,
		layerRect,
		transientAlloc))
	{
		if (nodeIt.node.GetNodeType() != NodeType::Split)
			continue;

		auto& splitNode = static_cast<DA_SplitNode&>(nodeIt.node);
		auto const resizeHandle = DA_BuildSplitNodeResizeHandleRect(
			nodeIt.rect,
			resizeHandleThickness,
			resizeHandleLength,
			splitNode.split,
			splitNode.dir);
		auto const pointerInsideHandle = resizeHandle.PointIsInside(pos);
		if (pointerInsideHandle) {
			return &splitNode;
		}
	}

	return nullptr;
}