#include <DEngine/Gui/ScrollArea.hpp>

#include <DEngine/Std/Trait.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	[[nodiscard]] static Rect GetChildRect(
		Rect widgetRect,
		SizeHint childSizeHint,
		u32 scrollbarPos,
		u32 scrollbarThickness)
	{
		Rect childRect = {};
		childRect.position = widgetRect.position;
		if (childSizeHint.expandX)
		{
			childRect.extent.width = widgetRect.extent.width - scrollbarThickness;
		}
		else
		{
			childRect.extent.width = widgetRect.extent.width - scrollbarThickness;
		}
		if (childSizeHint.expandY)
		{
			childRect.extent.height = widgetRect.extent.height;
		}
		else
		{
			childRect.position.y -= scrollbarPos;
			childRect.extent.height = Math::Max(childSizeHint.preferred.height, widgetRect.extent.height);
		}
		return childRect;
	}

	static Rect GetScrollbarRect(
		u32 thickness,
		u32 position,
		Rect widgetRect,
		u32 widgetHeight)
	{
		Rect scrollbarRect = {};
		scrollbarRect.extent.width = thickness;
		scrollbarRect.extent.height = widgetRect.extent.height;
		scrollbarRect.position.x = widgetRect.position.x + widgetRect.extent.width - thickness;
		scrollbarRect.position.y = widgetRect.position.y;
		if (widgetRect.extent.height < widgetHeight)
		{
			f32 factor = (f32)widgetRect.extent.height / widgetHeight;
			if (factor < 1.f)
				scrollbarRect.extent.height *= factor;

			f32 test = (f32)position / (widgetHeight - widgetRect.extent.height);

			scrollbarRect.position.y += test * (widgetRect.extent.height - scrollbarRect.extent.height);
		}
		return scrollbarRect;
	}

	static constexpr u8 cursorPointerId = (u8)-1;

	// Returns true if the pointer is occluded.
	template<class T> requires (Std::Trait::isSame<T, CursorMoveEvent>)
	[[nodiscard]] static bool PointerMove(
		ScrollArea& scrollArea,
		Context& ctx,
		WindowID windowId,
		Rect const& widgetRect,
		Rect const& visibleRect,
		u8 pointerId,
		Math::Vec2 pointerPos,
		bool occluded,
		T const& event)
	{
		bool returnVal = false;

		bool pointerIsInside = widgetRect.PointIsInside(pointerPos) && visibleRect.PointIsInside(pointerPos);

		// If the pointer is inside, we always want to return occluded.
		if (pointerIsInside)
			returnVal = true;

		// If we don't have a widget, we always want to make sure we're in normal state.
		if (!scrollArea.widget)
		{
			scrollArea.scrollbarState = ScrollArea::ScrollbarState_Normal{};
			scrollArea.scrollbarPos = 0;

			// If we don't have a widget, we don't want to do anything.
			return returnVal;
		}

		// If we are hovering and the pointer becomes occluded, go into normal state.
		if (auto ptr = scrollArea.scrollbarState.ToPtr<ScrollArea::ScrollbarState_Hovered>();
			ptr && ptr->pointerId == pointerId && occluded && pointerIsInside)
		{
			scrollArea.scrollbarState = ScrollArea::ScrollbarState_Normal{};
			return returnVal;
		}

		SizeHint const childSizeHint = scrollArea.widget->GetSizeHint(ctx);

		Rect const childRect = impl::GetChildRect(
			widgetRect,
			childSizeHint,
			scrollArea.scrollbarPos,
			scrollArea.scrollbarThickness);

		if (auto pressedState = scrollArea.scrollbarState.ToPtr<ScrollArea::ScrollbarState_Pressed>())
		{
			if (widgetRect.extent.height < childRect.extent.height)
			{
				// The scroll-bar is currently pressed so we need to move it with the pointer.
				
				// Find out where mouse is relative to the scroll bar position
				i32 test = (i32)pointerPos.y - widgetRect.position.y - pressedState->pointerRelativePosY;
				u32 scrollBarHeight = (f32)widgetRect.extent.height * (f32)widgetRect.extent.height / (f32)childRect.extent.height;
				f32 factor = (f32)test / (widgetRect.extent.height - scrollBarHeight);
				factor = Math::Clamp(factor, 0.f, 1.f);
				scrollArea.scrollbarPos = u32(factor * (childRect.extent.height - widgetRect.extent.height));
			}
		}
		else
		{
			Rect scrollBarRect = impl::GetScrollbarRect(
				scrollArea.scrollbarThickness,
				scrollArea.scrollbarPos,
				widgetRect,
				childRect.extent.height);

			bool cursorIsHovering = scrollBarRect.PointIsInside(pointerPos) && visibleRect.PointIsInside(pointerPos);
			if (cursorIsHovering && !occluded)
			{
				ScrollArea::ScrollbarState_Hovered newState = {};
				newState.pointerId = pointerId;
				scrollArea.scrollbarState = newState;
			}
			else
			{
				scrollArea.scrollbarState = ScrollArea::ScrollbarState_Normal{};
			}
		}

		// We only want one of either.
		if constexpr (Std::Trait::isSame<T, CursorMoveEvent>)
		{
			scrollArea.widget->CursorMove(
				ctx,
				windowId,
				childRect,
				visibleRect,
				event,
				occluded);
		}
		else
		{
			static_assert(false, "Problemo");
		}

		return returnVal;
	}

	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static PointerType ToPointerType(Gui::CursorButton in) noexcept
	{
		switch (in)
		{
			case Gui::CursorButton::Primary: return PointerType::Primary;
			case Gui::CursorButton::Secondary: return PointerType::Secondary;
			default: break;
		}
		DENGINE_IMPL_UNREACHABLE();
		return {};
	}
	[[nodiscard]] static bool PointerPress(
		ScrollArea& scrollArea,
		Context& ctx,
		WindowID windowId,
		Rect const& widgetRect,
		Rect const& visibleRect,
		u8 pointerId,
		PointerType pointerType,
		Math::Vec2 pointerPos,
		bool pointerPressed,
		CursorClickEvent const& event)
	{
		bool pointerIsInside = widgetRect.PointIsInside(pointerPos) && visibleRect.PointIsInside(pointerPos);
		if (!pointerIsInside)
			return false;
	
		// If we don't have a widget, we always want to make sure we're in normal state.
		if (!scrollArea.widget)
		{
			scrollArea.scrollbarState = ScrollArea::ScrollbarState_Normal{};
			scrollArea.scrollbarPos = 0;

			// If we don't have a widget, we don't want to do anything.
			// Since we're inside the ScrollArea, we just consume the event
			// and return.
			return true;
		}

		if (!scrollArea.scrollbarState.IsA<ScrollArea::ScrollbarState_Normal>() && !pointerPressed)
		{
			u8 currentPointerId;
			if (auto ptr = scrollArea.scrollbarState.ToPtr<ScrollArea::ScrollbarState_Hovered>())
				currentPointerId = ptr->pointerId;
			else if (auto ptr = scrollArea.scrollbarState.ToPtr<ScrollArea::ScrollbarState_Pressed>())
				currentPointerId = ptr->pointerId;
			else
				DENGINE_IMPL_UNREACHABLE();
			if (currentPointerId == pointerId)
			{
				// Return to normal state, consume event.
				scrollArea.scrollbarState = ScrollArea::ScrollbarState_Normal{};
				scrollArea.scrollbarPos = 0;
				return true;
			}
		}

		SizeHint const childSizeHint = scrollArea.widget->GetSizeHint(ctx);

		Rect const childRect = impl::GetChildRect(
			widgetRect,
			childSizeHint,
			scrollArea.scrollbarPos,
			scrollArea.scrollbarThickness);

		Rect const scrollbarRect = impl::GetScrollbarRect(
			scrollArea.scrollbarThickness,
			scrollArea.scrollbarPos,
			widgetRect,
			childRect.extent.height);

		bool pointerHoversScrollbar = scrollbarRect.PointIsInside(pointerPos) && visibleRect.PointIsInside(pointerPos);
		if (pointerHoversScrollbar)
		{
			if (pointerPressed)
			{
				ScrollArea::ScrollbarState_Pressed newState = {};
				newState.pointerId = pointerId;
				newState.pointerRelativePosY = pointerPos.y - scrollbarRect.position.y;
				scrollArea.scrollbarState = newState;
			}
		}
		else
		{
			scrollArea.widget->CursorPress(
				ctx,
				windowId,
				childRect,
				Rect::Intersection(childRect, visibleRect),
				{ (i32)pointerPos.x, (i32)pointerPos.y },
				event);
		}

		return true;
	}
}

SizeHint ScrollArea::GetSizeHint(
	Context const& ctx) const
{
	SizeHint returnVal = {};
	returnVal.preferred = { 50, 50 };
	returnVal.expandX = true;
	returnVal.expandY = true;

	Gui::SizeHint childSizeHint = widget->GetSizeHint(ctx);
	returnVal.preferred.width = childSizeHint.preferred.width;

	returnVal.preferred.width += scrollbarThickness;

	return returnVal;
}

void ScrollArea::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	if (Rect::Intersection(widgetRect, visibleRect).IsNothing())
		return;

	SizeHint childSizeHint = widget->GetSizeHint(ctx);

	auto scrollbarPos = this->scrollbarPos;
	if (childSizeHint.expandY)
		scrollbarPos = 0;

	Rect childRect = impl::GetChildRect(
		widgetRect,
		childSizeHint,
		scrollbarPos,
		scrollbarThickness);

	Rect childVisibleRect = Rect::Intersection(childRect, visibleRect);

	if (widget && !childVisibleRect.IsNothing())
	{
		auto scopedScissor = DrawInfo::ScopedScissor(drawInfo, childVisibleRect);
		widget->Render(
			ctx,
			framebufferExtent,
			childRect,
			childVisibleRect,
			drawInfo);
	}

	// Draw scrollbar
	Rect scrollBarRect = impl::GetScrollbarRect(
		scrollbarThickness,
		scrollbarPos,
		widgetRect,
		childRect.extent.height);

	Math::Vec4 color = {};
	if (scrollbarState.IsA<ScrollbarState_Normal>())
		color = { 0.5f, 0.5f, 0.5f, 1.f };
	else if (scrollbarState.IsA<ScrollbarState_Hovered>())
		color = { 0.75f, 0.75f, 0.75f, 1.f };
	else if (scrollbarState.IsA<ScrollbarState_Pressed>())
		color = { 1.f, 1.f, 1.f, 1.f };
	else
		DENGINE_IMPL_UNREACHABLE();

	drawInfo.PushFilledQuad(scrollBarRect, color);
}

bool ScrollArea::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event,
	bool occluded)
{
	return impl::PointerMove(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		impl::cursorPointerId,
		{ (f32)event.position.x, (f32)event.position.y },
		occluded,
		event);
}

bool ScrollArea::CursorPress(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	return impl::PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		impl::cursorPointerId,
		impl::ToPointerType(event.button),
		{ (f32)cursorPos.x, (f32)cursorPos.y },
		event.clicked,
		event);
}

void ScrollArea::TouchEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event,
	bool occluded)
{
	/*
	ParentType::TouchEvent(
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		event,
		occluded);

	if (widget)
	{
		SizeHint childSizeHint = widget->GetSizeHint(ctx);

		Rect childRect = impl::GetChildRect(
			widgetRect,
			childSizeHint,
			scrollBarPos,
			scrollBarThickness);

		Rect scrollBarRect = impl::GetScrollbarRect(
			*this,
			widgetRect,
			childRect.extent.height);

		bool cursorIsHovered = scrollBarRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);
		if (cursorIsHovered && event.type == TouchEventType::Down && !scrollBarTouchIndex.HasValue())
		{
			scrollBarState = ScrollBarState::Pressed;
			scrollBarCursorRelativePosY = (i32)event.position.y - scrollBarRect.position.y;
			scrollBarTouchIndex = event.id;
		}

		if (event.type == TouchEventType::Up && scrollBarTouchIndex.HasValue() && scrollBarTouchIndex.Value() == event.id)
		{
			scrollBarState = ScrollBarState::Normal;
			scrollBarTouchIndex = Std::nullOpt;
		}

		if (event.type == TouchEventType::Moved && scrollBarTouchIndex.HasValue() && scrollBarTouchIndex.Value() == event.id)
		{
			if (widgetRect.extent.height < childSizeHint.preferred.height)
			{
				// The scroll-bar is currently pressed so we need to move it with the cursor.
				// Find out where mouse is relative to the scroll bar position
				i32 test = event.position.y - widgetRect.position.y - scrollBarCursorRelativePosY;
				u32 scrollBarHeight = widgetRect.extent.height * widgetRect.extent.height / childSizeHint.preferred.height;
				f32 factor = (f32)test / (widgetRect.extent.height - scrollBarHeight);
				factor = Math::Clamp(factor, 0.f, 1.f);
				scrollBarPos = u32(factor * (childSizeHint.preferred.height - widgetRect.extent.height));
			}
		}

		widget->TouchEvent(
			ctx,
			windowId,
			childRect,
			Rect::Intersection(childRect, visibleRect),
			event,
			occluded);
	}
	*/
}

void ScrollArea::InputConnectionLost()
{
	widget->InputConnectionLost();
}

void ScrollArea::CharEnterEvent(Context& ctx)
{
	widget->CharEnterEvent(ctx);
}

void ScrollArea::CharEvent(Context& ctx, u32 utfValue)
{
	widget->CharEvent(ctx, utfValue);
}

void ScrollArea::CharRemoveEvent(Context& ctx)
{
	widget->CharRemoveEvent(ctx);
}