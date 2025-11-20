#include <DEngine/Gui/StdWidgets/CollapsingHeader.hpp>


#include <DEngine/Gui/StdWidgets/ButtonUtils.hpp>

#include <DEngine/Std/Containers/FnRef.hpp>

import DEngine.Gui.DrawEngine;
import DEngine.Std.Array;
import DEngine.Std.RangeFnRef;

using namespace DEngine;
using namespace DEngine::Gui;

struct CollapsingHeader::Impl
{
public:
	static constexpr f32 textScale = 1.25f;

	struct CustomData {
		explicit CustomData(RectCollection::AllocRefT alloc) :
			glyphRects{ alloc } {}

		Extent titleTextOuterExtent = {};
		// This holds the height from the SizeHint we got when gathering size-hint for the header.
		u32 headerSizeHintHeight = 0;
		u32 textMarginPx = 0;
		u32 cornerRadiusPx = 0;
		u32 contentShadowHeightPx = 0;
		u32 contentMarginPx = 0;
		FontFaceSizeId fontSizeId = FontFaceSizeId::Invalid;
		// Only included when rendering.
		Std::Vec<Rect, RectCollection::AllocRefT> glyphRects;

		// Only set after rects have been resolved.
		u32 headerHeightPx = 0;
	};

	[[nodiscard]] static Rect BuildHeaderRect(
		Math::Vec2Int widgetPos,
		u32 widgetWidth,
		u32 headerHeightPx) noexcept
	{
		Rect returnVal = {};
		returnVal.position = widgetPos;
		returnVal.extent.width = widgetWidth;
		returnVal.extent.height = headerHeightPx;
		return returnVal;
	}

	[[nodiscard]] static Rect BuildChildRect(
		Rect const& headerRect,
		u32 widgetHeight,
		u32 margin) noexcept
	{
		Rect returnVal = headerRect;
		returnVal.position.x += (i32)margin;
		returnVal.position.y += (i32)returnVal.extent.height + margin;

		i32 temp = (i32)widgetHeight - (i32)headerRect.extent.height - (margin * 2);
		temp = Std::Max(temp, 0);

		returnVal.extent.height = temp;
		returnVal.extent.width = Std::Max((i32)returnVal.extent.width - (i32)(margin * 2), 0);
		return returnVal;
	}

	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static PointerType ToPointerType(CursorButton in) noexcept
	{
		switch (in)
		{
			case CursorButton::Primary: return PointerType::Primary;
			case CursorButton::Secondary: return PointerType::Secondary;
			default: break;
		}
		DENGINE_IMPL_UNREACHABLE();
		return {};
	}

	static constexpr u8 cursorPointerId = (u8)-1;

	struct PointerPress_Pointer {
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
		bool consumed;
	};
	using PointerPress_DispatchFnT = Std::FnRef<TouchEventConsumption(
		Widget&,
		Std::Opt<RectCollection::Iter> const& rectCollIter,
		bool)>;
	struct PointerPress_Params {
		RectCollection const& rectColl;
		CollapsingHeader& widget;
		Std::Opt<RectCollection::Iter> const& rectCollIter;
		PointerPress_Pointer const& pointer;
		PointerPress_DispatchFnT const& dispatchFn;
	};

	[[nodiscard]] static TouchEventConsumption PointerPress(PointerPress_Params const& params) {
		auto const& rectColl = params.rectColl;
		auto const& rectCollIter = params.rectCollIter;
		auto& widget = params.widget;
		auto const& pointer = params.pointer;

		auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
		DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
		auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
		auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

		auto const* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(widget);
		DENGINE_IMPL_GUI_ASSERT(customDataPtr);
		auto const& customData = *customDataPtr;
		auto contentMarginPx = customData.contentMarginPx;

		TouchEventConsumption result = { .consumed = pointer.consumed };

		// Check if we are inside the header.
		auto const headerRect = BuildHeaderRect(
			absWidgetRect.position,
			absWidgetRect.extent.width,
			customData.headerHeightPx);

		auto const insideHeader =
			absVisibleRect.PointIsInside(pointer.pos) &&
			headerRect.PointIsInside(pointer.pos);

		auto const oldCollapsed = widget.collapsed;

		if (widget.headerPointerId.HasValue() && pointer.type == PointerType::Primary) {
			u8 heldPointerId = widget.headerPointerId.Value();
			if (heldPointerId == pointer.id && !pointer.pressed) {
				widget.headerPointerId = Std::nullOpt;
				if (insideHeader) {
					widget.collapsed = !widget.collapsed;
					if (widget.collapseFn) {
						widget.collapseFn(widget);
					}
				}
			}
		}
		else if (!widget.headerPointerId.HasValue() &&
			 insideHeader &&
			 pointer.pressed &&
			 pointer.type == PointerType::Primary &&
	         !result.consumed)
		{
			widget.headerPointerId = pointer.id;
			result.consumed = true;
		}

		// The "collapsed" variable might change depending on the
		// event handling above, so we use the old one.
		if (!oldCollapsed) {
			auto const childRect = BuildChildRect(
				headerRect,
				absWidgetRect.extent.height,
				contentMarginPx);
			auto const insideContent =
				childRect.PointIsInside(pointer.pos) &&
				absVisibleRect.PointIsInside(pointer.pos);

			// We only want to forward the event if it's currently not collapsed.
			if (!widget.collapsed && widget.child) {
				auto childResult = params.dispatchFn(
					*widget.child,
					Std::nullOpt,
					result.consumed);
				result.consumed = result.consumed || childResult.consumed || insideContent;
				result.claimDragPriority = result.claimDragPriority || childResult.claimDragPriority;
			}
		}

		result.consumed = result.consumed || insideHeader;
		return result;
	}

	struct PointerMove_Pointer
	{
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};
	using PointerMove_DispatchFnT = Std::FnRef<void(
		Widget&,
		Std::Opt<RectCollection::Iter> const& rectCollIter,
		bool)>;
	struct PointerMove_Params {
		CollapsingHeader& widget;
		RectCollection const& rectColl;
		Std::Opt<RectCollection::Iter> const& rectCollIter;
		PointerMove_Pointer const& pointer;
		PointerMove_DispatchFnT const& dispatchFn;
	};

	[[nodiscard]] static bool PointerMove(PointerMove_Params const& params)
	{
		auto& widget = params.widget;
		auto& rectColl = params.rectColl;
		auto& rectCollIter = params.rectCollIter;
		auto& pointer = params.pointer;

		auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
		DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
		auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
		auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

		auto* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(widget);
		DENGINE_IMPL_GUI_ASSERT(customDataPtr);
		auto& customData = *customDataPtr;

		auto headerHeightPx = customData.headerHeightPx;

		auto headerRect = Impl::BuildHeaderRect(
			absWidgetRect.position,
			absWidgetRect.extent.width,
			headerHeightPx);

		auto const insideHeader = PointIsInAll(pointer.pos, { absVisibleRect, headerRect });

		if (pointer.id == cursorPointerId) {
			widget.hoveredByCursor = insideHeader && !pointer.occluded;
		}

		// If the move event was consumed before it reached us, and it was the same as the pointer holding our
		// header, then we stop tracking the tap gesture for this finger.
		if (widget.headerPointerId.Has()) {
			auto const headerPointerId = widget.headerPointerId.Get();
			if (pointer.id == headerPointerId && pointer.occluded) {
				widget.headerPointerId = Std::nullOpt;
			}
		}

		if (widget.child && !widget.collapsed) {
			auto const occludedForChild = pointer.occluded || insideHeader;
			params.dispatchFn(
				*widget.child,
				Std::nullOpt,
				occludedForChild);
		}

		auto const pointerOccluded = PointIsInAll(
			pointer.pos,
			{ absWidgetRect, absVisibleRect });
		return pointerOccluded;
	}
};

Widget::GetSizeHint2_ReturnT CollapsingHeader::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	auto const& window = params.window;
	auto const& fontScale = params.FontScale();
	auto const& minimumHeightCm = params.MinimumHeightCm();
	auto& pusher = params.pusher;
	auto& textEngine = params.textEngine;

	auto const& pusherIt = pusher.AddEntry(*this);

	auto& customData = pusher.AttachCustomData(pusherIt, Impl::CustomData{ pusher.Alloc() });
	if (pusher.IncludeRendering()) {
		customData.glyphRects.Resize(title.size());
	}

	CollapsingHeader::Theme theme = {};
	if (this->getThemeFn)
		theme = this->getThemeFn({ .widget = *this, .appData = params.appData });
	auto textMarginPx = theme.textMargin.ResolvePx(window.dpi, window.contentScale);

	auto headerSizeHintResult = ButtonUtils::GetSizeHint({
			.textEngine = textEngine,
			.window = window,
			.textSpan = { title.data(), title.size() },
			.fontScale = fontScale * Impl::textScale,
			.textMarginPx = textMarginPx,
			.minimumHeightCm = minimumHeightCm,
		},
		customData.glyphRects.Data());

	customData.fontSizeId = headerSizeHintResult.fontFaceSizeId;
	customData.headerSizeHintHeight = headerSizeHintResult.sizeHint.minimum.height;
	customData.textMarginPx = textMarginPx;
	customData.titleTextOuterExtent = headerSizeHintResult.textOuterExtent;
	customData.cornerRadiusPx = theme.cornerRadius.ResolvePx(window.dpi, window.contentScale);
	customData.contentShadowHeightPx = theme.contentShadowHeight.ResolvePx(window.dpi, window.contentScale);
	customData.contentMarginPx = theme.contentMargin.ResolvePx(window.dpi, window.contentScale);

	auto returnVal = headerSizeHintResult.sizeHint;

	// Add the child if there is one
	if (!collapsed && child) {
		auto childHint = child->GetSizeHint2(params).sizeHint;
		childHint.minimum.AddPadding(customData.contentMarginPx);

		returnVal.minimum.width = Std::Max(
			returnVal.minimum.width,
			childHint.minimum.width);

		returnVal.minimum.height += childHint.minimum.height;
	}

	pusher.SetSizeHint(pusherIt, returnVal, true);

	return {
		.iter = pusherIt,
		.sizeHint = returnVal };
}

void CollapsingHeader::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& pusher = params.pusher;

	auto* customDataPtr = pusher.GetCustomData<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;
	auto contentMarginPx = customData.contentMarginPx;

	customData.headerHeightPx = Std::Min(customData.headerSizeHintHeight, widgetRect.extent.height);
	auto headerHeightPx = customData.headerHeightPx;

	auto headerRect = Impl::BuildHeaderRect(
		widgetRect.position,
		widgetRect.extent.width,
		headerHeightPx);

	if (!collapsed && child) {
		auto const& childWidget = *child;

		auto const childRect = Impl::BuildChildRect(
			headerRect,
			widgetRect.extent.height,
			contentMarginPx);
		auto childVisibleRect = Rect::Intersection(visibleRect, childRect);

		auto childEntry = pusher.GetEntry(childWidget);
		pusher.SetLocalRectPair(childEntry, { childRect, childVisibleRect });
		childWidget.BuildChildRects(
			params,
			childRect,
			childVisibleRect,
			Std::nullOpt);
	}
}

bool CollapsingHeader::CursorMove(
	CursorMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto cursorPos = params.CursorPos();

	Impl::PointerMove_Pointer pointer = {};
	pointer.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointer.occluded = occluded;
	pointer.id = Impl::cursorPointerId;
	auto dispatch = [&params](
		Widget& childIn,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool occluded)
	{
		childIn.CursorMove(
			params,
			childRectCollIter,
			occluded);
	};

	Impl::PointerMove_Params temp = {
		.widget = *this,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.dispatchFn = dispatch,
	};
	return Impl::PointerMove(temp);
}

bool CollapsingHeader::CursorPress2(
	CursorPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	Impl::PointerPress_Pointer pointer = {
		.id = Impl::cursorPointerId,
		.type = Impl::ToPointerType(params.CursorButton()),
		.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y },
		.pressed = params.CursorPressed(),
		.consumed = consumed, };

	auto dispatch = [&params](
		Widget& childIn,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool occluded)
	{
		return TouchEventConsumption {
			.consumed = childIn.CursorPress2(
				params,
				childRectCollIter,
				occluded)
			};
	};

	Impl::PointerPress_Params tempParams = {
		.rectColl = params.rectCollection,
		.widget = *this,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.dispatchFn = dispatch,
	};
	auto temp = Impl::PointerPress(tempParams);
	return temp.consumed;
}

TouchEventConsumption CollapsingHeader::WidgetEvent_TouchMove(
	WidgetEvent_TouchMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	Impl::PointerMove_Pointer pointer = {};
	pointer.pos = params.event.position;
	pointer.occluded = occluded;
	pointer.id = params.event.id;
	auto dispatch = [&params](
		Widget& childIn,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool occluded)
	{
		childIn.WidgetEvent_TouchMove(
			params,
			childRectCollIter,
			occluded);
	};

	Impl::PointerMove_Params temp = {
		.widget = *this,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.dispatchFn = dispatch,
	};
	// PointerMove returns bool, so convert to TouchEventConsumption
	bool consumed = Impl::PointerMove(temp);
	return { .consumed = consumed };
}

TouchEventConsumption CollapsingHeader::WidgetEvent_TouchPress(
	WidgetEvent_TouchPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	Impl::PointerPress_Pointer pointer = {};
	pointer.pos = params.event.position;
	pointer.consumed = consumed;
	pointer.id = params.event.id;
	pointer.type = Impl::PointerType::Primary;
	pointer.pressed = params.event.pressed;
	auto dispatch = [&params](
		Widget& childIn,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool occluded)
	{
		return childIn.WidgetEvent_TouchPress(
			params,
			childRectCollIter,
			occluded);
	};

	Impl::PointerPress_Params temp = {
		.rectColl = params.rectCollection,
		.widget = *this,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.dispatchFn = dispatch,
	};
	return Impl::PointerPress(temp);
}

void CollapsingHeader::WidgetEvent_TextInput(
	AllocRef const& transientAlloc,
	WidgetEvent_TextInputParams const& event)
{
	if (!collapsed && child) {
		child->WidgetEvent_TextInput(transientAlloc, event);
	}
}

void CollapsingHeader::WidgetEvent_EndTextInputSession(
	AllocRef const& transientAlloc,
	WidgetEvent_EndTextInputSessionParams const& event)
{
	if (!collapsed && child) {
		child->WidgetEvent_EndTextInputSession(transientAlloc, event);
	}
}

void CollapsingHeader::Render2(
	Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& rectColl = params.rectCollection;
	auto& drawInfo = params.drawEngine;
	auto const& window = params.window;

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	if (Rect::Intersection(absWidgetRect, absVisibleRect).IsNothing())
		return;

	auto* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;
	DENGINE_IMPL_GUI_ASSERT(customData.glyphRects.Size() == title.size());
	auto fontSizeId = customData.fontSizeId;
	auto textOuterExtent = customData.titleTextOuterExtent;
	auto marginAmount = customData.textMarginPx;
	auto cornerRadiusPx = customData.cornerRadiusPx;
	auto contentShadowHeightPx = customData.contentShadowHeightPx;
	auto headerHeightPx = customData.headerHeightPx;

	auto const headerRect = Impl::BuildHeaderRect(
		absWidgetRect.position,
		(int)absWidgetRect.extent.width,
		headerHeightPx);

	// Make the mask.
	auto mask = drawInfo.PushRectMaskScoped(absWidgetRect, cornerRadiusPx, DrawEngine::MaskOp::Outside);

	if (!collapsed && child) {
		auto contentBackgroundRect = Rect {
			.position = { absWidgetRect.position.x, headerRect.position.y + (i32)headerRect.extent.height },
			.extent = { absWidgetRect.extent.width, absWidgetRect.extent.height - headerRect.extent.height } };

		if (!contentBackgroundRect.GetIntersect(absVisibleRect).IsNothing()) {
			drawInfo.PushFilledQuad(contentBackgroundRect, this->contentBackgroundColor);
			child->Render2(
				params,
				Std::nullOpt);
			auto dropShadowRect = Rect {
				.position = { absWidgetRect.position.x, headerRect.position.y + (i32)headerRect.extent.height },
				.extent = { absWidgetRect.extent.width, contentShadowHeightPx}, };
			drawInfo.Gradient(
				dropShadowRect,
				{ 0, 0, 0, this->contentShadowAlpha },
				{ 0, 0, 0, 0 },
				0);
		}
	}

	// If the headerRect is completely non-visible, we can skip rendering any of it.
	if (Rect::Intersection(headerRect, absVisibleRect).IsNothing())
		return;

	Math::Vec4 headerBgColor;
	Math::Vec4 textColor = { 1.f, 1.f, 1.f, 1.f };
	if (headerPointerId.HasValue()) {
		headerBgColor = { 1.f, 1.f, 1.f, 1.f };
		textColor = { 0.f, 0.f, 0.f, 1.f };
	}
	else {
		if (collapsed) {
			if (!hoveredByCursor)
				headerBgColor = collapsedColor;
			else
				headerBgColor = { 0.4f, 0.4f, 0.4f, 1.f };
		} else {
			if (!hoveredByCursor)
				headerBgColor = expandedColor;
			else
				headerBgColor = { 0.7f, 0.7f, 0.7f, 1.f };
		}
	}
	auto btnIsFocused = params.focusedWidget.Has() && params.focusedWidget.Get().widget == this;
	if (btnIsFocused) {
		headerBgColor += { 0.1f, 0.1f, 0.1f, 0 };
	}

	drawInfo.PushFilledQuad(
		headerRect,
		headerBgColor);

	auto drawScissor = DrawEngine::ScopedScissor(drawInfo, headerRect);

	auto textRect = Rect{ headerRect.position, textOuterExtent };
	textRect.position.x += (int)marginAmount;
	textRect.position.y += CenterRangeOffset((int)headerRect.extent.height, (int)textOuterExtent.height);

	drawInfo.PushText(
		fontSizeId,
		{ title.data(), title.size() },
		customData.glyphRects.Data(),
		textRect.position,
		textColor);
}

void CollapsingHeader::AccessibilityTest(
	AccessibilityTest_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& accessPusher = params.pusher;
	auto& transientAlloc = params.transientAlloc;
	auto& rectColl = params.rectColl;

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	if (Rect::Intersection(absWidgetRect, absVisibleRect).IsNothing())
		return;

	auto* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;
	auto textMarginPx = customData.textMarginPx;
	auto contentMarginPx = customData.contentMarginPx;
	auto headerHeightPx = customData.headerHeightPx;

	auto const headerRect = Impl::BuildHeaderRect(
		absWidgetRect.position,
		(int)absWidgetRect.extent.width,
		headerHeightPx);

	auto const headerAccessRect = headerRect.GetIntersect(absVisibleRect);
	if (!headerAccessRect.IsNothing()) {
		auto const textOffset = accessPusher.PushText({ title.data(), title.size() });
		AccessibilityInfoElement accessItem = {};
		accessItem.rect = headerAccessRect;
		accessItem.textStart = textOffset;
		accessItem.textCount = title.size();
		accessItem.isClickable = true;
		accessPusher.PushElement(
			reinterpret_cast<uSize>(this),
			accessItem);
	}

	// Draw the child and its background.
	auto childRect = Impl::BuildChildRect(
		headerRect,
		absWidgetRect.extent.height,
		contentMarginPx);
	if (!collapsed && child && !childRect.GetIntersect(absVisibleRect).IsNothing()) {
		child->AccessibilityTest(
			params,
			Std::nullOpt);
	}
}

void CollapsingHeader::WidgetEvent_Navigation(
	Widget_NavigationParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	Widget_NavigationState& event)
{
	auto const& rectColl = params.rectCollection;
	auto currentFocusOpt = params.currentFocusOpt;

	auto entryOpt = rectColl.GetEntry(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(entryOpt.Has());
	auto const& entry = entryOpt.Get();

	auto const& widgetRectPair = rectColl.GetRect(entry);
	auto const& widgetRect = widgetRectPair.widgetRect;

	auto const& customDataPtr = rectColl.GetCustomData<Impl::CustomData>(entry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto const& customData = *customDataPtr;

	auto headerRect = Impl::BuildHeaderRect(
		widgetRect.position,
		widgetRect.extent.width,
		customData.headerHeightPx);

	// If there is no focus widget, or we hit a boundary from outside, focus the header itself
	if (event.State() == Widget_NavigationEventState::NotConsumed) {
		if (!currentFocusOpt.Has()) {
			event.Consume(this);
			return;
		}
		if (currentFocusOpt.Get().widget == this) {
			// If the header button is in focus, we are uncollapsed and we are trying to go direction
			// down, then try navigating into the child.
			auto tryChild =
				!this->collapsed
				&& this->child.Has()
				&& params.IsDirectionQuadrantDown();
			if (tryChild) {
				event.HitBoundary(headerRect);
				this->child->WidgetEvent_Navigation(
					params,
					Std::nullOpt,
					event);
				if (event.IsConsumed()) {
					return;
				}
			}
			event.HitBoundary(headerRect);
			return;
		}

		// There is focus but it is not this Widget. It could be in the subtree.
		if (!this->collapsed && this->child.Has()) {
			this->child->WidgetEvent_Navigation(
				params,
				Std::nullOpt,
				event);
			if (event.IsConsumed()) {
				return;
			}
			// If the child hit a boundary and we are doing direction up, then we should
			// focus the header button.
			if (event.IsHitBoundary() && params.IsDirectionQuadrantUp()) {
				event.Consume(this);
				return;
			}
		}

		return;
	}

	if (event.State() == Widget_NavigationEventState::HitBoundary) {
		DENGINE_IMPL_GUI_ASSERT(params.CurrentFocusWidget() != this);
		if (this->collapsed || !this->child.Has()) {
			event.Consume(this);
			return;
		}

		if (!params.directional) {
			DENGINE_IMPL_GUI_UNREACHABLE();
		}

		// If we are expanded, we need to do hit detection test between child and header button.
		auto childEntryOpt = rectColl.GetEntry(*this->child);
		DENGINE_IMPL_GUI_ASSERT(childEntryOpt.Has());
		auto childEntry = childEntryOpt.Get();

		// If the child is not invokable, we can skip hit detection and just focus our header
		// button.
		if (!rectColl.GetIsInvokable(childEntry)) {
			event.Consume(this);
			return;
		}

		auto childRectPair = rectColl.GetRect(childEntry);

		Std::Array<Rect, 2> rectCandidates = {
			headerRect,
			childRectPair.widgetRect };

		auto hitResultOpt = Gui::FindBestNavItem2(
			rectColl.GetRect(params.currentFocusOpt.Get().iter).widgetRect,
			params.angle,
			Std::RangeFnRef<Std::Opt<Rect>>{ rectCandidates.Size(), [&](int index){
				return rectCandidates[index];
			}});
		if (hitResultOpt.Has()) {
			auto hitResult = hitResultOpt.Get();
			if (hitResult.index == 0) {
				event.Consume(this);
				return;
			}
			this->child->WidgetEvent_Navigation(
				params,
				childEntry,
				event);
		}
	}
}

bool CollapsingHeader::WidgetEvent_Activate(
	WidgetEvent_ActivateParams const&,
	Std::Opt<RectCollection::Iter> const&)
{
	this->collapsed = !this->collapsed;
	if (this->collapseFn) {
		this->collapseFn(*this);
	}
	return true;
}
