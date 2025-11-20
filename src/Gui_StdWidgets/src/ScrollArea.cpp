#include <DEngine/Gui/StdWidgets/ScrollArea.hpp>

#include <DEngine/Std/Containers/FnRef.hpp>

#include <DEngine/Gui/DebugLog.hpp>

import DEngine.Gui.DrawEngine;
import DEngine.Math.Common;

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	[[nodiscard]] static i32 ResolveScrollbarThickness(
		EventWindowInfo const& window,
		f32 minimumTouchSizeCm)
	{
		return (i32)window.scrollBarWidthPx
			.Or(CmToPixels(minimumTouchSizeCm, window.dpi));
	}

	[[nodiscard]] static constexpr Extent BuildChildExtent(
		Extent const& contentView,
		u32 margin,
		SizeHint const& childSizeHint)
	{
		Extent childRect = {};
		childRect.width = contentView.width - margin * 2;

		if (childSizeHint.expandY)
			childRect.height = Math::Max(childSizeHint.minimum.height, contentView.height);
		else
			childRect.height = childSizeHint.minimum.height;

		childRect.height += margin * 2;

		return childRect;
	}

	[[nodiscard]] static Rect GetContentViewRect(Rect const& widgetRect) {
		Rect contentArea = widgetRect;
		return contentArea;
	}

	[[nodiscard]] static Rect GetChildRect(
		Rect const& scrollAreaRect,
		u32 marginAmount,
		SizeHint const& childSizeHint,
		u32 scrollbarPos)
	{
		Rect childRect = {};
		childRect.position = scrollAreaRect.position + Math::Vec2Int{ (i32)marginAmount, (i32)marginAmount };
		childRect.extent = BuildChildExtent(scrollAreaRect.extent, marginAmount, childSizeHint);
		if (!childSizeHint.expandY && (childRect.extent.height > scrollAreaRect.extent.height))
			childRect.position.y -= (i32)scrollbarPos;
		return childRect;
	}

	[[nodiscard]] static u32 GetScrollbarHeight(u32 contentViewHeight, u32 childHeight) {
		f32 factor = (f32)contentViewHeight / (f32)childHeight;
		factor = Math::Min(factor, 1.f);
		return (u32)Math::Round((f32)contentViewHeight * factor);
	}

	[[nodiscard]] static Rect GetScrollbarRect(
		u32 thickness,
		u32 position,
		u32 scrollbarPadding,
		Rect widgetRect,
		u32 childHeight)
	{
		Rect scrollbarRect = {};
		scrollbarRect.extent.width = thickness;
		scrollbarRect.extent.height = GetScrollbarHeight(widgetRect.extent.height, childHeight);
		scrollbarRect.position.x = widgetRect.position.x + (i32)widgetRect.extent.width - (i32)thickness - (i32)scrollbarPadding;
		scrollbarRect.position.y = widgetRect.position.y;

		if (widgetRect.extent.height < childHeight) {
			f32 test = (f32)position / (f32)(childHeight - widgetRect.extent.height);
			scrollbarRect.position.y += (int)Math::Round(
				test * (f32)(widgetRect.extent.height - scrollbarRect.extent.height));
		}
		return scrollbarRect;
	}

	[[nodiscard]] static constexpr u32 GetCorrectedScrollbarPos(
		u32 contentViewHeight,
		u32 totalChildheight,
		u32 currScrollbarPos) noexcept
	{
		// We can fit the entire child into our content,
		// means no scrollbar offset needed.
		if (contentViewHeight >= totalChildheight)
			return 0;
		if (totalChildheight + currScrollbarPos >= contentViewHeight) {
			// The offset makes the content be offset longer that it should be.
			// Correct it to be maximum offset possible.
			return Math::Min(currScrollbarPos, totalChildheight - contentViewHeight);
		}

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
	struct PointerMoveEvent {
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};

	// TouchEventConsumption is the consumption info from child
	using PointerMove_DispatchFnT = Std::FnRef<TouchEventConsumption(Widget&, Std::Opt<RectCollection::Iter> const&, bool)>;
	struct PointerMove_Params {
		ScrollArea& scrollArea;
		i32 const& scrollbarThickness;
		Distance const& scrollbarPadding;
		RectCollection const& rectCollection;
		Std::Opt<RectCollection::Iter> const& rectCollIter;
		EventWindowInfo const& eventWindowInfo;
		PointerMoveEvent const& pointer;
		PointerMove_DispatchFnT const& dispatchFn;
	};

	struct PointerPressEvent {
		u8 id;
		Math::Vec2 pos;
		bool pressed;
		PointerType type;
	};

	using PointerPress_DispatchFnT = Std::FnRef<TouchEventConsumption(Widget&, Std::Opt<RectCollection::Iter> const&, bool)>;
	struct PointerPress_Params {
		ScrollArea& scrollArea;
		i32 scrollbarThickness;
		Distance const& scrollbarPadding;
		RectCollection const& rectCollection;
		Std::Opt<RectCollection::Iter> const& rectCollIter;
		EventWindowInfo const& eventWindowInfo;
		PointerPressEvent const& pointer;
		bool eventConsumed;
		PointerPress_DispatchFnT const& dispatchFn;
	};
}

class Gui::impl::SA_Impl
{
public:
	// Returns TouchEventConsumption from handling the move event.
	[[nodiscard]] static TouchEventConsumption PointerMove(PointerMove_Params const& params);

	// Returns TouchEventConsumption from handling the press event.
	[[nodiscard]] static TouchEventConsumption PointerPress(PointerPress_Params const& params);

	static void ConditionallyResetState(ScrollArea& scrollArea);
};

void Gui::impl::SA_Impl::ConditionallyResetState(ScrollArea& scrollArea) {
	if (!scrollArea.child.Has()) {
		scrollArea.currScrollbarPos = 0;
		scrollArea.scrollbarHoldData = Std::nullOpt;
		scrollArea.scrollbarHoveredByCursor = false;
		scrollArea.touchScrollHoldData = Std::nullOpt;
	}
}

TouchEventConsumption Gui::impl::SA_Impl::PointerMove(PointerMove_Params const& params) {
	auto& scrollArea = params.scrollArea;
	auto& rectColl = params.rectCollection;
	auto& rectCollIter = params.rectCollIter;
	auto& pointer = params.pointer;
	auto scrollbarThickness = params.scrollbarThickness;
	auto scrollbarPadding = params.scrollbarPadding;
	auto const& windowInfo = params.eventWindowInfo;

	auto rectPairOpt = rectColl.GetRect(scrollArea, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	auto const contentArea = impl::GetContentViewRect(absWidgetRect);

	auto const pointerInside =
		absWidgetRect.PointIsInside(pointer.pos) &&
		absVisibleRect.PointIsInside(pointer.pos);

	auto scrollbarPaddingPx = scrollbarPadding.ResolvePx(windowInfo.dpi, windowInfo.contentScale);

	TouchEventConsumption resultConsumption = { .consumed = pointer.occluded };

	ConditionallyResetState(scrollArea);

	// First make sure we have the right position for the scrollbar.
	u32 correctedScrollbarPos = 0;
	Rect childRect = {};
	if (scrollArea.child) {
		auto childRectPairOpt = rectColl.GetRect(*scrollArea.child);
		DENGINE_IMPL_GUI_ASSERT(childRectPairOpt.Has());
		auto const& childRectPair = childRectPairOpt.Get();
		childRect = childRectPair.widgetRect;
		correctedScrollbarPos = impl::GetCorrectedScrollbarPos(
			contentArea.extent.height,
			childRect.extent.height,
			scrollArea.currScrollbarPos);
	}

	if (scrollArea.child) {
		if (scrollArea.scrollbarHoldData.Has()) {
			auto const& holdData = scrollArea.scrollbarHoldData.Value();
			if (pointer.id == holdData.pointerId) {
				// The scroll-bar is currently pressed so we need to move it with the pointer.
				u32 scrollBarHeight = GetScrollbarHeight(
					contentArea.extent.height,
					childRect.extent.height);

				// Set the pixel offset of the scrolling based on where the pointer is.
				// I honestly don't remember how the math works.
				f32 test = pointer.pos.y - (f32)absWidgetRect.position.y - holdData.pointerRelativePosY;
				f32 factor = test / (f32)(absWidgetRect.extent.height - scrollBarHeight);
				factor = Math::Clamp(factor, 0.f, 1.f);
				correctedScrollbarPos =
					(u32)Math::Round(factor * (f32)(childRect.extent.height - absWidgetRect.extent.height));
				resultConsumption.consumed = true;
			}
		} else {
			correctedScrollbarPos = impl::GetCorrectedScrollbarPos(
				contentArea.extent.height,
				childRect.extent.height,
				scrollArea.currScrollbarPos);

			auto const scrollbarRect = impl::GetScrollbarRect(
				scrollbarThickness,
				correctedScrollbarPos,
				scrollbarPaddingPx,
				absWidgetRect,
				childRect.extent.height);
			auto const pointerInsideScrollbar = scrollbarRect.PointIsInside(pointer.pos);
			if (!pointer.occluded && pointerInsideScrollbar && pointer.id == impl::cursorPointerId) {
				scrollArea.scrollbarHoveredByCursor = true;
				resultConsumption.consumed = true;
			} else {
				scrollArea.scrollbarHoveredByCursor = false;
			}
			scrollArea.currScrollbarPos = correctedScrollbarPos;
		}


		// If the move is already consumed, cancel the gesture tracking
		if (scrollArea.touchScrollHoldData.Has()) {
			auto const holdData = scrollArea.touchScrollHoldData.Get();
			if (holdData.pointerId == pointer.id && resultConsumption.consumed) {
				scrollArea.touchScrollHoldData = Std::nullOpt;
			}
		}

		if (scrollArea.touchScrollHoldData.Has()) {
			auto& holdData = scrollArea.touchScrollHoldData.Get();
			auto const touchScrollSlopPx =
				windowInfo.touchScrollSlopPx.Or(scrollArea.defaultTouchScrollSlopPx);
			// Check if we moved outside the touchScrollSlop range
			auto const pointerPosRelativeToWidgetY = pointer.pos.y - (f32)absWidgetRect.position.y;
			bool const beginTouchScrolling =
				Math::Abs(pointerPosRelativeToWidgetY - holdData.pointerRelativeStartPosY) >= (f32)touchScrollSlopPx
				&& !holdData.widgetRelativePosYAtStart.Has()
				&& pointer.id == holdData.pointerId;

			if (beginTouchScrolling) {
				// Now we have to begin touch scrolling.
				holdData.widgetRelativePosYAtStart = pointer.pos.y - (f32)childRect.position.y;
				resultConsumption.consumed = true;
			} else if (holdData.widgetRelativePosYAtStart.Has() && holdData.pointerId == pointer.id) {

				// Apply scrolling

				auto tempScrollbarPos =
					(f32)absWidgetRect.position.y
					+ holdData.widgetRelativePosYAtStart.Get()
					- pointer.pos.y;
				tempScrollbarPos = Math::Max(
					0.f,
					tempScrollbarPos);

				correctedScrollbarPos = impl::GetCorrectedScrollbarPos(
					contentArea.extent.height,
					childRect.extent.height,
					(u32)Math::Round(tempScrollbarPos));

				scrollArea.currScrollbarPos = correctedScrollbarPos;

				resultConsumption.consumed = true;
			}
		}
	}
	else { // We don't have any child, so we can just set everything so the scrollbar is disabled.
		scrollArea.scrollbarHoldData = Std::nullOpt;
		scrollArea.scrollbarHoveredByCursor = false;
		scrollArea.touchScrollHoldData = Std::nullOpt;
	}

	scrollArea.currScrollbarPos = correctedScrollbarPos;

	// Pass down the event
	if (scrollArea.child) {
		auto const pointerConsumedForChild = resultConsumption.consumed;

		auto& child = *scrollArea.child;
		auto childResult = params.dispatchFn(
			child,
			Std::nullOpt,
			pointerConsumedForChild);

		// Propagate child's consumption info
		resultConsumption.consumed = resultConsumption.consumed || childResult.consumed;
		resultConsumption.claimDragPriority = resultConsumption.claimDragPriority || childResult.claimDragPriority;
	}

	resultConsumption.consumed = resultConsumption.consumed || pointerInside;
	return resultConsumption;
}

TouchEventConsumption Gui::impl::SA_Impl::PointerPress(PointerPress_Params const& params)
{
	auto& scrollArea = params.scrollArea;
	auto& rectColl = params.rectCollection;
	auto& rectCollIter = params.rectCollIter;
	auto& pointer = params.pointer;
	auto const& window = params.eventWindowInfo;
	auto scrollbarThickness = params.scrollbarThickness;
	auto scrollbarPadding = params.scrollbarPadding;

	auto scrollbarPaddingPx = scrollbarPadding.ResolvePx(window.dpi, window.contentScale);

	auto rectPairOpt = rectColl.GetRect(scrollArea, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	auto const pointerInside = absWidgetRect
		.GetIntersect(absVisibleRect)
		.PointIsInside(pointer.pos);

	TouchEventConsumption resultConsumption = { .consumed = params.eventConsumed };

	auto const contentArea = impl::GetContentViewRect(absWidgetRect);

	if (!scrollArea.child.Has()) {
		// We have no child, just make sure scrollbar is disabled.
		scrollArea.currScrollbarPos = 0;
		scrollArea.scrollbarHoldData = Std::nullOpt;
		scrollArea.touchScrollHoldData = Std::nullOpt;
		scrollArea.scrollbarHoveredByCursor = false;
	}

	// First check our behavior depending on whether we are holding the scrollbar already.
	if (scrollArea.scrollbarHoldData.Has()) {
		// We are holding the scrollbar, check if this pointer was the one holding it,
		// and if it has been released. If so, delete HoldData.
		auto const heldData = scrollArea.scrollbarHoldData.Get();
		if (heldData.pointerId == pointer.id && !pointer.pressed && pointer.type == PointerType::Primary) {
			scrollArea.scrollbarHoldData = Std::nullOpt;
		}
		if (!scrollArea.child.Has()) {
			scrollArea.scrollbarHoldData = Std::nullOpt;
		}
	}
	if (scrollArea.touchScrollHoldData.Has()) {
		auto const touchScrollHoldData = scrollArea.touchScrollHoldData.Get();
		bool const stopTrackingTouchScroll =
			!pointer.pressed
			&& touchScrollHoldData.pointerId == pointer.id
			&& pointer.type == impl::PointerType::Primary;
		if (stopTrackingTouchScroll) {
			scrollArea.touchScrollHoldData = Std::nullOpt;
		}
		if (!scrollArea.child.Has()) {
			scrollArea.touchScrollHoldData = Std::nullOpt;
		}
	}

	// First check if we hit the scrollbar
	u32 correctedScrollbarPos = 0;
	if (scrollArea.child.Has()) {
		// We only have a scrollbar if we have child.
		auto& child = *scrollArea.child;
		auto childRectPairOpt = rectColl.GetRect(child);
		DENGINE_IMPL_GUI_ASSERT(childRectPairOpt.Has());
		auto const& childRectPair = childRectPairOpt.Get();
		auto const& childRect = childRectPair.widgetRect;

		correctedScrollbarPos = impl::GetCorrectedScrollbarPos(
			contentArea.extent.height,
			childRect.extent.height,
			scrollArea.currScrollbarPos);

		auto const scrollbarRect = impl::GetScrollbarRect(
			scrollbarThickness,
			correctedScrollbarPos,
			scrollbarPaddingPx,
			absWidgetRect,
			childRect.extent.height);
		auto const pointerInsideScrollbar = scrollbarRect.PointIsInside(pointer.pos);


		bool startHoldingScrollbar =
			!scrollArea.scrollbarHoldData.Has()
			&& !scrollArea.touchScrollHoldData.Has()
			&& pointer.pressed
			&& pointerInsideScrollbar
			&& pointer.type == PointerType::Primary
			&& !resultConsumption.consumed;
		if (startHoldingScrollbar) {
			// We hit the scrollbar, start holding it.
			ScrollArea::Scrollbar_Pressed_T newPressedData = {};
			newPressedData.pointerId = pointer.id;
			newPressedData.pointerRelativePosY = pointer.pos.y - (f32)scrollbarRect.position.y;
			scrollArea.scrollbarHoldData = newPressedData;
			resultConsumption.consumed = true;
		}
	}

	//scrollArea.currScrollbarPos = correctedScrollbarPos;

	// Dispatch to child first
	TouchEventConsumption childResult = {};
	if (scrollArea.child) {
		auto& child = *scrollArea.child;
		childResult = params.dispatchFn(
			child,
			Std::nullOpt,
			resultConsumption.consumed);

		// Propagate child's consumption info
		resultConsumption.consumed = resultConsumption.consumed || childResult.consumed;
		resultConsumption.claimDragPriority = childResult.claimDragPriority;
	}

	// If we haven't consumed this event, and the child didn't claim drag priority,
	// store the position of the pointer so we can track how much it moves, and then eventually start touch-scrolling.
	bool const startTrackingTouchScroll =
		pointer.pressed
		&& scrollArea.child.Has()
		&& pointerInside
		&& !childResult.claimDragPriority  // CRITICAL: Respect child's drag priority claim
		&& pointer.id != impl::cursorPointerId
		&& pointer.type == impl::PointerType::Primary
		&& !scrollArea.touchScrollHoldData.Has()
		&& !scrollArea.scrollbarHoldData.Has();
	if (startTrackingTouchScroll) {
		ScrollArea::TouchScroll_Pressed_T newPressedData = {};
		newPressedData.pointerId = pointer.id;
		newPressedData.pointerRelativeStartPosY = pointer.pos.y - (f32)absWidgetRect.position.y;
		scrollArea.touchScrollHoldData = newPressedData;
	}

	resultConsumption.consumed = resultConsumption.consumed || pointerInside;
	return resultConsumption;
}

Widget::GetSizeHint2_ReturnT ScrollArea::GetSizeHint2(GetSizeHint2_Params const& params) const
{
	auto& pusher = params.pusher;
	auto const& minimumSizeCm = params.MinimumHeightCm();

	SizeHint childSizeHint = {};
	if (child) {
		childSizeHint = child->GetSizeHint2(params).sizeHint;
	}

	auto scrollbarThickness = impl::ResolveScrollbarThickness(params.window, minimumSizeCm);

	SizeHint returnVal = {};
	// TODO: Don't use arbitrary numbers
	returnVal.minimum.width = 150 + scrollbarThickness;
	returnVal.minimum.height = 150;

	returnVal.expandX = true;
	returnVal.expandY = true;

	auto entry = pusher.AddEntry(*this);
	pusher.SetSizeHint(entry, returnVal, false);
	return {
		.iter = entry,
		.sizeHint = returnVal };
}

void ScrollArea::BuildChildRects(
	BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& pusher = params.pusher;

	ScrollArea::Theme theme;
	if (this->getThemeFn)
		theme = this->getThemeFn({ .widget = *this, .appData = params.appData });
	auto childPaddingPx = theme.childPadding.ResolvePx(params.window.dpi, params.window.contentScale);

	if (child) {
		auto const& childWidget = *child;
		auto const childEntry = pusher.GetEntry(childWidget);
		auto const& childSizeHint = pusher.GetSizeHint(childEntry);

		auto const contentArea = impl::GetContentViewRect(widgetRect);

		auto childExtent = impl::BuildChildExtent(contentArea.extent, childPaddingPx, childSizeHint);

		auto correctedScrollbarPos = impl::GetCorrectedScrollbarPos(
			contentArea.extent.height,
			childExtent.height,
			currScrollbarPos);

		auto childRect = impl::GetChildRect(
			widgetRect,
			childPaddingPx,
			childSizeHint,
			correctedScrollbarPos);

		auto const childVisibleRect = Rect::Intersection(visibleRect, contentArea);

		pusher.SetLocalRectPair(childEntry, { childRect, childVisibleRect });
		childWidget.BuildChildRects(
			params,
			childRect,
			childVisibleRect,
			Std::nullOpt);
	}
}

void ScrollArea::CursorExit() {
	scrollbarHoveredByCursor = false;

	if (child) {
		child->CursorExit();
	}
}

bool ScrollArea::CursorMove(
	CursorMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto const& minimumSizeCm = params.MinimumSizeCm();

	ScrollArea::Theme theme;
	if (this->getThemeFn) {
		theme = this->getThemeFn({ .widget = *this, .appData = Std::ConstAnyRef{ params.customData } });
	}

	impl::PointerMoveEvent pointerMove = {};
	pointerMove.id = impl::cursorPointerId;
	pointerMove.pos = { (f32)params.CursorPos().x, (f32)params.CursorPos().y };
	pointerMove.occluded = occluded;
	auto dispatchFn = [&params](
		Widget& childIn,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool newOccluded) -> TouchEventConsumption
	{
		// Cursor events still return bool, so we convert
		bool consumed = childIn.CursorMove(
			params,
			childRectCollIter,
			newOccluded);
		return { .consumed = consumed };
	};

	impl::PointerMove_Params temp {
		.scrollArea = *this,
		.scrollbarThickness = impl::ResolveScrollbarThickness(params.window, minimumSizeCm),
		.scrollbarPadding = theme.scrollbarPadding,
		.rectCollection = params.rectCollection,
		.rectCollIter = rectCollIter,
		.eventWindowInfo = params.window,
		.pointer = pointerMove,
		.dispatchFn = dispatchFn, };

	auto result = impl::SA_Impl::PointerMove(temp);
	return result.consumed;
}

bool ScrollArea::CursorPress2(
	CursorPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	auto const& minimumSizeCm = params.MinimumSizeCm();

	ScrollArea::Theme theme;
	if (this->getThemeFn) {
		theme = this->getThemeFn({ .widget = *this, .appData = Std::ConstAnyRef(params.customData) });
	}

	impl::PointerPressEvent pointerPressEvent = {
		.id = impl::cursorPointerId,
		.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y },
		.pressed = params.CursorPressed(),
		.type = impl::ToPointerType(params.CursorButton()), };

	auto dispatchFn = [&params](
		Widget& childIn,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool newOccluded) -> TouchEventConsumption
	{
		// Cursor events still return bool, so we convert
		bool consumed = childIn.CursorPress2(
			params,
			childRectCollIter,
			newOccluded);
		return { .consumed = consumed };
	};

	impl::PointerPress_Params temp = {
		.scrollArea = *this,
		.scrollbarThickness = impl::ResolveScrollbarThickness(params.window, minimumSizeCm),
		.scrollbarPadding = theme.scrollbarPadding,
		.rectCollection = params.rectCollection,
		.rectCollIter = rectCollIter,
		.eventWindowInfo = params.window,
		.pointer = pointerPressEvent,
		.eventConsumed = consumed,
		.dispatchFn = dispatchFn, };

	auto result = impl::SA_Impl::PointerPress(temp);
	return result.consumed;
}

TouchEventConsumption ScrollArea::WidgetEvent_TouchMove(
	WidgetEvent_TouchMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto const& minimumSizeCm = params.MinimumSizeCm();

	ScrollArea::Theme theme;
	if (this->getThemeFn) {
		theme = this->getThemeFn({ .widget = *this, .appData = Std::ConstAnyRef(params.customData) });
	}

	impl::PointerMoveEvent pointerMove = {};
	pointerMove.id = params.event.id;
	pointerMove.pos = params.event.position;
	pointerMove.occluded = occluded;
	auto dispatchFn = [&params](
		Widget& childIn,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool newOccluded) -> TouchEventConsumption
	{
		return childIn.WidgetEvent_TouchMove(
			params,
			childRectCollIter,
			newOccluded);
	};

	impl::PointerMove_Params temp {
		.scrollArea = *this,
		.scrollbarThickness = impl::ResolveScrollbarThickness(params.window, minimumSizeCm),
		.scrollbarPadding = theme.scrollbarPadding,
		.rectCollection = params.rectCollection,
		.rectCollIter = rectCollIter,
		.eventWindowInfo = params.window,
		.pointer = pointerMove,
		.dispatchFn = dispatchFn, };

	return impl::SA_Impl::PointerMove(temp);
}

TouchEventConsumption ScrollArea::WidgetEvent_TouchPress(
	WidgetEvent_TouchPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	auto const& minimumSizeCm = params.MinimumSizeCm();

	ScrollArea::Theme theme;
	if (this->getThemeFn) {
		theme = this->getThemeFn({ .widget = *this, .appData = Std::ConstAnyRef(params.customData) });
	}

	impl::PointerPressEvent pointerPressEvent = {
		.id = params.event.id,
		.pos = params.event.position,
		.pressed = params.event.pressed,
		.type = impl::PointerType::Primary, };

	auto dispatchFn = [&params](
		Widget& childIn,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool newOccluded) -> TouchEventConsumption
	{
		return childIn.WidgetEvent_TouchPress(
			params,
			childRectCollIter,
			newOccluded);
	};

	impl::PointerPress_Params temp = {
		.scrollArea = *this,
		.scrollbarThickness = impl::ResolveScrollbarThickness(params.window, minimumSizeCm),
		.scrollbarPadding = theme.scrollbarPadding,
		.rectCollection = params.rectCollection,
		.rectCollIter = rectCollIter,
		.eventWindowInfo = params.window,
		.pointer = pointerPressEvent,
		.eventConsumed = consumed,
		.dispatchFn = dispatchFn, };
	return impl::SA_Impl::PointerPress(temp);
}

void ScrollArea::WidgetEvent_TextInput(
	AllocRef const& transientAlloc,
	WidgetEvent_TextInputParams const& event)
{
	if (child) {
		child->WidgetEvent_TextInput(transientAlloc, event);
	}
}

void ScrollArea::WidgetEvent_TextSelection(
	AllocRef const& transientAlloc,
	WidgetEvent_TextSelectionParams const& event)
{
	if (child) {
		child->WidgetEvent_TextSelection(transientAlloc, event);
	}
}

void ScrollArea::WidgetEvent_EndTextInputSession(
	AllocRef const& transientAlloc,
	WidgetEvent_EndTextInputSessionParams const& event)
{
	if (child) {
		child->WidgetEvent_EndTextInputSession(transientAlloc, event);
	}
}

void ScrollArea::Render2(
	Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& rectColl = params.rectCollection;
	auto& drawInfo = params.drawEngine;
	auto const& window = params.window;
	auto const& minimumSizeCm = params.MinimumSizeCm();

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	auto scrollbarThickness = impl::ResolveScrollbarThickness(window, minimumSizeCm);

	ScrollArea::Theme theme;
	if (this->getThemeFn) {
		theme = this->getThemeFn({ .widget = *this, .appData = params.appData });
	}
	auto scrollbarPaddingPx = theme.scrollbarPadding.ResolvePx(window.dpi, window.contentScale);

	if (Rect::Intersection(absWidgetRect, absVisibleRect).IsNothing())
		return;

	RectPair childRectPair = {};

	if (child) {
		auto childIterOpt = rectColl.GetEntry(*child);
		DENGINE_IMPL_GUI_ASSERT(childIterOpt.Has());
		auto const& childIter = childIterOpt.Value();
		childRectPair = rectColl.GetRect(childIter);

		if (!childRectPair.visibleRect.IsNothing()) {
			auto const scopedScissor = DrawEngine::ScopedScissor(
				drawInfo,
				childRectPair.visibleRect);
			child->Render2(
				params,
				childIter);
		}
	}

	auto correctedScrollbarPos = currScrollbarPos;
	if (child.Has()) {
		correctedScrollbarPos = impl::GetCorrectedScrollbarPos(
			absWidgetRect.extent.height,
			childRectPair.widgetRect.extent.height,
			currScrollbarPos);
	}

	// Draw scrollbar
	auto const scrollBarRect = impl::GetScrollbarRect(
		scrollbarThickness,
		correctedScrollbarPos,
		scrollbarPaddingPx,
		absWidgetRect,
		childRectPair.widgetRect.extent.height);

	if (scrollBarRect.GetIntersect(absVisibleRect).IsNothing()) {
		return;
	}

	Math::Vec4 color = scrollbarInactiveColor;
	if (scrollbarHoldData.HasValue()) {
		color = { 1.f, 1.f, 1.f, 1.f };
	} else {
		color = scrollbarInactiveColor;
		if (scrollbarHoveredByCursor) {
			color.w += 0.1f;
		}
	}

	for (auto i = 0; i < 4; i++)
		color[i] = Math::Clamp(color[i], 0.f, 1.f);

	// Calculate radius
	auto minDimension = Math::Min(scrollBarRect.extent.width, scrollBarRect.extent.height);
	auto radius = (int)Math::Floor((f32)minDimension * 0.5f);
	auto shadowFalloffPx = radius;
	auto shadowFalloffAlpha = 0.2f;

	{
		auto mask = drawInfo.PushRectMaskScoped(scrollBarRect, radius, DrawEngine::MaskOp::Inside);
		(void)mask;
		drawInfo.PushRectShadow(
			scrollBarRect,
			radius,
			shadowFalloffPx,
			shadowFalloffAlpha);
	}
	drawInfo.PushFilledQuad(scrollBarRect, color, radius);
}

void ScrollArea::AccessibilityTest(
	AccessibilityTest_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& rectColl = params.rectColl;

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	if (Rect::Intersection(absWidgetRect, absVisibleRect).IsNothing())
		return;

	if (child) {
		auto childIterOpt = rectColl.GetEntry(*child);
		DENGINE_IMPL_GUI_ASSERT(childIterOpt.Has());
		auto const& childIter = childIterOpt.Value();
		auto const& childRectPair = rectColl.GetRect(childIter);
		if (!childRectPair.visibleRect.IsNothing()) {
			child->AccessibilityTest(
				params,
				childIter);
		}
	}
}