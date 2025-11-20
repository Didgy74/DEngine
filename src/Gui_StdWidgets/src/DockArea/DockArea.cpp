#include <DEngine/Gui/StdWidgets/DockArea.hpp>

#include "Impl.hpp"

import DEngine.Gui.DrawEngine;
import DEngine.Gui.TextEngine;
import DEngine.Math.Common;

using namespace DEngine;
using namespace DEngine::Gui;
using namespace DEngine::Gui::impl;

DockArea::DockArea()
{
}

void DockArea::AddWindow(
	Std::Str title,
	Math::Vec4 color,
	Std::Box<Widget>&& widget)
{
	DENGINE_IMPL_GUI_ASSERT(widget.Has());

	Math::Vec2Int spawnPos = {};
	if (layers.size() >= 2) {
		if (layers.front().rect.position == spawnPos) {
			spawnPos.x += 150;
			spawnPos.y += 150;
		}
	}

	layers.emplace(layers.begin());
	DockArea::Layer& newLayer = layers.front();

	newLayer.rect = { spawnPos, defaultLayerExtent };

	auto* node = new DA_WindowNode;
	newLayer.root = node;

	node->tabs.emplace_back(DA_WindowTab());

	auto& newWindow = node->tabs.back();
	newWindow.title = { title.Data(), title.Size() };
	newWindow.color = color;
	newWindow.widget = static_cast<Std::Box<Widget>&&>(widget);
}

bool DockArea::CursorMove(
	CursorMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto& rectColl = params.rectCollection;

	auto* customDataPtr = rectColl.GetCustomData<impl::DA_CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto& customData = *customDataPtr;

	DA_PointerMove_Pointer pointer = {};
	pointer.id = cursorPointerId;
	pointer.pos = { (f32)params.CursorPos().x, (f32)params.CursorPos().y };

	auto childDispatchFn = [&params](
		Widget& child,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool childConsumed)
	{
		bool consumed = child.CursorMove(
			params,
			childRectCollIter,
			childConsumed);
		return TouchEventConsumption { .consumed = consumed };
	};

	DA_PointerMove_Params temp {
		.dockArea = *this,
		.textManager = params.textEngine,
		.transientAlloc = params.transientAlloc,
		.rectColl = params.rectCollection,
		.customData = customData,
		.rectCollIter = rectCollIter,
		.pointer = pointer, };

	auto returnVal = DA_PointerMove(
		temp,
		occluded,
		childDispatchFn);
	return returnVal.consumed;
}

bool DockArea::CursorPress2(
	CursorPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	auto* customDataPtr = params.rectCollection.GetCustomData<impl::DA_CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto& customData = *customDataPtr;

	DA_PointerPress_Pointer pointer = {
		.id = cursorPointerId,
		.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y },
		.pressed = params.CursorPressed(),
	};

	auto childDispatchFn = [&params](
		Widget& child,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool childConsumed)
	{
		return TouchEventConsumption {
			.consumed = child.CursorPress2(
				params,
				childRectCollIter,
				childConsumed),
		};
	};

	DA_PointerPress_Params temp {
		.dockArea = *this,
		.transientAlloc = params.transientAlloc,
		.rectColl = params.rectCollection,
		.customData = customData,
		.textManager = params.textEngine,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.childDispatchFn = childDispatchFn };
	auto returnVal = DA_PointerPress(temp, consumed);
	return returnVal.consumed;
}

TouchEventConsumption DockArea::WidgetEvent_TouchMove(
	WidgetEvent_TouchMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto* customDataPtr = params.rectCollection.GetCustomData<impl::DA_CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto& customData = *customDataPtr;

	DA_PointerMove_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;

	auto childDispatchFn = [&params](
		Widget& child,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool childConsumed)
	{
		return child.WidgetEvent_TouchMove(
			params,
			childRectCollIter,
			childConsumed);
	};

	DA_PointerMove_Params temp {
		.dockArea = *this,
		.textManager = params.textEngine,
		.transientAlloc = params.transientAlloc,
		.rectColl = params.rectCollection,
		.customData = customData,
		.rectCollIter = rectCollIter,
		.pointer = pointer, };

	return DA_PointerMove(
		temp,
		occluded,
		childDispatchFn);
}

TouchEventConsumption DockArea::WidgetEvent_TouchPress(
	WidgetEvent_TouchPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	auto* customDataPtr = params.rectCollection.GetCustomData<impl::DA_CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto& customData = *customDataPtr;

	DA_PointerPress_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.pressed = params.event.pressed;

	auto childDispatchFn = [&params](
		Widget& child,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool childConsumed)
	{
		return child.WidgetEvent_TouchPress(
			params,
			childRectCollIter,
			childConsumed);
	};

	DA_PointerPress_Params temp {
		.dockArea = *this,
		.transientAlloc = params.transientAlloc,
		.rectColl = params.rectCollection,
		.customData = customData,
		.textManager = params.textEngine,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.childDispatchFn = childDispatchFn };
	return DA_PointerPress(temp, consumed);
}

Widget::GetSizeHint2_ReturnT DockArea::GetSizeHint2(Widget::GetSizeHint2_Params const& params) const
{
	auto const& dockArea = *this;
	auto const& window = params.window;
	auto const& fontScale = params.FontScale();
	auto const& contentScale = params.ContentScale();
	auto const& minimumHeightCm = params.MinimumHeightCm();
	auto& textEngine = params.textEngine;
	auto& pusher = params.pusher;

	for (auto const& layerIt : DA_BuildLayerItPairFrontToBack(dockArea)) {
		for (auto const& itResult : DA_BuildNodeItPair(
			layerIt.node,
			params.transientAlloc))
		{
			if (itResult.node.GetNodeType() != NodeType::Window) {
				continue;
			}

			auto const& node = static_cast<DA_WindowNode const&>(itResult.node);
			auto const& activeTab = node.tabs[node.activeTabIndex];
			if (activeTab.widget) {
				auto const& widget = *activeTab.widget;
				[[maybe_unused]] auto unused = widget.GetSizeHint2(params);
			}
		}
	}

	SizeHint sizeHint = {};
	sizeHint.expandX = true;
	sizeHint.expandY = true;
	sizeHint.minimum = { 400, 400 };

	auto entry = pusher.AddEntry(*this);
	auto& customData = pusher.AttachCustomData(entry, impl::DA_CustomData{});

	DockArea::Theme theme;
	if (this->getThemeFn)
		theme = this->getThemeFn({ .widget = *this, .appData = params.appData });
	auto textMarginPx = theme.tabTextMargin.ResolvePx(window.dpi, contentScale);

	auto normalTextScale = fontScale * contentScale;
	auto normalFontSizeId = textEngine.GetFontFaceSizeId(normalTextScale, window.dpi, window.dpi);
	auto normalTextHeight = textEngine.GetFontFaceSizeMetrics(normalFontSizeId).lineHeight;
	auto normalTabHeight = normalTextHeight + 2*textMarginPx;
	normalTabHeight = Std::Max(normalTabHeight, (u32)CmToPixels(minimumHeightCm, window.dpi));

	customData.fontSizeId = normalFontSizeId;
	customData.tabMarginPx = textMarginPx;
	customData.tabTotalHeightPx = normalTabHeight;
	customData.tabCornerRadiusPx = theme.tabCornerRadius.ResolvePx(window.dpi, window.contentScale);

	customData.resizeHandleThickness = CmToPixels(minimumHeightCm, window.dpi);
	customData.resizeHandleLength = 2 * CmToPixels(minimumHeightCm, window.dpi);

	customData.gizmoExtent.width = CmToPixels(minimumHeightCm, window.dpi);
	customData.gizmoExtent.height = CmToPixels(minimumHeightCm, window.dpi);

	customData.layerCornerRadiusPx = theme.layerCornerRadius.ResolvePx(window.dpi, window.contentScale);
	customData.layerShadowFalloffPx = theme.layerShadowFalloff.ResolvePx(window.dpi, window.contentScale);

	pusher.SetSizeHint(entry, sizeHint, false);
	return {
		.iter = entry,
		.sizeHint = sizeHint };
}

void DockArea::BuildChildRects(
	BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& dockArea = *this;
	auto& textManager = params.textEngine;
	auto& transientAlloc = params.transientAlloc;
	auto& pusher = params.pusher;

	auto const* customDataPtr = pusher.GetCustomData<impl::DA_CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto const& customData = *customDataPtr;
	auto totalTabHeightPx = customData.tabTotalHeightPx;

	for (auto const& layerIt : DA_BuildLayerItPairFrontToBack(dockArea)) {
		auto const& layerRect = layerIt.BuildLayerRect(widgetRect);
		for (auto const& nodeIt :
			DA_BuildNodeItPair(layerIt.node, layerRect, transientAlloc))
		{
			if (nodeIt.node.GetNodeType() != NodeType::Window)
				continue;

			auto& node = reinterpret_cast<DA_WindowNode const&>(nodeIt.node);
			auto const& nodeRect = nodeIt.rect;
			auto const [titlebarRect, contentRect] = DA_WindowNode_BuildPrimaryRects(nodeRect, totalTabHeightPx);
			auto const& activeTab = node.tabs[node.activeTabIndex];
			if (activeTab.widget) {
				auto& widget = *activeTab.widget;
				auto entry = pusher.GetEntry(widget);
				auto const visibleIntersection = Intersection(visibleRect, contentRect);
				pusher.SetLocalRectPair(entry, { contentRect, visibleIntersection });
				widget.BuildChildRects(
					params,
					contentRect,
					visibleIntersection,
					Std::nullOpt);
			}
		}
	}
}

namespace DEngine::Gui::impl
{
	// Renders the window node for this layer.
	struct DA_Layer_Render_WindowNodes_Params {
		DockArea const& dockArea;
		FontFaceSizeId fontSizeId;
		u32 const& tabTotalHeightPx;
		u32 const& tabHoriMarginPx;
		u32 const& tabCornerRadiusPx;
		bool isBackLayer = {};
		Rect const& layerRect;
		Rect const& visibleRect;
		DA_Node const& rootNode;
		Widget::Render_Params const& widgetRenderParams;
	};
	void DA_Layer_Render_WindowNodes(
		DA_Layer_Render_WindowNodes_Params const& params)
	{
		auto& rootNode = params.rootNode;
		auto layerRect = params.layerRect;
		auto& transientAlloc = params.widgetRenderParams.transientAlloc;

		for (auto const& nodeIt : DA_BuildNodeItPair(
			rootNode,
			layerRect,
			transientAlloc))
		{
			if (nodeIt.node.GetNodeType() != NodeType::Window)
				continue;

			auto const& node = static_cast<DA_WindowNode const&>(nodeIt.node);
			auto const& nodeRect = nodeIt.rect;
			Render_WindowNode(impl::Render_WindowNode_Params{
				.windowNode = node,
				.dockArea = params.dockArea,
				.fontSizeId = params.fontSizeId,
				.tabTotalHeightPx = params.tabTotalHeightPx,
				.textMarginAmountPx = params.tabHoriMarginPx,
				.tabCornerRadiusPx = params.tabCornerRadiusPx,
				.isBackLayer = params.isBackLayer,
				.nodeRect = nodeRect,
				.visibleRect = params.visibleRect,
				.widgetRenderParams = params.widgetRenderParams,
		  	});
		}
	}

	void DA_Layer_Render_SplitNodeHandles(
		DockArea const& dockArea,
		int resizeHandleThickness,
		int resizeHandleLength,
		Rect const& layerRect,
		Rect const& visibleRect,
		DA_Node const& rootNode,
		Widget::Render_Params const& params)
	{
		auto& transientAlloc = params.transientAlloc;
		auto& drawInfo = params.drawEngine;

		// Then render all the split node handles for this layer on top
		for (auto const& nodeIt : DA_BuildNodeItPair(
			rootNode,
			layerRect,
			transientAlloc))
		{
			if (nodeIt.node.GetNodeType() != NodeType::Split)
				continue;

			auto const& node = static_cast<DA_SplitNode const&>(nodeIt.node);
			auto const& nodeRect = nodeIt.rect;
			auto const resizeHandleRect = DA_BuildSplitNodeResizeHandleRect(
				nodeRect,
				resizeHandleThickness,
				resizeHandleLength,
				node.split,
				node.dir);

			// Calculate radius
			auto minDimension = Std::Min(resizeHandleRect.extent.width, resizeHandleRect.extent.height);
			auto radius = (int)Std::Floor((f32)minDimension * 0.5f);
			auto shadowFalloffPx = radius;
			auto shadowFalloffAlpha = 0.2f;

			{
				auto mask = drawInfo.PushRectMaskScoped(resizeHandleRect, radius, DrawEngine::MaskOp::Inside);
				(void)mask;
				drawInfo.PushRectShadow(
					resizeHandleRect,
					radius,
					shadowFalloffPx,
					shadowFalloffAlpha);
			}
			drawInfo.PushFilledQuad(
				resizeHandleRect,
				dockArea.colors.resizeHandle,
				radius);
		}
	}

	void DA_Render_OuterLayoutGizmos(
		DrawEngine& drawInfo,
		Rect const& outerRect,
		Extent const& gizmoSize,
		Math::Vec4 const& color)
	{
		// Iterate over each outer gizmo.
		for (int i = 0; i < (int)DA_OuterLayoutGizmo::COUNT; i += 1) {
			auto gizmo = (DA_OuterLayoutGizmo)i;
			auto gizmoRect = DA_BuildOuterLayoutGizmoRect(
				outerRect,
				gizmo,
				gizmoSize);
			auto minDimension = Std::Min(gizmoSize.width, gizmoSize.height);
			auto radius = (int)Std::Floor((f32)minDimension * 0.25f);
			drawInfo.PushFilledQuad(gizmoRect, color, radius);
		}
	}

	void DA_Render_InnerDockingGizmos(
		DrawEngine& drawInfo,
		Rect const& outerRect,
		Extent const& gizmoSize,
		Math::Vec4 const& color)
	{
		// Iterate over each outer gizmo.
		for (int i = 0; i < (int)DA_InnerDockingGizmo::COUNT; i += 1) {
			auto gizmo = (DA_InnerDockingGizmo)i;
			auto gizmoRect = DA_BuildInnerDockingGizmoRect(
				outerRect,
				gizmo,
				gizmoSize);
			auto minDimension = Math::Min(gizmoSize.width, gizmoSize.height);
			auto radius = (int)Math::Floor((f32)minDimension * 0.25f);
			drawInfo.PushFilledQuad(gizmoRect, color, radius);
		}
	}
}

void DockArea::CursorExit()
{
	auto& dockArea = *this;
	for (auto const& layer : DA_BuildLayerItPairFrontToBack(dockArea).Reverse()) {
		auto& rootNode = layer.node;
	}
}

void DockArea::WidgetEvent_TextInput(
	AllocRef const& transientAlloc,
	WidgetEvent_TextInputParams const& event)
{
	auto& dockArea = *this;
	for (auto const& layerIt : DA_BuildLayerItPairFrontToBack(dockArea)) {
		for (auto const& nodeIt : DA_BuildNodeItPair(
			layerIt.node, transientAlloc))
		{
			if (nodeIt.node.GetNodeType() != NodeType::Window) {
				continue;
			}

			auto& node = static_cast<DA_WindowNode&>(nodeIt.node);
			auto& activeTab = node.tabs[node.activeTabIndex];
			if (activeTab.widget) {
				activeTab.widget->WidgetEvent_TextInput(transientAlloc, event);
			}
		}
	}
}

void DockArea::WidgetEvent_EndTextInputSession(
	AllocRef const& transientAlloc,
	WidgetEvent_EndTextInputSessionParams const& event)
{
	auto& dockArea = *this;
	for (auto const& layerIt : DA_BuildLayerItPairFrontToBack(dockArea)) {
		for (auto const& nodeIt : DA_BuildNodeItPair(layerIt.node, transientAlloc)) {
			if (nodeIt.node.GetNodeType() != NodeType::Window) {
				continue;
			}

			auto& node = static_cast<DA_WindowNode&>(nodeIt.node);
			auto& activeTab = node.tabs[node.activeTabIndex];
			if (activeTab.widget) {
				activeTab.widget->WidgetEvent_EndTextInputSession(transientAlloc, event);
			}
		}
	}
}

void DockArea::Layer::Clear()
{
	if (root)
		delete root;
	root = nullptr;
}

void DockArea::Render2(
	Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& dockArea = *this;
	auto& transientAlloc = params.transientAlloc;
	auto& rectColl = params.rectCollection;
	auto& drawInfo = params.drawEngine;

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	auto const* customDataPtr = rectColl.GetCustomData<impl::DA_CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto const& customData = *customDataPtr;

	auto gizmoExtent = customData.gizmoExtent;
	auto fontSizeId = customData.fontSizeId;
	auto tabTotalHeightPx = customData.tabTotalHeightPx;
	auto tabTextMarginPx = customData.tabMarginPx;
	auto resizeHandleThickness = customData.resizeHandleThickness;
	auto resizeHandleLength = customData.resizeHandleLength;
	auto layerCornerRadiusPx = customData.layerCornerRadiusPx;
	auto layerShadowFalloffPx = customData.layerShadowFalloffPx;

	auto scopedScissor = DrawEngine::ScopedScissor(
		drawInfo,
		absWidgetRect.GetIntersect(absVisibleRect));

	for (auto const layerIt : DA_BuildLayerItPairFrontToBack(dockArea).Reverse()) {
		auto const& layerRect = layerIt.BuildLayerRect(absWidgetRect);
		auto shadowAlpha = 0.25f;

		if (!layerIt.IsBackgroundLayer()) {
			drawInfo.PushRectShadow(layerRect, layerCornerRadiusPx, (f32)layerShadowFalloffPx, shadowAlpha);
		}

		Std::Opt<DrawEngine::ScopedMask> mask;
		if (!layerIt.IsBackgroundLayer()) {
			mask = drawInfo.PushRectMaskScoped(layerRect, layerCornerRadiusPx, DrawEngine::MaskOp::Outside);
		}
		(void)mask;

		DA_Layer_Render_WindowNodes({
			.dockArea = dockArea,
			.fontSizeId = fontSizeId,
			.tabTotalHeightPx = tabTotalHeightPx,
			.tabHoriMarginPx = tabTextMarginPx,
			.tabCornerRadiusPx = customData.tabCornerRadiusPx,
			.isBackLayer = layerIt.layerIndex == layerIt.layerCount - 1,
			.layerRect = layerRect,
			.visibleRect = absVisibleRect,
			.rootNode = layerIt.node,
			.widgetRenderParams = params,
		});

		// Then render all the split node handles for this layer on top
		DA_Layer_Render_SplitNodeHandles(
			dockArea,
			resizeHandleThickness, resizeHandleLength,
			layerRect,
			absVisibleRect,
			layerIt.node,
			params);
	}


	// If we are in the moving-state, we want to draw gizmos on top of everything else.
	if (stateData.IsA<DockArea::State_Moving>()) {
		auto const& stateMoving = stateData.Get<DockArea::State_Moving>();

		// If we are hovering a gizmo, we want to display the highlight
		// We might be hovering one of the outer layer back gizmos.
		Std::Opt<Rect> highlightRect;
		Std::Opt<Rect> hoveredWindowRect;
		if (stateMoving.hoveredGizmoOpt.Has())
		{
			auto const& hoveredGizmo = stateMoving.hoveredGizmoOpt.Get();
			if (hoveredGizmo.GetWindowNode())
			{
				auto nodeRect = DA_FindNodeRect(
					dockArea,
					absWidgetRect,
					hoveredGizmo.GetWindowNode(),
					transientAlloc);
				hoveredWindowRect = nodeRect;
				if (hoveredGizmo.gizmo.Has() && hoveredGizmo.gizmoIsInner)
				{
					highlightRect = DA_BuildDockingHighlightRect(
						nodeRect,
						(DA_InnerDockingGizmo)hoveredGizmo.gizmo.Get());
				}
			}

			// This code handles the hover highlights
			if (hoveredGizmo.gizmo.Has() && !hoveredGizmo.gizmoIsInner)
			{
				highlightRect = DA_BuildDockingHighlightRect(
					absWidgetRect,
					(DA_OuterLayoutGizmo)hoveredGizmo.gizmo.Get());
			}
		}
		if (highlightRect.Has()) {
			drawInfo.PushFilledQuad(
				highlightRect.Get(),
				dockArea.colors.dockingHighlight);
		}
		if (hoveredWindowRect.Has()) {
			DA_Render_InnerDockingGizmos(
				drawInfo,
				hoveredWindowRect.Get(),
				gizmoExtent,
				colors.resizeHandle);
		}

		Rect const backLayerRect = absWidgetRect;
		DA_Render_OuterLayoutGizmos(
			drawInfo,
			backLayerRect,
			gizmoExtent,
			colors.resizeHandle);

		// Draw the delete gizmo
		auto deleteGizmoRect = DA_BuildDeleteGizmoRect(backLayerRect, gizmoExtent);
		drawInfo.PushFilledQuad(deleteGizmoRect, colors.deleteLayerGizmo);
	}
}

void DockArea::AccessibilityTest(
	AccessibilityTest_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& accessPusher = params.pusher;
	auto& textManager = params.textManager;
	auto& transientAlloc = params.transientAlloc;
	auto const& rectColl = params.rectColl;

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	auto const* customDataPtr = rectColl.GetCustomData<impl::DA_CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto const& customData = *customDataPtr;

	// TODO: We don't support multiple virtual accessibility nodes inside one Widget, so just treat the first layer.
	// If that first layer is a WindowNode, we make a node for this DockArea as the first tab.
	// If that first layer is a split node, traverse split nodes and propagate accessibility info.
	// Only highlight the first tab that we discover.

	auto& dockArea = *this;
	bool foundFirstTab = false;
	for (auto const& layerIt : DA_BuildLayerItPairFrontToBack(dockArea)) {

		auto const& layerRect = layerIt.BuildLayerRect(absWidgetRect);
		for (auto const& nodeIt : DA_BuildNodeItPair(layerIt.node, layerRect, transientAlloc)) {

			// Keep iterating whenever we hit split nodes.
			if (nodeIt.node.GetNodeType() != NodeType::Window)
				continue;

			auto& node = static_cast<DA_WindowNode const&>(nodeIt.node);
			auto nodeRect = nodeIt.rect;
			auto totalTabHeightPx = customData.tabTotalHeightPx;
			auto const [titlebarRect, contentRect] = DA_WindowNode_BuildPrimaryRects(nodeRect, totalTabHeightPx);

			// Make a AccessNode for the very first window tab.
			if (!foundFirstTab) {
				foundFirstTab = true;
				auto& tabs = node.tabs;
				auto fontSizeId = customData.fontSizeId;
				auto tabMargin = customData.tabMarginPx;

				auto tabRects = DA_WindowNode_BuildTabRects(
					titlebarRect,
					{ tabs.data(), tabs.size() },
					fontSizeId,
					tabMargin,
					textManager,
					transientAlloc);

				DENGINE_IMPL_GUI_ASSERT(tabRects.Size() > 0);

				auto const textStartOffset = accessPusher.PushText({ tabs[0].title.data() , tabs[0].title.size() });
				AccessibilityInfoElement accessItem{};
				accessItem.rect = tabRects[0];
				accessItem.textStart = textStartOffset;
				accessItem.textCount = (int)tabs[0].title.size();
				accessPusher.PushElement(
					reinterpret_cast<uSize>(this),
					accessItem);
			}

			DENGINE_IMPL_GUI_ASSERT(node.activeTabIndex < node.tabs.size());
			auto const& tab = node.tabs[node.activeTabIndex];
			if (tab.widget.Has()) {
				tab.widget->AccessibilityTest(
					params,
					Std::nullOpt);
			}
			// Stop iterating after we do the active window tab
			break;
		}

		// Stop iterating after the first layer
		break;
	}

}