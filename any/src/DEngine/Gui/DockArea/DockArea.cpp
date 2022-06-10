#include <DEngine/Gui/DockArea.hpp>
#include <DEngine/Gui/DrawInfo.hpp>
#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Std/Utility.hpp>

#include <DEngine/Std/Containers/Array.hpp>

#include "Impl.hpp"

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
	Math::Vec2Int spawnPos = {};
	if (layers.size() >= 2)
	{
		if (layers.front().rect.position == spawnPos)
		{
			spawnPos.x += 150;
			spawnPos.y += 150;
		}
	}

	layers.emplace(layers.begin(), DockArea::Layer{});
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
	Widget::CursorMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	DA_PointerMove_Pointer pointer = {};
	pointer.id = cursorPointerId;
	pointer.pos = { (f32)params.event.position.x, (f32)params.event.position.y };

	auto childDispatchFn = [&params](
		Widget& child,
		Rect const& childRect,
		Rect const& childVisibleRect,
		bool childConsumed)
	{
		return child.CursorMove(
			params,
			childRect,
			childVisibleRect,
			childConsumed);
	};

	DA_PointerMove_Params2 temp {
		.dockArea = *this,
		.rectCollection = params.rectCollection,
		.textManager = params.textManager,
		.transientAlloc = params.transientAlloc,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer, };

	return DA_PointerMove(
		temp,
		occluded,
		childDispatchFn);
}

bool DockArea::CursorPress2(
	Widget::CursorPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	DA_PointerPress_Pointer pointer = {};
	pointer.id = cursorPointerId;
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.pressed = params.event.pressed;

	auto childDispatchFn = [&params](
		Widget& child,
		Rect const& childRect,
		Rect const& childVisibleRect,
		bool childConsumed)
	{
		return child.CursorPress2(
			params,
			childRect,
			childVisibleRect,
			childConsumed);
	};

	DA_PointerPress_Params2 temp {
		.dockArea = *this,
		.transientAlloc = params.transientAlloc,
		.rectCollection = params.rectCollection,
		.textManager = params.textManager,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointer,
		.childDispatchFn = childDispatchFn };
	return DA_PointerPress(temp, consumed);
}

SizeHint DockArea::GetSizeHint2(Widget::GetSizeHint2_Params const& params) const
{
	auto& dockArea = *this;
	auto& textManager = params.textManager;
	auto& pusher = params.pusher;

	for (auto const& layerIt : DA_BuildLayerItPair(dockArea))
	{
		for (auto const& itResult : DA_BuildNodeItPair(layerIt.node, params.transientAlloc))
		{
			if (itResult.node.GetNodeType() != NodeType::Window)
				continue;

			auto& node = static_cast<DA_WindowNode const&>(itResult.node);
			auto const& activeTab = node.tabs[node.activeTabIndex];
			if (activeTab.widget)
			{
				auto const& widget = *activeTab.widget;
				widget.GetSizeHint2(params);
			}
		}
	}

	SizeHint sizeHint = {};
	sizeHint.expandX = true;
	sizeHint.expandY = true;
	sizeHint.minimum = { 400, 400 };

	auto entry = pusher.AddEntry(*this);
	pusher.SetSizeHint(entry, sizeHint);
	return sizeHint;
}

void DockArea::BuildChildRects(
	BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& dockArea = *this;
	auto& textManager = params.textManager;
	auto& transientAlloc = params.transientAlloc;
	auto& pusher = params.pusher;

	auto const totalLineheight = textManager.GetLineheight() + (dockArea.tabTextMargin * 2);

	for (auto const& layerIt : DA_BuildLayerItPair(dockArea))
	{
		auto const& layerRect = layerIt.BuildLayerRect(widgetRect);
		for (auto const& nodeIt : DA_BuildNodeItPair(
			layerIt.node,
			layerRect,
			transientAlloc))
		{
			if (nodeIt.node.GetNodeType() != NodeType::Window)
				continue;

			auto& node = static_cast<DA_WindowNode const&>(nodeIt.node);
			auto const& nodeRect = nodeIt.rect;
			auto const [titlebarRect, contentRect] = DA_WindowNode_BuildPrimaryRects(nodeRect, totalLineheight);
			auto const& activeTab = node.tabs[node.activeTabIndex];
			if (activeTab.widget)
			{
				auto& widget = *activeTab.widget;
				auto entry = pusher.GetEntry(widget);
				auto const visibleIntersection = Intersection(visibleRect, contentRect);
				pusher.SetRectPair(entry, { contentRect, visibleIntersection });
				widget.BuildChildRects(
					params,
					contentRect,
					visibleIntersection);
			}
		}
	}
}

namespace DEngine::Gui::impl
{
	// Renders the window node for this layer.
	void DA_Layer_Render_WindowNodes(
		DockArea const& dockArea,
		Rect const& layerRect,
		Rect const& visibleRect,
		DA_Node const& rootNode,
		Widget::Render_Params const& params)
	{
		auto& transientAlloc = params.transientAlloc;

		for (auto const& nodeIt : DA_BuildNodeItPair(
			rootNode,
			layerRect,
			transientAlloc))
		{
			if (nodeIt.node.GetNodeType() != NodeType::Window)
				continue;

			auto const& node = static_cast<DA_WindowNode const&>(nodeIt.node);
			auto const& nodeRect = nodeIt.rect;
			Render_WindowNode(
				node,
				dockArea,
				nodeRect,
				visibleRect,
				params);
		}
	}

	void DA_Layer_Render_SplitNodeHandles(
		DockArea const& dockArea,
		Rect const& layerRect,
		Rect const& visibleRect,
		DA_Node const& rootNode,
		Widget::Render_Params const& params)
	{
		auto& transientAlloc = params.transientAlloc;
		auto& drawInfo = params.drawInfo;

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
				dockArea.resizeHandleThickness,
				dockArea.resizeHandleLength,
				node.split,
				node.dir);
			drawInfo.PushFilledQuad(
				resizeHandleRect,
				dockArea.colors.resizeHandle);
		}
	}

	void DA_Render_OuterLayoutGizmos(
		DrawInfo& drawInfo,
		Rect const& outerRect,
		u32 gizmoSize,
		Math::Vec4 const& color)
	{
		// Iterate over each outer gizmo.
		for (int i = 0; i < (int)DA_OuterLayoutGizmo::COUNT; i += 1)
		{
			auto gizmo = (DA_OuterLayoutGizmo)i;
			auto gizmoRect = DA_BuildOuterLayoutGizmoRect(
				outerRect,
				gizmo,
				gizmoSize);
			drawInfo.PushFilledQuad(gizmoRect, color);
		}
	}

	void DA_Render_InnerDockingGizmos(
		DrawInfo& drawInfo,
		Rect const& outerRect,
		u32 gizmoSize,
		Math::Vec4 const& color)
	{
		// Iterate over each outer gizmo.
		for (int i = 0; i < (int)DA_InnerDockingGizmo::COUNT; i += 1)
		{
			auto gizmo = (DA_InnerDockingGizmo)i;
			auto gizmoRect = DA_BuildInnerDockingGizmoRect(
				outerRect,
				gizmo,
				gizmoSize);
			drawInfo.PushFilledQuad(gizmoRect, color);
		}
	}
}

void DockArea::Render2(
	Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& dockArea = *this;
	auto& transientAlloc = params.transientAlloc;
	auto& drawInfo = params.drawInfo;

	auto scopedScissor = DrawInfo::ScopedScissor(
		drawInfo,
		widgetRect,
		visibleRect);

	for (auto const layerIt : DA_BuildLayerItPair(dockArea).Reverse())
	{
		auto const& layerRect = layerIt.BuildLayerRect(widgetRect);
		DA_Layer_Render_WindowNodes(
			dockArea,
			layerRect,
			visibleRect,
			layerIt.node,
			params);


		// Then render all the split node handles for this layer on top
		DA_Layer_Render_SplitNodeHandles(
			dockArea,
			layerRect,
			visibleRect,
			layerIt.node,
			params);
	}


	// If we are in the moving-state, we want to draw gizmos on top of everything else.
	if (stateData.IsA<DockArea::State_Moving>())
	{
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
					widgetRect,
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
					widgetRect,
					(DA_OuterLayoutGizmo)hoveredGizmo.gizmo.Get());
			}
		}
		if (highlightRect.Has())
		{
			drawInfo.PushFilledQuad(
				highlightRect.Get(),
				dockArea.colors.dockingHighlight);
		}
		if (hoveredWindowRect.Has())
		{
			DA_Render_InnerDockingGizmos(
				drawInfo,
				hoveredWindowRect.Get(),
				gizmoSize,
				colors.resizeHandle);
		}

		Rect const backLayerRect = widgetRect;
		DA_Render_OuterLayoutGizmos(
			drawInfo,
			backLayerRect,
			gizmoSize,
			colors.resizeHandle);

		// Draw the delete gizmo
		auto deleteGizmoRect = DA_BuildDeleteGizmoRect(backLayerRect, gizmoSize);
		drawInfo.PushFilledQuad(deleteGizmoRect, colors.deleteLayerGizmo);
	}
}

void DockArea::CursorExit(Context& ctx)
{
	auto& dockArea = *this;
	for (auto const& layer : DA_BuildLayerItPair(dockArea).Reverse())
	{
		auto& rootNode = layer.node;
	}
}

void DockArea::TextInput(
	Context& ctx,
	AllocRef const& transientAlloc,
	TextInputEvent const& event)
{
	auto& dockArea = *this;
	for (auto const& layerIt : DA_BuildLayerItPair(dockArea))
	{
		for (auto const& nodeIt : DA_BuildNodeItPair(
			layerIt.node, transientAlloc))
		{
			if (nodeIt.node.GetNodeType() != NodeType::Window)
				continue;

			auto& node = static_cast<DA_WindowNode&>(nodeIt.node);
			auto& activeTab = node.tabs[node.activeTabIndex];
			if (activeTab.widget)
			{
				activeTab.widget->TextInput(ctx, transientAlloc, event);
			}
		}
	}
}

void DockArea::EndTextInputSession(
	Context& ctx,
	AllocRef const& transientAlloc,
	EndTextInputSessionEvent const& event)
{
	auto& dockArea = *this;
	for (auto const& layerIt : DA_BuildLayerItPair(dockArea))
	{
		for (auto const& nodeIt : DA_BuildNodeItPair(layerIt.node, transientAlloc))
		{
			if (nodeIt.node.GetNodeType() != NodeType::Window)
				continue;

			auto& node = static_cast<DA_WindowNode&>(nodeIt.node);
			auto& activeTab = node.tabs[node.activeTabIndex];
			if (activeTab.widget)
				activeTab.widget->EndTextInputSession(ctx, transientAlloc, event);
		}
	}
}

void DockArea::Layer::Clear()
{
	if (root)
		delete root;
	root = nullptr;
}
