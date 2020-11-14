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
		static_assert(Std::Trait::IsSame<
			Std::Trait::RemoveConst<T>, 
			DockArea::Node>);
		
		DENGINE_IMPL_GUI_ASSERT(continueIterating);
		DockArea_IterateNode_Internal(
			node,
			rect,
			(T*)nullptr,
			continueIterating,
			splitCallableOrder,
			callable);
	}

	enum class DockArea_IterateTopLevelNodeMode
	{
		FrontToBack,
		BackToFront,
	};

	using DockArea_IterateTopLevelNodeCallbackPFN = void(*)(
		DockArea::TopLevelNode& topLevelNode,
		Rect topLevelRect,
		uSize topLevelIndex,
		bool& continueIterating);

	template<typename TopLevelNodeContainerType, typename Callable>
	void DockArea_IterateTopLevelNodes(
		DockArea_IterateTopLevelNodeMode iterateMode,
		TopLevelNodeContainerType& topLevelNodes,
		Rect widgetRect,
		bool& continueIterating,
		Callable callable)
	{
		static_assert(Std::Trait::IsSame<
			Std::Trait::RemoveConst<TopLevelNodeContainerType>,
			decltype(DockArea::topLevelNodes)>);

		uSize topLevelNodesLength = topLevelNodes.size();
		
		uSize n = 0;
		if (iterateMode == DockArea_IterateTopLevelNodeMode::BackToFront)
			n = topLevelNodesLength;
		while (true)
		{
			uSize i = 0;
			bool shouldContinue = false;
			if (iterateMode == DockArea_IterateTopLevelNodeMode::FrontToBack)
			{
				if (n < topLevelNodesLength)
					shouldContinue = true;
				i = n;
			}
			else if (iterateMode == DockArea_IterateTopLevelNodeMode::BackToFront)
			{
				if (n > 0)
					shouldContinue = true;
				i = n - 1;
			}
			if (!shouldContinue)
				break;

			auto& topLevelNode = topLevelNodes[i];
			Rect topLevelRect = topLevelNode.rect;
			topLevelRect.position += widgetRect.position;
			if (i == topLevelNodesLength - 1)
				topLevelRect = widgetRect;
			callable(
				topLevelNode, 
				topLevelRect, 
				i, 
				continueIterating);
			if (!continueIterating)
				return;

			if (iterateMode == DockArea_IterateTopLevelNodeMode::FrontToBack)
			{
				n += 1;
			}
			else if (iterateMode == DockArea_IterateTopLevelNodeMode::BackToFront)
			{
				n -= 1;
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

	static void DockArea_PushTopLevelToFront(
		std::vector<DockArea::TopLevelNode>& topLevelNodes,
		uSize index)
	{
		DENGINE_IMPL_GUI_ASSERT(index < topLevelNodes.size());
		if (index > 0)
		{
			DockArea::TopLevelNode tempTopLevelNode = Std::Move(topLevelNodes[index]);
			topLevelNodes.erase(topLevelNodes.begin() + index);
			topLevelNodes.insert(topLevelNodes.begin(), Std::Move(tempTopLevelNode));
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

DockArea::DockArea() : behaviorData{BehaviorData()}
{
	
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
		std::vector<DockArea::TopLevelNode> const& topLevelNodes,
		Rect widgetRect,
		Extent framebufferExtent,
		DrawInfo& drawInfo)
	{
		bool continueIterating = true;
		impl::DockArea_IterateTopLevelNodes(
			DockArea_IterateTopLevelNodeMode::BackToFront,
			topLevelNodes,
			widgetRect,
			continueIterating,
			[&ctx, &dockArea, &implData, &drawInfo](
				DockArea::TopLevelNode const& topLevelNode,
				Rect topLevelRect,
				uSize topLevelIndex,
				bool& continueIterating)
			{
				impl::DockArea_IterateNode(
					*topLevelNode.node,
					topLevelRect,
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
							if (window.widget || window.layout)
							{
								drawInfo.PushScissor(contentRect);
								if (window.widget)
								{
									window.widget->Render(
										ctx,
										drawInfo.GetFramebufferExtent(),
										contentRect,
										contentRect,
										drawInfo);
								}
								else
								{
									window.layout->Render(
										ctx,
										drawInfo.GetFramebufferExtent(),
										contentRect,
										contentRect,
										drawInfo);
								}
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
							drawInfo.PushFilledQuad(resizeHandleRect, { 0.75f, 0.75f, 0.75f, 1.f });
						}
					});

				impl::DockArea_IterateResizeRects(
					topLevelRect,
					dockArea.resizeAreaThickness,
					dockArea.resizeHandleLength,
					[&dockArea, &drawInfo, topLevelIndex](
						DockArea_ResizeRect side,
						Rect rect)
					{
						if (topLevelIndex != dockArea.topLevelNodes.size() - 1)
							drawInfo.PushFilledQuad(rect, { 1.f, 1.f, 1.f, 0.75f });
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
		DENGINE_IMPL_GUI_ASSERT(dockArea.behaviorData.moving.showLayoutNodePtr);
		DENGINE_IMPL_GUI_ASSERT(dockArea.behavior == DockArea::Behavior::Moving);

		Rect windowRect{};
		// First find the node
		bool nodeFound = false;
		bool continueIterating = true;
		impl::DockArea_IterateTopLevelNodes(
			DockArea_IterateTopLevelNodeMode::FrontToBack,
			dockArea.topLevelNodes,
			widgetRect,
			continueIterating,
			[&dockArea, &windowRect, &nodeFound](
				DockArea::TopLevelNode const& topLevelNode,
				Rect topLevelRect,
				uSize topLevelIndex,
				bool& continueIterating)
			{
				impl::DockArea_IterateNode(
					*topLevelNode.node,
					topLevelRect,
					continueIterating,
					impl::DockArea_IterateNode_SplitCallableOrder::First,
					[&dockArea, &windowRect, &nodeFound](
						DockArea::Node const& node,
						Rect rect,
						DockArea::Node const* parentNode,
						bool& continueIterating)
					{
						if (&node != dockArea.behaviorData.moving.showLayoutNodePtr)
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
		if (dockArea.behaviorData.moving.useHighlightGizmo)
		{
			Rect highlightRect = windowRect;
			switch (dockArea.behaviorData.moving.highlightGizmo)
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
	if (topLevelNodes.empty())
		return;

	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	drawInfo.PushScissor(visibleRect);

	impl::DockArea_RenderWindows(
		ctx,
		*this,
		implData,
		topLevelNodes,
		widgetRect,
		framebufferExtent,
		drawInfo);

	// We need to draw the layout gizmo stuff
	if (this->behavior == DockArea::Behavior::Moving)
	{
		if (this->behaviorData.moving.showLayoutNodePtr)
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
		impl::DockArea_IterateTopLevelNodes(
			impl::DockArea_IterateTopLevelNodeMode::FrontToBack,
			dockArea.topLevelNodes,
			widgetRect,
			continueIterating,
			[&ctx, windowId, &dockArea, &implData, event, cursorPos](
				DockArea::TopLevelNode& topLevelNode,
				Rect topLevelRect,
				uSize topLevelIndex,
				bool& continueIterating)
		{
			Std::Opt<impl::DockArea_ResizeRect> resizeRectHit;
			if (event.clicked && continueIterating && topLevelIndex != dockArea.topLevelNodes.size() - 1)
			{
				impl::DockArea_IterateResizeRects(
					topLevelRect,
					dockArea.resizeAreaThickness,
					dockArea.resizeHandleLength,
					[topLevelRect, &resizeRectHit, cursorPos](impl::DockArea_ResizeRect side, Rect rect)
					{
						if (!resizeRectHit.HasValue() && rect.PointIsInside(cursorPos))
							resizeRectHit = Std::Opt{ side };
					});
				if (resizeRectHit.HasValue())
				{
					continueIterating = false;
					impl::DockArea_PushTopLevelToFront(dockArea.topLevelNodes, topLevelIndex);
					dockArea.behavior = DockArea::Behavior::Resizing;
					dockArea.behaviorData.resizing = {};
					switch (resizeRectHit.Value())
					{
					case DockArea_ResizeRect::Top: dockArea.behaviorData.resizing.resizeSide = DockArea::ResizeSide::Top; break;
					case DockArea_ResizeRect::Bottom: dockArea.behaviorData.resizing.resizeSide = DockArea::ResizeSide::Bottom; break;
					case DockArea_ResizeRect::Left: dockArea.behaviorData.resizing.resizeSide = DockArea::ResizeSide::Left; break;
					case DockArea_ResizeRect::Right: dockArea.behaviorData.resizing.resizeSide = DockArea::ResizeSide::Right; break;
					}
				}
			}

			if (!resizeRectHit.HasValue() && topLevelRect.PointIsInside(cursorPos))
			{
				// Iterate through nodes, check if we hit a tab or the titlebar.
				impl::DockArea_IterateNode(
					*topLevelNode.node,
					topLevelRect,
					continueIterating,
					impl::DockArea_IterateNode_SplitCallableOrder::First,
					[&ctx, windowId, &dockArea, &implData, cursorPos, event, topLevelRect, topLevelIndex](
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
							if (topLevelIndex != dockArea.topLevelNodes.size() - 1)
								impl::DockArea_PushTopLevelToFront(dockArea.topLevelNodes, topLevelIndex);
							dockArea.behavior = DockArea::Behavior::Resizing;
							dockArea.behaviorData.resizing = {};
							dockArea.behaviorData.resizing.resizingSplit = true;
							dockArea.behaviorData.resizing.resizeSplitNode = &node;
							dockArea.behaviorData.resizing.resizingSplitIsFrontNode = topLevelIndex != dockArea.topLevelNodes.size() - 1;
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
											if (topLevelIndex != dockArea.topLevelNodes.size() - 1)
												impl::DockArea_PushTopLevelToFront(dockArea.topLevelNodes, topLevelIndex);
											dockArea.behavior = DockArea::Behavior::HoldingTab;
											dockArea.behaviorData.holdingTab = {};
											dockArea.behaviorData.holdingTab.holdingTab = &node;
											dockArea.behaviorData.holdingTab.holdingFrontWindow = topLevelIndex != dockArea.topLevelNodes.size() - 1;
											dockArea.behaviorData.holdingTab.cursorPosRelative = cursorPos - tabRect.position;
											break;
										}
									}
								}
								if (!tabWasHit && topLevelIndex != dockArea.topLevelNodes.size() - 1)
								{
									// We hit the title bar but not any tabs, start moving it and bring it to front
									dockArea.behavior = DockArea::Behavior::Moving;
									dockArea.behaviorData.moving = {};
									dockArea.behaviorData.moving.windowMovedRelativePos = cursorPos - topLevelRect.position;
									impl::DockArea_PushTopLevelToFront(dockArea.topLevelNodes, topLevelIndex);
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
								else if (window.layout)
								{
									window.layout->CursorClick(
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
				dockArea.topLevelNodes.erase(dockArea.topLevelNodes.begin());
			}

			// Find out if we need to dock a window into another.
			if (continueIterating)
			{
				impl::DockArea_IterateTopLevelNodes(
					impl::DockArea_IterateTopLevelNodeMode::FrontToBack,
					dockArea.topLevelNodes,
					widgetRect,
					continueIterating,
					[&dockArea, &implData, cursorPos](
						DockArea::TopLevelNode& topLevelNode,
						Rect topLevelRect,
						uSize topLevelIndex,
						bool& continueIterating)
					{
						// We don't need to test the front-most node, since that's the one being moved.
						if (topLevelIndex == 0)
							return;

						// We search for the node in which we are showing layout gizmos
						impl::DockArea_IterateNode(
							*topLevelNode.node,
							topLevelRect,
							continueIterating,
							impl::DockArea_IterateNode_SplitCallableOrder::First,
							[&dockArea, &implData, cursorPos](
								DockArea::Node& node,
								Rect rect,
								DockArea::Node* parentNode,
								bool& continueIterating)
							{
								if (&node != dockArea.behaviorData.moving.showLayoutNodePtr)
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
													nodeA = Std::Move(dockArea.topLevelNodes.front().node);
													dockArea.topLevelNodes.erase(dockArea.topLevelNodes.begin());
												}

												if (gizmo == DockArea_LayoutGizmo::Center && dockArea.topLevelNodes.front().node->type == DockArea::Node::Type::Window)
												{
													node.selectedWindow = node.windows.size() + dockArea.topLevelNodes.front().node->selectedWindow;
													for (auto& window : dockArea.topLevelNodes.front().node->windows)
														node.windows.emplace_back(Std::Move(window));
													dockArea.topLevelNodes.erase(dockArea.topLevelNodes.begin());
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
			dockArea.behavior = DockArea::Behavior::Normal;
			dockArea.behaviorData.normal = {};
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
			dockArea.behavior = DockArea::Behavior::Normal;
			dockArea.behaviorData.normal = {};
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
			dockArea.behavior = DockArea::Behavior::Normal;
			dockArea.behaviorData.normal = {};
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

	if (this->behavior == Behavior::Normal)
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
	else if (this->behavior == Behavior::Moving)
	{
		impl::DockArea_CursorClick_Behavior_Moving(
			*this,
			widgetRect,
			visibleRect,
			cursorPos,
			event,
			implData);
	}
	else if (this->behavior == Behavior::Resizing)
	{
		impl::DockArea_CursorClick_Behavior_Resizing(
			*this,
			widgetRect,
			visibleRect,
			event);
	}
	else if (this->behavior == Behavior::HoldingTab)
	{
		impl::DockArea_CursorClick_Behavior_HoldingTab(
			*this,
			widgetRect,
			visibleRect,
			cursorPos,
			event);
	}
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
		impl::DockArea_IterateTopLevelNodes(
			impl::DockArea_IterateTopLevelNodeMode::FrontToBack,
			dockArea.topLevelNodes,
			widgetRect,
			continueIterating,
			[&ctx, windowId, &dockArea, &implData, event, &rectWasHit](
				DockArea::TopLevelNode& topLevelNode,
				Rect topLevelRect,
				uSize topLevelIndex,
				bool& continueIterating)
			{
				// First we check if we hit a resize rect. We do not check this for the rear-most one
				// Because it is always full-size.
				if (rectWasHit)
					return;
				Std::Opt<DockArea_ResizeRect> resizeRectHit;
				if (topLevelIndex != dockArea.topLevelNodes.size() - 1)
				{
					DockArea_IterateResizeRects(
						topLevelRect,
						dockArea.resizeAreaThickness,
						dockArea.resizeHandleLength,
						[topLevelRect, event, &resizeRectHit](DockArea_ResizeRect side, Rect rect)
					{
						if (!resizeRectHit.HasValue() && rect.PointIsInside(event.position))
							resizeRectHit = Std::Opt{ side };
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
				if (continueIterating && topLevelRect.PointIsInside(event.position))
				{
					continueIterating = false;
					bool continueIterating2 = true;
					impl::DockArea_IterateNode(
						*topLevelNode.node,
						topLevelRect,
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
									else if (window.layout)
									{
										window.layout->CursorMove(
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
			// First move the window.
			auto& topLevelNode = dockArea.topLevelNodes.front();
			topLevelNode.rect.position = cursorPos - widgetRect.position - dockArea.behaviorData.moving.windowMovedRelativePos;

			// Check if mouse is covering the delete gizmo

			bool continueIterating = true;
			bool windowContentWasHit = false;
			impl::DockArea_IterateTopLevelNodes(
				impl::DockArea_IterateTopLevelNodeMode::FrontToBack,
				dockArea.topLevelNodes,
				widgetRect,
				continueIterating,
				[&dockArea, &implData, cursorPos, &windowContentWasHit](
					DockArea::TopLevelNode& topLevelNode,
					Rect topLevelRect,
					uSize topLevelIndex,
					bool& continueIterating)
				{
					// Jump over the first top-level node. It's the one we're moving.
					if (topLevelIndex == 0)
						return;
					impl::DockArea_IterateNode(
						*topLevelNode.node,
						topLevelRect,
						continueIterating,
						impl::DockArea_IterateNode_SplitCallableOrder::First,
						[&dockArea, &implData, cursorPos, &windowContentWasHit](
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
								dockArea.behaviorData.moving.showLayoutNodePtr = &node;
								continueIterating = false;
								windowContentWasHit = true;

								// Check if we are hovering any of the layout gizmos, and apply the highlight rect if we are.
								dockArea.behaviorData.moving.useHighlightGizmo = false;
								impl::DockArea_IterateLayoutGizmos(
									contentRect,
									dockArea.gizmoSize,
									dockArea.gizmoPadding,
									[&dockArea, cursorPos](
										impl::DockArea_LayoutGizmo gizmo,
										Rect rect)
									{
										if (!dockArea.behaviorData.moving.useHighlightGizmo && rect.PointIsInside(cursorPos))
										{
											dockArea.behaviorData.moving.useHighlightGizmo = true;
											dockArea.behaviorData.moving.highlightGizmo = gizmo;
										}
									});
							}
						});
				});
			if (!windowContentWasHit)
				dockArea.behaviorData.moving.showLayoutNodePtr = nullptr;
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
		DENGINE_IMPL_GUI_ASSERT(!dockArea.topLevelNodes.empty());
		
		if (dockArea.behaviorData.resizing.resizingSplit)
		{
			auto& topLevelNode = dockArea.behaviorData.resizing.resizingSplitIsFrontNode ? dockArea.topLevelNodes.front() : dockArea.topLevelNodes.back();
			Rect windowRect = topLevelNode.rect;
			if (!dockArea.behaviorData.resizing.resizingSplitIsFrontNode)
			{
				windowRect.position = {};
				windowRect.extent = widgetRect.extent;
			}
				
			bool continueIterating = true;
			impl::DockArea_IterateNode(
				*topLevelNode.node,
				windowRect,
				continueIterating,
				impl::DockArea_IterateNode_SplitCallableOrder::First,
				[&dockArea, cursorPos, widgetRect](
					DockArea::Node& node,
					Rect rect,
					DockArea::Node* parentNode,
					bool& continueIterating)
				{
					if ((node.type == DockArea::Node::Type::HoriSplit || node.type == DockArea::Node::Type::VertSplit) &&
							 &node == dockArea.behaviorData.resizing.resizeSplitNode)
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
			auto& topLevelNode = dockArea.topLevelNodes.front();
			if (dockArea.behaviorData.resizing.resizeSide == DockArea::ResizeSide::Top)
			{
				i32 prevY = topLevelNode.rect.position.y;
				i32 a = cursorPos.y - widgetRect.position.y;
				i32 b = topLevelNode.rect.position.y + (i32)topLevelNode.rect.extent.height - 100;
				topLevelNode.rect.position.y = Math::Min(a, b);
				topLevelNode.rect.extent.height += prevY - topLevelNode.rect.position.y;
			}
			else if (dockArea.behaviorData.resizing.resizeSide == DockArea::ResizeSide::Bottom)
			{
				topLevelNode.rect.extent.height = Math::Max(0, cursorPos.y - widgetRect.position.y - topLevelNode.rect.position.y);
			}
			else if (dockArea.behaviorData.resizing.resizeSide == DockArea::ResizeSide::Left)
			{
				i32 prevX = topLevelNode.rect.position.x;
				i32 a = cursorPos.x - widgetRect.position.x;
				i32 b = topLevelNode.rect.position.x + (i32)topLevelNode.rect.extent.width - 100;
				topLevelNode.rect.position.x = Math::Min(a, b);
				topLevelNode.rect.extent.width += prevX - topLevelNode.rect.position.x;
			}
			else if (dockArea.behaviorData.resizing.resizeSide == DockArea::ResizeSide::Right)
			{
				topLevelNode.rect.extent.width = Math::Max(0, cursorPos.x - widgetRect.position.x - topLevelNode.rect.position.x);
			}
			topLevelNode.rect.extent.height = Math::Max(topLevelNode.rect.extent.height, 100U);
			topLevelNode.rect.extent.width = Math::Max(topLevelNode.rect.extent.width, 100U);
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
			auto& topLevelNode = dockArea.behaviorData.holdingTab.holdingFrontWindow ? dockArea.topLevelNodes.front() : dockArea.topLevelNodes.back();
			Rect topLevelRect = topLevelNode.rect;
			topLevelRect.position += widgetRect.position;
			if (!dockArea.behaviorData.holdingTab.holdingFrontWindow)
			{
				topLevelRect = widgetRect;
			}
			bool continueIterating = true;
			impl::DockArea_IterateNode(
				*topLevelNode.node,
				topLevelRect,
				continueIterating,
				impl::DockArea_IterateNode_SplitCallableOrder::First,
				[&dockArea, &implData, widgetRect, cursorPos](
					DockArea::Node& node,
					Rect rect,
					DockArea::Node* parentNode,
					bool& continueIterating)
				{
					if (node.type != DockArea::Node::Type::Window)
						return;
					if (&node != dockArea.behaviorData.holdingTab.holdingTab)
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
						DockArea::TopLevelNode newTopLevelNode{};
						newTopLevelNode.rect = rect;
						newTopLevelNode.rect.position = cursorPos - widgetRect.position - dockArea.behaviorData.holdingTab.cursorPosRelative;
						DockArea::Node* newNode = new DockArea::Node;
						newTopLevelNode.node = Std::Box{ newNode };
						newNode->type = DockArea::Node::Type::Window;
						newNode->windows.push_back(Std::Move(node.windows[node.selectedWindow]));
						dockArea.topLevelNodes.emplace(dockArea.topLevelNodes.begin(), Std::Move(newTopLevelNode));

						// Remove the window from the node
						node.windows.erase(node.windows.begin() + node.selectedWindow);
						if (node.selectedWindow > 0)
							node.selectedWindow -= 1;

						// We are now moving this tab.
						Math::Vec2Int cursorRelativePos = dockArea.behaviorData.holdingTab.cursorPosRelative;
						dockArea.behavior = DockArea::Behavior::Moving;
						dockArea.behaviorData.moving = {};
						dockArea.behaviorData.moving.windowMovedRelativePos = cursorRelativePos;

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

	if (this->behavior == Behavior::Normal)
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
	else if (this->behavior == Behavior::Moving)
	{
		impl::DockArea_CursorMove_Behavior_Moving(
			*this,
			implData,
			widgetRect,
			visibleRect,
			event.position);
	}
	else if (this->behavior == Behavior::Resizing)
	{
		impl::DockArea_CursorMove_Behavior_Resizing(
			*this,
			implData,
			widgetRect,
			visibleRect,
			event.position);
	}
	else if (this->behavior == Behavior::HoldingTab)
	{
		impl::DockArea_CursorMove_Behavior_HoldingTab(
			*this,
			implData,
			widgetRect,
			visibleRect,
			event.position);
	}
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
			impl::DockArea_IterateTopLevelNodes(
				impl::DockArea_IterateTopLevelNodeMode::FrontToBack,
				dockArea.topLevelNodes,
				widgetRect,
				continueIterating,
				[&ctx, &dockArea, &implData, windowId, event](
					DockArea::TopLevelNode &topLevelNode,
					Rect topLevelRect,
					uSize topLevelIndex,
					bool &continueIterating)
				{
					Std::Opt<impl::DockArea_ResizeRect> resizeRectHit;
					if (event.type == TouchEventType::Down && continueIterating &&
							topLevelIndex != dockArea.topLevelNodes.size() - 1)
					{
						impl::DockArea_IterateResizeRects(
							topLevelRect,
							dockArea.resizeAreaThickness,
							dockArea.resizeHandleLength,
							[topLevelRect, &resizeRectHit, event](impl::DockArea_ResizeRect side, Rect rect)
							{
								if (!resizeRectHit.HasValue() && rect.PointIsInside(event.position))
									resizeRectHit = Std::Opt{side};
							});
						if (resizeRectHit.HasValue())
						{
							continueIterating = false;
							impl::DockArea_PushTopLevelToFront(dockArea.topLevelNodes, topLevelIndex);
							dockArea.behavior = DockArea::Behavior::Resizing;
							dockArea.behaviorData.resizing = {};
							switch (resizeRectHit.Value())
							{
								case impl::DockArea_ResizeRect::Top:
									dockArea.behaviorData.resizing.resizeSide = DockArea::ResizeSide::Top;
									break;
								case impl::DockArea_ResizeRect::Bottom:
									dockArea.behaviorData.resizing.resizeSide = DockArea::ResizeSide::Bottom;
									break;
								case impl::DockArea_ResizeRect::Left:
									dockArea.behaviorData.resizing.resizeSide = DockArea::ResizeSide::Left;
									break;
								case impl::DockArea_ResizeRect::Right:
									dockArea.behaviorData.resizing.resizeSide = DockArea::ResizeSide::Right;
									break;
							}
						}
					}
					if (!resizeRectHit.HasValue() && topLevelRect.PointIsInside(event.position))
					{
						// Iterate through nodes, check if we hit a tab or the titlebar.
						impl::DockArea_IterateNode(
							*topLevelNode.node,
							topLevelRect,
							continueIterating,
							impl::DockArea_IterateNode_SplitCallableOrder::First,
							[&ctx, &dockArea, &implData, windowId, event, topLevelRect, topLevelIndex](
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
											if (topLevelIndex != dockArea.topLevelNodes.size() - 1)
												impl::DockArea_PushTopLevelToFront(dockArea.topLevelNodes, topLevelIndex);
											dockArea.behavior = DockArea::Behavior::Resizing;
											dockArea.behaviorData.resizing = {};
											dockArea.behaviorData.resizing.resizingSplit = true;
											dockArea.behaviorData.resizing.resizeSplitNode = &node;
											dockArea.behaviorData.resizing.resizingSplitIsFrontNode =
												topLevelIndex != dockArea.topLevelNodes.size() - 1;
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
												for (uSize windowIndex = 0;
														 !tabWasHit && windowIndex < node.windows.size(); windowIndex += 1)
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
														if (topLevelIndex != dockArea.topLevelNodes.size() - 1)
															impl::DockArea_PushTopLevelToFront(dockArea.topLevelNodes,
																																 topLevelIndex);
														dockArea.behavior = DockArea::Behavior::HoldingTab;
														dockArea.behaviorData.holdingTab = {};
														dockArea.behaviorData.holdingTab.holdingTab = &node;
														dockArea.behaviorData.holdingTab.holdingFrontWindow =
															topLevelIndex != dockArea.topLevelNodes.size() - 1;
														dockArea.behaviorData.holdingTab.cursorPosRelative =
															Math::Vec2Int{(i32) event.position.x, (i32) event.position.y} -
															tabRect.position;
														break;
													}
												}
											}
											if (!tabWasHit && topLevelIndex != dockArea.topLevelNodes.size() - 1)
											{
												// We hit the title bar but not any tabs, start moving it and bring it to front
												dockArea.behavior = DockArea::Behavior::Moving;
												dockArea.behaviorData.moving = {};
												dockArea.behaviorData.moving.windowMovedRelativePos =
													Math::Vec2Int{(i32) event.position.x, (i32) event.position.y} -
													topLevelRect.position;
												impl::DockArea_PushTopLevelToFront(dockArea.topLevelNodes, topLevelIndex);
											}
										}
										if (!titleBarWasHit)
										{
											Rect contentRect = rect;
											contentRect.position.y += titlebarRect.extent.height;
											contentRect.extent.height -= titlebarRect.extent.height;
											if (contentRect.PointIsInside(
												Math::Vec2Int{(i32) event.position.x, (i32) event.position.y}))
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
												else if (window.layout)
												{
													window.layout->TouchEvent(
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
				auto& topLevelNode = dockArea.topLevelNodes.front();
				topLevelNode.rect.position = cursorPos - widgetRect.position - dockArea.behaviorData.moving.windowMovedRelativePos;

				bool continueIterating = true;

				bool windowContentWasHit = false;
				impl::DockArea_IterateTopLevelNodes(
					impl::DockArea_IterateTopLevelNodeMode::FrontToBack,
					dockArea.topLevelNodes,
					widgetRect,
					continueIterating,
					[&dockArea, &implData, cursorPos, &windowContentWasHit](
						DockArea::TopLevelNode& topLevelNode,
						Rect topLevelRect,
						uSize topLevelIndex,
						bool& continueIterating)
					{
						// Jump over the first top-level node. It's the one we're moving.
						if (topLevelIndex == 0)
							return;
						impl::DockArea_IterateNode(
							*topLevelNode.node,
							topLevelRect,
							continueIterating,
							impl::DockArea_IterateNode_SplitCallableOrder::First,
							[&dockArea, &implData, cursorPos, &windowContentWasHit](
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
									dockArea.behaviorData.moving.showLayoutNodePtr = &node;
									continueIterating = false;
									windowContentWasHit = true;

									// Check if we are hovering any of the layout gizmos, and apply the highlight rect if we are.
									dockArea.behaviorData.moving.useHighlightGizmo = false;
									impl::DockArea_IterateLayoutGizmos(
										contentRect,
										dockArea.gizmoSize,
										dockArea.gizmoPadding,
										[&dockArea, cursorPos](
											impl::DockArea_LayoutGizmo gizmo,
											Rect rect)
										{
											if (!dockArea.behaviorData.moving.useHighlightGizmo && rect.PointIsInside(cursorPos))
											{
												dockArea.behaviorData.moving.useHighlightGizmo = true;
												dockArea.behaviorData.moving.highlightGizmo = gizmo;
											}
										});
								}
							});
					});
				if (!windowContentWasHit)
					dockArea.behaviorData.moving.showLayoutNodePtr = nullptr;
			}
		}
		else if (event.id == 0 && event.type == TouchEventType::Up)
		{
			// We don't need to test the front-most node, since that's the one being moved.
			bool continueIterating = true;

			// Check if we are above the delete gizmo
			Rect deleteGizmo = impl::DockArea_GetDeleteGizmoRect(widgetRect, dockArea.gizmoSize);
			if (deleteGizmo.PointIsInside(cursorPos))
			{
				// Delete the front-most top-level node.
				continueIterating = false;
				dockArea.topLevelNodes.erase(dockArea.topLevelNodes.begin());
			}

			if (continueIterating)
			{
				impl::DockArea_IterateTopLevelNodes(
					impl::DockArea_IterateTopLevelNodeMode::FrontToBack,
					dockArea.topLevelNodes,
					widgetRect,
					continueIterating,
					[&dockArea, &implData, event](
						DockArea::TopLevelNode& topLevelNode,
						Rect topLevelRect,
						uSize topLevelIndex,
						bool& continueIterating)
					{
						if (topLevelIndex == 0)
							return;
						DockArea_IterateNode(
							*topLevelNode.node,
							topLevelRect,
							continueIterating,
							impl::DockArea_IterateNode_SplitCallableOrder::First,
							[&dockArea, &implData, event](
								DockArea::Node& node,
								Rect rect,
								DockArea::Node* parentNode,
								bool& continueIterating)
							{
								if (&node != dockArea.behaviorData.moving.showLayoutNodePtr)
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
													nodeA = Std::Move(dockArea.topLevelNodes.front().node);
													dockArea.topLevelNodes.erase(dockArea.topLevelNodes.begin());
													dockArea.behavior = DockArea::Behavior::Normal;
													dockArea.behaviorData.normal = {};
												}
												else if (gizmo == DockArea_LayoutGizmo::Center && dockArea.topLevelNodes.front().node->type == DockArea::Node::Type::Window)
												{
													node.selectedWindow = node.windows.size() + dockArea.topLevelNodes.front().node->selectedWindow;
													for (auto& window : dockArea.topLevelNodes.front().node->windows)
														node.windows.emplace_back(Std::Move(window));
													dockArea.topLevelNodes.erase(dockArea.topLevelNodes.begin());
													dockArea.behavior = DockArea::Behavior::Normal;
													dockArea.behaviorData.normal = {};
												}
											}
										});
								}
							});
						if (!continueIterating)
							return;
					});
			}

			dockArea.behavior = DockArea::Behavior::Normal;
			dockArea.behaviorData.normal = {};
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
			dockArea.behavior = DockArea::Behavior::Normal;
			dockArea.behaviorData.normal = {};
		}
		else if (event.id == 0 && event.type == TouchEventType::Moved)
		{
			bool cursorInsideWidget = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);
			if (!cursorInsideWidget)
				return;
			DENGINE_IMPL_GUI_ASSERT(!dockArea.topLevelNodes.empty());

			if (dockArea.behaviorData.resizing.resizingSplit)
			{
				auto& topLevelNode = dockArea.behaviorData.resizing.resizingSplitIsFrontNode ? dockArea.topLevelNodes.front() : dockArea.topLevelNodes.back();
				Rect windowRect = topLevelNode.rect;
				if (!dockArea.behaviorData.resizing.resizingSplitIsFrontNode)
				{
					windowRect.position = {};
					windowRect.extent = widgetRect.extent;
				}

				bool continueIterating = true;
				impl::DockArea_IterateNode(
					*topLevelNode.node,
					windowRect,
					continueIterating,
					impl::DockArea_IterateNode_SplitCallableOrder::First,
					[&dockArea, event, widgetRect](
						DockArea::Node& node,
						Rect rect,
						DockArea::Node* parentNode,
						bool& continueIterating)
					{
						if ((node.type == DockArea::Node::Type::HoriSplit || node.type == DockArea::Node::Type::VertSplit) &&
								&node == dockArea.behaviorData.resizing.resizeSplitNode)
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
				auto& topLevelNode = dockArea.topLevelNodes.front();
				if (dockArea.behaviorData.resizing.resizeSide == DockArea::ResizeSide::Top)
				{
					i32 prevY = topLevelNode.rect.position.y;
					i32 a = event.position.y - widgetRect.position.y;
					i32 b = topLevelNode.rect.position.y + (i32)topLevelNode.rect.extent.height - 100;
					topLevelNode.rect.position.y = Math::Min(a, b);
					topLevelNode.rect.extent.height += prevY - topLevelNode.rect.position.y;
				}
				else if (dockArea.behaviorData.resizing.resizeSide == DockArea::ResizeSide::Bottom)
				{
					topLevelNode.rect.extent.height = Math::Max(0, (i32)event.position.y - widgetRect.position.y - topLevelNode.rect.position.y);
				}
				else if (dockArea.behaviorData.resizing.resizeSide == DockArea::ResizeSide::Left)
				{
					i32 prevX = topLevelNode.rect.position.x;
					i32 a = event.position.x - widgetRect.position.x;
					i32 b = topLevelNode.rect.position.x + (i32)topLevelNode.rect.extent.width - 100;
					topLevelNode.rect.position.x = Math::Min(a, b);
					topLevelNode.rect.extent.width += prevX - topLevelNode.rect.position.x;
				}
				else if (dockArea.behaviorData.resizing.resizeSide == DockArea::ResizeSide::Right)
				{
					topLevelNode.rect.extent.width = Math::Max(0, (i32)event.position.x - widgetRect.position.x - topLevelNode.rect.position.x);
				}
				topLevelNode.rect.extent.height = Math::Max(topLevelNode.rect.extent.height, 100U);
				topLevelNode.rect.extent.width = Math::Max(topLevelNode.rect.extent.width, 100U);
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
			dockArea.behavior = DockArea::Behavior::Normal;
			dockArea.behaviorData.normal = {};
		}
		else if (event.id == 0 && event.type == TouchEventType::Moved)
		{
			auto cursorPos = Math::Vec2Int{ (i32)event.position.x, (i32)event.position.y };
			bool cursorInsideWidget = widgetRect.PointIsInside(cursorPos);

			// First we check if we need to disconnect any tabs
			if (cursorInsideWidget)
			{
				auto &topLevelNode = dockArea.behaviorData.holdingTab.holdingFrontWindow ? dockArea.topLevelNodes.front() : dockArea.topLevelNodes.back();
				Rect topLevelRect = topLevelNode.rect;
				topLevelRect.position += widgetRect.position;
				if (!dockArea.behaviorData.holdingTab.holdingFrontWindow)
				{
					topLevelRect = widgetRect;
				}
				bool continueIterating = true;
				impl::DockArea_IterateNode(
					*topLevelNode.node,
					topLevelRect,
					continueIterating,
					impl::DockArea_IterateNode_SplitCallableOrder::First,
					[&dockArea, &implData, widgetRect, cursorPos](
						DockArea::Node &node,
						Rect rect,
						DockArea::Node *parentNode,
						bool &continueIterating)
					{
						if (node.type != DockArea::Node::Type::Window)
							return;
						if (&node != dockArea.behaviorData.holdingTab.holdingTab)
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
							DockArea::TopLevelNode newTopLevelNode{};
							newTopLevelNode.rect = rect;
							newTopLevelNode.rect.position = cursorPos - widgetRect.position -
																							dockArea.behaviorData.holdingTab.cursorPosRelative;
							DockArea::Node *newNode = new DockArea::Node;
							newTopLevelNode.node = Std::Box{newNode};
							newNode->type = DockArea::Node::Type::Window;
							newNode->windows.push_back(Std::Move(node.windows[node.selectedWindow]));
							dockArea.topLevelNodes.emplace(dockArea.topLevelNodes.begin(), Std::Move(newTopLevelNode));

							// Remove the window from the node
							node.windows.erase(node.windows.begin() + node.selectedWindow);
							if (node.selectedWindow > 0)
								node.selectedWindow -= 1;

							// We are now moving this tab.
							Math::Vec2Int cursorRelativePos = dockArea.behaviorData.holdingTab.cursorPosRelative;
							dockArea.behavior = DockArea::Behavior::Moving;
							dockArea.behaviorData.moving = {};
							dockArea.behaviorData.moving.windowMovedRelativePos = cursorRelativePos;

							if (node.windows.empty())
							{
								// We want to turn the parent-node from a split into a window-node.
								DENGINE_IMPL_GUI_ASSERT(parentNode);
								DENGINE_IMPL_GUI_ASSERT(
									parentNode->split.a == &node || parentNode->split.b == &node);
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

	if (this->behavior == DockArea::Behavior::Normal)
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
	else if (this->behavior == DockArea::Behavior::Moving)
	{
		impl::DockArea_TouchEvent_Behavior_Moving(
			ctx,
			implData,
			dockArea,
			widgetRect,
			visibleRect,
			event);
	}
	else if (this->behavior == DockArea::Behavior::Resizing)
	{
		impl::DockArea_TouchEvent_Behavior_Resizing(
			ctx,
			implData,
			dockArea,
			widgetRect,
			visibleRect,
			event);
	}
	else if (this->behavior == DockArea::Behavior::HoldingTab)
	{
		impl::DockArea_TouchEvent_Behavior_HoldingTab(
			ctx,
			implData,
			dockArea,
			widgetRect,
			visibleRect,
			event);
	}
}

void DEngine::Gui::DockArea::CharEnterEvent(Context& ctx)
{
	// Only send events to the window in focus?

	bool continueIterating = true;
	impl::DockArea_IterateTopLevelNodes(
		impl::DockArea_IterateTopLevelNodeMode::FrontToBack,
		topLevelNodes,
		Rect{},
		continueIterating,
		[&ctx](
			DockArea::TopLevelNode& topLevelNode,
			Rect topLevelRect,
			uSize topLevelIndex,
			bool& continueIterating)
		{
			impl::DockArea_IterateNode(
				*topLevelNode.node,
				topLevelRect,
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
						else if (window.layout)
							window.layout->CharEnterEvent(ctx);
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
	impl::DockArea_IterateTopLevelNodes(
		impl::DockArea_IterateTopLevelNodeMode::FrontToBack,
		topLevelNodes,
		Rect{},
		continueIterating,
		[&ctx, utfValue](
			DockArea::TopLevelNode& topLevelNode,
			Rect topLevelRect,
			uSize topLevelIndex,
			bool& continueIterating)
		{
			impl::DockArea_IterateNode(
				*topLevelNode.node,
				topLevelRect,
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
						else if (window.layout)
							window.layout->CharEvent(
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
	impl::DockArea_IterateTopLevelNodes(
		impl::DockArea_IterateTopLevelNodeMode::FrontToBack,
		topLevelNodes,
		Rect{},
		continueIterating,
		[&ctx](
			DockArea::TopLevelNode& topLevelNode,
			Rect topLevelRect,
			uSize topLevelIndex,
			bool& continueIterating)
		{
			impl::DockArea_IterateNode(
				*topLevelNode.node,
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
						else if (window.layout)
							window.layout->CharRemoveEvent(
								ctx);
					}
				});
		});
}
