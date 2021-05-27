#include <DEngine/Gui/ScrollArea.hpp>

#include <DEngine/Std/Trait.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	[[nodiscard]] static constexpr Extent GetChildExtent(
		Extent const& scrollAreaExtent,
		u32 scrollbarThickness,
		SizeHint const& childSizeHint)
	{
		Extent childRect = {};
		if (childSizeHint.expandX)
		{
			childRect.width = scrollAreaExtent.width - scrollbarThickness;
		}
		else
		{
			childRect.width = scrollAreaExtent.width - scrollbarThickness;
		}
		if (childSizeHint.expandY)
		{
			childRect.height = scrollAreaExtent.height;
		}
		else
		{
			childRect.height = Math::Max(childSizeHint.preferred.height, scrollAreaExtent.height);
		}
		return childRect;
	}

	[[nodiscard]] static constexpr Rect GetChildRect(
		Rect const& widgetRect,
		SizeHint const& childSizeHint,
		u32 scrollbarPos,
		u32 scrollbarThickness)
	{
		Rect childRect = {};
		childRect.position = widgetRect.position;
		childRect.extent = GetChildExtent(widgetRect.extent, scrollbarThickness, childSizeHint);
		if (!childSizeHint.expandY && (childRect.extent.height > widgetRect.extent.height))
			childRect.position.y -= scrollbarPos;
		return childRect;
	}

	[[nodiscard]] static constexpr Rect GetScrollbarRect(
		u32 thickness,
		u32 position,
		Rect widgetRect,
		u32 childHeight)
	{
		Rect scrollbarRect = {};
		scrollbarRect.extent.width = thickness;
		scrollbarRect.extent.height = widgetRect.extent.height;
		scrollbarRect.position.x = widgetRect.position.x + widgetRect.extent.width - thickness;
		scrollbarRect.position.y = widgetRect.position.y;
		if (widgetRect.extent.height < childHeight)
		{
			f32 factor = (f32)widgetRect.extent.height / childHeight;
			if (factor < 1.f)
				scrollbarRect.extent.height *= factor;

			f32 test = (f32)position / (childHeight - widgetRect.extent.height);

			scrollbarRect.position.y += test * (widgetRect.extent.height - scrollbarRect.extent.height);
		}
		return scrollbarRect;
	}

	[[nodiscard]] static constexpr u32 GetCorrectedScrollbarPos(
		u32 contentHeight,
		u32 totalChildheight,
		u32 currScrollbarPos) noexcept
	{
		// We can fit the entire child into our content,
		// means no scrollbar offset needed.
		if (contentHeight >= totalChildheight)
			return 0;
		else if (contentHeight + currScrollbarPos >= contentHeight)
		{
			// The offset makes the content be offset longer that it should be.
			// Correct it to be maximum offset possible.
			return Math::Min(currScrollbarPos, totalChildheight - contentHeight);
		}
		else
			return currScrollbarPos;
	}

	static void CorrectState(
		ScrollArea& scrollArea,
		Rect const& widgetRect,
		Extent const& childExtent) noexcept
	{
		// If we don't have a widget, we always want to make sure we're in normal state.
		if (!scrollArea.widget)
		{
			scrollArea.scrollbarState = ScrollArea::ScrollbarState_Normal{};
			scrollArea.currScrollbarPos = 0;
		}
		else
		{
			// We want to correct our scrollbar position based on whatever size the child has now.
			scrollArea.currScrollbarPos = GetCorrectedScrollbarPos(
				widgetRect.extent.height,
				childExtent.height,
				scrollArea.currScrollbarPos);
		}
	}

	static constexpr u8 cursorPointerId = (u8)-1;

	struct PointerMoveEvent
	{
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};

	// T should either be CursorMoveEvent or TouchMoveEvent.
	// 
	// Returns true if the pointer is occluded.
	template<class T>
	static bool PointerMove(
		ScrollArea& scrollArea,
		Context& ctx,
		WindowID windowId,
		Rect const& widgetRect,
		Rect const& visibleRect,
		PointerMoveEvent const& pointer,
		T const& event)
	{
		SizeHint childSizeHint = {};
		if (scrollArea.widget)
			childSizeHint = scrollArea.widget->GetSizeHint(ctx);

		auto childExtent = GetChildExtent(widgetRect.extent, scrollArea.scrollbarThickness, childSizeHint);

		CorrectState(
			scrollArea,
			widgetRect,
			childExtent);

		bool pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);

		// If we don't have a widget, we don't want to do anything.
		if (!scrollArea.widget)
			return pointerInside;

		// If we are hovering and the pointer becomes occluded, go into normal state.
		if (auto ptr = scrollArea.scrollbarState.ToPtr<ScrollArea::ScrollbarState_Hovered>();
			ptr && ptr->pointerId == pointer.id && pointer.occluded && pointerInside)
		{
			scrollArea.scrollbarState = ScrollArea::ScrollbarState_Normal{};
			return pointerInside;
		}

		Rect const childRect = impl::GetChildRect(
			widgetRect,
			childSizeHint,
			scrollArea.currScrollbarPos,
			scrollArea.scrollbarThickness);

		if (auto pressedState = scrollArea.scrollbarState.ToPtr<ScrollArea::ScrollbarState_Pressed>())
		{
			if (pressedState->pointerId == pointer.id && widgetRect.extent.height < childRect.extent.height)
			{
				// The scroll-bar is currently pressed so we need to move it with the pointer.
				
				// Find out where mouse is relative to the scroll bar position
				i32 test = pointer.pos.y - widgetRect.position.y - pressedState->pointerRelativePosY;
				u32 scrollBarHeight = (f32)widgetRect.extent.height * (f32)widgetRect.extent.height / (f32)childRect.extent.height;
				f32 factor = (f32)test / (f32)(widgetRect.extent.height - scrollBarHeight);
				factor = Math::Clamp(factor, 0.f, 1.f);
				scrollArea.currScrollbarPos = u32(factor * (f32)(childRect.extent.height - widgetRect.extent.height));
			}
		}
		else
		{
			Rect scrollBarRect = impl::GetScrollbarRect(
				scrollArea.scrollbarThickness,
				scrollArea.currScrollbarPos,
				widgetRect,
				childRect.extent.height);

			bool pointerOverScrollbar = scrollBarRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);
			if (pointerOverScrollbar && !pointer.occluded)
			{
				ScrollArea::ScrollbarState_Hovered newState = {};
				newState.pointerId = pointer.id;
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
				Rect::Intersection(childRect, visibleRect),
				event,
				pointer.occluded);
		}
		else if constexpr (Std::Trait::isSame<T, TouchMoveEvent>)
		{
			scrollArea.widget->TouchMoveEvent(
				ctx,
				windowId,
				childRect,
				Rect::Intersection(childRect, visibleRect),
				event,
				pointer.occluded);
		}

		return pointerInside;
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
	struct PointerPressEvent
	{
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
	};

	// T should either be CursorPressEvent or TouchPressEvent.
	template<class T>
	[[nodiscard]] static bool PointerPress(
		ScrollArea& scrollArea,
		Context& ctx,
		WindowID windowId,
		Rect const& widgetRect,
		Rect const& visibleRect,
		PointerPressEvent const& pointer,
		T const& event)
	{
		SizeHint childSizeHint = {};
		if (scrollArea.widget)
			childSizeHint = scrollArea.widget->GetSizeHint(ctx);

		auto const childExtent = GetChildExtent(widgetRect.extent, scrollArea.scrollbarThickness, childSizeHint);

		CorrectState(
			scrollArea,
			widgetRect,
			childExtent);

		bool pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);
		if (!pointerInside && pointer.pressed)
			return false;

		if (!scrollArea.widget)
		{
			// If we don't have a widget, we don't want to do anything.
			// Since we're inside the ScrollArea, we just consume the event
			// and return.
			return true;
		}

		auto const scrollbarRect = impl::GetScrollbarRect(
			scrollArea.scrollbarThickness,
			scrollArea.currScrollbarPos,
			widgetRect,
			childExtent.height);

		bool pointerHoversScrollbar = scrollbarRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);

		// If we are unpressed while previously pressing the scrollbar, with the previous pointerID
		// return to normal state.
		if (auto ptr = scrollArea.scrollbarState.ToPtr<ScrollArea::ScrollbarState_Pressed>(); 
			ptr && ptr->pointerId == pointer.id && !pointer.pressed)
		{
			// Return to normal state, if we are the cursor and inside scrollbare
			if (pointer.id == cursorPointerId && pointerHoversScrollbar)
				scrollArea.scrollbarState = ScrollArea::ScrollbarState_Hovered{};
			else
				scrollArea.scrollbarState = ScrollArea::ScrollbarState_Normal{};

			// We don't want to return early when the pointer was unpressed,
			// we want to dispatch it to the widget.
		}

		if (pointerHoversScrollbar && pointer.pressed && !scrollArea.scrollbarState.IsA<ScrollArea::ScrollbarState_Pressed>())
		{
			ScrollArea::ScrollbarState_Pressed newState = {};
			newState.pointerId = pointer.id;
			newState.pointerRelativePosY = pointer.pos.y - scrollbarRect.position.y;
			scrollArea.scrollbarState = newState;
			return true;
		}
		
		bool pointerInsideContent = pointerInside && !pointerHoversScrollbar;

		Rect const childRect = impl::GetChildRect(
			widgetRect,
			childSizeHint,
			scrollArea.currScrollbarPos,
			scrollArea.scrollbarThickness);

		// We want to dispatch the press if
		// The pointer was unpressed
		// OR if the pointer was pressed and was inside the content rect.
		if (!pointer.pressed || (pointer.pressed && pointerInsideContent))
		{
			if constexpr (Std::Trait::isSame<T, CursorClickEvent>)
			{
				scrollArea.widget->CursorPress(
					ctx,
					windowId,
					childRect,
					Rect::Intersection(childRect, visibleRect),
					{ (i32)pointer.pos.x, (i32)pointer.pos.y },
					event);
			}
			else if constexpr (Std::Trait::isSame<T, TouchPressEvent>)
			{
				scrollArea.widget->TouchPressEvent(
					ctx,
					windowId,
					childRect,
					Rect::Intersection(childRect, visibleRect),
					event);
			}
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

	auto childSizeHint = widget->GetSizeHint(ctx);

	auto childExtent = impl::GetChildExtent(widgetRect.extent, scrollbarThickness, childSizeHint);
	
	auto scrollbarPos = impl::GetCorrectedScrollbarPos(
		widgetRect.extent.height,
		childExtent.height,
		this->currScrollbarPos);

	auto childRect = impl::GetChildRect(
		widgetRect,
		childSizeHint,
		scrollbarPos,
		scrollbarThickness);

	auto childVisibleRect = Rect::Intersection(childRect, visibleRect);

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
	auto scrollBarRect = impl::GetScrollbarRect(
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

bool ScrollArea::CursorPress(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	impl::PointerPressEvent pointerPress = {};
	pointerPress.id = impl::cursorPointerId;
	pointerPress.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointerPress.type = impl::ToPointerType(event.button);
	pointerPress.pressed = event.clicked;
	return impl::PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointerPress,
		event);
}

bool ScrollArea::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event,
	bool occluded)
{
	impl::PointerMoveEvent pointerMove = {};
	pointerMove.id = impl::cursorPointerId;
	pointerMove.occluded = occluded;
	pointerMove.pos = { (f32)event.position.x, (f32)event.position.y };
	return impl::PointerMove(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointerMove,
		event);
}

bool ScrollArea::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
	impl::PointerPressEvent pointerPress = {};
	pointerPress.id = event.id;
	pointerPress.pos = event.position;
	pointerPress.type = impl::PointerType::Primary;
	pointerPress.pressed = event.pressed;
	return impl::PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointerPress,
		event);
}

bool ScrollArea::TouchMoveEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchMoveEvent event,
	bool occluded)
{
	impl::PointerMoveEvent pointerMove = {};
	pointerMove.id = event.id;
	pointerMove.occluded = occluded;
	pointerMove.pos = event.position;
	return impl::PointerMove(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointerMove,
		event);
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