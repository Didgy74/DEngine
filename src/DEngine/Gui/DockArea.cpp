#include <DEngine/Gui/DockArea.hpp>

#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/impl/ImplData.hpp>

#include <DEngine/Containers/Opt.hpp>
#include <DEngine/Utility.hpp>

#include <DEngine/Application.hpp>

#include <iostream>

namespace DEngine::Gui::impl
{
	using DockArea_WindowCallablePFN = void(*)(
		DockArea::Node& node,
		Rect rect,
		DockArea::Node* parentNode,
		bool& continueIterating);

	using DockArea_SplitCallablePFN = void(*)(
		DockArea::Node& node,
		Rect rect,
		bool& continueIterating);

	template<typename T, typename WindowCallable, typename SplitCallable>
	void DockArea_IterateThroughNode_Internal(
		T& node,
		Rect rect,
		T* parentNode,
		bool& continueIterating,
		Std::Opt<WindowCallable> windowCallable,
		Std::Opt<SplitCallable> splitCallable)
	{
		if (node.type == DockArea::Node::Type::Window)
		{
			DENGINE_IMPL_GUI_ASSERT(!node.windows.empty());
			if (windowCallable.HasValue())
				windowCallable.Value()(
					node,
					rect,
					parentNode,
					continueIterating);
			return;
		}

		DENGINE_IMPL_GUI_ASSERT(node.type == DockArea::Node::Type::HoriSplit || node.type == DockArea::Node::Type::VertSplit);
		if (splitCallable.HasValue())
			splitCallable.Value()(
				node,
				rect,
				continueIterating);
		if (!continueIterating)
			return;

		Rect childRect = rect;
		if (node.type == DockArea::Node::Type::HoriSplit)
			childRect.extent.width = u32(rect.extent.width * node.split.split);
		else
			childRect.extent.height = u32(rect.extent.height * node.split.split);

		DockArea_IterateThroughNode_Internal(
			*node.split.a,
			childRect,
			&node,
			continueIterating,
			windowCallable,
			splitCallable);
		if (!continueIterating)
			return;

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

		DockArea_IterateThroughNode_Internal(
			*node.split.b,
			childRect,
			&node,
			continueIterating,
			windowCallable,
			splitCallable);
	}

	template<typename T, typename WindowCallable, typename SplitCallable>
	void DockArea_IterateThroughNode(
		T& node,
		Rect rect,
		bool& continueIterating,
		Std::Opt<WindowCallable> windowCallable,
		Std::Opt<SplitCallable> splitCallable)
	{
		DENGINE_IMPL_GUI_ASSERT(continueIterating);
		DockArea_IterateThroughNode_Internal(
			node,
			rect,
			(T*)nullptr,
			continueIterating,
			windowCallable,
			splitCallable);
	}

	enum class DockArea_ResizeRect { Top, Bottom, Left, Right };
	using DockArea_IterateResizeRectCallbackPFN = void(*)(
		DockArea_ResizeRect side,
		Rect rect);
	template<typename Callable>
	void DockArea_IterateThroughResizeRects(
		Rect rect,
		u32 resizeAreaWidth,
		Callable callable)
	{
		Rect topRect = rect;
		topRect.extent.height = resizeAreaWidth;
		topRect.position.y -= topRect.extent.height / 2;
		callable(DockArea_ResizeRect::Top, topRect);
		Rect bottomRect = rect;
		bottomRect.extent.height = resizeAreaWidth;
		bottomRect.position.y += rect.extent.height - (bottomRect.extent.height / 2);
		callable(DockArea_ResizeRect::Bottom, bottomRect);
		Rect leftRect = rect;
		leftRect.extent.width = resizeAreaWidth;
		leftRect.position.x -= leftRect.extent.width / 2;
		callable(DockArea_ResizeRect::Left, leftRect);
		Rect rightRect = rect;
		rightRect.extent.width = resizeAreaWidth;
		rightRect.position.x += rect.extent.width - rightRect.extent.width / 2;
		callable(DockArea_ResizeRect::Right, rightRect);
	}

	enum class DockArea_LayoutGizmo { Top, Bottom, Left, Right, Center };
	using DockArea_IterateLayoutGizmoCallbackPFN = void(*)(
		DockArea_LayoutGizmo gizmo,
		Rect rect);
	template<typename Callable>
	void DockArea_IterateThroughLayoutGizmos(
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

}

using namespace DEngine;
using namespace DEngine::Gui;

DockArea::DockArea()
{
}

SizeHint DockArea::SizeHint(
	Context const& ctx) const
{
	Gui::SizeHint returnVal{};
	returnVal.expand = true;

	return returnVal;
}

SizeHint DockArea::SizeHint_Tick(
	Context const& ctx)
{
	Gui::SizeHint returnVal{};
	returnVal.expand = true;

	return returnVal;
}

namespace DEngine::Gui::impl
{
	static auto DockArea_GetRenderLambda(
		Context const& ctx,
		ImplData& implData,
		DrawInfo& drawInfo,
		Extent framebufferExtent)
	{
		return [&ctx, &implData, &drawInfo, framebufferExtent](
			DockArea::Node const& node,
			Rect rect,
			DockArea::Node const* parentNode,
			bool& continueIterating)
		{
			// First draw titlebar and main window background
			auto& window = node.windows[node.selectedWindow];
			Rect titleBarRect{};
			titleBarRect.position = rect.position;
			titleBarRect.extent.width = rect.extent.width;
			titleBarRect.extent.height = implData.textManager.lineheight;
			Gfx::GuiDrawCmd cmd{};
			cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
			cmd.filledMesh.color = window.titleBarColor;
			cmd.filledMesh.mesh = drawInfo.GetQuadMesh();
			cmd.rectPosition.x = f32(titleBarRect.position.x) / framebufferExtent.width;
			cmd.rectPosition.y = f32(titleBarRect.position.y) / framebufferExtent.height;
			cmd.rectExtent.x = f32(titleBarRect.extent.width) / framebufferExtent.width;
			cmd.rectExtent.y = f32(titleBarRect.extent.height) / framebufferExtent.height;
			drawInfo.drawCmds.push_back(cmd);

			Rect contentRect{};
			contentRect.position.x = titleBarRect.position.x;
			contentRect.position.y = titleBarRect.position.y + titleBarRect.extent.height;
			contentRect.extent.width = titleBarRect.extent.width;
			contentRect.extent.height = rect.extent.height - titleBarRect.extent.height;

			// Draw the main window
			//Gfx::GuiDrawCmd cmd{};
			cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
			cmd.filledMesh.color = window.titleBarColor * 0.5f;
			cmd.filledMesh.color.w = 0.95f;
			cmd.filledMesh.mesh = drawInfo.GetQuadMesh();
			cmd.rectPosition.x = f32(contentRect.position.x) / framebufferExtent.width;
			cmd.rectPosition.y = f32(contentRect.position.y) / framebufferExtent.height;
			cmd.rectExtent.x = f32(contentRect.extent.width) / framebufferExtent.width;
			cmd.rectExtent.y = f32(contentRect.extent.height) / framebufferExtent.height;
			drawInfo.drawCmds.push_back(cmd);

			if (window.widget || window.layout)
			{
				drawInfo.PushScissor(contentRect);
				if (window.widget)
				{
					window.widget->Render(
						ctx,
						framebufferExtent,
						contentRect,
						contentRect,
						drawInfo);
				}
				else
				{
					window.layout->Render(
						ctx,
						framebufferExtent,
						contentRect,
						contentRect,
						drawInfo);
				}
				drawInfo.PopScissor();
			}

			// Draw all the tabs
			u32 tabHoriOffset = 0;
			for (uSize i = 0; i < node.windows.size(); i += 1)
			{
				auto& window = node.windows[i];
				Gui::SizeHint titleBarSizeHint = impl::TextManager::GetSizeHint(
					implData.textManager,
					window.title);
				Rect tabRect{};
				tabRect.position = rect.position;
				tabRect.position.x += tabHoriOffset;
				tabRect.extent = titleBarSizeHint.preferred;
				// Draw tab highlight
				if (i == node.selectedWindow)
				{
					Gfx::GuiDrawCmd cmd{};
					cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
					cmd.filledMesh.color = window.titleBarColor + Math::Vec4{ 0.25f, 0.25f, 0.25f, 1.f };
					cmd.filledMesh.mesh = drawInfo.GetQuadMesh();
					cmd.rectPosition.x = f32(tabRect.position.x) / framebufferExtent.width;
					cmd.rectPosition.y = f32(tabRect.position.y) / framebufferExtent.height;
					cmd.rectExtent.x = f32(tabRect.extent.width) / framebufferExtent.width;
					cmd.rectExtent.y = f32(tabRect.extent.height) / framebufferExtent.height;
					drawInfo.drawCmds.push_back(cmd);
				}

				impl::TextManager::RenderText(
					implData.textManager,
					window.title,
					{ 1.f, 1.f, 1.f, 1.f },
					framebufferExtent,
					tabRect,
					drawInfo);

				tabHoriOffset += tabRect.extent.width;
			}
		};
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

	for (uSize n = topLevelNodes.size(); n > 0; n -= 1)
	{
		uSize i = n - 1;
		auto& topLevelNode = topLevelNodes[i];
		Rect rectToUse = topLevelNode.rect;
		rectToUse.position += widgetRect.position;
		bool continueIterating = true;
		impl::DockArea_IterateThroughNode(
			*topLevelNode.node,
			rectToUse,
			continueIterating,
			Std::Opt{ impl::DockArea_GetRenderLambda(
				ctx, 
				implData, 
				drawInfo, 
				framebufferExtent) },
			Std::Opt<impl::DockArea_SplitCallablePFN>{});
	}

	// We need to draw the layout gizmo stuff
	if (showLayoutNodePtr)
	{
		for (uSize i = 0; i < topLevelNodes.size(); i += 1)
		{
			auto lambda = [this, &implData, &drawInfo, framebufferExtent](
				Node const& node,
				Rect rect,
				Node const* parentNode,
				bool& continueIterating)
			{
				if (&node != showLayoutNodePtr)
					return;
				continueIterating = false;
				
				auto& window = node.windows[node.selectedWindow];

				Rect contentRect = rect;
				contentRect.position.y += implData.textManager.lineheight;
				contentRect.extent.height -= implData.textManager.lineheight;

				impl::DockArea_IterateThroughLayoutGizmos(
					contentRect,
					this->gizmoSize,
					this->gizmoPadding,
					[&drawInfo, framebufferExtent](
						impl::DockArea_LayoutGizmo gizmo,
						Rect rect)
					{
						Gfx::GuiDrawCmd cmd{};
						cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
						cmd.filledMesh.color = { 1.f, 1.f, 1.f, 1.f };
						cmd.filledMesh.color.w = 0.9f;
						cmd.filledMesh.mesh = drawInfo.GetQuadMesh();
						cmd.rectPosition.x = (f32)rect.position.x / framebufferExtent.width;
						cmd.rectPosition.y = (f32)rect.position.y / framebufferExtent.height;
						cmd.rectExtent.x = (f32)rect.extent.width / framebufferExtent.width;
						cmd.rectExtent.y = (f32)rect.extent.height / framebufferExtent.height;
						drawInfo.drawCmds.push_back(cmd);
					});
			};

			auto& topLevelNode = topLevelNodes[i];
			Rect rectToUse = topLevelNode.rect;
			rectToUse.position += widgetRect.position;
			bool continueIterating = true;
			impl::DockArea_IterateThroughNode(
				*topLevelNode.node,
				rectToUse,
				continueIterating,
				Std::Opt{ lambda },
				Std::Opt<impl::DockArea_SplitCallablePFN>());
			if (!continueIterating)
				break;
		}
	}

	if (highlightRect.HasValue())
	{
		// Add the layout-thing highlight
		Gfx::GuiDrawCmd cmd{};
		cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
		cmd.filledMesh.color = { 0.f, 0.5f, 1.f, 0.25f };
		cmd.filledMesh.mesh = drawInfo.GetQuadMesh();
		cmd.rectPosition.x = f32(highlightRect.Value().position.x) / framebufferExtent.width;
		cmd.rectPosition.y = f32(highlightRect.Value().position.y) / framebufferExtent.height;
		cmd.rectExtent.x = (f32)highlightRect.Value().extent.width / framebufferExtent.width;
		cmd.rectExtent.y = (f32)highlightRect.Value().extent.height / framebufferExtent.height;

		drawInfo.drawCmds.push_back(cmd);
	}

	drawInfo.PopScissor();
}

void DockArea::Tick(
	Context& ctx, 
	Rect widgetRect, 
	Rect visibleRect)
{
	for (uSize topLevelIndex = 0; topLevelIndex < topLevelNodes.size(); topLevelIndex += 1)
	{
		auto lambda = [&ctx](
			Node& node,
			Rect rect,
			Node* parentNode,
			bool& continueIterating)
		{
			// Iterate over each window in a node
			for (uSize windowIndex = 0; windowIndex < node.windows.size(); windowIndex += 1)
			{
				auto& window = node.windows[windowIndex];
				Rect childVisibleRect = windowIndex == node.selectedWindow ? rect : Rect{};
				if (window.widget)
				{
					window.widget->Tick(
						ctx,
						rect,
						childVisibleRect);
				}
				else if (window.layout)
				{
					window.layout->Tick(
						ctx,
						rect,
						childVisibleRect);
				}
			}
		};

		auto& topLevelNode = topLevelNodes[topLevelIndex];
		Rect rectToUse = topLevelNode.rect;
		rectToUse.position += widgetRect.position;
		bool continueIterating = true;
		impl::DockArea_IterateThroughNode(
			*topLevelNode.node,
			rectToUse,
			continueIterating,
			Std::Opt{ lambda },
			Std::Opt<impl::DockArea_SplitCallablePFN>());
	}
}

namespace DEngine::Gui::impl
{
	static void DockArea_CursorMove_Behavior_Normal(
		Context& ctx,
		DockArea& dockArea,
		ImplData& implData,
		Rect widgetRect,
		Rect visibleRect,
		Gui::CursorMoveEvent event)
	{
		// Check if we are hovering a resize rect.
		bool rectWasHit = false;
		// Iterate over every window.
		for (uSize topLevelIndex = 0; topLevelIndex < dockArea.topLevelNodes.size(); topLevelIndex += 1)
		{
			auto& topLevelNode = dockArea.topLevelNodes[topLevelIndex];
			Rect rect = topLevelNode.rect;
			rect.position += widgetRect.position;
			bool continueIterating = true;

			Std::Opt<DockArea_ResizeRect> resizeRectHit;
			DockArea_IterateThroughResizeRects(
				rect,
				dockArea.resizeAreaRect,
				[&resizeRectHit, event](DockArea_ResizeRect side, Rect rect)
				{
					if (!resizeRectHit.HasValue() && rect.PointIsInside(event.position))
						resizeRectHit = Std::Opt{ side };
				});
			if (resizeRectHit.HasValue())
			{
				continueIterating = false;
				rectWasHit = true;
				switch (resizeRectHit.Value())
				{
				case DockArea_ResizeRect::Top:
				case DockArea_ResizeRect::Bottom:
					App::SetCursor(App::WindowID(0), App::CursorType::VerticalResize);
					break;
				case DockArea_ResizeRect::Left:
				case DockArea_ResizeRect::Right:
					App::SetCursor(App::WindowID(0), App::CursorType::HorizontalResize);
					break;
				}
			}

			if (continueIterating && !rectWasHit && rect.PointIsInside(event.position))
			{
				// Send the move event to the widget
				auto lambda = [&ctx, &implData, event](
					DockArea::Node& node,
					Rect rect,
					DockArea::Node* parentNode,
					bool& continueIterating)
				{
					if (rect.PointIsInside(event.position))
						continueIterating = false;
					auto& window = node.windows[node.selectedWindow];
					Rect contentRect = rect;
					contentRect.position.y += implData.textManager.lineheight;
					contentRect.extent.height -= implData.textManager.lineheight;
					if (contentRect.PointIsInside(event.position))
					{
						if (window.widget)
						{
							window.widget->CursorMove(
								ctx,
								contentRect,
								contentRect,
								event);
						}
						else if (window.layout)
						{
							window.layout->CursorMove(
								ctx,
								contentRect,
								contentRect,
								event);
						}
					}
				};

				auto lambda2 = [event, &dockArea, &rectWasHit](
					DockArea::Node& node,
					Rect rect,
					bool& continueIterating)
				{
					DENGINE_IMPL_GUI_ASSERT(node.type == DockArea::Node::Type::HoriSplit || node.type == DockArea::Node::Type::VertSplit);

					if (node.type == DockArea::Node::Type::HoriSplit)
					{
						// calc the rect
						Rect middleRect = rect;
						middleRect.extent.width = dockArea.resizeAreaRect;
						middleRect.position.x += u32(rect.extent.width * node.split.split) - middleRect.extent.width / 2;
						if (middleRect.PointIsInside(event.position))
						{
							continueIterating = false;
							rectWasHit = true;
							App::SetCursor(App::WindowID(0), App::CursorType::HorizontalResize);
							return;
						}
					}
					else if (node.type == DockArea::Node::Type::VertSplit)
					{
						// calc the rect
						Rect middleRect = rect;
						middleRect.extent.height = dockArea.resizeAreaRect;
						middleRect.position.y += u32(rect.extent.height * node.split.split) - middleRect.extent.height / 2;
						if (middleRect.PointIsInside(event.position))
						{
							continueIterating = false;
							rectWasHit = true;
							App::SetCursor(App::WindowID(0), App::CursorType::VerticalResize);
							return;
						}
					}
				};
				DockArea_IterateThroughNode(
					*topLevelNode.node,
					rect,
					continueIterating,
					Std::Opt{ lambda },
					Std::Opt{ lambda2 });

				continueIterating = false;
			}

			if (!continueIterating)
				break;
		}
		if (!rectWasHit)
		{
			App::SetCursor(App::WindowID(0), App::CursorType::Arrow);
		}
	}

	static void DockArea_CursorMove_Behavior_Moving(
		DockArea& dockArea,
		ImplData& implData,
		Rect widgetRect,
		Rect visibleRect,
		Math::Vec2Int cursorPos)
	{
		// Check if we should display layout gizmos and highlight rect.

		bool cursorInsideWidget = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);
		if (cursorInsideWidget)
		{
			auto& topLevelNode = dockArea.topLevelNodes[0];
			topLevelNode.rect.position = cursorPos - widgetRect.position - dockArea.windowMovedRelativePos;

			bool windowWasHit = false;

			// Iterate over other windows, see if we are hovering layout gizmos in them first
			for (uSize i = 1; i < dockArea.topLevelNodes.size(); i += 1)
			{
				auto const& otherNode = dockArea.topLevelNodes[i];
				auto lambda = [&implData, cursorPos, &dockArea, &windowWasHit](
					DockArea::Node const& node,
					Rect rect,
					DockArea::Node const* parentNode,
					bool& continueIterating)
				{
					auto& window = node.windows[node.selectedWindow];
					// If we are inside the titlebar-area, stop then stop iterating.
					Rect titleBarRect{};
					titleBarRect.position = rect.position;
					titleBarRect.extent.width = rect.extent.width;
					titleBarRect.extent.height = implData.textManager.lineheight;
					if (titleBarRect.PointIsInside(cursorPos))
					{
						continueIterating = false;
						windowWasHit = true;
						return;
					}

					// Check if we are within the window's main area, and if so, display the layout gizmo stuff
					Rect contentRect = rect;
					contentRect.position.y += titleBarRect.extent.height;
					contentRect.extent.height -= titleBarRect.extent.height;
					if (contentRect.PointIsInside(cursorPos))
					{
						continueIterating = false;
						dockArea.showLayoutNodePtr = &node;
						windowWasHit = true;

						dockArea.highlightRect = Std::nullOpt;
						DockArea_IterateThroughLayoutGizmos(
							contentRect,
							dockArea.gizmoSize,
							dockArea.gizmoPadding,
							[&dockArea, contentRect, cursorPos](
								DockArea_LayoutGizmo gizmo,
								Rect rect)
							{
								if (!dockArea.highlightRect.HasValue() && rect.PointIsInside(cursorPos))
								{
									Rect highlightRectToUse = contentRect;
									switch (gizmo)
									{
									case DockArea_LayoutGizmo::Left:
										highlightRectToUse.extent.width = highlightRectToUse.extent.width * 0.5f;
										break;
									case DockArea_LayoutGizmo::Right:
										highlightRectToUse.extent.width = highlightRectToUse.extent.width * 0.5f;
										highlightRectToUse.position.x += highlightRectToUse.extent.width;
										break;
									case DockArea_LayoutGizmo::Top:
										highlightRectToUse.extent.height = highlightRectToUse.extent.height * 0.5f;
										break;
									case DockArea_LayoutGizmo::Bottom:
										highlightRectToUse.extent.height = highlightRectToUse.extent.height * 0.5f;
										highlightRectToUse.position.y += highlightRectToUse.extent.height;
										break;
									default:
										break;
									}
									dockArea.highlightRect = highlightRectToUse;
								}
							});
					}
				};
				Rect rect = otherNode.rect;
				rect.position += widgetRect.position;
				bool continueIterating = true;
				DockArea_IterateThroughNode(
					*otherNode.node,
					rect,
					continueIterating,
					Std::Opt{ lambda },
					Std::Opt<DockArea_SplitCallablePFN>());
				if (!continueIterating)
					break;
			}
			if (!windowWasHit)
			{
				dockArea.showLayoutNodePtr = nullptr;
				dockArea.highlightRect = Std::nullOpt;
			}
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

		if (cursorInsideWidget)
		{
			DENGINE_IMPL_GUI_ASSERT(!dockArea.topLevelNodes.empty());
			auto& topLevelNode = dockArea.topLevelNodes[0];

			if (dockArea.resizingSplit)
			{
				// Find the node we are resizing
				auto lambda = [&dockArea, widgetRect, cursorPos](
					DockArea::Node& node,
					Rect rect,
					bool& continueIterating)
				{
					if ((node.type == DockArea::Node::Type::HoriSplit || node.type == DockArea::Node::Type::VertSplit) &&
						&node == dockArea.resizeSplitNode)
					{
						continueIterating = false;

						if (node.type == DockArea::Node::Type::HoriSplit)
						{
							// Find out the position of the cursor in range 0-1 of the node.
							f32 test = f32(cursorPos.x - widgetRect.position.x - rect.position.x) / rect.extent.width;
							test = Math::Clamp(test, 0.1f, 0.9f);
							node.split.split = test;
							return;
						}
						else if (node.type == DockArea::Node::Type::VertSplit)
						{
							// Find out the position of the cursor in range 0-1 of the node.
							f32 test = f32(cursorPos.y - widgetRect.position.y - rect.position.y) / rect.extent.height;
							test = Math::Clamp(test, 0.1f, 0.9f);
							node.split.split = test;
							return;
						}

					}
				};
				bool continueIterating = true;
				DockArea_IterateThroughNode(
					*topLevelNode.node,
					topLevelNode.rect,
					continueIterating,
					Std::Opt<DockArea_WindowCallablePFN>{},
					Std::Opt{ lambda });
			}
			else
			{
				if (dockArea.resizeSide == DockArea::ResizeSide::Top)
				{
					i32 prevY = topLevelNode.rect.position.y;
					i32 a = cursorPos.y - widgetRect.position.y;
					i32 b = topLevelNode.rect.position.y + (i32)topLevelNode.rect.extent.height - 100;
					topLevelNode.rect.position.y = Math::Min(a, b);
					topLevelNode.rect.extent.height += prevY - topLevelNode.rect.position.y;
				}
				else if (dockArea.resizeSide == DockArea::ResizeSide::Bottom)
				{
					topLevelNode.rect.extent.height = cursorPos.y - widgetRect.position.y - topLevelNode.rect.position.y;
				}
				else if (dockArea.resizeSide == DockArea::ResizeSide::Left)
				{
					i32 prevX = topLevelNode.rect.position.x;
					i32 a = cursorPos.x - widgetRect.position.x;
					i32 b = topLevelNode.rect.position.x + (i32)topLevelNode.rect.extent.width - 100;
					topLevelNode.rect.position.x = Math::Min(a, b);
					topLevelNode.rect.extent.width += prevX - topLevelNode.rect.position.x;
				}
				else if (dockArea.resizeSide == DockArea::ResizeSide::Right)
				{
					topLevelNode.rect.extent.width = cursorPos.x - widgetRect.position.x - topLevelNode.rect.position.x;
				}

				topLevelNode.rect.extent.height = Math::Max(topLevelNode.rect.extent.height, 100U);
				topLevelNode.rect.extent.width = Math::Max(topLevelNode.rect.extent.width, 100U);
			}
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
		if (cursorInsideWidget && dockArea.holdingTab)
		{
			for (uSize topLevelNodeIndex = 0; topLevelNodeIndex < dockArea.topLevelNodes.size(); topLevelNodeIndex += 1)
			{
				auto lambda = [&implData, cursorPos, &dockArea](
					DockArea::Node& node,
					Rect rect,
					DockArea::Node const* parentNode,
					bool& continueIterating)
				{
					if (&node != dockArea.holdingTab)
						return;
					continueIterating = false;

					// We shouldn't be in the moving-tab-state if the node holds only one tab.
					DENGINE_IMPL_GUI_ASSERT(node.windows.size() > 1);

					// First find out the rect of the tab we are holding
					Rect tabRect = rect;
					for (uSize nodeWindowIndex = 0; nodeWindowIndex < node.windows.size(); nodeWindowIndex += 1)
					{
						auto& window = node.windows[nodeWindowIndex];
						Gui::SizeHint titleBarSizeHint = impl::TextManager::GetSizeHint(
							implData.textManager,
							window.title);
						tabRect.position.x += titleBarSizeHint.preferred.width;
						if (nodeWindowIndex == node.selectedWindow)
						{
							tabRect.extent = titleBarSizeHint.preferred;
							break;
						}
					}

					if ((cursorPos.y <= tabRect.position.y - dockArea.tabDisconnectRadius) ||
						(cursorPos.y >= tabRect.position.y + tabRect.extent.height + dockArea.tabDisconnectRadius))
					{
						// We want to disconnect this tab into a new top-level node.
						DockArea::TopLevelNode newTopLevelNode{};
						newTopLevelNode.rect = rect;
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
						dockArea.holdingTab = nullptr;
						dockArea.currentBehavior = DockArea::Behavior::Moving;
					}

				};

				auto& topLevelNode = dockArea.topLevelNodes[topLevelNodeIndex];
				Rect yo = topLevelNode.rect;
				yo.position += widgetRect.position;
				bool continueIterating = true;
				DockArea_IterateThroughNode(
					*topLevelNode.node,
					yo,
					continueIterating,
					Std::Opt{ lambda },
					Std::Opt<DockArea_SplitCallablePFN>());
				if (!continueIterating)
					break;
			}
		}
	}
}

void DockArea::CursorMove(
	Context& ctx, 
	Rect widgetRect, 
	Rect visibleRect, 
	CursorMoveEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	bool cursorInsideWidget = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);

	if (this->currentBehavior == Behavior::Normal)
	{
		impl::DockArea_CursorMove_Behavior_Normal(
			ctx,
			*this,
			implData,
			widgetRect,
			visibleRect,
			event);
	}
	else if (this->currentBehavior == Behavior::Moving)
	{
		impl::DockArea_CursorMove_Behavior_Moving(
			*this,
			implData,
			widgetRect,
			visibleRect,
			event.position);
	}
	else if (this->currentBehavior == Behavior::Resizing)
	{
		impl::DockArea_CursorMove_Behavior_Resizing(
			*this,
			implData,
			widgetRect,
			visibleRect,
			event.position);
	}
	else if (this->currentBehavior == Behavior::HoldingTab)
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
	static void DockArea_CursorClick_Behavior_Normal(
		Context& ctx,
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

		Std::Opt<uSize> topLevelNodeToPushToFront;
		for (uSize topLevelIndex = 0; topLevelIndex < dockArea.topLevelNodes.size(); topLevelIndex += 1)
		{
			auto& topLevelNode = dockArea.topLevelNodes[topLevelIndex];
			Rect topLevelRect = topLevelNode.rect;
			topLevelRect.position += widgetRect.position;
			bool continueIterating = true;

			auto lambda = [&ctx, &dockArea, &implData, event, cursorPos, widgetRect, &topLevelNodeToPushToFront, topLevelIndex, topLevelRect](
				DockArea::Node& node,
				Rect rect,
				DockArea::Node* parentNode,
				bool& continueIterating)
			{
				if (rect.PointIsInside(cursorPos))
					continueIterating = false;

				// After we check if should hold a tab, or move the window
				// Check if we are inside title bar first
				Rect titleBarRect{};
				auto& window = node.windows[node.selectedWindow];
				titleBarRect.position = rect.position;
				titleBarRect.extent = { rect.extent.width, implData.textManager.lineheight };
				if (titleBarRect.PointIsInside(cursorPos))
				{
					continueIterating = false;

					// Check if we hit one of the tabs, if so, select it
					bool tabWasHit = false;
					if (node.windows.size() > 1)
					{
						// We have multiple tabs, dragging out a tab means removing it from the list of tabs
						u32 tabHoriOffset = 0;
						for (uSize windowIndex = 0; windowIndex < node.windows.size(); windowIndex += 1)
						{
							auto& window = node.windows[windowIndex];
							Gui::SizeHint titleBarSizeHint = impl::TextManager::GetSizeHint(
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
								topLevelNodeToPushToFront = Std::Opt{ topLevelIndex };
								dockArea.currentBehavior = DockArea::Behavior::HoldingTab;
								dockArea.holdingTab = &node;
								dockArea.windowMovedRelativePos = cursorPos - tabRect.position;
								break;
							}
						}
					}
					else
					{
						// We only have one tab.
						auto& window = node.windows[node.selectedWindow];
						Gui::SizeHint titleBarSizeHint = impl::TextManager::GetSizeHint(
							implData.textManager,
							window.title);
						Rect tabRect{};
						tabRect.position = rect.position;
						tabRect.extent = titleBarSizeHint.preferred;
						if (parentNode && tabRect.PointIsInside(cursorPos))
						{
							// We only have one tab. Dragging it out means splitting the Hori/Vert split if we have a parent.
							DockArea::TopLevelNode topLevelNode{};
							topLevelNode.rect = rect;
							topLevelNode.rect.position -= widgetRect.position;
							parentNode->type = DockArea::Node::Type::Window;
							if (parentNode->split.a == &node)
							{
								topLevelNode.node = Std::Move(parentNode->split.a);
								parentNode->windows = Std::Move(parentNode->split.b->windows);
								parentNode->split.b = {};
							}
							else
							{
								topLevelNode.node = Std::Move(parentNode->split.b);
								parentNode->windows = Std::Move(parentNode->split.a->windows);
								parentNode->split.a = {};
							}
							dockArea.topLevelNodes.emplace(dockArea.topLevelNodes.begin(), Std::Move(topLevelNode));
							dockArea.currentBehavior = DockArea::Behavior::Moving;
							dockArea.windowMovedRelativePos = cursorPos - rect.position;
							tabWasHit = true;
							return;
						}
					}
					if (!tabWasHit)
					{
						// We hit the title bar but not any tabs, start moving it and bring it to front
						dockArea.currentBehavior = DockArea::Behavior::Moving;
						dockArea.windowMovedRelativePos = cursorPos - topLevelRect.position;
						topLevelNodeToPushToFront = Std::Opt{ topLevelIndex };
					}
					return;
				}

				// We did not hit the titlebar, check if we hit the window content
				Rect contentRect{};
				contentRect.position.x = titleBarRect.position.x;
				contentRect.position.y = titleBarRect.position.y + titleBarRect.extent.height;
				contentRect.extent.width = titleBarRect.extent.width;
				contentRect.extent.height = rect.extent.height - titleBarRect.extent.height;
				if (contentRect.PointIsInside(cursorPos))
				{
					continueIterating = false;
					// Bring to front
					// Send event to inner widget/layout
					// Then don't pass the event to windows further back.
					topLevelNodeToPushToFront = Std::Opt{ topLevelIndex };

					auto& window = node.windows[node.selectedWindow];
					if (window.widget)
					{
						window.widget->CursorClick(
							ctx,
							contentRect,
							contentRect,
							cursorPos,
							event);
					}
					else if (window.layout)
					{
						window.layout->CursorClick(
							ctx,
							contentRect,
							contentRect,
							cursorPos,
							event);
					}

					return;
				}
			};

			auto lambda2 = [event, &dockArea, cursorPos, &topLevelNodeToPushToFront, topLevelIndex](
				DockArea::Node const& node,
				Rect rect,
				bool& continueIterating)
			{
				DENGINE_IMPL_GUI_ASSERT(node.type == DockArea::Node::Type::HoriSplit || node.type == DockArea::Node::Type::VertSplit);

				if (node.type == DockArea::Node::Type::HoriSplit)
				{
					// calc the rect
					Rect middleRect = rect;
					middleRect.extent.width = dockArea.resizeAreaRect;
					middleRect.position.x += u32(rect.extent.width * node.split.split) - middleRect.extent.width / 2;
					if (middleRect.PointIsInside(cursorPos))
					{
						continueIterating = false;
						dockArea.currentBehavior = DockArea::Behavior::Resizing;
						dockArea.resizingSplit = true;
						dockArea.resizeSplitNode = &node;
						topLevelNodeToPushToFront = { topLevelIndex };
						return;
					}
				}
				else if (node.type == DockArea::Node::Type::VertSplit)
				{
					// calc the rect
					Rect middleRect = rect;
					middleRect.extent.height = dockArea.resizeAreaRect;
					middleRect.position.y += u32(rect.extent.height * node.split.split) - middleRect.extent.height / 2;
					if (middleRect.PointIsInside(cursorPos))
					{
						continueIterating = false;
						dockArea.currentBehavior = DockArea::Behavior::Resizing;
						dockArea.resizingSplit = true;
						dockArea.resizeSplitNode = &node;
						topLevelNodeToPushToFront = { topLevelIndex };
						return;
					}
				}
			};

			if (event.clicked)
			{
				Std::Opt<DockArea_ResizeRect> resizeRectHit;
				DockArea_IterateThroughResizeRects(
					topLevelRect,
					dockArea.resizeAreaRect,
					[&resizeRectHit, cursorPos](DockArea_ResizeRect side, Rect rect)
				{
					if (!resizeRectHit.HasValue() && rect.PointIsInside(cursorPos))
						resizeRectHit = Std::Opt{ side };
				});
				if (resizeRectHit.HasValue())
				{
					continueIterating = false;
					topLevelNodeToPushToFront = { topLevelIndex };
					dockArea.currentBehavior = DockArea::Behavior::Resizing;
					dockArea.resizingSplit = false;
					dockArea.resizeSplitNode = nullptr;
					switch (resizeRectHit.Value())
					{
					case DockArea_ResizeRect::Top: dockArea.resizeSide = DockArea::ResizeSide::Top; break;
					case DockArea_ResizeRect::Bottom: dockArea.resizeSide = DockArea::ResizeSide::Bottom; break;
					case DockArea_ResizeRect::Left: dockArea.resizeSide = DockArea::ResizeSide::Left; break;
					case DockArea_ResizeRect::Right: dockArea.resizeSide = DockArea::ResizeSide::Right; break;
					}
				}

				if (!continueIterating)
					break;
			}

			DockArea_IterateThroughNode(
				*topLevelNode.node,
				topLevelRect,
				continueIterating,
				Std::Opt{ lambda },
				Std::Opt{ lambda2 });
			if (!continueIterating)
				break;
		}
		if (topLevelNodeToPushToFront.HasValue())
		{
			uSize i = topLevelNodeToPushToFront.Value();
			DockArea::TopLevelNode tempTopLevelNode = Std::Move(dockArea.topLevelNodes[i]);
			dockArea.topLevelNodes.erase(dockArea.topLevelNodes.begin() + i);
			dockArea.topLevelNodes.insert(dockArea.topLevelNodes.begin(), Std::Move(tempTopLevelNode));
		}
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

		// Find out if we need to dock a window into another.
		if (isInsideWidget && !event.clicked)
		{
			// We don't need to test the front-most node, since that's the one being moved.
			for (uSize i = 1; i < dockArea.topLevelNodes.size(); i++)
			{
				auto lambda = [&dockArea, &implData, cursorPos](
					DockArea::Node& node,
					Rect rect,
					DockArea::Node* parentNode,
					bool& continueIterating)
				{
					if (&node != dockArea.showLayoutNodePtr)
						return;
					continueIterating = false;

					auto& window = node.windows[node.selectedWindow];

					// Check if we are within the window's main area first
					Rect contentRect = rect;
					contentRect.position.y += implData.textManager.lineheight;
					contentRect.extent.height -= implData.textManager.lineheight;
					if (contentRect.PointIsInside(cursorPos))
					{
						bool test = false;
						DockArea_IterateThroughLayoutGizmos(
							contentRect,
							dockArea.gizmoSize,
							dockArea.gizmoPadding,
							[&dockArea, &node, cursorPos, &test](
								DockArea_LayoutGizmo gizmo,
								Rect rect)
							{
								if (!test && rect.PointIsInside(cursorPos))
								{
									test = true;
									if (gizmo == DockArea_LayoutGizmo::Left || gizmo == DockArea_LayoutGizmo::Right)
										node.type = DockArea::Node::Type::HoriSplit;
									else if (gizmo == DockArea_LayoutGizmo::Top || gizmo == DockArea_LayoutGizmo::Bottom)
										node.type = DockArea::Node::Type::VertSplit;
									if (gizmo == DockArea_LayoutGizmo::Left || gizmo == DockArea_LayoutGizmo::Right ||
											gizmo == DockArea_LayoutGizmo::Top || gizmo == DockArea_LayoutGizmo::Bottom)
									{
										// Turn this node into a horisplit-node
										DockArea::Node* newNode = new DockArea::Node;
										auto& nodeA = gizmo == DockArea_LayoutGizmo::Left || gizmo == DockArea_LayoutGizmo::Top ? node.split.a : node.split.b;
										auto& nodeB = gizmo == DockArea_LayoutGizmo::Left || gizmo == DockArea_LayoutGizmo::Top ? node.split.b : node.split.a;
										nodeB = Std::Box{ newNode };
										newNode->type = DockArea::Node::Type::Window;
										newNode->windows = Std::Move(node.windows);
										nodeA = Std::Move(dockArea.topLevelNodes[0].node);
										dockArea.topLevelNodes.erase(dockArea.topLevelNodes.begin());
									}

									if (gizmo == DockArea_LayoutGizmo::Center && dockArea.topLevelNodes[0].node->type == DockArea::Node::Type::Window)
									{
										for (auto& window : dockArea.topLevelNodes[0].node->windows)
											node.windows.emplace_back(Std::Move(window));
										node.selectedWindow = node.windows.size() - 1;
										dockArea.topLevelNodes.erase(dockArea.topLevelNodes.begin());
									}
								}
							});
					}
				};

				auto& topLevelNode = dockArea.topLevelNodes[i];
				Rect topLevelNodeRect = topLevelNode.rect;
				topLevelNodeRect.position += widgetRect.position;
				bool continueIterating = true;
				DockArea_IterateThroughNode(
					*topLevelNode.node,
					topLevelNodeRect,
					continueIterating,
					Std::Opt{ lambda },
					Std::Opt<DockArea_SplitCallablePFN>());
				if (!continueIterating)
					break;
			}
		}

		if (!event.clicked)
		{
			dockArea.currentBehavior = DockArea::Behavior::Normal;
			dockArea.showLayoutNodePtr = nullptr;
			dockArea.highlightRect = Std::nullOpt;
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
			dockArea.currentBehavior = DockArea::Behavior::Normal;
		}
	}

	static void DockArea_CursorClick_Behavior_HoldingTab(
		DockArea& dockArea,
		Rect widgetRect,
		Rect visibleRect,
		Math::Vec2Int cursorPos,
		CursorClickEvent event)
	{
		bool cursorInsideWidget = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);
		if (!event.clicked)
		{
			dockArea.currentBehavior = DockArea::Behavior::Normal;
			dockArea.holdingTab = nullptr;
		}
	}
}

void DockArea::CursorClick(
	Context& ctx, 
	Rect widgetRect, 
	Rect visibleRect, 
	Math::Vec2Int cursorPos, 
	CursorClickEvent event)
{
	bool isInsideWidget = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);

	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	if (this->currentBehavior == Behavior::Normal)
	{
		impl::DockArea_CursorClick_Behavior_Normal(
			ctx,
			*this,
			widgetRect,
			visibleRect,
			cursorPos,
			event,
			implData);
	}
	else if (this->currentBehavior == Behavior::Moving)
	{
		impl::DockArea_CursorClick_Behavior_Moving(
			*this,
			widgetRect,
			visibleRect,
			cursorPos,
			event,
			implData);
	}
	else if (this->currentBehavior == Behavior::Resizing)
	{
		impl::DockArea_CursorClick_Behavior_Resizing(
			*this,
			widgetRect,
			visibleRect,
			event);
	}
	else if (this->currentBehavior == Behavior::HoldingTab)
	{
		impl::DockArea_CursorClick_Behavior_HoldingTab(
			*this,
			widgetRect,
			visibleRect,
			cursorPos,
			event);
	}
}
