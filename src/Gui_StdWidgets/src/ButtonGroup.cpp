#include <DEngine/Gui/StdWidgets/ButtonGroup.hpp>


import DEngine.Gui.DrawEngine;
import DEngine.Gui.FontFaceSizeId;
import DEngine.Gui.TextEngine;

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	[[nodiscard]] static auto CountTotalTextGlyphs(ButtonGroup const& btnGroup) {
		u32 temp = 0;
		for (auto const& btn : btnGroup.buttons) {
			temp += (u32)btn.title.size();
		}
		return temp;
	}

	// Returns the widths of each child
	[[nodiscard]] static auto GetButtonWidths(
		Std::Span<u32 const> desiredButtonWidths,
		u32 widgetWidth,
		AllocRef const& transientAlloc)
	{
		auto const count = desiredButtonWidths.Size();

		u32 sumDesiredWidth = 0;
		for (auto const& desiredWidth : desiredButtonWidths)
			sumDesiredWidth += desiredWidth;

		f32 const scaleFactor = (f32)widgetWidth / (f32)sumDesiredWidth;

		auto returnWidths = Std::NewVec<u32>(transientAlloc);
		returnWidths.Resize(count);

		u32 remainingWidth = widgetWidth;
		for (int i = 0; i < count; i += 1) {
			u32 btnWidth;
			// If we're not the last element, use the scale factor instead.
			if (i < count - 1)
				btnWidth = (u32)Std::Round((f32)desiredButtonWidths[i] * scaleFactor);
			else
				btnWidth = remainingWidth;

			returnWidths[i] = btnWidth;
			remainingWidth -= btnWidth;
		}

		return returnWidths;
	}

	[[nodiscard]] static auto GetButtonRects(
		Std::Span<u32 const> desiredButtonWidths,
		Rect const& widgetRect,
		AllocRef const& transientAlloc)
	{
		auto const count = desiredButtonWidths.Size();

		u32 sumDesiredWidth = 0;
		for (auto const& desiredWidth : desiredButtonWidths)
			sumDesiredWidth += desiredWidth;

		f32 const scaleFactor = (f32)widgetRect.extent.width / (f32)sumDesiredWidth;

		auto returnRects = Std::NewVec_Reserve<Rect>(transientAlloc, count);

		u32 remainingWidth = widgetRect.extent.width;
		i32 horiOffset = 0;
		for (int i = 0; i < count; i += 1) {
			u32 btnWidth;
			if (i < count - 1)
				btnWidth = (u32)Std::Round((f32)desiredButtonWidths[i] * scaleFactor);
			else
				btnWidth = remainingWidth;

			Rect btnRect = {};
			btnRect.position = widgetRect.position;
			btnRect.position.x += horiOffset;
			btnRect.extent = { btnWidth, widgetRect.extent.height };
			horiOffset += (i32)btnRect.extent.width;
			remainingWidth -= btnWidth;
			returnRects.PushBack(btnRect);
		}

		return returnRects;
	}

	// Returns the index of the button, if any hit.
	[[nodiscard]] static Std::Opt<uSize> CheckHitButtons(
		Math::Vec2Int widgetPos,
		Std::Span<u32 const> widths,
		u32 height,
		Math::Vec2 pointPos) noexcept
	{
		Std::Opt<uSize> returnVal;
		u32 horiOffset = 0;
		for (u32 i = 0; i < widths.Size(); i += 1)
		{
			auto const& width = widths[i];

			Rect btnRect = {};
			btnRect.position = widgetPos;
			btnRect.position.x += (i32)horiOffset;
			btnRect.extent = { width, height };
			if (btnRect.PointIsInside(pointPos)) {
				returnVal = i;
				break;
			}
			horiOffset += btnRect.extent.width;
		}
		return returnVal;
	}
}

struct Gui::ButtonGroup::Impl
{
	// This data is only available when rendering.
	struct [[maybe_unused]] CustomData
	{
		explicit CustomData(RectCollection::AllocRefT const& alloc) :
			textOuterExtents{ alloc },
			desiredBtnWidths{ alloc },
			glyphRects{ alloc }
		{}

		// Needed for centering the text later.
		// Always available
		Std::Vec<Extent, RectCollection::AllocRefT> textOuterExtents;
		u32 textHeight = 0;
		u32 textMarginPx = 0;
		u32 cornerRadiusPx = 0;
		// We need to know how much the buttons asked for in order to
		// divide the buttons up correctly.
		Std::Vec<u32, RectCollection::AllocRefT> desiredBtnWidths;
		FontFaceSizeId fontSizeId = FontFaceSizeId::Invalid;

		// Should not be filled unless rendering is included.
		Std::Vec<Rect, RectCollection::AllocRefT> glyphRects;
	};

	static constexpr u8 cursorPointerId = (u8)-1;

	enum class PointerType : u8
	{
		Primary,
		Secondary,
	};
	[[nodiscard]] static constexpr PointerType ToPointerType(CursorButton in) noexcept
	{
		switch (in)
		{
			case CursorButton::Primary:
				return PointerType::Primary;
			case CursorButton::Secondary:
				return PointerType::Secondary;
			default:
				break;
		}
		DENGINE_IMPL_UNREACHABLE();
		return {};
	}
	struct PointerPress_Pointer {
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
		bool consumed;
	};

	struct PointerMove_Pointer {
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};

	struct PointerMove_Params {
		ButtonGroup& widget;
		RectCollection const& rectColl;
		Std::Opt<RectCollection::Iter> const& rectCollIter;
		AllocRef const& transientAlloc;
		PointerMove_Pointer const& pointer;
	};
	[[nodiscard]] static TouchEventConsumption PointerMove(
		PointerMove_Params const& params)
	{
		auto& widget = params.widget;
		auto& pointer = params.pointer;
		auto& rectColl = params.rectColl;
		auto const& rectCollIter = params.rectCollIter;
		auto& transientAlloc = params.transientAlloc;

		auto const* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(widget);
		DENGINE_IMPL_GUI_ASSERT(customDataPtr);
		auto const& customData = *customDataPtr;

		auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
		DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
		auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
		auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

		auto const pointerInside = PointIsInAll(pointer.pos, { absWidgetRect, absVisibleRect });

		if (pointer.id == cursorPointerId) {
			if (!pointerInside || pointer.occluded) {
				widget.cursorHoverIndex = Std::nullOpt;
			}
			else {
				auto const buttonWidths = impl::GetButtonWidths(
					customData.desiredBtnWidths.ToSpan(),
					absWidgetRect.extent.width,
					transientAlloc);

				// Assign the hit-test result directly so that the hover state is
				// also cleared when the cursor is inside the widget but not over
				// any button.
				widget.cursorHoverIndex = impl::CheckHitButtons(
					absWidgetRect.position,
					buttonWidths.ToSpan(),
					absWidgetRect.extent.height,
					pointer.pos);
			}
		}

		return { .consumed = pointerInside };
	}

	struct PointerPress_Params {
		ButtonGroup& widget;
		RectCollection const& rectColl;
		Std::Opt<RectCollection::Iter> const& rectCollIter;
		AllocRef const& transientAlloc;
		PointerPress_Pointer const& pointer;
	};
	[[nodiscard]] static TouchEventConsumption PointerPress(
		PointerPress_Params const& params)
	{
		auto& widget = params.widget;
		auto const& rectColl = params.rectColl;
		auto const& rectCollIter = params.rectCollIter;
		auto& transientAlloc = params.transientAlloc;
		auto const& pointer = params.pointer;

		DENGINE_IMPL_GUI_ASSERT(widget.activeIndex < widget.buttons.size());

		auto const* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(widget);
		DENGINE_IMPL_GUI_ASSERT(customDataPtr);
		auto const& customData = *customDataPtr;
		auto const& desiredBtnWidths = customData.desiredBtnWidths;

		auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
		DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
		auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
		auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

		auto const pointerInside = PointIsInAll(pointer.pos, { absWidgetRect, absVisibleRect });

		auto const buttonWidths = impl::GetButtonWidths(
			desiredBtnWidths.ToSpan(),
			absWidgetRect.extent.width,
			transientAlloc);

		auto const hoveredIndexOpt = impl::CheckHitButtons(
			absWidgetRect.position,
			buttonWidths.ToSpan(),
			absWidgetRect.extent.height,
			pointer.pos);

		if (widget.heldPointerData.HasValue())
		{
			auto const heldPointer = widget.heldPointerData.Value();
			if (pointer.id == heldPointer.pointerId && !pointer.pressed)
			{
				widget.heldPointerData = Std::nullOpt;

				if (hoveredIndexOpt.HasValue() && hoveredIndexOpt.Value() == heldPointer.buttonIndex)
				{
					// Change the active index
					widget.activeIndex = hoveredIndexOpt.Value();
					if (widget.activeChangedCallback)
						widget.activeChangedCallback(widget, widget.activeIndex);
				}
			}
		}
		else {
			if (hoveredIndexOpt.HasValue() && pointer.pressed && !pointer.consumed) {
				ButtonGroup::HeldPointerData temp = {};
				temp.buttonIndex = hoveredIndexOpt.Value();
				temp.pointerId = pointer.id;
				widget.heldPointerData = temp;
				return { .consumed = pointerInside };
			}
		}

		return { .consumed = pointerInside };
	}
};

void ButtonGroup::AddButton(std::string const& title) {
	InternalButton newInternal = {};
	newInternal.title = title;
	buttons.push_back(newInternal);
}

u32 ButtonGroup::GetButtonCount() const
{
	return (u32)buttons.size();
}

u32 ButtonGroup::GetActiveButtonIndex() const
{
	return activeIndex;
}

Widget::GetSizeHint2_ReturnT ButtonGroup::GetSizeHint2(
	Widget::GetSizeHint2_Params const& params) const
{
	auto const& window = params.window;
	auto const& fontScale = params.FontScale();
	auto const& minimumHeightCm = params.MinimumHeightCm();
	auto& pusher = params.pusher;
	auto& textMgr = params.textEngine;

	auto const btnCount = buttons.size();

	auto customData = Impl::CustomData{ pusher.Alloc() };

	ButtonGroup::Theme theme;
	if (this->getThemeFn)
		theme = this->getThemeFn({ .widget = *this, .appData = params.appData });
	auto textMarginPx = theme.margin.ResolvePx(window.dpi, window.contentScale);
	customData.textMarginPx = textMarginPx;
	customData.cornerRadiusPx = theme.cornerRadius.ResolvePx(window.dpi, window.contentScale);

	customData.textOuterExtents.Resize(btnCount);
	if (pusher.IncludeRendering()) {
		customData.glyphRects.Resize(impl::CountTotalTextGlyphs(*this));
	}

	auto normalTextScale = fontScale * window.contentScale;
	auto normalFontSizeId =  textMgr.GetFontFaceSizeId(normalTextScale, window.dpi, window.dpi);
	auto normalHeight = textMgr.GetFontFaceSizeMetrics(normalFontSizeId).lineHeight;
	auto totalButtonHeight = normalHeight + 2*textMarginPx;

	customData.fontSizeId = normalFontSizeId;
	customData.textHeight = normalHeight;

	SizeHint returnVal = {};

	// Tracks the offset where we will push each
	// line of glyph rects into
	int glyphRectsOffsetTracker = 0;
	for (int i = 0; i < btnCount; i += 1) {
		auto& btn = buttons[i];
		DENGINE_IMPL_GUI_ASSERT(!btn.title.empty());
		auto const textLength = btn.title.size();

		// Load the outer extent of this button text
		// And store the rects of each glyph
		Std::Span<Rect> glyphRects = {};
		if (pusher.IncludeRendering())
			glyphRects = customData.glyphRects.ToSpan().Subspan(glyphRectsOffsetTracker, textLength);
		auto textOuterExtent = textMgr.GetOuterExtent(
			{ btn.title.data(), textLength },
			normalFontSizeId,
			glyphRects);

		glyphRectsOffsetTracker += (int)textLength;

		customData.textOuterExtents[i] = textOuterExtent;

		returnVal.minimum.width += textOuterExtent.width + 2 * textMarginPx;
	}

	customData.desiredBtnWidths.Resize(btnCount);
	for (int i = 0; i < btnCount; i++) {
		customData.desiredBtnWidths[i] = customData.textOuterExtents[i].width;
	}   returnVal.minimum.height = totalButtonHeight;

	// Clamp the size of the button to minimum size
	returnVal.minimum.width = Std::Max(
		returnVal.minimum.width,
		(u32)btnCount * (u32)CmToPixels(minimumHeightCm, window.dpi));
	returnVal.minimum.height = Std::Max(
		returnVal.minimum.height,
		(u32)CmToPixels(minimumHeightCm, window.dpi));

	auto entry = pusher.PushWithCustom(
		*this,
		returnVal,
		true,
		Std::Move(customData));
	return {
		.iter = entry,
		.sizeHint = returnVal };
}

void ButtonGroup::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& pusher = params.pusher;

	auto entryOpt = pusher.GetEntry(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(entryOpt.Has());
	auto entry = entryOpt.Get();

	auto customDataPtr = pusher.GetCustomData<Impl::CustomData>(entry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto& customData = *pusher.GetCustomData<Impl::CustomData>(entry);
	DENGINE_IMPL_GUI_ASSERT(customData.textOuterExtents.Size() == this->buttons.size());

	auto buttonRectsOut = Std::NewVec_Reserve<RectPair>(
		params.transientAlloc,
		this->buttons.size());
	// Need to add secondary items, one for each button
	i32 xOffset = 0;
	for (int i = 0; i < this->buttons.size(); i++) {
		auto btnWidth = customData.textOuterExtents[i].width;
		auto outRect = RectPair{};
		outRect.widgetRect.position = widgetRect.position;
		outRect.widgetRect.position.x += xOffset;
		outRect.widgetRect.extent = {
			.width = btnWidth,
			.height = widgetRect.extent.height };
		outRect.visibleRect = visibleRect;
		buttonRectsOut.PushBack(outRect);
		xOffset += btnWidth;
	}

	pusher.SetSecondaryItems(
		entry,
		buttonRectsOut.ToSpan());
}

bool ButtonGroup::CursorMove(
	CursorMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto cursorPos = params.CursorPos();
	auto const& rectColl = params.rectCollection;

	Impl::PointerMove_Pointer pointer = {
		.id = Impl::cursorPointerId,
		.pos = { (f32)cursorPos.x, (f32)cursorPos.y },
		.occluded = occluded,
	};

	Impl::PointerMove_Params tempParams {
		.widget = *this,
		.rectColl = rectColl,
		.rectCollIter = rectCollIter,
		.transientAlloc = params.transientAlloc,
		.pointer = pointer, };
	auto temp = Impl::PointerMove(tempParams);
	return temp.consumed;
}

bool ButtonGroup::CursorPress2(
	Widget::CursorPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	auto const& rectColl = params.rectCollection;

	Impl::PointerPress_Pointer pointer = {
		.id = Impl::cursorPointerId,
		.type = Impl::ToPointerType(params.CursorButton()),
		.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y },
		.pressed = params.CursorPressed(),
		.consumed = consumed, };

	Impl::PointerPress_Params tempParams {
		.widget = *this,
		.rectColl = rectColl,
		.rectCollIter = rectCollIter,
		.transientAlloc = params.transientAlloc,
		.pointer = pointer, };
	auto temp = Impl::PointerPress(tempParams);
	return temp.consumed;
}

void ButtonGroup::CursorExit() {
	cursorHoverIndex = Std::nullOpt;
}

TouchEventConsumption ButtonGroup::WidgetEvent_TouchMove(
	WidgetEvent_TouchMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	Impl::PointerMove_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.occluded = occluded;

	Impl::PointerMove_Params tempParams {
		.widget = *this,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.transientAlloc = params.transientAlloc,
		.pointer = pointer, };
	return Impl::PointerMove(tempParams);
}

TouchEventConsumption ButtonGroup::WidgetEvent_TouchPress(
	WidgetEvent_TouchPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	Impl::PointerPress_Pointer pointer = {
		.id = params.event.id,
		.type =	Impl::PointerType::Primary,
		.pos = params.event.position,
		.pressed = params.event.pressed,
		.consumed = consumed, };

	Impl::PointerPress_Params tempParams {
		.widget = *this,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.transientAlloc = params.transientAlloc,
		.pointer = pointer, };
	return Impl::PointerPress(tempParams);
}

void ButtonGroup::Render2(
	Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	DENGINE_IMPL_GUI_ASSERT(activeIndex < buttons.size());

	auto& drawInfo = params.drawEngine;
	auto const& rectColl = params.rectCollection;
	auto& transientAlloc = params.transientAlloc;

	auto const btnCount = buttons.size();

	auto const* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto const& customData = *customDataPtr;
	auto cornerRadiusPx = customData.cornerRadiusPx;

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;

	auto const& desiredBtnWidths = customData.desiredBtnWidths;
	auto const btnRects = impl::GetButtonRects(
		desiredBtnWidths.ToSpan(),
		absWidgetRect,
		transientAlloc);

	int glyphRectIndexOffset = 0;
	for (uSize i = 0; i < btnCount; i += 1) {
		auto const& textOuterExtent = customData.textOuterExtents[i];

		auto const& btnRect = btnRects[i];

		Math::Vec4 color = colors.inactiveColor;

		if (cursorHoverIndex.HasValue() && cursorHoverIndex.Value() == i)
			color = colors.hoveredColor;
		if (heldPointerData.HasValue() && heldPointerData.Value().buttonIndex == i)
			color = colors.hoveredColor;
		if (i == activeIndex)
			color = colors.activeColor;

		bool btnIsFocused = false;
		if (params.focusedWidget.Has()) {
			auto const& focusedItem = params.focusedWidget.Get();
			// It's not legal for the focusedItem to point to this Widget without pointing
			// to a sub-item.
			DENGINE_IMPL_GUI_ASSERT(focusedItem.widget != this || focusedItem.secondaryIndex.Has());
			btnIsFocused =
				focusedItem.widget == this
				&& focusedItem.secondaryIndex.Has()
				&& focusedItem.secondaryIndex.Value() == i;
		}
		if (btnIsFocused)
			color += Math::Vec4{ 0.1f, 0.1f, 0.1f, 0.f };

		Math::Vec4Int radii = {};
		if (i == 0) {
			radii[0] = (i32)cornerRadiusPx;
			radii[1] = (i32)cornerRadiusPx;
		}
		if (i == btnCount - 1) {
			radii[2] = (i32)cornerRadiusPx;
			radii[3] = (i32)cornerRadiusPx;
		}
		drawInfo.PushFilledQuad(btnRect, color, radii);

		// Then draw text
		auto const& btnText = buttons[i].title;
		auto textLen = buttons[i].title.size();
		Std::Span btnGlyphRects = { &customData.glyphRects[glyphRectIndexOffset], textLen };
		auto centeringOffset = Extent::CenteringOffset(btnRect.extent, textOuterExtent);
		centeringOffset.x = Std::Max(centeringOffset.x, 0);
		centeringOffset.y = Std::Max(centeringOffset.y, 0);
		auto textRect = Rect{ btnRect.position + centeringOffset, textOuterExtent };
		drawInfo.PushText(
			customData.fontSizeId,
			{ btnText.data(), btnText.size() },
			btnGlyphRects.Data(),
			textRect.position,
			{ 1.f, 1.f, 1.f, 1.f });

		glyphRectIndexOffset += (int)textLen;
	}
}

void ButtonGroup::AccessibilityTest(
	AccessibilityTest_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	DENGINE_IMPL_GUI_ASSERT(activeIndex < buttons.size());

	if (buttons.empty()) {
		return;
	}

	auto& accessPusher = params.pusher;
	auto const& rectColl = params.rectColl;
	auto const& transientAlloc = params.transientAlloc;

	auto const* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto const& customData = *customDataPtr;

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;

	auto const& desiredBtnWidths = customData.desiredBtnWidths;
	auto const btnRects = impl::GetButtonRects(
		desiredBtnWidths.ToSpan(),
		absWidgetRect,
		transientAlloc);

	auto const& btnRect = btnRects[0];

	auto const& btnText = buttons[0].title;

	auto const textOffset = accessPusher.PushText({ btnText.data(), btnText.size()});
	AccessibilityInfoElement accessItem = {};
	accessItem.rect = btnRect;
	accessItem.textStart = textOffset;
	accessItem.textCount = btnText.size();
	accessPusher.PushElement(
		*reinterpret_cast<uSize const*>(this),
		accessItem);
}

void ButtonGroup::WidgetEvent_Navigation(
	Widget_NavigationParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIterParam,
	Widget_NavigationState& event)
{
	// TODO: Unimplemented.
	DENGINE_IMPL_GUI_ASSERT(!this->buttons.empty());

	auto const& transientAlloc = params.transientAlloc;

	auto directionalNav = params.directional;
	auto const& rectColl = params.rectCollection;

	auto rectCollIterOpt = rectColl.GetEntry(*this, rectCollIterParam);
	DENGINE_IMPL_GUI_ASSERT(rectCollIterOpt.Has());
	auto entry = rectCollIterOpt.Get();

	auto const* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(entry);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr != nullptr);
	auto const& customData = *customDataPtr;

	auto widgetRect = rectColl.GetRect(entry).widgetRect;

	if (event.State() == Widget_NavigationEventState::NotConsumed && !params.currentFocusOpt.Has()) {
		if (this->buttons.empty()) {
			return;
		}

		if (!directionalNav) {
			// TODO: Unimplemented
			DENGINE_IMPL_GUI_UNREACHABLE();
		}

		// TODO: Unimplemented

		// If we are coming from top or bottom quadrant, choose our currently selected item
		if (DirAngleIsQuadrantDown(params.angle) || DirAngleIsQuadrantUp(params.angle)) {
			event.Consume(this, this->activeIndex);
			return;
		}
		if (DirAngleIsQuadrantRight(params.angle)) {
			event.Consume(this, 0);
			return;
		}
		event.Consume(this, this->buttons.size() - 1);
		return;
	}

	// If focus is somewhere inside this Widget, we want to try to either consume it or hit a boundary.
	bool widgetIsFocused =
		event.State() == Widget_NavigationEventState::NotConsumed
		&& params.currentFocusOpt.Has()
		&& params.currentFocusOpt.Get().widget == this;
	if (widgetIsFocused) {
		auto const& currentFocusItem = params.currentFocusOpt.Get();
		DENGINE_IMPL_GUI_ASSERT(
			currentFocusItem.secondaryIndex.Has() &&
			currentFocusItem.secondaryIndex.Get() <= this->buttons.size());

		auto focusedIndex = currentFocusItem.secondaryIndex.Get();

		Rect btnRect = {};
		btnRect.position = widgetRect.position;
		i32 horiOffset = 0;
		for (int i = 0; i < focusedIndex; i += 1) {
			horiOffset += (i32)customData.textOuterExtents[i].width;
		}
		btnRect.position.x += horiOffset;
		btnRect.extent = {
			customData.textOuterExtents[focusedIndex].width,
			widgetRect.extent.height };

		if (directionalNav) {
			if (Gui::DirAngleIsQuadrantLeft(params.angle)) {
				if (focusedIndex > 0) {
					event.Consume(this, focusedIndex - 1);
					return;
				}
				event.HitBoundary(btnRect);
				return;
			}
			if (Gui::DirAngleIsQuadrantRight(params.angle)) {
				if (focusedIndex < this->buttons.size() - 1) {
					event.Consume(this, focusedIndex + 1);
					return;
				}
				event.HitBoundary(btnRect);
				return;
			}
			if (Gui::DirAngleIsQuadrantUp(params.angle) || Gui::DirAngleIsQuadrantDown(params.angle)) {
				event.HitBoundary(btnRect);
				return;
			}
		} else {
			// TODO: Non-directional navigation is unimplemented
			DENGINE_IMPL_GUI_UNREACHABLE();
		}
	}

	if (event.State() == Widget_NavigationEventState::HitBoundary) {
		DENGINE_IMPL_GUI_ASSERT(params.currentFocusOpt.Has());

		auto focusRect = rectColl.GetRect(params.currentFocusOpt.Get().iter);

		auto count = rectColl.GetSecondaryCount(entry);
		DENGINE_IMPL_GUI_ASSERT(count == this->buttons.size());

		auto const& desiredBtnWidths = customData.desiredBtnWidths;

		auto btnRects = impl::GetButtonRects(
			desiredBtnWidths.ToSpan(),
			widgetRect,
			transientAlloc);

		auto newFocusChildOpt = Gui::FindBestNavItem2(
			focusRect.widgetRect,
			params.angle,
			{ (int)count, [&](int i) { return btnRects[i]; } });
		if (newFocusChildOpt.Has()) {
			auto secondaryIndex = (uSize)newFocusChildOpt.Get().index;
			event.Consume(this, secondaryIndex);
		}
	}
}

bool ButtonGroup::WidgetEvent_Activate(
	WidgetEvent_ActivateParams const& params,
	Std::Opt<RectCollection::Iter> const&)
{
	// It's illegal for ButtonGroup to be in focus with no secondary index.
	DENGINE_IMPL_ASSERT(params.focusSecondaryIndex.Has());

	auto focusedIndex = params.focusSecondaryIndex.Get();
	DENGINE_IMPL_ASSERT(focusedIndex < this->buttons.size());

	if (focusedIndex != this->activeIndex) {
		this->activeIndex = focusedIndex;
		if (this->activeChangedCallback) {
			this->activeChangedCallback(*this, focusedIndex);
		}
		return true;
	}

	return false;
}
