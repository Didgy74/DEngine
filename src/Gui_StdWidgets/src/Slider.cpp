#include <DEngine/Gui/StdWidgets/Slider.hpp>

#include <DEngine/Gui/StdWidgets/ButtonUtils.hpp>

#include <iostream>

import DEngine.Gui.DrawEngine;

using namespace DEngine;
using namespace DEngine::Gui;

class [[maybe_unused]] Slider::Impl
{
public:
	// This data is only available when rendering.
	struct [[maybe_unused]] CustomData {
		explicit CustomData(RectCollection::AllocRefT const& alloc) {}
	};

	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static PointerType ToPointerType(CursorButton in) noexcept {
		switch (in) {
			case CursorButton::Primary: return PointerType::Primary;
			case CursorButton::Secondary: return PointerType::Secondary;
			default:
				DENGINE_IMPL_UNREACHABLE();
				return {};
		}
	}

	static constexpr u8 cursorPointerId = (u8)-1;

	// The slider must never render or interact larger than the minimum interactible
	// size. When the Widget rect is taller than that, the slider is clamped to the
	// minimum height and vertically centered within the Widget rect.
	[[nodiscard]] static Rect BuildSliderRect(Rect const& widgetRect, i32 minimumHeightPx) {
		Rect out = widgetRect;
		out.extent.height = Std::Min(widgetRect.extent.height, (u32)minimumHeightPx);
		out.position.y += Extent::CenteringOffsetVert((i32)widgetRect.extent.height, (i32)out.extent.height);
		return out;
	}

	struct PointerMove_Pointer {
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};

	struct PointerMove_Params {
		Slider& widget;
		RectCollection const& rectColl;
		Std::Opt<RectCollection::Iter> const& rectCollIter;
		PointerMove_Pointer const& pointer;
		Std::AnyRef customData;
		i32 minimumHeightPx;
	};

	[[nodiscard]] static TouchEventConsumption PointerMove(PointerMove_Params const& params);

	struct PointerPress_Pointer {
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
		bool consumed;
	};

	struct PointerPress_Params {
		Slider& btn;
		RectCollection const& rectColl;
		Std::Opt<RectCollection::Iter> const& rectCollIter;
		PointerPress_Pointer const& pointer;
		Std::AnyRef customData;
		i32 minimumHeightPx;
	};

	// Return true if the event is consumed.
	[[nodiscard]] static TouchEventConsumption PointerPress(PointerPress_Params const& params);
};

TouchEventConsumption Slider::Impl::PointerPress(PointerPress_Params const& params) {
	auto& widget = params.btn;
	auto& rectColl = params.rectColl;
	auto& rectCollIter = params.rectCollIter;
	auto const& pointer = params.pointer;
	auto const& customData = params.customData;

	auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	TouchEventConsumption returnVal = { .consumed = pointer.consumed };

	auto const pointerInside =
		absWidgetRect.PointIsInside(pointer.pos) &&
		absVisibleRect.PointIsInside(pointer.pos);

	if (!pointerInside && !widget.heldPointerId.Has())
		return {};

	auto const sliderRect = BuildSliderRect(absWidgetRect, params.minimumHeightPx);

	auto circleRadiusPx = (i32)Std::Floor((f32)sliderRect.extent.height * 0.5f);

	if (widget.heldPointerId.Has()) {
		if (widget.heldPointerId.Get() == pointer.id && !pointer.pressed) {
			widget.heldPointerId = Std::nullOpt;
		}
	} else {
		if (pointer.pressed && !pointer.consumed) {
			// Move the slider to the correct position.
			// We need to account for the circle size at the start and end, because we want the
			// circle's center to track the pointer, and the circle's center cannot go entirely to the end.
			f32 relativeX =
				(f32)(pointer.pos.x - sliderRect.position.x - circleRadiusPx)
				/ (f32)(sliderRect.extent.width - 2*circleRadiusPx);
			widget.xPosNorm = Std::Clamp(relativeX, 0.f, 1.f);
			widget.heldPointerId = pointer.id;
			returnVal = { .consumed = true, .claimDragPriority = true };

			if (widget.valueChangedFn) {
				widget.valueChangedFn({ .appData = customData, .newValue = widget.xPosNorm });
			}
		}
	}

	return returnVal;
}

TouchEventConsumption Gui::Slider::Impl::PointerMove(Impl::PointerMove_Params const& params)
{
	auto& widget = params.widget;
	auto& rectColl = params.rectColl;
	auto& rectCollIter = params.rectCollIter;
	auto const& pointer	= params.pointer;
	auto const& customData = params.customData;

	auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;


	TouchEventConsumption returnVal = { .consumed = pointer.occluded };

	auto const sliderRect = BuildSliderRect(absWidgetRect, params.minimumHeightPx);

	auto circleRadiusPx = (u32)Std::Floor((f32)sliderRect.extent.height * 0.5f);

	if (widget.heldPointerId.Has() && widget.heldPointerId.Get() == pointer.id) {
		f32 relativeX = (pointer.pos.x - sliderRect.position.x - circleRadiusPx) / (f32)(sliderRect.extent.width - 2*circleRadiusPx);
		widget.xPosNorm = Std::Clamp(relativeX, 0.f, 1.f);
		returnVal = { .consumed = true, .claimDragPriority = true };

		if (widget.valueChangedFn) {
			widget.valueChangedFn({ .appData = customData, .newValue = widget.xPosNorm });
		}
	}

	auto const pointerInside =
		absWidgetRect.PointIsInside(pointer.pos)
		&& absVisibleRect.PointIsInside(pointer.pos);
	if (pointerInside) {
		returnVal.consumed = true;
		returnVal.claimDragPriority = true;
	}
	return returnVal;
}

Slider::Slider()
{
}

void Slider::CursorExit() {

}

bool Slider::CursorMove(
	CursorMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	Impl::PointerMove_Pointer pointer = {};
	pointer.id = Impl::cursorPointerId;
	pointer.pos = { (f32)params.CursorPos().x, (f32)params.CursorPos().y };
	pointer.occluded = occluded;

	Impl::PointerMove_Params temp = {
		.widget = *this,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.customData = params.customData,
		.minimumHeightPx = CmToPixels(params.MinimumSizeCm(), params.window.dpi), };

	auto result = Impl::PointerMove(temp);
	return result.consumed;
}

bool Slider::CursorPress2(
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
        .customData = params.customData,
		.minimumHeightPx = CmToPixels(params.MinimumSizeCm(), params.window.dpi) };

	auto result = Impl::PointerPress(temp);
	return result.consumed;
}

TouchEventConsumption Slider::WidgetEvent_TouchMove(
	WidgetEvent_TouchMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	Impl::PointerMove_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.occluded = occluded;

	Impl::PointerMove_Params temp = {
		.widget = *this,
		.rectColl = params.rectCollection,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.customData = params.customData,
		.minimumHeightPx = CmToPixels(params.MinimumSizeCm(), params.window.dpi) };

	return Impl::PointerMove(temp);
}

TouchEventConsumption Slider::WidgetEvent_TouchPress(
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
		.customData = params.customData,
		.minimumHeightPx = CmToPixels(params.MinimumSizeCm(), params.window.dpi) };

	return Impl::PointerPress(temp);
}

Widget::GetSizeHint2_ReturnT Slider::GetSizeHint2(GetSizeHint2_Params const& params) const {
	auto& textMgr = params.textEngine;
	auto& pusher = params.pusher;
	auto const& window = params.window;
	auto const& minimumHeightCm = params.MinimumHeightCm();

	auto pusherIt = pusher.AddEntry(*this);
	auto& customData = pusher.AttachCustomData(pusherIt, Impl::CustomData{ pusher.Alloc() });

	SizeHint sizeHint = {};
	sizeHint.expandX = true;
	sizeHint.minimum.width = 200;
	sizeHint.minimum.height = CmToPixels(minimumHeightCm, window.dpi);

	pusher.SetSizeHint(pusherIt, sizeHint, false);
	return {
		.iter = pusherIt,
		.sizeHint = sizeHint };
}

void Slider::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
}

void Slider::Render2(
	Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto const& widget = *this;
	auto const& rectColl = params.rectCollection;
	auto& drawEngine = params.drawEngine;
	auto const& window = params.window;
	auto const& minimumHeightCm = params.MinimumSizeCm();

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	auto const intersection = Rect::Intersection(absWidgetRect, absVisibleRect);
	if (intersection.IsNothing()) {
		return;
	}

	auto minimumHeightPx = CmToPixels(minimumHeightCm, window.dpi);

	// The slider is clamped to the minimum interactible size and vertically centered
	// within the Widget rect, so it never renders larger than necessary.
	auto const sliderRect = Impl::BuildSliderRect(absWidgetRect, minimumHeightPx);

	// Draw thin line that spans the entire widget.
	Rect thinLineRect = sliderRect;
	thinLineRect.extent.width -= minimumHeightPx;
	thinLineRect.extent.height = (u32)Std::Floor((f32)sliderRect.extent.height * 0.3f);

	auto thinLineCornerRadius = (u32)Std::Floor((f32)thinLineRect.extent.height * 0.5f);

	thinLineRect.extent.width += 2*thinLineCornerRadius;

	auto centeringOffset = Extent::CenteringOffset(sliderRect.extent, thinLineRect.extent);

	thinLineRect.position = sliderRect.position + centeringOffset;

	if (!thinLineRect.GetIntersect(absVisibleRect).IsNothing()) {
		Math::Vec4 thinLineColor = { 1.f, 1.f, 1.f, 0.25f };
		drawEngine.PushFilledQuad(thinLineRect, thinLineColor, thinLineCornerRadius);
	}

	Math::Vec4 circleColor = { 1.f, 1.f, 1.f, 0.5f };

	auto circleRadiusPx = (u32)Std::Floor((f32)sliderRect.extent.height * 0.5f);

	Rect circleRect = {};
	circleRect.position = sliderRect.position;
	circleRect.extent.width = sliderRect.extent.height;
	circleRect.extent.height = sliderRect.extent.height;
	circleRect.position.x += (i32)Std::Round(widget.xPosNorm * (f32)(sliderRect.extent.width - 2*circleRadiusPx));

	if (!circleRect.GetIntersect(absVisibleRect).IsNothing()) {
		drawEngine.PushFilledQuad(circleRect, circleColor, circleRadiusPx);
	}
}

void Slider::AccessibilityTest(
	AccessibilityTest_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{

}