#include <DEngine/Editor/Joystick.hpp>

#include <DEngine/Gui/RectCollection.hpp>

import DEngine.Gui.DrawEngine;
import DEngine.Math.Common;

namespace DEngine::Editor::impl
{
	[[nodiscard]] static Math::Vec2 GetNormalizedVec(
		Gui::Rect widgetRect,
		Math::Vec2 pointerPos) noexcept
	{
		// Calculate distance from center
		auto outerRadius = Std::Min(widgetRect.extent.width, widgetRect.extent.height) / 2;

		auto widgetCenter = widgetRect.position + Math::Vec2Int{ (i32)outerRadius, (i32)outerRadius };
		auto relativeVec = pointerPos - Math::Vec2{ (f32)widgetCenter.x, (f32)widgetCenter.y };
		relativeVec *= 1.f / outerRadius;
		if (relativeVec.MagnitudeSqrd() >= 1.f)
			relativeVec.Normalize();

		return relativeVec;
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

	struct PointerPress_Pointer
	{
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
	};

	[[nodiscard]] static Gui::TouchEventConsumption Joystick_PointerPress(
		Joystick& widget,
		Gui::RectCollection const& rectColl,
		Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
		PointerPress_Pointer const& pointer,
		bool eventConsumed)
	{
		if (eventConsumed && pointer.pressed)
			return { .consumed = false };

		if (pointer.pressed && widget.pressedData.HasValue())
			return { .consumed = false };

		if (pointer.type != PointerType::Primary)
			return { .consumed = false };

		if (widget.pressedData.HasValue())
		{
			auto const activeData = widget.pressedData.Value();
			if (activeData.pointerId == pointer.id && !pointer.pressed)
			{
				// Disactivate the thing
				widget.pressedData = Std::nullOpt;
				return { .consumed = false };
			}
		}

		if (!pointer.pressed)
			return { .consumed = false };

		auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
		DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
		auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
		auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

		// Calculate distance from center
		auto outerRadius = Std::Min(absWidgetRect.extent.width, absWidgetRect.extent.height) / 2;
		auto widgetCenter = absWidgetRect.position + Math::Vec2Int{ (i32)outerRadius, (i32)outerRadius };
		auto relativeVec = pointer.pos - Math::Vec2{ (f32)widgetCenter.x, (f32)widgetCenter.y };
		bool pointerIsInside = relativeVec.MagnitudeSqrd() <= (f32)Math::Sqrd(outerRadius);
		if (!pointerIsInside && pointer.pressed)
			return { .consumed = false };

		Joystick::PressedData newData = {};
		newData.pointerId = pointer.id;
		newData.currPos = GetNormalizedVec(absWidgetRect, pointer.pos);

		widget.pressedData = newData;

		return { .consumed = true, .claimDragPriority = true };
	}

	[[nodiscard]] static Gui::TouchEventConsumption Joystick_PointerMove(
		Joystick& widget,
		Gui::RectCollection const& rectColl,
		Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
		u8 pointerId,
		Math::Vec2 pointerPos)
	{
		if (!widget.pressedData.HasValue())
			return { .consumed = false };
		auto& activeData = widget.pressedData.Value();
		if (activeData.pointerId != pointerId)
			return { .consumed = false };

		auto rectPairOpt = rectColl.GetRect(widget, rectCollIter);
		DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
		auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
		auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

		activeData.currPos = GetNormalizedVec(absWidgetRect, pointerPos);

		return { .consumed = false, .claimDragPriority = true };
	}
}

using namespace DEngine;
using namespace DEngine::Editor;

Gui::Widget::GetSizeHint2_ReturnT Joystick::GetSizeHint2(Gui::Widget::GetSizeHint2_Params const& params) const
{
	auto const& window = params.window;
	auto const& minimumSizeCm = params.MinimumHeightCm();

	u32 sizeInPixels = Gui::CmToPixels(minimumSizeCm, window.dpi);

	Gui::SizeHint returnVal = {};
	returnVal.minimum = { 2 * sizeInPixels, 2 * sizeInPixels };

	auto& pusher = params.pusher;

	auto entry = pusher.AddEntry(*this);
	pusher.SetSizeHint(entry, returnVal, false);
	return {
		.iter = entry,
		.sizeHint = returnVal };
}

void Joystick::Render2(
	Render_Params const& params,
	Std::Opt<Gui::RectCollection::Iter> const& rectCollIter) const
{
	auto& rectColl = params.rectCollection;
	auto& drawInfo = params.drawEngine;

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	if (Gui::Rect::Intersection({ absWidgetRect, absVisibleRect }).IsNothing())
		return;

	auto outerDiameter = Math::Min(absWidgetRect.extent.width, absWidgetRect.extent.height);
	auto outerRadius = (int)Math::Floor((f32)outerDiameter * 0.5f);
	auto outerCircleRect = Gui::Rect {
		.position = absWidgetRect.position,
		.extent = { outerDiameter, outerDiameter } };
	if (!Gui::Rect::Intersection({ outerCircleRect, absVisibleRect }).IsNothing()) {
		drawInfo.PushFilledQuad(
			outerCircleRect,
			{ 0.f, 0.f, 0.f, 0.25f },
			outerRadius);
	}

	auto innerDiameter = (int)Math::Round((f32)outerDiameter * 0.5f);
	auto innerRadius = (int)Math::Floor((f32)innerDiameter * 0.5f);
	auto innerPos = absWidgetRect.position;
	innerPos.x += innerRadius;
	innerPos.y += innerRadius;
	auto joystickPos = GetVector() * (f32)outerRadius;
	innerPos.x += (int)Math::Round(joystickPos.x);
	innerPos.y += (int)Math::Round(joystickPos.y);
	auto innerCircleRect = Gui::Rect {
		.position = innerPos,
		.extent = { (u32)innerDiameter, (u32)innerDiameter } };
	if (!Gui::Rect::Intersection({ innerCircleRect, absVisibleRect }).IsNothing()) {
		drawInfo.PushFilledQuad(
			innerCircleRect,
			{ 1.f, 1.f, 1.f, 0.75f },
			innerRadius);
	}
}

bool Joystick::CursorPress2(
	Gui::Widget::CursorPressParams const& params,
	Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {
		.id = impl::cursorPointerId,
		.type = impl::ToPointerType(params.CursorButton()),
		.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y },
		.pressed = params.CursorPressed() };

	auto temp = impl::Joystick_PointerPress(
		*this,
		params.rectCollection,
		rectCollIter,
		pointer,
		consumed);
	return temp.consumed;
}

bool Joystick::CursorMove(
	CursorMoveParams const& params,
	Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto cursorPos = params.CursorPos();
	auto temp = impl::Joystick_PointerMove(
		*this,
		params.rectCollection,
		rectCollIter,
		impl::cursorPointerId,
		{ (f32)cursorPos.x, (f32)cursorPos.y });
	return temp.consumed;
}

Gui::TouchEventConsumption Joystick::WidgetEvent_TouchMove(
	WidgetEvent_TouchMoveParams const& params,
	Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	return impl::Joystick_PointerMove(
		*this,
		params.rectCollection,
		rectCollIter,
		params.event.id,
		params.event.position);
}

Gui::TouchEventConsumption Joystick::WidgetEvent_TouchPress(
	WidgetEvent_TouchPressParams const& params,
	Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {
		.id = params.event.id,
		.type = impl::PointerType::Primary,
		.pos = params.event.position,
		.pressed = params.event.pressed, };

	return impl::Joystick_PointerPress(
		*this,
		params.rectCollection,
		rectCollIter,
		pointer,
		consumed);
}