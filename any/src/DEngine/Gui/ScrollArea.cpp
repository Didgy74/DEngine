#include <DEngine/Gui/ScrollArea.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

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

			f32 test = (f32)position / (f32)(childHeight - widgetRect.extent.height);

			scrollbarRect.position.y += test * (f32)(widgetRect.extent.height - scrollbarRect.extent.height);
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

	static constexpr u8 cursorPointerId = (u8)-1;

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

	template<class T>
	struct PointerPress_Params
	{
		ScrollArea* scrollArea = nullptr;
		Context* ctx = nullptr;
		WindowID windowId = {};
		Rect const* widgetRect = nullptr;
		Rect const* visibleRect = nullptr;
		PointerPressEvent const* pointer = nullptr;
		T const* event = nullptr;
	};

	struct PointerMoveEvent
	{
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};

	template<class T>
	struct PointerMove_Params
	{
		ScrollArea* scrollArea = nullptr;
		Context* ctx = nullptr;
		WindowID windowId = {};
		Rect const* widgetRect = nullptr;
		Rect const* visibleRect = nullptr;
		PointerMoveEvent const* pointer = nullptr;
		T const* event = nullptr;
	};
}

class Gui::impl::SA_Impl
{
public:
	// T should either be CursorPressEvent or TouchPressEvent.
	//
	// Returns true if the event was consumed.
	template<class T>
	[[nodiscard]] static bool PointerPress(PointerPress_Params<T> const& params)
	{
		auto& scrollArea = *params.scrollArea;
		auto& widgetRect = *params.widgetRect;
		auto& visibleRect = *params.visibleRect;
		auto& pointer = *params.pointer;

		SizeHint childSizeHint = {};
		if (scrollArea.widget)
			childSizeHint = scrollArea.widget->GetSizeHint(*params.ctx);

		auto const childExtent = GetChildExtent(widgetRect.extent, scrollArea.scrollbarThickness, childSizeHint);

		auto eventConsumed = false;

		auto const pointerInside =
			widgetRect.PointIsInside(pointer.pos) &&
			visibleRect.PointIsInside(pointer.pos);

		if (pointerInside && pointer.pressed)
			eventConsumed = true;

		auto const scrollbarRect = impl::GetScrollbarRect(
			scrollArea.scrollbarThickness,
			scrollArea.currScrollbarPos,
			widgetRect,
			childExtent.height);

		auto const pointerInsideScrollbar =
			scrollbarRect.PointIsInside(pointer.pos) &&
			visibleRect.PointIsInside(pointer.pos);

		if (scrollArea.scrollbarHeldData.HasValue())
		{
			auto const heldData = scrollArea.scrollbarHeldData.Value();
			if (heldData.pointerId == pointer.id && !pointer.pressed)
			{
				scrollArea.scrollbarHeldData = Std::nullOpt;
			}
		}
		else
		{
			if (pointerInsideScrollbar && pointer.pressed)
			{
				ScrollArea::Scrollbar_Pressed_T newPressedData = {};
				newPressedData.pointerId = pointer.id;
				newPressedData.pointerRelativePosY = pointer.pos.y - scrollbarRect.position.y;
				scrollArea.scrollbarHeldData = newPressedData;
			}
		}

		Rect const childRect = impl::GetChildRect(
			widgetRect,
			childSizeHint,
			scrollArea.currScrollbarPos,
			scrollArea.scrollbarThickness);

		if (scrollArea.widget)
		{
			auto childConsumed = false;

			if constexpr (Std::Trait::isSame<T, CursorPressEvent>)
			{
				childConsumed = scrollArea.widget->CursorPress(
					*params.ctx,
					params.windowId,
					childRect,
					Rect::Intersection(childRect, visibleRect),
					{ (i32)pointer.pos.x, (i32)pointer.pos.y },
					*params.event);


			}
			else if constexpr (Std::Trait::isSame<T, TouchPressEvent>)
			{
				childConsumed = scrollArea.widget->TouchPressEvent(
					*params.ctx,
					params.windowId,
					childRect,
					Rect::Intersection(childRect, visibleRect),
					*params.event);

			}

			eventConsumed = eventConsumed || childConsumed;
		}

		return eventConsumed;
	}

	// T should either be CursorMoveEvent or TouchMoveEvent.
	//
	// Returns true if the pointer is occluded.
	template<class T>
	static bool PointerMove(PointerMove_Params<T> const& params)
	{
		auto& scrollArea = *params.scrollArea;
		auto& widgetRect = *params.widgetRect;
		auto& visibleRect = *params.visibleRect;
		auto& pointer = *params.pointer;

		SizeHint childSizeHint = {};
		if (scrollArea.widget)
			childSizeHint = scrollArea.widget->GetSizeHint(*params.ctx);

		auto const pointerInside =
			 widgetRect.PointIsInside(pointer.pos) &&
			 visibleRect.PointIsInside(pointer.pos);

		auto const childRect = impl::GetChildRect(
			widgetRect,
			childSizeHint,
			scrollArea.currScrollbarPos,
			scrollArea.scrollbarThickness);

		if (pointer.id == cursorPointerId)
		{
			auto const childExtent = GetChildExtent(widgetRect.extent, scrollArea.scrollbarThickness, childSizeHint);

			auto const scrollbarRect = impl::GetScrollbarRect(
				scrollArea.scrollbarThickness,
				scrollArea.currScrollbarPos,
				widgetRect,
				childExtent.height);

			auto const pointerInsideScrollbar =
				scrollbarRect.PointIsInside(pointer.pos) &&
				visibleRect.PointIsInside(pointer.pos);

			scrollArea.scrollbarHoveredByCursor = pointerInsideScrollbar && !pointer.occluded;
		}

		if (scrollArea.scrollbarHeldData.HasValue())
		{
			auto const& heldData = scrollArea.scrollbarHeldData.Value();
			if (heldData.pointerId == pointer.id && widgetRect.extent.height < childRect.extent.height)
			{
				// The scroll-bar is currently pressed so we need to move it with the pointer.

				// Find out where mouse is relative to the scroll bar position
				i32 test = pointer.pos.y - widgetRect.position.y - heldData.pointerRelativePosY;
				u32 scrollBarHeight = (f32)widgetRect.extent.height * (f32)widgetRect.extent.height / (f32)childRect.extent.height;
				f32 factor = (f32)test / (f32)(widgetRect.extent.height - scrollBarHeight);
				factor = Math::Clamp(factor, 0.f, 1.f);
				scrollArea.currScrollbarPos = u32(factor * (f32)(childRect.extent.height - widgetRect.extent.height));
			}
		}

		// We only want one of either.
		if constexpr (Std::Trait::isSame<T, CursorMoveEvent>)
		{
			scrollArea.widget->CursorMove(
				*params.ctx,
				params.windowId,
				childRect,
				Rect::Intersection(childRect, visibleRect),
				*params.event,
				pointer.occluded);
		}
		else if constexpr (Std::Trait::isSame<T, TouchMoveEvent>)
		{
			scrollArea.widget->TouchMoveEvent(
				*params.ctx,
				params.windowId,
				childRect,
				Rect::Intersection(childRect, visibleRect),
				*params.event,
				pointer.occluded);
		}

		return pointerInside;
	}

};

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

	Math::Vec4 color = scrollbarInactiveColor;
	if (scrollbarHeldData.HasValue())
	{
		color = { 1.f, 1.f, 1.f, 1.f };
	}
	else
	{
		color = scrollbarInactiveColor;
		if (scrollbarHoveredByCursor)
		{
			for (auto i = 0; i < 3; i++)
				color[i] += scrollbarHoverHighlight[i];
		}
	}

	for (auto i = 0; i < 4; i++)
		color[i] = Math::Clamp(color[i], 0.f, 1.f);

	drawInfo.PushFilledQuad(scrollBarRect, color);
}

bool ScrollArea::CursorPress(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorPressEvent event)
{
	impl::PointerPressEvent pointerPress = {};
	pointerPress.id = impl::cursorPointerId;
	pointerPress.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointerPress.type = impl::ToPointerType(event.button);
	pointerPress.pressed = event.pressed;

	impl::PointerPress_Params<decltype(event)> params = {};
	params.scrollArea = this;
	params.ctx = &ctx;
	params.windowId = windowId;
	params.widgetRect = &widgetRect;
	params.visibleRect = &visibleRect;
	params.pointer = &pointerPress;
	params.event = &event;

	return impl::SA_Impl::PointerPress(params);
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

	impl::PointerMove_Params<decltype(event)> params = {};
	params.scrollArea = this;
	params.ctx = &ctx;
	params.windowId = windowId;
	params.widgetRect = &widgetRect;
	params.visibleRect = &visibleRect;
	params.pointer = &pointerMove;
	params.event = &event;

	return impl::SA_Impl::PointerMove(params);
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

	impl::PointerPress_Params<decltype(event)> params = {};
	params.scrollArea = this;
	params.ctx = &ctx;
	params.windowId = windowId;
	params.widgetRect = &widgetRect;
	params.visibleRect = &visibleRect;
	params.pointer = &pointerPress;
	params.event = &event;

	return impl::SA_Impl::PointerPress(params);
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

	impl::PointerMove_Params<decltype(event)> params = {};
	params.scrollArea = this;
	params.ctx = &ctx;
	params.windowId = windowId;
	params.widgetRect = &widgetRect;
	params.visibleRect = &visibleRect;
	params.pointer = &pointerMove;
	params.event = &event;

	return impl::SA_Impl::PointerMove(params);
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

SizeHint ScrollArea::GetSizeHint2(Widget::GetSizeHint2_Params const& params) const
{
	if (widget)
	{
		auto const& child = *widget;
		child.GetSizeHint2(params);
	}

	SizeHint returnVal = {};

	returnVal.preferred.width = 150;
	returnVal.preferred.height = 150;

	returnVal.expandX = true;
	returnVal.expandY = true;

	params.pusher.PushSizeHint(*this, returnVal);

	return returnVal;
}

void ScrollArea::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	if (widget)
	{
		auto const childSizeHint = params.builder.GetSizeHint(*widget);

		auto correctedScrollbarPos = impl::GetCorrectedScrollbarPos(
			widgetRect.extent.height,
			childSizeHint.preferred.height,
			currScrollbarPos);

		auto const childRect = impl::GetChildRect(
			widgetRect,
			childSizeHint,
			correctedScrollbarPos,
			scrollbarThickness);

		auto const childVisibleRect = Rect::Intersection(childRect, visibleRect);

		auto const& child = *widget;

		params.builder.PushRect(child, { childRect, childVisibleRect });
		child.BuildChildRects(params, childRect, childVisibleRect);
	}
}

void ScrollArea::Render2(
	Widget::Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	if (Rect::Intersection(widgetRect, visibleRect).IsNothing())
		return;

	RectCombo childRectCombo = {};

	if (widget)
	{
		auto const& child = *widget;

		childRectCombo = params.rectCollection.GetRect(child);

		if (!childRectCombo.visibleRect.IsNothing())
		{
			auto const scopedScissor = DrawInfo::ScopedScissor(
				params.drawInfo,
				childRectCombo.visibleRect);

			child.Render2(
				params,
				childRectCombo.widgetRect,
				childRectCombo.visibleRect);
		}
	}

	auto correctedScrollbarPos = currScrollbarPos;
	if (widget)
	{
		correctedScrollbarPos = impl::GetCorrectedScrollbarPos(
			widgetRect.extent.height,
			childRectCombo.widgetRect.extent.height,
			currScrollbarPos);
	}

	// Draw scrollbar
	auto const scrollBarRect = impl::GetScrollbarRect(
		scrollbarThickness,
		correctedScrollbarPos,
		widgetRect,
		childRectCombo.widgetRect.extent.height);

	Math::Vec4 color = scrollbarInactiveColor;
	if (scrollbarHeldData.HasValue())
	{
		color = { 1.f, 1.f, 1.f, 1.f };
	}
	else
	{
		color = scrollbarInactiveColor;
		if (scrollbarHoveredByCursor)
		{
			for (auto i = 0; i < 3; i++)
				color[i] += scrollbarHoverHighlight[i];
		}
	}

	for (auto i = 0; i < 4; i++)
		color[i] = Math::Clamp(color[i], 0.f, 1.f);

	params.drawInfo.PushFilledQuad(scrollBarRect, color);
}
