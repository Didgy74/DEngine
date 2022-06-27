#include <DEngine/Gui/ScrollArea.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Std/Containers/FnRef.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	[[nodiscard]] static constexpr Extent BuildChildRect(
		Extent const& contentArea,
		u32 scrollbarThickness,
		SizeHint const& childSizeHint)
	{
		Extent childRect = {};
		childRect.width = contentArea.width - scrollbarThickness;

		if (childSizeHint.expandY)
		{
			childRect.height = Math::Max(childSizeHint.minimum.height, contentArea.height);
		}
		else
		{
			childRect.height = childSizeHint.minimum.height;
		}
		return childRect;
	}

	[[nodiscard]] static Rect GetContentArea(
		Rect const& widgetRect,
		u32 scrollbarThickness)
	{
		Rect contentArea = {};
		contentArea.position = widgetRect.position;
		contentArea.extent = widgetRect.extent;
		contentArea.extent.width -= scrollbarThickness;
		return contentArea;
	}

	[[nodiscard]] static Rect GetChildRect(
		Rect const& widgetRect,
		SizeHint const& childSizeHint,
		u32 scrollbarPos,
		u32 scrollbarThickness)
	{
		Rect childRect = {};
		childRect.position = widgetRect.position;
		childRect.extent = BuildChildRect(widgetRect.extent, scrollbarThickness, childSizeHint);
		if (!childSizeHint.expandY && (childRect.extent.height > widgetRect.extent.height))
			childRect.position.y -= scrollbarPos;
		return childRect;
	}

	[[nodiscard]] static u32 GetScrollbarHeight(
		u32 contentAreaHeight,
		u32 childHeight)
	{
		f32 factor = (f32)contentAreaHeight / (f32)childHeight;
		factor = Math::Min(factor, 1.f);
		return (u32)Math::Round((f32)contentAreaHeight * factor);
	}

	[[nodiscard]] static Rect GetScrollbarRect(
		u32 thickness,
		u32 position,
		Rect widgetRect,
		u32 childHeight)
	{
		Rect scrollbarRect = {};
		scrollbarRect.extent.width = thickness;
		scrollbarRect.extent.height = GetScrollbarHeight(
			widgetRect.extent.height,
			childHeight);
		scrollbarRect.position.x = (i32)widgetRect.position.x + widgetRect.extent.width - thickness;
		scrollbarRect.position.y = widgetRect.position.y;
		if (widgetRect.extent.height < childHeight)
		{
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
}

namespace DEngine::Gui::impl
{
	struct PointerMoveEvent
	{
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};

	using PointerMove_DispatchFnT = Std::FnRef<bool(Widget&, Rect const&, Rect const&, bool)>;
	struct PointerMove_Params
	{
		ScrollArea& scrollArea;
		RectCollection const& rectCollection;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerMoveEvent const& pointer;
		PointerMove_DispatchFnT const& dispatchFn;
	};

	struct PointerPressEvent
	{
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
	};

	using PointerPress_DispatchFnT = Std::FnRef<bool(Widget&, Rect const&, Rect const&, bool)>;
	struct PointerPress_Params
	{
		ScrollArea& scrollArea;
		RectCollection const& rectCollection;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerPressEvent const& pointer;
		bool eventConsumed;
		PointerPress_DispatchFnT const& dispatchFn;
	};
}

class Gui::impl::SA_Impl
{
public:
	// Returns true if the pointer is occluded.
	static bool PointerMove(PointerMove_Params const& params);

	// Returns true if the event was consumed.
	[[nodiscard]] static bool PointerPress(PointerPress_Params const& params);
};

bool Gui::impl::SA_Impl::PointerMove(PointerMove_Params const& params)
{
	auto& scrollArea = params.scrollArea;
	auto& rectCollection = params.rectCollection;
	auto& widgetRect = params.widgetRect;
	auto& visibleRect = params.visibleRect;
	auto& pointer = params.pointer;

	auto const contentArea = impl::GetContentArea(widgetRect, scrollArea.scrollbarThickness);

	// First we want to see if we hovered the scrollbar.
	u32 correctedScrollbarPos = 0;
	if (scrollArea.child)
	{
		auto& child = *scrollArea.child;
		auto const* childRectPairPtr = rectCollection.GetRect(child);
		DENGINE_IMPL_GUI_ASSERT(childRectPairPtr);
		auto const& childRectPair = *childRectPairPtr;
		auto const& childRect = childRectPair.widgetRect;

		if (scrollArea.scrollbarHoldData.HasValue())
		{
			auto const& holdData = scrollArea.scrollbarHoldData.Value();
			if (pointer.id == holdData.pointerId)
			{
				// The scroll-bar is currently pressed so we need to move it with the pointer.

				u32 scrollBarHeight = GetScrollbarHeight(contentArea.extent.height, childRect.extent.height);

				// Set the pixel offset of the scrolling based on where the pointer is.
				// I honestly don't remember how the math works.
				f32 test = pointer.pos.y - (f32)contentArea.position.y - holdData.pointerRelativePosY;
				f32 factor = test / (f32)(widgetRect.extent.height - scrollBarHeight);
				factor = Math::Clamp(factor, 0.f, 1.f);
				scrollArea.currScrollbarPos = (u32)Math::Round(factor * (f32)(childRect.extent.height - widgetRect.extent.height));
			}
		}
		else
		{
			correctedScrollbarPos = impl::GetCorrectedScrollbarPos(
				contentArea.extent.height,
				childRect.extent.height,
				scrollArea.currScrollbarPos);

			auto const scrollbarRect = impl::GetScrollbarRect(
				scrollArea.scrollbarThickness,
				correctedScrollbarPos,
				widgetRect,
				childRect.extent.height);
			auto const pointerInsideScrollbar = scrollbarRect.PointIsInside(pointer.pos);
			if (!pointer.occluded && pointerInsideScrollbar && pointer.id == impl::cursorPointerId)
			{
				scrollArea.scrollbarHoveredByCursor = true;
			}
			else
			{
				scrollArea.scrollbarHoveredByCursor = false;
			}

			scrollArea.currScrollbarPos = correctedScrollbarPos;
		}
	}
	else
	{
		// We don't have any child, so we can just set everything so the scrollbar is disabled.
		scrollArea.currScrollbarPos = 0;
		scrollArea.scrollbarHoldData = Std::nullOpt;
		scrollArea.scrollbarHoveredByCursor = false;
	}

	auto const pointerInside =
		widgetRect.PointIsInside(pointer.pos) &&
		visibleRect.PointIsInside(pointer.pos);

	// Pass down the event
	if (scrollArea.child)
	{
		auto const pointerInsideScrollbarArea = pointerInside && !contentArea.PointIsInside(pointer.pos);
		auto const pointerOccludedForChild = pointer.occluded || pointerInsideScrollbarArea;

		auto& child = *scrollArea.child;
		auto const* childRectPairPtr = rectCollection.GetRect(child);
		DENGINE_IMPL_GUI_ASSERT(childRectPairPtr);
		auto const& childRectPair = *childRectPairPtr;
		params.dispatchFn(
			child,
			childRectPair.widgetRect,
			contentArea,
			pointerOccludedForChild);
	}

	return pointerInside;
}

bool Gui::impl::SA_Impl::PointerPress(PointerPress_Params const& params)
{
	auto& scrollArea = params.scrollArea;
	auto& rectCollection = params.rectCollection;
	auto& widgetRect = params.widgetRect;
	auto& visibleRect = params.visibleRect;
	auto& pointer = params.pointer;

	auto const pointerInside =
		widgetRect.PointIsInside(pointer.pos) &&
		visibleRect.PointIsInside(pointer.pos);

	bool newConsumed = params.eventConsumed;

	auto const contentArea = impl::GetContentArea(widgetRect, scrollArea.scrollbarThickness);

	// First check if we hit the scrollbar
	u32 correctedScrollbarPos = 0;
	if (scrollArea.child)
	{
		// We only have a scrollbar if we have child.
		auto& child = *scrollArea.child;
		auto const* childRectPairPtr = rectCollection.GetRect(child);
		DENGINE_IMPL_GUI_ASSERT(childRectPairPtr);
		auto const& childRectPair = *childRectPairPtr;
		auto const& childRect = childRectPair.widgetRect;

		correctedScrollbarPos = impl::GetCorrectedScrollbarPos(
			contentArea.extent.height,
			childRect.extent.height,
			scrollArea.currScrollbarPos);

		// First check our behavior depending on whether we are holding the scrollbar already.
		if (scrollArea.scrollbarHoldData.HasValue())
		{
			// We are holding the scrollbar, check if this pointer was the one holding it, and if it has been
			// released. If so, delete HoldData.
			auto const heldData = scrollArea.scrollbarHoldData.Value();
			if (heldData.pointerId == pointer.id && !pointer.pressed)
			{
				scrollArea.scrollbarHoldData = Std::nullOpt;
			}
		}
		else if (pointer.pressed && !newConsumed)
		{
			// We are not holding the scrollbar. If this pointer was pressed and not already consumed, check if we
			// hit the scrollbar.

			auto const scrollbarRect = impl::GetScrollbarRect(
				scrollArea.scrollbarThickness,
				correctedScrollbarPos,
				widgetRect,
				childRect.extent.height);
			auto const pointerInsideScrollbar = scrollbarRect.PointIsInside(pointer.pos);

			if (pointerInsideScrollbar)
			{
				// We hit the scrollbar, start holding it.
				ScrollArea::Scrollbar_Pressed_T newPressedData = {};
				newPressedData.pointerId = pointer.id;
				newPressedData.pointerRelativePosY = pointer.pos.y - (f32)scrollbarRect.position.y;
				scrollArea.scrollbarHoldData = newPressedData;

				newConsumed = true;
			}
		}
	}
	else
	{
		// We have no child, just make sure scrollbar is disabled.
		scrollArea.currScrollbarPos = 0;
		scrollArea.scrollbarHoldData = Std::nullOpt;
		scrollArea.scrollbarHoveredByCursor = false;
	}

	scrollArea.currScrollbarPos = correctedScrollbarPos;


	// Dispatch to child.
	if (scrollArea.child)
	{
		auto const pointerInsideScrollbarArea =
			pointerInside &&
			!contentArea.PointIsInside(pointer.pos);
		newConsumed = newConsumed || pointerInsideScrollbarArea;

		auto& child = *scrollArea.child;
		auto const* childRectPairPtr = rectCollection.GetRect(child);
		DENGINE_IMPL_GUI_ASSERT(childRectPairPtr);
		auto const& childRectPair = *childRectPairPtr;
		params.dispatchFn(
			child,
			childRectPair.widgetRect,
			contentArea,
			newConsumed);
	}

	newConsumed = newConsumed || pointerInside;
	return newConsumed;
}

SizeHint ScrollArea::GetSizeHint2(Widget::GetSizeHint2_Params const& params) const
{
	auto& pusher = params.pusher;

	if (child)
	{
		child->GetSizeHint2(params);
	}

	SizeHint returnVal = {};

	returnVal.minimum.width = 150 + scrollbarThickness;
	returnVal.minimum.height = 150;

	returnVal.expandX = true;
	returnVal.expandY = true;

	auto entry = pusher.AddEntry(*this);
	pusher.SetSizeHint(entry, returnVal);
	return returnVal;
}

void ScrollArea::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& pusher = params.pusher;

	if (child)
	{
		auto const& childWidget = *child;
		auto const childEntry = pusher.GetEntry(childWidget);
		auto const& childSizeHint = pusher.GetSizeHint(childEntry);

		auto const contentArea = impl::GetContentArea(widgetRect, scrollbarThickness);

		auto correctedScrollbarPos = impl::GetCorrectedScrollbarPos(
			contentArea.extent.height,
			childSizeHint.minimum.height,
			currScrollbarPos);

		Rect childRect = impl::GetChildRect(
			widgetRect,
			childSizeHint,
			correctedScrollbarPos,
			scrollbarThickness);

		auto const childVisibleRect = Rect::Intersection(visibleRect, contentArea);

		pusher.SetRectPair(childEntry, { childRect, childVisibleRect });
		childWidget.BuildChildRects(params, childRect, childVisibleRect);
	}
}

void ScrollArea::Render2(
	Widget::Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& rectCollection = params.rectCollection;
	auto& drawInfo = params.drawInfo;


	if (Rect::Intersection(widgetRect, visibleRect).IsNothing())
		return;

	RectPair childRectCombo = {};

	if (child)
	{
		auto const* childRectComboPtr = rectCollection.GetRect(*child);
		DENGINE_IMPL_GUI_ASSERT(childRectComboPtr);
		childRectCombo = *childRectComboPtr;

		if (!childRectCombo.visibleRect.IsNothing())
		{
			auto const scopedScissor = DrawInfo::ScopedScissor(
				drawInfo,
				childRectCombo.visibleRect);

			child->Render2(
				params,
				childRectCombo.widgetRect,
				childRectCombo.visibleRect);
		}
	}

	auto correctedScrollbarPos = currScrollbarPos;
	if (child)
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
	if (scrollbarHoldData.HasValue())
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

void ScrollArea::CursorExit(Context& ctx)
{
	scrollbarHoveredByCursor = false;

	if (child)
	{
		child->CursorExit(ctx);
	}
}

bool ScrollArea::CursorMove(
	Widget::CursorMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	impl::PointerMoveEvent pointerMove = {};
	pointerMove.id = impl::cursorPointerId;
	pointerMove.pos = { (f32)params.event.position.x, (f32)params.event.position.y };
	pointerMove.occluded = occluded;
	auto dispatchFn = [&params](
		Widget& childIn,
		Rect const& childRect,
		Rect const& childVisibleRect,
		bool newOccluded) -> bool
	{
		return childIn.CursorMove(
			params,
			childRect,
			childVisibleRect,
			newOccluded);
	};

	impl::PointerMove_Params temp {
		.scrollArea = *this,
		.rectCollection = params.rectCollection,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointerMove,
		.dispatchFn = dispatchFn, };

	return impl::SA_Impl::PointerMove(temp);
}

bool ScrollArea::CursorPress2(
	Widget::CursorPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	impl::PointerPressEvent pointerPressEvent = {};
	pointerPressEvent.id = impl::cursorPointerId;
	pointerPressEvent.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointerPressEvent.pressed = params.event.pressed;
	pointerPressEvent.type = impl::ToPointerType(params.event.button);

	auto dispatchFn = [&params](
		Widget& childIn,
		Rect const& childRect,
		Rect const& childVisibleRect,
		bool newOccluded) -> bool
	{
		return childIn.CursorPress2(
			params,
			childRect,
			childVisibleRect,
			newOccluded);
	};

	impl::PointerPress_Params temp = {
		.scrollArea = *this,
		.rectCollection = params.rectCollection,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointerPressEvent,
		.eventConsumed = consumed,
		.dispatchFn = dispatchFn, };

	return impl::SA_Impl::PointerPress(temp);
}

bool ScrollArea::TouchMove2(
	TouchMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	impl::PointerMoveEvent pointerMove = {};
	pointerMove.id = params.event.id;
	pointerMove.pos = params.event.position;
	pointerMove.occluded = occluded;
	auto dispatchFn = [&params](
		Widget& childIn,
		Rect const& childRect,
		Rect const& childVisibleRect,
		bool newOccluded) -> bool
	{
		return childIn.TouchMove2(
			params,
			childRect,
			childVisibleRect,
			newOccluded);
	};

	impl::PointerMove_Params temp {
		.scrollArea = *this,
		.rectCollection = params.rectCollection,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointerMove,
		.dispatchFn = dispatchFn, };

	return impl::SA_Impl::PointerMove(temp);
}

bool ScrollArea::TouchPress2(
	TouchPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	impl::PointerPressEvent pointerPressEvent = {};
	pointerPressEvent.id = params.event.id;
	pointerPressEvent.pos = params.event.position;
	pointerPressEvent.pressed = params.event.pressed;
	pointerPressEvent.type = impl::PointerType::Primary;

	auto dispatchFn = [&params](
		Widget& childIn,
		Rect const& childRect,
		Rect const& childVisibleRect,
		bool newOccluded) -> bool
	{
		return childIn.TouchPress2(
			params,
			childRect,
			childVisibleRect,
			newOccluded);
	};

	impl::PointerPress_Params temp = {
		.scrollArea = *this,
		.rectCollection = params.rectCollection,
		.widgetRect = widgetRect,
		.visibleRect = visibleRect,
		.pointer = pointerPressEvent,
		.eventConsumed = consumed,
		.dispatchFn = dispatchFn, };
	return impl::SA_Impl::PointerPress(temp);
}

void ScrollArea::TextInput(
	Context& ctx,
	AllocRef const& transientAlloc,
	TextInputEvent const& event)
{
	if (child)
	{
		child->TextInput(ctx, transientAlloc, event);
	}
}

void ScrollArea::EndTextInputSession(
	Context& ctx,
	AllocRef const& transientAlloc,
	EndTextInputSessionEvent const& event)
{
	if (child)
	{
		child->EndTextInputSession(ctx, transientAlloc, event);
	}
}