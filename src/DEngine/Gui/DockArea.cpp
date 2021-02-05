#include <DEngine/Gui/DockArea.hpp>

#include <DEngine/Gui/Context.hpp>
#include "ImplData.hpp"

#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Std/Trait.hpp>
#include <DEngine/Std/Utility.hpp>

namespace DEngine::Gui::impl
{
	using DockArea_WindowCallablePFN = void(*)(
		DockArea::Node& node,
		Rect rect,
		DockArea::Node* parentNode,
		bool& continueIterating);

	enum class DockArea_IterateNode_SplitCallableOrder { First, Last };

	template<typename T, typename Callable>
	void DockArea_IterateNode_Internal(
		T& node,
		Rect rect,
		T* parentNode,
		bool& continueIterating,
		DockArea_IterateNode_SplitCallableOrder splitCallableOrder,
		Callable const& callable)
	{
		if (node.type == DockArea::Node::Type::Window)
		{
			DENGINE_IMPL_GUI_ASSERT(!node.windows.empty());
				callable(
					node,
					rect,
					parentNode,
					continueIterating);
			return;
		}

		DENGINE_IMPL_GUI_ASSERT(node.type == DockArea::Node::Type::HoriSplit || node.type == DockArea::Node::Type::VertSplit);

		if (node.type == DockArea::Node::Type::HoriSplit || node.type == DockArea::Node::Type::VertSplit)
		{
			if (splitCallableOrder == DockArea_IterateNode_SplitCallableOrder::First)
			{
				callable(
					node,
					rect,
					parentNode,
					continueIterating);
				if (!continueIterating)
					return;
			}

			Rect childRect = rect;
			if (node.type == DockArea::Node::Type::HoriSplit || node.type == DockArea::Node::Type::VertSplit)
			{
				if (node.type == DockArea::Node::Type::HoriSplit)
					childRect.extent.width = u32(rect.extent.width * node.split.split);
				else
					childRect.extent.height = u32(rect.extent.height * node.split.split);
				DockArea_IterateNode_Internal(
					*node.split.a,
					childRect,
					&node,
					continueIterating,
					splitCallableOrder,
					callable);
				if (!continueIterating)
					return;
			}

			if (node.type == DockArea::Node::Type::HoriSplit || node.type == DockArea::Node::Type::VertSplit)
			{
				if (node.type == DockArea::Node::Type::HoriSplit)
				{
					childRect.position.x += childRect.extent.width;
					childRect.extent.width = rect.extent.width - childRect.extent.width;
				}
				else
				{
					childRect.position.y += childRect.extent.height;
					childRect.extent.height = rect.extent.height - childRect.extent.height;
				}

				DockArea_IterateNode_Internal(
					*node.split.b,
					childRect,
					&node,
					continueIterating,
					splitCallableOrder,
					callable);
				if (!continueIterating)
					return;
			}

			if (node.type == DockArea::Node::Type::HoriSplit || node.type == DockArea::Node::Type::VertSplit)
			{
				if (splitCallableOrder == DockArea_IterateNode_SplitCallableOrder::Last)
				{
					callable(
						node,
						rect,
						parentNode,
						continueIterating);
					if (!continueIterating)
						return;
				}
			}
		}
	}

	template<typename T, typename Callable>
	void DockArea_IterateNode(
		T& node,
		Rect rect,
		bool& continueIterating,
		DockArea_IterateNode_SplitCallableOrder splitCallableOrder,
		Callable const& callable)
	{
		static_assert(
			Std::Trait::isSame<Std::Trait::RemoveConst<T>,	DockArea::Node>,
			"Template type error.");
		
		DENGINE_IMPL_GUI_ASSERT(continueIterating);
		DockArea_IterateNode_Internal(
			node,
			rect,
			(T*)nullptr,
			continueIterating,
			splitCallableOrder,
			callable);
	}

	enum class DockArea_IterateLayerMode
	{
		FrontToBack,
		BackToFront,
	};

	using DockArea_IterateLayerCallbackPFN = void(*)(
		DockArea::Layer& layer,
		Rect layerRect,
		uSize layerIndex,
		bool& continueIterating);

	template<typename LayerContainerT, typename Callable>
	void DockArea_IterateLayers(
		DockArea_IterateLayerMode iterateMode,
		LayerContainerT& layers,
		Rect widgetRect,
		bool& continueIterating,
		Callable callable)
	{
		static_assert(
			Std::Trait::isSame<Std::Trait::RemoveConst<LayerContainerT>, decltype(DockArea::layers)>,
			"Wrong type passed into Gui::impl::DockArea_IterateLayers");

		uSize layerCount = layers.size();
		
		uSize n = 0;
		if (iterateMode == DockArea_IterateLayerMode::BackToFront)
			n = layerCount;
		while (true)
		{
			uSize i = 0;
			bool shouldContinue = false;
			switch (iterateMode)
			{
			case DockArea_IterateLayerMode::FrontToBack:
			{
				if (n < layerCount)
					shouldContinue = true;
				i = n;
			}
			break;
			case DockArea_IterateLayerMode::BackToFront:
			{
				if (n > 0)
					shouldContinue = true;
				i = n - 1;
			}
			break;
			default:
				DENGINE_DETAIL_UNREACHABLE();
				break;
			}
			if (!shouldContinue)
				break;

			auto& layer = layers[i];
			Rect layerRect = layer.rect;
			layerRect.position += widgetRect.position;
			// If we are on the backmost layer, make it take up the size of the widget.
			if (i == layerCount - 1)
				layerRect = widgetRect;
			callable(
				layer, 
				layerRect, 
				i, 
				continueIterating);
			if (!continueIterating)
				return;

			switch (iterateMode)
			{
			case DockArea_IterateLayerMode::FrontToBack:
			{
				n += 1;
			}
			break;
			case DockArea_IterateLayerMode::BackToFront:
			{
				n -= 1;
			}
			break;
			default:
				DENGINE_DETAIL_UNREACHABLE();
				break;
			}
		}
	}

	enum class DockArea_ResizeRect { Top, Bottom, Left, Right };
	using DockArea_IterateResizeRectCallbackPFN = void(*)(
		DockArea_ResizeRect side,
		Rect rect);
	template<typename Callable>
	void DockArea_IterateResizeRects(
		Rect rect,
		u32 resizeAreaThickness,
		u32 resizeHandleLength,
		Callable callable)
	{
		{
			Rect topRect = rect;
			topRect.extent.width = resizeHandleLength;
			topRect.extent.height = resizeAreaThickness;
			topRect.position.x += rect.extent.width / 2 - topRect.extent.width;
			topRect.position.y -= topRect.extent.height / 2;
			callable(DockArea_ResizeRect::Top, topRect);
		}
		{
			Rect bottomRect = rect;
			bottomRect.extent.width = resizeHandleLength;
			bottomRect.extent.height = resizeAreaThickness;
			bottomRect.position.x += rect.extent.width / 2 - bottomRect.extent.width;
			bottomRect.position.y += rect.extent.height - (bottomRect.extent.height / 2);
			callable(DockArea_ResizeRect::Bottom, bottomRect);
		}
		{
			Rect leftRect = rect;
			leftRect.extent.width = resizeAreaThickness;
			leftRect.extent.height = resizeHandleLength;
			leftRect.position.x -= leftRect.extent.width / 2;
			leftRect.position.y += rect.extent.height / 2 - (leftRect.extent.height / 2);
			callable(DockArea_ResizeRect::Left, leftRect);
		}
		{
			Rect rightRect = rect;
			rightRect.extent.width = resizeAreaThickness;
			rightRect.extent.height = resizeHandleLength;
			rightRect.position.x += rect.extent.width - rightRect.extent.width / 2;
			rightRect.position.y += rect.extent.height / 2 - (rightRect.extent.height / 2);
			callable(DockArea_ResizeRect::Right, rightRect);
		}
	}

	using DockArea_IterateLayoutGizmosCallbackPFN = void(*)(
		DockArea_LayoutGizmo gizmo,
		Rect rect);
	template<typename Callable>
	void DockArea_IterateLayoutGizmos(
		Rect rect,
		u32 gizmoSize,
		u32 gizmoPadding,
		Callable callable)
	{
		Rect gizmoRect{};
		gizmoRect.position.x = rect.position.x + (rect.extent.width / 2) - (gizmoSize / 2);
		gizmoRect.position.y = rect.position.y + (rect.extent.height / 2) - (gizmoSize / 2);
		gizmoRect.extent = { gizmoSize, gizmoSize };
		// Left gizmo
		gizmoRect.position.x -= gizmoRect.extent.width + gizmoPadding;
		callable(DockArea_LayoutGizmo::Left, gizmoRect);
		// Middle gizmo
		gizmoRect.position.x += gizmoRect.extent.width + gizmoPadding;
		callable(DockArea_LayoutGizmo::Center, gizmoRect);
		// Right gizmo
		gizmoRect.position.x += gizmoRect.extent.width + gizmoPadding;
		callable(DockArea_LayoutGizmo::Right, gizmoRect);
		// Top gizmo
		gizmoRect.position.x = rect.position.x + (rect.extent.width / 2) - (gizmoSize / 2);
		gizmoRect.position.y -= gizmoRect.extent.width + gizmoPadding;
		callable(DockArea_LayoutGizmo::Top, gizmoRect);
		// Bottom gizmo
		gizmoRect.position.y += (gizmoRect.extent.width + gizmoPadding) * 2;
		callable(DockArea_LayoutGizmo::Bottom, gizmoRect);
	}

	static Rect DockArea_GetSplitResizeHandle(
		DockArea::Node::Type nodeType,
		Rect nodeRect,
		f32 split,
		u32 thickness,
		u32 handleLength)
	{
		DENGINE_IMPL_GUI_ASSERT(nodeType == DockArea::Node::Type::HoriSplit || nodeType == DockArea::Node::Type::VertSplit);
		// calc the rect
		Rect middleRect = nodeRect;
		// Check if we are hovering the resize line. Then switch to resize mode if we are.
		if (nodeType == DockArea::Node::Type::HoriSplit)
		{
			middleRect.extent.width = thickness;
			middleRect.extent.height = handleLength;
			middleRect.position.x += u32(nodeRect.extent.width * split) - middleRect.extent.width / 2;
			middleRect.position.y += u32(nodeRect.extent.height / 2) - middleRect.extent.height / 2;
		}
		else
		{
			middleRect.extent.width = handleLength;
			middleRect.extent.height = thickness;
			middleRect.position.x += u32(nodeRect.extent.width / 2) - middleRect.extent.width / 2;
			middleRect.position.y += u32(nodeRect.extent.height * split) - middleRect.extent.height / 2;
		}
		return middleRect;
	}

	static void DockArea_PushLayerToFront(
		std::vector<DockArea::Layer>& layers,
		uSize index)
	{
		DENGINE_IMPL_GUI_ASSERT(index < layers.size());
		if (index > 0)
		{
			DockArea::Layer tempLayer = Std::Move(layers[index]);
			layers.erase(layers.begin() + index);
			layers.insert(layers.begin(), Std::Move(tempLayer));
		}
	}

	static Rect DockArea_GetDeleteGizmoRect(
		Rect widgetRect,
		u32 gizmoSize)
	{
		Rect deleteGizmoRect{};
		deleteGizmoRect.position.x += widgetRect.position.x;
		deleteGizmoRect.position.x += (i32)widgetRect.extent.width / 2;
		deleteGizmoRect.position.x -= gizmoSize / 2;
		deleteGizmoRect.position.y += widgetRect.position.y;
		deleteGizmoRect.position.y += widgetRect.extent.height;
		deleteGizmoRect.position.y -= gizmoSize * 2;
		deleteGizmoRect.extent.width = gizmoSize;
		deleteGizmoRect.extent.height = gizmoSize;
		return deleteGizmoRect;
	}
}

using namespace DEngine;
using namespace DEngine::Gui;

DockArea::DockArea()
{
	behaviorData.Set(BehaviorData_Normal{});
}

SizeHint DockArea::GetSizeHint(
	Context const& ctx) const
{
	SizeHint returnVal{};
	returnVal.preferred = { 50, 50 };
	returnVal.expandX = true;
	returnVal.expandY = true;

	return returnVal;
}

namespace DEngine::Gui::impl
{
	static void DockArea_RenderWindows(
		Context const& ctx,
		DockArea const& dockArea,
		ImplData& implData,
		std::vector<DockArea::Layer> const& layers,
		Rect widgetRect,
		Extent framebufferExtent,
		DrawInfo& drawInfo)
	{
		bool continueIterating = true;
		impl::DockArea_IterateLayers(
			DockArea_IterateLayerMode::BackToFront,
			layers,
			widgetRect,
			continueIterating,
			[&ctx, &dockArea, &implData, &drawInfo](
				DockArea::Layer const& layer,
				Rect layerRect,
				uSize layerIndex,
				bool& continueIterating)
			{
				impl::DockArea_IterateNode(
					*layer.node,
					layerRect,
					continueIterating,
					impl::DockArea_IterateNode_SplitCallableOrder::Last,
					[&ctx, &dockArea, &implData, &drawInfo](
						DockArea::Node const& node,
						Rect rect,
						DockArea::Node const* parentNode,
						bool& continueIterating)
					{
						if (node.type == DockArea::Node::Type::Window)
						{
							// First draw titlebar and main window background
							auto& window = node.windows[node.selectedWindow];

							Rect titleBarRect{};
							titleBarRect.position = rect.position;
							titleBarRect.extent.width = rect.extent.width;
							titleBarRect.extent.height = implData.textManager.lineheight;
							drawInfo.PushFilledQuad(titleBarRect, window.titleBarColor);

							Rect contentRect{};
							contentRect.position.x = titleBarRect.position.x;
							contentRect.position.y = titleBarRect.position.y + titleBarRect.extent.height;
							contentRect.extent.width = titleBarRect.extent.width;
							contentRect.extent.height = rect.extent.height - titleBarRect.extent.height;

							// Draw the main window
							Math::Vec4 contentColor = window.titleBarColor * 0.5f;
							contentColor.w = 0.975f;
							drawInfo.PushFilledQuad(contentRect, contentColor);

							// Draw all the tabs
							u32 tabHoriOffset = 0;
							uSize nodeWindowsLength = node.windows.size();
							for (uSize i = 0; i < nodeWindowsLength; i += 1)
							{
								auto& window = node.windows[i];
								SizeHint titleBarSizeHint = impl::TextManager::GetSizeHint(
									implData.textManager,
									window.title);
								Rect tabRect{};
								tabRect.position = rect.position;
								tabRect.position.x += tabHoriOffset;
								tabRect.extent = titleBarSizeHint.preferred;
								// Draw tab highlight
								if (i == node.selectedWindow)
									drawInfo.PushFilledQuad(tabRect, window.titleBarColor + Math::Vec4{ 0.25f, 0.25f, 0.25f, 1.f });

								impl::TextManager::RenderText(
									implData.textManager,
									window.title,
									{ 1.f, 1.f, 1.f, 1.f },
									tabRect,
									drawInfo);

								tabHoriOffset += tabRect.extent.width;
							}

							// Draw the window content
							if (window.widget)
							{
								drawInfo.PushScissor(contentRect);
								window.widget->Render(
									ctx,
									drawInfo.GetFramebufferExtent(),
									contentRect,
									contentRect,
									drawInfo);
								drawInfo.PopScissor();
							}
						}
						else if (node.type == DockArea::Node::Type::HoriSplit || node.type == DockArea::Node::Type::VertSplit)
						{
							Rect resizeHandleRect = impl::DockArea_GetSplitResizeHandle(
								node.type,
								rect,
								node.split.split,
								dockArea.resizeAreaThickness,
								dockArea.resizeHandleLength);
							drawInfo.PushFilledQuad(resizeHandleRect, dockArea.resizeHandleColor);
						}
					});

				impl::DockArea_IterateResizeRects(
					layerRect,
					dockArea.resizeAreaThickness,
					dockArea.resizeHandleLength,
					[&dockArea, &drawInfo, layerIndex](
						DockArea_ResizeRect side,
						Rect rect)
					{
						if (layerIndex != dockArea.layers.size() - 1)
							drawInfo.PushFilledQuad(rect, dockArea.resizeHandleColor);
					});
			});
	}

	static void DockArea_RenderLayoutGizmos(
		DockArea const& dockArea,
		ImplData const& implData,
		Rect widgetRect,
		Extent framebufferExtent,
		DrawInfo& drawInfo)
	{
		//DENGINE_IMPL_GUI_ASSERT(dockArea.behaviorData.GetIndex() == (int)DockArea::Behavior::Moving);
		//DENGINE_IMPL_GUI_ASSERT(dockArea.behaviorData.moving.showLayoutNodePtr);
		
		auto& behaviorDataMoving = dockArea.behaviorData.Get<DockArea::BehaviorData_Moving>();

		DENGINE_IMPL_GUI_ASSERT(behaviorDataMoving.showLayoutNodePtr);

		Rect windowRect{};
		// First find the node
		bool nodeFound = false;
		bool continueIterating = true;
		impl::DockArea_IterateLayers(
			DockArea_IterateLayerMode::FrontToBack,
			dockArea.layers,
			widgetRect,
			continueIterating,
			[&behaviorDataMoving, &windowRect, &nodeFound](
				DockArea::Layer const& layer,
				Rect layerRect,
				uSize layerIndex,
				bool& continueIterating)
			{
				impl::DockArea_IterateNode(
					*layer.node,
					layerRect,
					continueIterating,
					impl::DockArea_IterateNode_SplitCallableOrder::First,
					[&behaviorDataMoving, &windowRect, &nodeFound](
						DockArea::Node const& node,
						Rect rect,
						DockArea::Node const* parentNode,
						bool& continueIterating)
					{
						if (&node != behaviorDataMoving.showLayoutNodePtr)
							return;
						continueIterating = false;
						nodeFound = true;
						windowRect = rect;
					});
			});

		// Then draw the gizmos.
		DENGINE_IMPL_GUI_ASSERT(nodeFound);
		windowRect.position.y += implData.textManager.lineheight;
		windowRect.extent.height -= implData.textManager.lineheight;
		DockArea_IterateLayoutGizmos(
			windowRect,
			dockArea.gizmoSize,
			dockArea.gizmoPadding,
			[&drawInfo](
				impl::DockArea_LayoutGizmo gizmo,
				Rect rect)
			{
				drawInfo.PushFilledQuad(rect, { 1.f, 1.f, 1.f, 0.75f });
			});

		// Draw the highlight rect on top
		if (behaviorDataMoving.useHighlightGizmo)
		{
			Rect highlightRect = windowRect;
			switch (behaviorDataMoving.highlightGizmo)
			{
			case impl::DockArea_LayoutGizmo::Top:
				highlightRect.extent.height = highlightRect.extent.height / 2;
				break;
			case impl::DockArea_LayoutGizmo::Bottom:
				highlightRect.extent.height = highlightRect.extent.height / 2;
				highlightRect.position.y += highlightRect.extent.height;
				break;
			case impl::DockArea_LayoutGizmo::Left:
				highlightRect.extent.width = highlightRect.extent.width / 2;
				break;
			case impl::DockArea_LayoutGizmo::Right:
				highlightRect.extent.width = highlightRect.extent.width / 2;
				highlightRect.position.x += highlightRect.extent.width;
				break;
			default:
				break;
			}
			drawInfo.PushFilledQuad(highlightRect, { 0.f, 0.5f, 1.f, 0.25f });
		}
	}
}

void DockArea::Render(
	Context const& ctx, 
	Extent framebufferExtent, 
	Rect widgetRect,
	Rect visibleRect, 
	DrawInfo& drawInfo) const
{
	if (layers.empty())
		return;

	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	drawInfo.PushScissor(visibleRect);

	impl::DockArea_RenderWindows(
		ctx,
		*this,
		implData,
		layers,
		widgetRect,
		framebufferExtent,
		drawInfo);

	// We need to draw the layout gizmo stuff
	if (auto ptr = this->behaviorData.ToPtr<BehaviorData_Moving>())
	{
		if (ptr->showLayoutNodePtr)
		{
			impl::DockArea_RenderLayoutGizmos(
				*this,
				implData,
				widgetRect,
				framebufferExtent,
				drawInfo);
		}

		Rect deleteGizmoRect = impl::DockArea_GetDeleteGizmoRect(widgetRect, this->gizmoSize);
		drawInfo.PushFilledQuad(deleteGizmoRect, { 1.f, 0.f, 0.f, 0.75f });
		
	}

	drawInfo.PopScissor();
}


namespace DEngine::Gui::impl
{
	static void DockArea_CursorClick_Behavior_Normal(
		Context& ctx,
		WindowID windowId,
		DockArea& dockArea,
		Rect widgetRect,
		Rect visibleRect,
		Math::Vec2Int cursorPos,
		CursorClickEvent event,
		ImplData& implData)
	{
		bool cursorInsideWidget = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);
		if (!cursorInsideWidget)
			return;
		// First check if we hit the resize rects
		bool continueIterating = true;
		impl::DockArea_IterateLayers(
			impl::DockArea_IterateLayerMode::FrontToBack,
			dockArea.layers,
			widgetRect,
			continueIterating,
			[&ctx, windowId, &dockArea, &implData, event, cursorPos](
				DockArea::Layer& layer,
				Rect layerRect,
				uSize layerIndex,
				bool& continueIterating)
		{
			Std::Opt<impl::DockArea_ResizeRect> resizeRectHit;
			if (event.clicked && continueIterating && layerIndex != dockArea.layers.size() - 1)
			{
				impl::DockArea_IterateResizeRects(
					layerRect,
					dockArea.resizeAreaThickness,
					dockArea.resizeHandleLength,
					[layerRect, &resizeRectHit, cursorPos](impl::DockArea_ResizeRect side, Rect rect)
					{
						if (!resizeRectHit.HasValue() && rect.PointIsInside(cursorPos))
							resizeRectHit = Std::Opt<impl::DockArea_ResizeRect>{ side };
					});
				if (resizeRectHit.HasValue())
				{
					continueIterating = false;
					impl::DockArea_PushLayerToFront(dockArea.layers, layerIndex);
					dockArea.behaviorData.Set(DockArea::BehaviorData_Resizing{});
					auto& behaviorData = dockArea.behaviorData.Get<DockArea::BehaviorData_Resizing>();
					switch (resizeRectHit.Value())
					{
					case DockArea_ResizeRect::Top: behaviorData.resizeSide = DockArea::ResizeSide::Top; break;
					case DockArea_ResizeRect::Bottom: behaviorData.resizeSide = DockArea::ResizeSide::Bottom; break;
					case DockArea_ResizeRect::Left: behaviorData.resizeSide = DockArea::ResizeSide::Left; break;
					case DockArea_ResizeRect::Right: behaviorData.resizeSide = DockArea::ResizeSide::Right; break;
					default:
						DENGINE_DETAIL_UNREACHABLE();
						break;
					}
				}
			}

			if (!resizeRectHit.HasValue() && layerRect.PointIsInside(cursorPos))
			{
				// Iterate through nodes, check if we hit a tab or the titlebar.
				impl::DockArea_IterateNode(
					*layer.node,
					layerRect,
					continueIterating,
					impl::DockArea_IterateNode_SplitCallableOrder::First,
					[&ctx, windowId, &dockArea, &implData, cursorPos, event, layerRect, layerIndex](
						DockArea::Node& node,
						Rect rect,
						DockArea::Node* parentNode,
						bool& continueIterating)
				{
					if (event.clicked && (node.type == DockArea::Node::Type::HoriSplit || node.type == DockArea::Node::Type::VertSplit))
					{
						Rect resizeHandleRect = impl::DockArea_GetSplitResizeHandle(
							node.type,
							rect,
							node.split.split,
							dockArea.resizeAreaThickness,
							dockArea.resizeHandleLength);
						if (resizeHandleRect.PointIsInside(cursorPos))
						{
							continueIterating = false;
							if (layerIndex != dockArea.layers.size() - 1)
								impl::DockArea_PushLayerToFront(dockArea.layers, layerIndex);
							dockArea.behaviorData.Set(DockArea::BehaviorData_Resizing{});
							auto& behaviorData = dockArea.behaviorData.Get<DockArea::BehaviorData_Resizing>();
							behaviorData.resizingSplit = true;
							behaviorData.resizeSplitNode = &node;
							behaviorData.resizingSplitIsFrontNode = layerIndex != dockArea.layers.size() - 1;
							return;
						}
					}
					else if (node.type == DockArea::Node::Type::Window)
					{
						DENGINE_IMPL_GUI_ASSERT(!node.windows.empty());
						if (rect.PointIsInside(cursorPos))
						{
							continueIterating = false;

							Rect titlebarRect = rect;
							titlebarRect.extent.height = implData.textManager.lineheight;
							if (event.clicked && titlebarRect.PointIsInside(cursorPos))
							{
								// Check if we hit one of the tabs, if so, select it
								bool tabWasHit = false;
								if (parentNode || node.windows.size() > 1)
								{
									u32 tabHoriOffset = 0;
									for (uSize windowIndex = 0; !tabWasHit && windowIndex < node.windows.size(); windowIndex += 1)
									{
										auto& window = node.windows[windowIndex];
										SizeHint titleBarSizeHint = impl::TextManager::GetSizeHint(
											implData.textManager,
											window.title);
										Rect tabRect{};
										tabRect.position = rect.position;
										tabRect.position.x += tabHoriOffset;
										tabRect.extent = titleBarSizeHint.preferred;
										tabHoriOffset += tabRect.extent.width;
										if (tabRect.PointIsInside(cursorPos))
										{
											tabWasHit = true;
											// Select this window to display, and bring it the top-level-node to front.
											node.selectedWindow = windowIndex;
											if (layerIndex != dockArea.layers.size() - 1)
												impl::DockArea_PushLayerToFront(dockArea.layers, layerIndex);
											dockArea.behaviorData.Set(DockArea::BehaviorData_HoldingTab{});
											auto& behaviorData = dockArea.behaviorData.Get<DockArea::BehaviorData_HoldingTab>();
											behaviorData.holdingTab = &node;
											behaviorData.holdingFrontWindow = layerIndex != dockArea.layers.size() - 1;
											behaviorData.cursorPosRelative = cursorPos - tabRect.position;
											break;
										}
									}
								}
								if (!tabWasHit && layerIndex != dockArea.layers.size() - 1)
								{
									// We hit the title bar but not any tabs, start moving it and bring it to front
									impl::DockArea_PushLayerToFront(dockArea.layers, layerIndex);
									dockArea.behaviorData.Set(DockArea::BehaviorData_Moving{});
									auto& behaviorData = dockArea.behaviorData.Get<DockArea::BehaviorData_Moving>();
									behaviorData.windowMovedRelativePos = cursorPos - layerRect.position;
								}
							}
							Rect contentRect = rect;
							contentRect.position.y += titlebarRect.extent.height;
							contentRect.extent.height -= titlebarRect.extent.height;
							if (contentRect.PointIsInside(cursorPos))
							{
								auto& window = node.windows[node.selectedWindow];
								if (window.widget)
								{
									window.widget->CursorClick(
										ctx,
										windowId,
										contentRect,
										contentRect,
										cursorPos,
										event);
								}
							}
						}
					}
				});
			}
		});
	}

	static void DockArea_CursorClick_Behavior_Moving(
		DockArea& dockArea,
		Rect widgetRect,
		Rect visibleRect,
		Math::Vec2Int cursorPos,
		CursorClickEvent event,
		ImplData& implData)
	{
		bool isInsideWidget = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);

		if (isInsideWidget && !event.clicked)
		{
			bool continueIterating = true;

			// Check if we are above the delete gizmo
			Rect deleteGizmo = impl::DockArea_GetDeleteGizmoRect(widgetRect, dockArea.gizmoSize);
			if (deleteGizmo.PointIsInside(cursorPos))
			{
				// Delete the front-most top-level node.
				continueIterating = false;
				dockArea.layers.erase(dockArea.layers.begin());
			}

			// Find out if we need to dock a window into another.
			if (continueIterating)
			{
				impl::DockArea_IterateLayers(
					impl::DockArea_IterateLayerMode::FrontToBack,
					dockArea.layers,
					widgetRect,
					continueIterating,
					[&dockArea, &implData, cursorPos](
						DockArea::Layer& layer,
						Rect layerRect,
						uSize layerIndex,
						bool& continueIterating)
					{
						// We don't need to test the front-most node, since that's the one being moved.
						if (layerIndex == 0)
							return;

						// We search for the node in which we are showing layout gizmos
						impl::DockArea_IterateNode(
							*layer.node,
							layerRect,
							continueIterating,
							impl::DockArea_IterateNode_SplitCallableOrder::First,
							[&dockArea, &implData, cursorPos](
								DockArea::Node& node,
								Rect rect,
								DockArea::Node* parentNode,
								bool& continueIterating)
							{
								if (&node != dockArea.behaviorData.Get<DockArea::BehaviorData_Moving>().showLayoutNodePtr)
									return;

								continueIterating = false;
								auto& window = node.windows[node.selectedWindow];
								// Check if we are within the window's main area first
								Rect contentRect = rect;
								contentRect.position.y += implData.textManager.lineheight;
								contentRect.extent.height -= implData.textManager.lineheight;
								if (contentRect.PointIsInside(cursorPos))
								{
									bool gizmoWasHit = false;
									DockArea_IterateLayoutGizmos(
										contentRect,
										dockArea.gizmoSize,
										dockArea.gizmoPadding,
										[&dockArea, &node, cursorPos, &gizmoWasHit](
											DockArea_LayoutGizmo gizmo,
											Rect rect)
										{
											if (!gizmoWasHit && rect.PointIsInside(cursorPos))
											{
												gizmoWasHit = true;
												if (gizmo == DockArea_LayoutGizmo::Left || gizmo == DockArea_LayoutGizmo::Right)
													node.type = DockArea::Node::Type::HoriSplit;
												else if (gizmo == DockArea_LayoutGizmo::Top || gizmo == DockArea_LayoutGizmo::Bottom)
													node.type = DockArea::Node::Type::VertSplit;
												if (gizmo == DockArea_LayoutGizmo::Left || gizmo == DockArea_LayoutGizmo::Right ||
													gizmo == DockArea_LayoutGizmo::Top || gizmo == DockArea_LayoutGizmo::Bottom)
												{
													DockArea::Node* newNode = new DockArea::Node;
													auto& nodeA = gizmo == DockArea_LayoutGizmo::Left || gizmo == DockArea_LayoutGizmo::Top ? node.split.a : node.split.b;
													auto& nodeB = gizmo == DockArea_LayoutGizmo::Left || gizmo == DockArea_LayoutGizmo::Top ? node.split.b : node.split.a;
													nodeB = Std::Box{ newNode };
													newNode->type = DockArea::Node::Type::Window;
													newNode->windows = Std::Move(node.windows);
													newNode->selectedWindow = node.selectedWindow;
													node.selectedWindow = 0;
													nodeA = Std::Move(dockArea.layers.front().node);
													dockArea.layers.erase(dockArea.layers.begin());
												}

												if (gizmo == DockArea_LayoutGizmo::Center && dockArea.layers.front().node->type == DockArea::Node::Type::Window)
												{
													node.selectedWindow = node.windows.size() + dockArea.layers.front().node->selectedWindow;
													for (auto& window : dockArea.layers.front().node->windows)
														node.windows.emplace_back(Std::Move(window));
													dockArea.layers.erase(dockArea.layers.begin());
												}
											}
										});
								}
							});
					});
			}
		}

		if (!event.clicked)
		{
			dockArea.behaviorData.Set(DockArea::BehaviorData_Normal{});
		}
	}

	static void DockArea_CursorClick_Behavior_Resizing(
		DockArea& dockArea,
		Rect widgetRect,
		Rect visibleRect,
		CursorClickEvent event)
	{
		if (!event.clicked)
		{
			dockArea.behaviorData.Set(DockArea::BehaviorData_Normal{});
		}
	}

	static void DockArea_CursorClick_Behavior_HoldingTab(
		DockArea& dockArea,
		Rect widgetRect,
		Rect visibleRect,
		Math::Vec2Int cursorPos,
		CursorClickEvent event)
	{
		if (!event.clicked)
		{
			dockArea.behaviorData.Set(DockArea::BehaviorData_Normal{});
		}
	}
}

void DockArea::CursorClick(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	bool isInsideWidget = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);

	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	if (this->behaviorData.ToPtr<BehaviorData_Normal>())
	{
		impl::DockArea_CursorClick_Behavior_Normal(
			ctx,
			windowId,
			*this,
			widgetRect,
			visibleRect,
			cursorPos,
			event,
			implData);
	}
	else if (this->behaviorData.ToPtr<BehaviorData_Moving>())
	{
		impl::DockArea_CursorClick_Behavior_Moving(
			*this,
			widgetRect,
			visibleRect,
			cursorPos,
			event,
			implData);
	}
	else if (this->behaviorData.ToPtr<BehaviorData_Resizing>())
	{
		impl::DockArea_CursorClick_Behavior_Resizing(
			*this,
			widgetRect,
			visibleRect,
			event);
	}
	else if (this->behaviorData.ToPtr<BehaviorData_HoldingTab>())
	{
		impl::DockArea_CursorClick_Behavior_HoldingTab(
			*this,
			widgetRect,
			visibleRect,
			cursorPos,
			event);
	}
	else
		DENGINE_DETAIL_UNREACHABLE();
}


namespace DEngine::Gui::impl
{
	static void DockArea_CursorMove_Behavior_Normal(
		Context& ctx,
		WindowID windowId,
		DockArea& dockArea,
		ImplData& implData,
		Rect widgetRect,
		Rect visibleRect,
		Gui::CursorMoveEvent event)
	{
		// Check if we are hovering a resize rect.
		bool rectWasHit = false;
		// Iterate over every window.
		bool continueIterating = true;
		impl::DockArea_IterateLayers(
			impl::DockArea_IterateLayerMode::FrontToBack,
			dockArea.layers,
			widgetRect,
			continueIterating,
			[&ctx, windowId, &dockArea, &implData, event, &rectWasHit](
				DockArea::Layer& layer,
				Rect layerRect,
				uSize layerIndex,
				bool& continueIterating)
			{
				// First we check if we hit a resize rect. We do not check this for the rear-most one
				// Because it is always full-size.
				if (rectWasHit)
					return;
				Std::Opt<DockArea_ResizeRect> resizeRectHit;
				if (layerIndex != dockArea.layers.size() - 1)
				{
					DockArea_IterateResizeRects(
						layerRect,
						dockArea.resizeAreaThickness,
						dockArea.resizeHandleLength,
						[layerRect, event, &resizeRectHit](DockArea_ResizeRect side, Rect rect)
					{
						if (!resizeRectHit.HasValue() && rect.PointIsInside(event.position))
							resizeRectHit = Std::Opt<DockArea_ResizeRect>{ side };
					});
				}

				if (continueIterating && resizeRectHit.HasValue())
				{
					continueIterating = false;
					rectWasHit = true;
					switch (resizeRectHit.Value())
					{
					case DockArea_ResizeRect::Top:
					case DockArea_ResizeRect::Bottom:
						ctx.GetWindowHandler().SetCursorType(windowId, CursorType::VerticalResize);
						break;
					case DockArea_ResizeRect::Left:
					case DockArea_ResizeRect::Right:
						ctx.GetWindowHandler().SetCursorType(windowId, CursorType::HorizontalResize);
						break;
					}
					return;
				}
				if (continueIterating && layerRect.PointIsInside(event.position))
				{
					continueIterating = false;
					bool continueIterating2 = true;
					impl::DockArea_IterateNode(
						*layer.node,
						layerRect,
						continueIterating2,
						impl::DockArea_IterateNode_SplitCallableOrder::First,
						[&ctx, windowId, &implData, &dockArea, event, &rectWasHit](
							DockArea::Node& node,
							Rect rect,
							DockArea::Node* parentNode,
							bool& continueIterating)
						{
							if (node.type == DockArea::Node::Type::HoriSplit || node.type == DockArea::Node::Type::VertSplit)
							{
								Rect resizeHandleRect = impl::DockArea_GetSplitResizeHandle(
									node.type,
									rect,
									node.split.split,
									dockArea.resizeAreaThickness,
									dockArea.resizeHandleLength);
								if (resizeHandleRect.PointIsInside(event.position))
								{
									continueIterating = false;
									rectWasHit = true;
									if (node.type == DockArea::Node::Type::HoriSplit)
										ctx.GetWindowHandler().SetCursorType(windowId, CursorType::HorizontalResize);
									else if (node.type == DockArea::Node::Type::VertSplit)
										ctx.GetWindowHandler().SetCursorType(windowId, CursorType::VerticalResize);
									return;
								}
							}
							else if (node.type == DockArea::Node::Type::Window)
							{
								Rect contentRect = rect;
								contentRect.position.y += implData.textManager.lineheight;
								contentRect.extent.height -= implData.textManager.lineheight;
								if (contentRect.PointIsInside(event.position))
								{
									auto& window = node.windows[node.selectedWindow];
									if (window.widget)
									{
										window.widget->CursorMove(
											ctx,
											windowId,
											contentRect,
											contentRect,
											event);
									}
								}
							}
						});
				}
			});
		if (rectWasHit)
			return;
		else
			ctx.GetWindowHandler().SetCursorType(windowId, CursorType::Arrow);
	}

	static void DockArea_CursorMove_Behavior_Moving(
		DockArea& dockArea,
		ImplData& implData,
		Rect widgetRect,
		Rect visibleRect,
		Math::Vec2Int cursorPos)
	{
		bool cursorInsideWidget = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);
		if (cursorInsideWidget)
		{
			auto& behaviorData = dockArea.behaviorData.Get<DockArea::BehaviorData_Moving>();

			// First move the window.
			auto& layer = dockArea.layers.front();
			layer.rect.position = cursorPos - widgetRect.position - behaviorData.windowMovedRelativePos;

			// Check if mouse is covering the delete gizmo

			bool continueIterating = true;
			bool windowContentWasHit = false;
			impl::DockArea_IterateLayers(
				impl::DockArea_IterateLayerMode::FrontToBack,
				dockArea.layers,
				widgetRect,
				continueIterating,
				[&dockArea, &implData, cursorPos, &windowContentWasHit, &behaviorData](
					DockArea::Layer& layer,
					Rect layerRect,
					uSize layerIndex,
					bool& continueIterating)
				{
					// Jump over the first layer. It's the one we're moving.
					if (layerIndex == 0)
						return;
					impl::DockArea_IterateNode(
						*layer.node,
						layerRect,
						continueIterating,
						impl::DockArea_IterateNode_SplitCallableOrder::First,
						[&dockArea, &implData, cursorPos, &windowContentWasHit, &behaviorData](
							DockArea::Node& node,
							Rect rect,
							DockArea::Node* parentNode,
							bool& continueIterating)
						{
							if (node.type != DockArea::Node::Type::Window)
								return;
							// If we hit the titlebar, we stop iterating.
							Rect titlebarRect = rect;
							titlebarRect.extent.height = implData.textManager.lineheight;
							if (titlebarRect.PointIsInside(cursorPos))
								continueIterating = false;
							// Check if the cursor is inside the contentRect.
							Rect contentRect = rect;
							contentRect.extent.height -= titlebarRect.extent.height;
							contentRect.position.y += titlebarRect.extent.height;
							if (contentRect.PointIsInside(cursorPos))
							{
								behaviorData.showLayoutNodePtr = &node;
								continueIterating = false;
								windowContentWasHit = true;

								// Check if we are hovering any of the layout gizmos, and apply the highlight rect if we are.
								behaviorData.useHighlightGizmo = false;
								impl::DockArea_IterateLayoutGizmos(
									contentRect,
									dockArea.gizmoSize,
									dockArea.gizmoPadding,
									[&dockArea, cursorPos, &behaviorData](
										impl::DockArea_LayoutGizmo gizmo,
										Rect rect)
									{
										if (!behaviorData.useHighlightGizmo && rect.PointIsInside(cursorPos))
										{
											behaviorData.useHighlightGizmo = true;
											behaviorData.highlightGizmo = gizmo;
										}
									});
							}
						});
				});
			if (!windowContentWasHit)
				behaviorData.showLayoutNodePtr = nullptr;
		}
	}

	static void DockArea_CursorMove_Behavior_Resizing(
		DockArea& dockArea,
		ImplData& implData,
		Rect widgetRect,
		Rect visibleRect,
		Math::Vec2Int cursorPos)
	{
		bool cursorInsideWidget = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);
		if (!cursorInsideWidget)
			return;
		DENGINE_IMPL_GUI_ASSERT(!dockArea.layers.empty());
		
		auto& behaviorData = dockArea.behaviorData.Get<DockArea::BehaviorData_Resizing>();

		if (behaviorData.resizingSplit)
		{
			auto& layer = behaviorData.resizingSplitIsFrontNode ? dockArea.layers.front() : dockArea.layers.back();
			Rect windowRect = layer.rect;
			if (!behaviorData.resizingSplitIsFrontNode)
			{
				windowRect.position = {};
				windowRect.extent = widgetRect.extent;
			}
				
			bool continueIterating = true;
			impl::DockArea_IterateNode(
				*layer.node,
				windowRect,
				continueIterating,
				impl::DockArea_IterateNode_SplitCallableOrder::First,
				[&dockArea, cursorPos, widgetRect, &behaviorData](
					DockArea::Node& node,
					Rect rect,
					DockArea::Node* parentNode,
					bool& continueIterating)
				{
					if ((node.type == DockArea::Node::Type::HoriSplit || node.type == DockArea::Node::Type::VertSplit) &&
							 &node == behaviorData.resizeSplitNode)
					{
						continueIterating = false;

						if (node.type == DockArea::Node::Type::HoriSplit)
						{
							// Find out the position of the cursor in range 0-1 of the node.
							f32 test = f32(cursorPos.x - widgetRect.position.x - rect.position.x) / rect.extent.width;
							node.split.split = Math::Clamp(test, 0.1f, 0.9f);
						}
						else if (node.type == DockArea::Node::Type::VertSplit)
						{
							// Find out the position of the cursor in range 0-1 of the node.
							f32 test = f32(cursorPos.y - widgetRect.position.y - rect.position.y) / rect.extent.height;
							node.split.split = Math::Clamp(test, 0.1f, 0.9f);
						}
					}
				});
		}
		else
		{
			auto& layer = dockArea.layers.front();
			if (behaviorData.resizeSide == DockArea::ResizeSide::Top)
			{
				i32 prevY = layer.rect.position.y;
				i32 a = cursorPos.y - widgetRect.position.y;
				i32 b = layer.rect.position.y + (i32)layer.rect.extent.height - 100;
				layer.rect.position.y = Math::Min(a, b);
				layer.rect.extent.height += prevY - layer.rect.position.y;
			}
			else if (behaviorData.resizeSide == DockArea::ResizeSide::Bottom)
			{
				layer.rect.extent.height = Math::Max(0, cursorPos.y - widgetRect.position.y - layer.rect.position.y);
			}
			else if (behaviorData.resizeSide == DockArea::ResizeSide::Left)
			{
				i32 prevX = layer.rect.position.x;
				i32 a = cursorPos.x - widgetRect.position.x;
				i32 b = layer.rect.position.x + (i32)layer.rect.extent.width - 100;
				layer.rect.position.x = Math::Min(a, b);
				layer.rect.extent.width += prevX - layer.rect.position.x;
			}
			else if (behaviorData.resizeSide == DockArea::ResizeSide::Right)
			{
				layer.rect.extent.width = Math::Max(0, cursorPos.x - widgetRect.position.x - layer.rect.position.x);
			}
			layer.rect.extent.height = Math::Max(layer.rect.extent.height, 100U);
			layer.rect.extent.width = Math::Max(layer.rect.extent.width, 100U);
		}
	}

	static void DockArea_CursorMove_Behavior_HoldingTab(
		DockArea& dockArea,
		ImplData& implData,
		Rect widgetRect,
		Rect visibleRect,
		Math::Vec2Int cursorPos)
	{
		bool cursorInsideWidget = widgetRect.PointIsInside(cursorPos);

		// First we check if we need to disconnect any tabs
		if (cursorInsideWidget)
		{
			auto& behaviorData = dockArea.behaviorData.Get<DockArea::BehaviorData_HoldingTab>();
			auto& layer = behaviorData.holdingFrontWindow ? dockArea.layers.front() : dockArea.layers.back();
			Rect layerRect = layer.rect;
			layerRect.position += widgetRect.position;
			if (!behaviorData.holdingFrontWindow)
			{
				layerRect = widgetRect;
			}
			bool continueIterating = true;
			impl::DockArea_IterateNode(
				*layer.node,
				layerRect,
				continueIterating,
				impl::DockArea_IterateNode_SplitCallableOrder::First,
				[&dockArea, &implData, widgetRect, cursorPos, &behaviorData](
					DockArea::Node& node,
					Rect rect,
					DockArea::Node* parentNode,
					bool& continueIterating)
				{
					if (node.type != DockArea::Node::Type::Window)
						return;
					if (&node != behaviorData.holdingTab)
						return;

					// First find out the rect of the tab we are holding
					Rect tabRect{};
					tabRect.position = rect.position;
					tabRect.extent.height = implData.textManager.lineheight;
					for (uSize nodeWindowIndex = 0; nodeWindowIndex < node.windows.size(); nodeWindowIndex += 1)
					{
						auto& window = node.windows[nodeWindowIndex];
						SizeHint titleBarSizeHint = impl::TextManager::GetSizeHint(
							implData.textManager,
							window.title);
						tabRect.position.x += titleBarSizeHint.preferred.width;
						if (nodeWindowIndex == node.selectedWindow)
						{
							tabRect.extent.width = titleBarSizeHint.preferred.width;
							break;
						}
					}

					if ((cursorPos.y <= tabRect.position.y - dockArea.tabDisconnectRadius) ||
						(cursorPos.y >= tabRect.position.y + tabRect.extent.height + dockArea.tabDisconnectRadius))
					{
						// We want to disconnect this tab into a new top-level node.
						DockArea::Layer newLayer{};
						newLayer.rect = rect;
						newLayer.rect.position = cursorPos - widgetRect.position - behaviorData.cursorPosRelative;
						DockArea::Node* newNode = new DockArea::Node;
						newLayer.node = Std::Box{ newNode };
						newNode->type = DockArea::Node::Type::Window;
						newNode->windows.push_back(Std::Move(node.windows[node.selectedWindow]));
						dockArea.layers.emplace(dockArea.layers.begin(), Std::Move(newLayer));

						// Remove the window from the node
						node.windows.erase(node.windows.begin() + node.selectedWindow);
						if (node.selectedWindow > 0)
							node.selectedWindow -= 1;

						// We are now moving this tab.
						Math::Vec2Int cursorRelativePos = behaviorData.cursorPosRelative;
						dockArea.behaviorData.Set(DockArea::BehaviorData_Moving{});
						auto& behaviorDataMoving = dockArea.behaviorData.Get<DockArea::BehaviorData_Moving>();
						behaviorDataMoving.windowMovedRelativePos = cursorRelativePos;

						if (node.windows.empty())
						{
							// We want to turn the parent-node from a split into a window-node.
							DENGINE_IMPL_GUI_ASSERT(parentNode);
							DENGINE_IMPL_GUI_ASSERT(parentNode->split.a == &node || parentNode->split.b == &node);
							if (parentNode->split.a == &node)
							{
								DockArea::Node temp = Std::Move(*parentNode);
								*parentNode = Std::Move(*temp.split.b);
							}
							else
							{
								DockArea::Node temp = Std::Move(*parentNode);
								*parentNode = Std::Move(*temp.split.a);
							}
						}
					}
				});
		}
	}
}

void DockArea::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect, 
	Rect visibleRect, 
	CursorMoveEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	bool cursorInsideWidget = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);

	if (this->behaviorData.ToPtr<BehaviorData_Normal>())
	{
		impl::DockArea_CursorMove_Behavior_Normal(
			ctx,
			windowId,
			*this,
			implData,
			widgetRect,
			visibleRect,
			event);
	}
	else if (this->behaviorData.ToPtr<BehaviorData_Moving>())
	{
		impl::DockArea_CursorMove_Behavior_Moving(
			*this,
			implData,
			widgetRect,
			visibleRect,
			event.position);
	}
	else if (this->behaviorData.ToPtr<BehaviorData_Resizing>())
	{
		impl::DockArea_CursorMove_Behavior_Resizing(
			*this,
			implData,
			widgetRect,
			visibleRect,
			event.position);
	}
	else if (this->behaviorData.ToPtr<BehaviorData_HoldingTab>())
	{
		impl::DockArea_CursorMove_Behavior_HoldingTab(
			*this,
			implData,
			widgetRect,
			visibleRect,
			event.position);
	}
	else
		DENGINE_DETAIL_UNREACHABLE();
}

namespace DEngine::Gui::impl
{
	static void DockArea_TouchEvent_Behavior_Normal(
		Context& ctx,
		ImplData& implData,
		DockArea& dockArea,
		WindowID windowId,
		Rect widgetRect,
		Rect visibleRect,
		Gui::TouchEvent event)
	{
		Math::Vec2Int cursorPos = { (i32)event.position.x, (i32)event.position.y };
		bool isInsideWidget = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);
		if (isInsideWidget)
		{
			bool continueIterating = true;
			impl::DockArea_IterateLayers(
				impl::DockArea_IterateLayerMode::FrontToBack,
				dockArea.layers,
				widgetRect,
				continueIterating,
				[&ctx, &dockArea, &implData, windowId, event](
					DockArea::Layer& layer,
					Rect layerRect,
					uSize layerIndex,
					bool& continueIterating)
			{
				Std::Opt<impl::DockArea_ResizeRect> resizeRectHit;
				if (event.type == TouchEventType::Down && continueIterating &&
					layerIndex != dockArea.layers.size() - 1)
				{
					impl::DockArea_IterateResizeRects(
						layerRect,
						dockArea.resizeAreaThickness,
						dockArea.resizeHandleLength,
						[layerRect, &resizeRectHit, event](impl::DockArea_ResizeRect side, Rect rect)
					{
						if (!resizeRectHit.HasValue() && rect.PointIsInside(event.position))
							resizeRectHit = Std::Opt<impl::DockArea_ResizeRect>{ side };
					});
					if (resizeRectHit.HasValue())
					{
						continueIterating = false;
						impl::DockArea_PushLayerToFront(dockArea.layers, layerIndex);
						dockArea.behaviorData.Set(DockArea::BehaviorData_Resizing{});
						auto& behaviorData = dockArea.behaviorData.Get<DockArea::BehaviorData_Resizing>();
						switch (resizeRectHit.Value())
						{
							case impl::DockArea_ResizeRect::Top:
								behaviorData.resizeSide = DockArea::ResizeSide::Top;
								break;
							case impl::DockArea_ResizeRect::Bottom:
								behaviorData.resizeSide = DockArea::ResizeSide::Bottom;
								break;
							case impl::DockArea_ResizeRect::Left:
								behaviorData.resizeSide = DockArea::ResizeSide::Left;
								break;
							case impl::DockArea_ResizeRect::Right:
								behaviorData.resizeSide = DockArea::ResizeSide::Right;
								break;
						}
						}
					}
					if (!resizeRectHit.HasValue() && layerRect.PointIsInside(event.position))
					{
						// Iterate through nodes, check if we hit a tab or the titlebar.
						impl::DockArea_IterateNode(
							*layer.node,
							layerRect,
							continueIterating,
							impl::DockArea_IterateNode_SplitCallableOrder::First,
							[&ctx, &dockArea, &implData, windowId, event, layerRect, layerIndex](
								DockArea::Node &node,
								Rect rect,
								DockArea::Node *parentNode,
								bool &continueIterating)
							{
								if ((node.type == DockArea::Node::Type::HoriSplit ||
										 node.type == DockArea::Node::Type::VertSplit))
								{
									if (event.type == TouchEventType::Down)
									{
										Rect resizeHandleRect = impl::DockArea_GetSplitResizeHandle(
											node.type,
											rect,
											node.split.split,
											dockArea.resizeAreaThickness,
											dockArea.resizeHandleLength);
										if (resizeHandleRect.PointIsInside(event.position))
										{
											continueIterating = false;
											if (layerIndex != dockArea.layers.size() - 1)
												impl::DockArea_PushLayerToFront(dockArea.layers, layerIndex);
											dockArea.behaviorData.Set(DockArea::BehaviorData_Resizing{});
											auto& behaviorData = dockArea.behaviorData.Get< DockArea::BehaviorData_Resizing>();
											behaviorData.resizingSplit = true;
											behaviorData.resizeSplitNode = &node;
											behaviorData.resizingSplitIsFrontNode = layerIndex != dockArea.layers.size() - 1;
											return;
										}
									}
								}
								else if (node.type == DockArea::Node::Type::Window)
								{
									DENGINE_IMPL_GUI_ASSERT(!node.windows.empty());
									if (rect.PointIsInside(event.position))
									{
										continueIterating = false;

										Rect titlebarRect = rect;
										titlebarRect.extent.height = implData.textManager.lineheight;
										bool titleBarWasHit = false;
										if (event.type == TouchEventType::Down &&
												titlebarRect.PointIsInside(event.position))
										{
											titleBarWasHit = true;
											// Check if we hit one of the tabs, if so, select it
											bool tabWasHit = false;
											if (parentNode || node.windows.size() > 1)
											{
												u32 tabHoriOffset = 0;
												for (uSize windowIndex = 0; !tabWasHit && windowIndex < node.windows.size(); windowIndex += 1)
												{
													auto &window = node.windows[windowIndex];
													SizeHint titleBarSizeHint = impl::TextManager::GetSizeHint(
														implData.textManager,
														window.title);
													Rect tabRect{};
													tabRect.position = rect.position;
													tabRect.position.x += tabHoriOffset;
													tabRect.extent = titleBarSizeHint.preferred;
													tabHoriOffset += tabRect.extent.width;
													if (tabRect.PointIsInside(event.position))
													{
														tabWasHit = true;
														// Select this window to display, and bring it the top-level-node to front.
														node.selectedWindow = windowIndex;
														if (layerIndex != dockArea.layers.size() - 1)
															impl::DockArea_PushLayerToFront(dockArea.layers, layerIndex);
														dockArea.behaviorData.Set(DockArea::BehaviorData_HoldingTab{});
														auto& behaviorData = dockArea.behaviorData.Get<DockArea::BehaviorData_HoldingTab>();
														behaviorData.holdingTab = &node;
														behaviorData.holdingFrontWindow = layerIndex != dockArea.layers.size() - 1;
														behaviorData.cursorPosRelative = Math::Vec2Int{ (i32)event.position.x, (i32)event.position.y } - tabRect.position;
														break;
													}
												}
											}
											if (!tabWasHit && layerIndex != dockArea.layers.size() - 1)
											{
												// We hit the title bar but not any tabs, start moving it and bring it to front
												dockArea.behaviorData.Set(DockArea::BehaviorData_Moving{});
												auto& behaviorData = dockArea.behaviorData.Get< DockArea::BehaviorData_Moving>();
												behaviorData.windowMovedRelativePos = Math::Vec2Int{ (i32)event.position.x, (i32)event.position.y } - layerRect.position;
												impl::DockArea_PushLayerToFront(dockArea.layers, layerIndex);
											}
										}
										if (!titleBarWasHit)
										{
											Rect contentRect = rect;
											contentRect.position.y += titlebarRect.extent.height;
											contentRect.extent.height -= titlebarRect.extent.height;
											if (contentRect.PointIsInside(Math::Vec2Int{ (i32)event.position.x, (i32)event.position.y }))
											{
												auto &window = node.windows[node.selectedWindow];
												if (window.widget)
												{
													window.widget->TouchEvent(
														ctx,
														windowId,
														contentRect,
														contentRect,
														event);
												}
											}
										}
									}
								}
							});
					}
				});
		}
	}

	static void DockArea_TouchEvent_Behavior_Moving(
		Context& ctx,
		ImplData& implData,
		DockArea& dockArea,
		Rect widgetRect,
		Rect visibleRect,
		Gui::TouchEvent event)
	{
		Math::Vec2Int cursorPos = Math::Vec2Int{ (i32)event.position.x, (i32)event.position.y };
		if (event.id == 0 && event.type == TouchEventType::Moved)
		{
			bool cursorInsideWidget = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);
			if (cursorInsideWidget)
			{
				// First move the window.
				auto& layer = dockArea.layers.front();
				auto& behaviorData = dockArea.behaviorData.Get<DockArea::BehaviorData_Moving>();
				layer.rect.position = cursorPos - widgetRect.position - behaviorData.windowMovedRelativePos;

				bool continueIterating = true;

				bool windowContentWasHit = false;
				impl::DockArea_IterateLayers(
					impl::DockArea_IterateLayerMode::FrontToBack,
					dockArea.layers,
					widgetRect,
					continueIterating,
					[&dockArea, &implData, cursorPos, &windowContentWasHit, &behaviorData](
						DockArea::Layer& layer,
						Rect layerRect,
						uSize layerIndex,
						bool& continueIterating)
					{
						// Jump over the first top-level node. It's the one we're moving.
						if (layerIndex == 0)
							return;
						impl::DockArea_IterateNode(
							*layer.node,
							layerRect,
							continueIterating,
							impl::DockArea_IterateNode_SplitCallableOrder::First,
							[&dockArea, &implData, cursorPos, &windowContentWasHit, &behaviorData](
								DockArea::Node& node,
								Rect rect,
								DockArea::Node* parentNode,
								bool& continueIterating)
							{
								if (node.type != DockArea::Node::Type::Window)
									return;
								// If we hit the titlebar, we stop iterating.
								Rect titlebarRect = rect;
								titlebarRect.extent.height = implData.textManager.lineheight;
								if (titlebarRect.PointIsInside(cursorPos))
									continueIterating = false;
								// Check if the cursor is inside the contentRect.
								Rect contentRect = rect;
								contentRect.extent.height -= titlebarRect.extent.height;
								contentRect.position.y += titlebarRect.extent.height;
								if (contentRect.PointIsInside(cursorPos))
								{
									behaviorData.showLayoutNodePtr = &node;
									continueIterating = false;
									windowContentWasHit = true;

									// Check if we are hovering any of the layout gizmos, and apply the highlight rect if we are.
									behaviorData.useHighlightGizmo = false;
									impl::DockArea_IterateLayoutGizmos(
										contentRect,
										dockArea.gizmoSize,
										dockArea.gizmoPadding,
										[&dockArea, cursorPos, &behaviorData](
											impl::DockArea_LayoutGizmo gizmo,
											Rect rect)
										{
											if (!behaviorData.useHighlightGizmo && rect.PointIsInside(cursorPos))
											{
												behaviorData.useHighlightGizmo = true;
												behaviorData.highlightGizmo = gizmo;
											}
										});
								}
							});
					});
				if (!windowContentWasHit)
					behaviorData.showLayoutNodePtr = nullptr;
			}
		}
		else if (event.id == 0 && event.type == TouchEventType::Up)
		{
			auto& behaviorData = dockArea.behaviorData.Get<DockArea::BehaviorData_Moving>();
			
			// We don't need to test the front-most node, since that's the one being moved.
			bool continueIterating = true;

			// Check if we are above the delete gizmo
			Rect deleteGizmo = impl::DockArea_GetDeleteGizmoRect(widgetRect, dockArea.gizmoSize);
			if (deleteGizmo.PointIsInside(cursorPos))
			{
				// Delete the front-most top-level node.
				continueIterating = false;
				dockArea.layers.erase(dockArea.layers.begin());
			}

			if (continueIterating)
			{
				impl::DockArea_IterateLayers(
					impl::DockArea_IterateLayerMode::FrontToBack,
					dockArea.layers,
					widgetRect,
					continueIterating,
					[&dockArea, &implData, event, &behaviorData](
						DockArea::Layer& layer,
						Rect layerRect,
						uSize layerIndex,
						bool& continueIterating)
					{
						if (layerIndex == 0)
							return;
						DockArea_IterateNode(
							*layer.node,
							layerRect,
							continueIterating,
							impl::DockArea_IterateNode_SplitCallableOrder::First,
							[&dockArea, &implData, event, &behaviorData](
								DockArea::Node& node,
								Rect rect,
								DockArea::Node* parentNode,
								bool& continueIterating)
							{
								if (&node != behaviorData.showLayoutNodePtr)
									return;
								auto cursorPos = Math::Vec2Int{ (i32)event.position.x, (i32)event.position.y };
								continueIterating = false;
								auto& window = node.windows[node.selectedWindow];
								// Check if we are within the window's main area first
								Rect contentRect = rect;
								contentRect.position.y += implData.textManager.lineheight;
								contentRect.extent.height -= implData.textManager.lineheight;
								if (contentRect.PointIsInside(cursorPos))
								{
									bool gizmoWasHit = false;
									DockArea_IterateLayoutGizmos(
										contentRect,
										dockArea.gizmoSize,
										dockArea.gizmoPadding,
										[&dockArea, &node, cursorPos, &gizmoWasHit](
											DockArea_LayoutGizmo gizmo,
											Rect rect)
										{
											if (!gizmoWasHit && rect.PointIsInside(cursorPos))
											{
												gizmoWasHit = true;
												if (gizmo == DockArea_LayoutGizmo::Left || gizmo == DockArea_LayoutGizmo::Right)
													node.type = DockArea::Node::Type::HoriSplit;
												else if (gizmo == DockArea_LayoutGizmo::Top || gizmo == DockArea_LayoutGizmo::Bottom)
													node.type = DockArea::Node::Type::VertSplit;
												if (gizmo == DockArea_LayoutGizmo::Left || gizmo == DockArea_LayoutGizmo::Right ||
														gizmo == DockArea_LayoutGizmo::Top || gizmo == DockArea_LayoutGizmo::Bottom)
												{
													DockArea::Node* newNode = new DockArea::Node;
													auto& nodeA = gizmo == DockArea_LayoutGizmo::Left || gizmo == DockArea_LayoutGizmo::Top ? node.split.a : node.split.b;
													auto& nodeB = gizmo == DockArea_LayoutGizmo::Left || gizmo == DockArea_LayoutGizmo::Top ? node.split.b : node.split.a;
													nodeB = Std::Box{ newNode };
													newNode->type = DockArea::Node::Type::Window;
													newNode->windows = Std::Move(node.windows);
													newNode->selectedWindow = node.selectedWindow;
													node.selectedWindow = 0;
													nodeA = Std::Move(dockArea.layers.front().node);
													dockArea.layers.erase(dockArea.layers.begin());
													
													dockArea.behaviorData.Set(DockArea::BehaviorData_Normal{});
												}
												else if (gizmo == DockArea_LayoutGizmo::Center && dockArea.layers.front().node->type == DockArea::Node::Type::Window)
												{
													node.selectedWindow = node.windows.size() + dockArea.layers.front().node->selectedWindow;
													for (auto& window : dockArea.layers.front().node->windows)
														node.windows.emplace_back(Std::Move(window));
													dockArea.layers.erase(dockArea.layers.begin());
													dockArea.behaviorData.Set(DockArea::BehaviorData_Normal{});
												}
											}
										});
								}
							});
						if (!continueIterating)
							return;
					});
			}

			dockArea.behaviorData.Set(DockArea::BehaviorData_Normal{});
		}
	}

	static void DockArea_TouchEvent_Behavior_Resizing(
		Context& ctx,
		ImplData& implData,
		DockArea& dockArea,
		Rect widgetRect,
		Rect visibleRect,
		Gui::TouchEvent event)
	{
		if (event.id == 0 && event.type == TouchEventType::Up)
		{
			dockArea.behaviorData.Set(DockArea::BehaviorData_Normal{});
		}
		else if (event.id == 0 && event.type == TouchEventType::Moved)
		{
			bool cursorInsideWidget = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);
			if (!cursorInsideWidget)
				return;
			DENGINE_IMPL_GUI_ASSERT(!dockArea.layers.empty());

			auto& behaviorData = dockArea.behaviorData.Get<DockArea::BehaviorData_Resizing>();

			if (behaviorData.resizingSplit)
			{
				auto& layer = behaviorData.resizingSplitIsFrontNode ? dockArea.layers.front() : dockArea.layers.back();
				Rect windowRect = layer.rect;
				if (!behaviorData.resizingSplitIsFrontNode)
				{
					windowRect.position = {};
					windowRect.extent = widgetRect.extent;
				}

				bool continueIterating = true;
				impl::DockArea_IterateNode(
					*layer.node,
					windowRect,
					continueIterating,
					impl::DockArea_IterateNode_SplitCallableOrder::First,
					[&dockArea, event, widgetRect, &behaviorData](
						DockArea::Node& node,
						Rect rect,
						DockArea::Node* parentNode,
						bool& continueIterating)
					{
						if ((node.type == DockArea::Node::Type::HoriSplit || node.type == DockArea::Node::Type::VertSplit) &&
								&node == behaviorData.resizeSplitNode)
						{
							continueIterating = false;

							if (node.type == DockArea::Node::Type::HoriSplit)
							{
								// Find out the position of the cursor in range 0-1 of the node.
								f32 test = f32(event.position.x - widgetRect.position.x - rect.position.x) / rect.extent.width;
								node.split.split = Math::Clamp(test, 0.1f, 0.9f);
							}
							else if (node.type == DockArea::Node::Type::VertSplit)
							{
								// Find out the position of the cursor in range 0-1 of the node.
								f32 test = f32(event.position.y - widgetRect.position.y - rect.position.y) / rect.extent.height;
								node.split.split = Math::Clamp(test, 0.1f, 0.9f);
							}
						}
					});
			}
			else
			{
				auto& layer = dockArea.layers.front();
				if (behaviorData.resizeSide == DockArea::ResizeSide::Top)
				{
					i32 prevY = layer.rect.position.y;
					i32 a = event.position.y - widgetRect.position.y;
					i32 b = layer.rect.position.y + (i32)layer.rect.extent.height - 100;
					layer.rect.position.y = Math::Min(a, b);
					layer.rect.extent.height += prevY - layer.rect.position.y;
				}
				else if (behaviorData.resizeSide == DockArea::ResizeSide::Bottom)
				{
					layer.rect.extent.height = Math::Max(0, (i32)event.position.y - widgetRect.position.y - layer.rect.position.y);
				}
				else if (behaviorData.resizeSide == DockArea::ResizeSide::Left)
				{
					i32 prevX = layer.rect.position.x;
					i32 a = event.position.x - widgetRect.position.x;
					i32 b = layer.rect.position.x + (i32)layer.rect.extent.width - 100;
					layer.rect.position.x = Math::Min(a, b);
					layer.rect.extent.width += prevX - layer.rect.position.x;
				}
				else if (behaviorData.resizeSide == DockArea::ResizeSide::Right)
				{
					layer.rect.extent.width = Math::Max(0, (i32)event.position.x - widgetRect.position.x - layer.rect.position.x);
				}
				layer.rect.extent.height = Math::Max(layer.rect.extent.height, 100U);
				layer.rect.extent.width = Math::Max(layer.rect.extent.width, 100U);
			}
		}
	}

	static void DockArea_TouchEvent_Behavior_HoldingTab(
		Context& ctx,
		ImplData& implData,
		DockArea& dockArea,
		Rect widgetRect,
		Rect visibleRect,
		Gui::TouchEvent event)
	{
		if (event.id == 0 && event.type == TouchEventType::Up)
		{
			dockArea.behaviorData.Set(DockArea::BehaviorData_Normal{});
		}
		else if (event.id == 0 && event.type == TouchEventType::Moved)
		{
			auto cursorPos = Math::Vec2Int{ (i32)event.position.x, (i32)event.position.y };
			bool cursorInsideWidget = widgetRect.PointIsInside(cursorPos);

			// First we check if we need to disconnect any tabs
			if (cursorInsideWidget)
			{
				auto& behaviorData = dockArea.behaviorData.Get<DockArea::BehaviorData_HoldingTab>();
				auto& layer = behaviorData.holdingFrontWindow ? dockArea.layers.front() : dockArea.layers.back();
				Rect layerRect = layer.rect;
				layerRect.position += widgetRect.position;
				if (!behaviorData.holdingFrontWindow)
				{
					layerRect = widgetRect;
				}
				bool continueIterating = true;
				impl::DockArea_IterateNode(
					*layer.node,
					layerRect,
					continueIterating,
					impl::DockArea_IterateNode_SplitCallableOrder::First,
					[&dockArea, &implData, widgetRect, cursorPos, &behaviorData](
						DockArea::Node &node,
						Rect rect,
						DockArea::Node *parentNode,
						bool &continueIterating)
					{
						if (node.type != DockArea::Node::Type::Window)
							return;
						if (&node != behaviorData.holdingTab)
							return;

						// First find out the rect of the tab we are holding
						Rect tabRect{};
						tabRect.position = rect.position;
						tabRect.extent.height = implData.textManager.lineheight;
						for (uSize nodeWindowIndex = 0;
								 nodeWindowIndex < node.windows.size(); nodeWindowIndex += 1)
						{
							auto &window = node.windows[nodeWindowIndex];
							SizeHint titleBarSizeHint = impl::TextManager::GetSizeHint(
								implData.textManager,
								window.title);
							tabRect.position.x += titleBarSizeHint.preferred.width;
							if (nodeWindowIndex == node.selectedWindow)
							{
								tabRect.extent.width = titleBarSizeHint.preferred.width;
								break;
							}
						}

						if ((cursorPos.y <= tabRect.position.y - dockArea.tabDisconnectRadius) ||
								(cursorPos.y >=
								 tabRect.position.y + tabRect.extent.height + dockArea.tabDisconnectRadius))
						{
							// We want to disconnect this tab into a new top-level node.
							DockArea::Layer newLayer{};
							newLayer.rect = rect;
							newLayer.rect.position = cursorPos - widgetRect.position - behaviorData.cursorPosRelative;
							DockArea::Node *newNode = new DockArea::Node;
							newLayer.node = Std::Box{ newNode };
							newNode->type = DockArea::Node::Type::Window;
							newNode->windows.push_back(Std::Move(node.windows[node.selectedWindow]));
							dockArea.layers.emplace(dockArea.layers.begin(), Std::Move(newLayer));

							// Remove the window from the node
							node.windows.erase(node.windows.begin() + node.selectedWindow);
							if (node.selectedWindow > 0)
								node.selectedWindow -= 1;

							// We are now moving this tab.
							Math::Vec2Int cursorRelativePos = behaviorData.cursorPosRelative;
							dockArea.behaviorData.Set(DockArea::BehaviorData_Moving{});
							auto& behaviorDataMoving = dockArea.behaviorData.Get<DockArea::BehaviorData_Moving>();
							behaviorDataMoving.windowMovedRelativePos = cursorRelativePos;

							if (node.windows.empty())
							{
								// We want to turn the parent-node from a split into a window-node.
								DENGINE_IMPL_GUI_ASSERT(parentNode);
								DENGINE_IMPL_GUI_ASSERT(parentNode->split.a == &node || parentNode->split.b == &node);
								if (parentNode->split.a == &node)
								{
									DockArea::Node temp = Std::Move(*parentNode);
									*parentNode = Std::Move(*temp.split.b);
								}
								else
								{
									DockArea::Node temp = Std::Move(*parentNode);
									*parentNode = Std::Move(*temp.split.a);
								}
							}
						}
					});
			}
		}
	}
}

void DockArea::TouchEvent(
	Context& ctx, 
	WindowID windowId,
	Rect widgetRect, 
	Rect visibleRect,
	Gui::TouchEvent event)
{
	auto& dockArea = *this;
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	if (this->behaviorData.ToPtr<DockArea::BehaviorData_Normal>())
	{
		impl::DockArea_TouchEvent_Behavior_Normal(
			ctx,
			implData,
			dockArea,
			windowId,
			widgetRect,
			visibleRect,
			event);
	}
	else if (this->behaviorData.ToPtr<DockArea::BehaviorData_Moving>())
	{
		impl::DockArea_TouchEvent_Behavior_Moving(
			ctx,
			implData,
			dockArea,
			widgetRect,
			visibleRect,
			event);
	}
	else if (this->behaviorData.ToPtr<DockArea::BehaviorData_Resizing>())
	{
		impl::DockArea_TouchEvent_Behavior_Resizing(
			ctx,
			implData,
			dockArea,
			widgetRect,
			visibleRect,
			event);
	}
	else if (this->behaviorData.ToPtr<DockArea::BehaviorData_HoldingTab>())
	{
		impl::DockArea_TouchEvent_Behavior_HoldingTab(
			ctx,
			implData,
			dockArea,
			widgetRect,
			visibleRect,
			event);
	}
	else
		DENGINE_DETAIL_UNREACHABLE();
}

void DEngine::Gui::DockArea::InputConnectionLost()
{
	bool continueIterating = true;
	impl::DockArea_IterateLayers(
		impl::DockArea_IterateLayerMode::FrontToBack,
		layers,
		Rect{},
		continueIterating,
		[](
			DockArea::Layer& layer,
			Rect layerRect,
			uSize layerIndex,
			bool& continueIterating)
		{
			impl::DockArea_IterateNode(
				*layer.node,
				layerRect,
				continueIterating,
				impl::DockArea_IterateNode_SplitCallableOrder::First,
				[](
					DockArea::Node& node,
					Rect rect,
					DockArea::Node* parentNode,
					bool& continueIterating)
				{
					if (node.type == Node::Type::Window)
					{
						auto& window = node.windows[node.selectedWindow];
						if (window.widget)
							window.widget->InputConnectionLost();
					}
				});
		});
}

void DEngine::Gui::DockArea::CharEnterEvent(Context& ctx)
{
	// Only send events to the window in focus?

	bool continueIterating = true;
	impl::DockArea_IterateLayers(
		impl::DockArea_IterateLayerMode::FrontToBack,
		layers,
		Rect{},
		continueIterating,
		[&ctx](
			DockArea::Layer& layer,
			Rect layerRect,
			uSize layerIndex,
			bool& continueIterating)
		{
			impl::DockArea_IterateNode(
				*layer.node,
				layerRect,
				continueIterating,
				impl::DockArea_IterateNode_SplitCallableOrder::First,
				[&ctx](
					DockArea::Node& node,
					Rect rect,
					DockArea::Node* parentNode,
					bool& continueIterating)
				{
					if (node.type == Node::Type::Window)
					{
						auto& window = node.windows[node.selectedWindow];
						if (window.widget)
							window.widget->CharEnterEvent(ctx);
					}
				});
		});
}

void DockArea::CharEvent(
	Context& ctx,
	u32 utfValue)
{
	// Only send events to the window in focus?

	bool continueIterating = true;
	impl::DockArea_IterateLayers(
		impl::DockArea_IterateLayerMode::FrontToBack,
		layers,
		Rect{},
		continueIterating,
		[&ctx, utfValue](
			DockArea::Layer& layer,
			Rect layerRect,
			uSize layerIndex,
			bool& continueIterating)
		{
			impl::DockArea_IterateNode(
				*layer.node,
				layerRect,
				continueIterating,
				impl::DockArea_IterateNode_SplitCallableOrder::First,
				[&ctx, utfValue](
					DockArea::Node& node,
					Rect rect,
					DockArea::Node* parentNode,
					bool& continueIterating)
				{
					if (node.type == Node::Type::Window)
					{
						auto& window = node.windows[node.selectedWindow];
						if (window.widget)
							window.widget->CharEvent(
								ctx,
								utfValue);
					}
				});
		});
}

void DockArea::CharRemoveEvent(
	Context& ctx)
{
	bool continueIterating = true;
	impl::DockArea_IterateLayers(
		impl::DockArea_IterateLayerMode::FrontToBack,
		layers,
		Rect{},
		continueIterating,
		[&ctx](
			DockArea::Layer& layer,
			Rect layerRect,
			uSize layerIndex,
			bool& continueIterating)
		{
			impl::DockArea_IterateNode(
				*layer.node,
				{},
				continueIterating,
				impl::DockArea_IterateNode_SplitCallableOrder::First,
				[&ctx](
					DockArea::Node& node,
					Rect rect,
					DockArea::Node* parentNode,
					bool& continueIterating)
				{
					if (node.type == Node::Type::Window)
					{
						auto& window = node.windows[node.selectedWindow];
						if (window.widget)
							window.widget->CharRemoveEvent(
								ctx);
					}
				});
		});
}
