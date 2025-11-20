#include <DEngine/Gui/StdWidgets/Button.hpp>

#include <DEngine/Gui/StdWidgets/ButtonUtils.hpp>

import DEngine.Gui.DrawEngine;

using namespace DEngine;
using namespace DEngine::Gui;

class [[maybe_unused]] Button::Impl
{
public:
	// This data is only available when rendering.
	struct [[maybe_unused]] CustomData {
		explicit CustomData(RectCollection::AllocRefT const& alloc) : glyphRects{ alloc } {}

		FontFaceSizeId fontSizeId = FontFaceSizeId::Invalid;
		Extent textOuterExtent = {};
		u32 cornerRadiusPx = 0;
		Std::Vec<Rect, RectCollection::AllocRefT> glyphRects;
	};

	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static PointerType ToPointerType(CursorButton in) noexcept
	{
		switch (in)
		{
			case CursorButton::Primary: return PointerType::Primary;
			case CursorButton::Secondary: return PointerType::Secondary;
			default:
				DENGINE_IMPL_UNREACHABLE();
				return {};
		}
	}

	static constexpr u8 cursorPointerId = (u8)-1;

	struct PointerMove_Pointer {
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};

	struct PointerMove_Params {
		Button& btn;
		RectCollection const& rectColl;
		Std::Opt<RectCollection::Iter> const& rectCollIter;
		PointerMove_Pointer const& pointer;
	};

	struct PointerPress_Pointer {
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
		bool consumed;
	};

	struct PointerPress_Params {
		Button& btn;
		RectCollection const& rectColl;
		Std::Opt<RectCollection::Iter> const& rectCollIter;
		PointerPress_Pointer const& pointer;
		Std::AnyRef customData;
	};

	// Return true if the event is consumed.
	[[nodiscard]] static TouchEventConsumption PointerPress(PointerPress_Params const& params);

	[[nodiscard]] static TouchEventConsumption PointerMove(PointerMove_Params const& params);
};

TouchEventConsumption Gui::Button::Impl::PointerPress(PointerPress_Params const& params)
{
	auto& widget = params.btn;
	auto& rectColl = params.rectColl;
	auto& rectCollIter = params.rectCollIter;
	auto const& pointer = params.pointer;

	auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	bool consumed = pointer.consumed;

	auto const pointerInside =
		absWidgetRect.PointIsInside(pointer.pos) &&
		absVisibleRect.PointIsInside(pointer.pos);

	consumed = consumed || pointerInside;

	if (pointer.type != PointerType::Primary)
		return { .consumed = consumed, .claimDragPriority = false };

	if (widget.heldPointerId.HasValue()) {
		auto const currPointerId = widget.heldPointerId.Value();
		if (params.pointer.id == currPointerId && !params.pointer.pressed && !params.pointer.consumed) {
			widget.heldPointerId = Std::nullOpt;

			if (pointerInside)
				widget.Activate(params.customData);
		}
	} else {
		if (pointerInside && params.pointer.pressed && !params.pointer.consumed)
			widget.heldPointerId = params.pointer.id;
	}

	return { .consumed = consumed, .claimDragPriority = false };
}

TouchEventConsumption Gui::Button::Impl::PointerMove(PointerMove_Params const& params)
{
	auto& widget = params.btn;
	auto& rectColl = params.rectColl;
	auto& rectCollIter = params.rectCollIter;
	auto const& pointer	= params.pointer;

	auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	auto const pointerInside =
		absWidgetRect.PointIsInside(pointer.pos) &&
		absVisibleRect.PointIsInside(pointer.pos);

	if (pointer.id == Impl::cursorPointerId) {
		widget.hoveredByCursor = pointerInside && !params.pointer.occluded;
	}

	// Clear hover state if occluded, even if pointer is still inside
	if (pointer.occluded)
		widget.hoveredByCursor = false;
	auto clearHeldPointer =
		widget.heldPointerId.Has()
		&& pointer.id == widget.heldPointerId.Get()
		&& pointer.occluded;
	if (clearHeldPointer) {
		widget.heldPointerId = Std::nullOpt;
	}
	return { .consumed = pointerInside };
}

Button::Button()
{
	text = "Button";
}

void Button::Activate(Std::AnyRef customData)
{
	if (type == Type::Toggle) {
		m_toggled = !m_toggled;
	}

	if (activateFn)
		activateFn({.btn = *this, .appData = customData });
}

void Button::SetToggled(bool toggled)
{
	this->m_toggled = toggled;
}

bool Button::GetToggled() const
{
	return m_toggled;
}

void Button::CursorExit() {
	hoveredByCursor = false;
}

bool Button::CursorMove(
	CursorMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto cursorPos = params.CursorPos();

	Impl::PointerMove_Pointer pointer = {};
	pointer.id = Impl::cursorPointerId;
	pointer.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointer.occluded = occluded;

	Impl::PointerMove_Params temp = {
		.btn = *this,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.pointer = pointer };

	auto result = Impl::PointerMove(temp);
	return result.consumed;
}

bool Button::CursorPress2(
	Widget::CursorPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	Impl::PointerPress_Pointer pointer = {};
	pointer.id = Impl::cursorPointerId;
	pointer.type = Impl::ToPointerType(params.CursorButton());
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.pressed = params.CursorPressed();
	pointer.consumed = consumed;

	Impl::PointerPress_Params temp {
		.btn = *this,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
        .customData = params.customData };

	auto result = Impl::PointerPress(temp);
	return result.consumed;
}

TouchEventConsumption Button::WidgetEvent_TouchMove(
	WidgetEvent_TouchMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	Impl::PointerMove_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.occluded = occluded;

	Impl::PointerMove_Params temp = {
		.btn = *this,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.pointer = pointer };

	return Impl::PointerMove(temp);
}

TouchEventConsumption Button::WidgetEvent_TouchPress(
	WidgetEvent_TouchPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	Impl::PointerPress_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.type = Impl::PointerType::Primary;
	pointer.pos = params.event.position;
	pointer.pressed = params.event.pressed;
	pointer.consumed = consumed;

	Impl::PointerPress_Params temp = {
		.btn = *this,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.customData = params.customData };

	return Impl::PointerPress(temp);
}

Widget::GetSizeHint2_ReturnT Button::GetSizeHint2(GetSizeHint2_Params const& params) const {
	auto& textMgr = params.textEngine;
	auto const& fontScale = params.FontScale();
	auto const& minimumHeightCm = params.MinimumHeightCm();
	auto& pusher = params.pusher;
	auto const& window = params.window;

	auto pusherIt = pusher.AddEntry(*this);

	auto& customData = pusher.AttachCustomData(pusherIt, Impl::CustomData{ pusher.Alloc() });
	if (pusher.IncludeRendering()) {
		customData.glyphRects.Resize(text.size());
	}

	Theme theme = {};
	if (this->getThemeFn) {
		theme = this->getThemeFn({ .widget = *this, .appData = params.appData });
	}

	auto textMarginPx = theme.margin.ResolvePx(window.dpi, window.contentScale);
	auto cornerRadiusPx = theme.cornerRadius.ResolvePx(window.dpi, window.contentScale);

	auto getSizeHintResult = ButtonUtils::GetSizeHint({
			.textEngine = textMgr,
			.window = window,
			.textSpan = { this->text.data(), this->text.size() },
			.fontScale = fontScale,
			.textMarginPx = textMarginPx,
			.minimumHeightCm = minimumHeightCm,
		},
		customData.glyphRects.Data());

	auto sizeHint = getSizeHintResult.sizeHint;
	customData.fontSizeId = getSizeHintResult.fontFaceSizeId;
	customData.textOuterExtent = getSizeHintResult.textOuterExtent;
	customData.cornerRadiusPx = cornerRadiusPx;

	pusher.SetSizeHint(pusherIt, sizeHint, true);
	return {
		.iter = pusherIt,
		.sizeHint = sizeHint };
}

void Button::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
}

void Button::Render2(
	Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& rectColl = params.rectCollection;
	auto& drawInfo = params.drawEngine;

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	auto const intersection = Rect::Intersection(absWidgetRect, absVisibleRect);
	if (intersection.IsNothing())
		return;

	Math::Vec4 currColor = {};
	Math::Vec4 currTextColor = {};

	if (heldPointerId.HasValue()) {
		// We have a finger pressing on the button atm
		currColor = colors.pressed;
		currTextColor = colors.pressedText;
	}
	else {
		if (type == Type::Toggle) {
			if (m_toggled) {
				currColor = colors.toggled;
				currTextColor = colors.toggledText;
			} else {
				currColor = colors.normal;
				currTextColor = colors.normalText;
			}
		} else if (type == Type::Push) {
			currColor = colors.normal;
			currTextColor = colors.normalText;
		}
		else
			DENGINE_IMPL_UNREACHABLE();

		bool isFocused = params.focusedWidget.Has() && params.focusedWidget.Get().widget == this;
		if (hoveredByCursor || isFocused) {
			currColor += hoverOverlayColor.AsVec4(0.f);
		}
	}

	currColor = currColor.ClampAll(0.f, 1.f);
	currTextColor = currTextColor.ClampAll(0.f, 1.f);

	auto* customDataPtr = rectColl.GetCustomData<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;

	auto cornerRadiusPx = customData.cornerRadiusPx;

	drawInfo.PushFilledQuad(
		absWidgetRect,
		currColor,
		cornerRadiusPx);

	auto centeringOffset = Extent::CenteringOffset(absWidgetRect.extent, customData.textOuterExtent);
	centeringOffset.x = Std::Max(centeringOffset.x, 0);
	centeringOffset.y = Std::Max(centeringOffset.y, 0);

	auto textRect = Rect{ absWidgetRect.position + centeringOffset, customData.textOuterExtent };

	// Used for debugging to display the textRect, to see if the bounding box is correct.
	//drawInfo.PushFilledQuad(textRect, { 1, 0, 0, 0.5f });

	auto scissor = DrawEngine::ScopedScissor(drawInfo, textRect, absWidgetRect);
	drawInfo.PushText(
		customData.fontSizeId,
		{ text.data(), text.size() },
		customData.glyphRects.Data(),
		textRect.position,
		currTextColor);
}

void Button::AccessibilityTest(
	AccessibilityTest_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& rectColl = params.rectColl;
	auto& pusher = params.pusher;

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	AccessibilityInfoElement element = {};
	element.textStart = pusher.PushText({ this->text.data(), this->text.size() });
	element.textCount = (int)this->text.size();
	element.rect = Intersection(absWidgetRect, absVisibleRect);
	element.isClickable = true;
	// TODO: For now we just cast this item to u64
	pusher.PushElement(reinterpret_cast<uSize>(this), element);
}

bool Button::WidgetEvent_Activate(
	WidgetEvent_ActivateParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter)
{
	Activate(params.customData);
	return false;
}
