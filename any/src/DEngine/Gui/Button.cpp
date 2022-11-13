#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Std/Containers/Vec.hpp>

#include <DEngine/Math/Common.hpp>

namespace DEngine::Gui::impl
{
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

	struct PointerMove_Pointer
	{
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};

	struct PointerMove_Params
	{
		Button& btn;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerMove_Pointer const& pointer;
	};

	struct PointerPress_Pointer
	{
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
		bool consumed;
	};

	struct PointerPress_Params
	{
		Button& btn;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerPress_Pointer const& pointer;
	};

	[[nodiscard]] static Math::Vec2Int BuildTextOffset(Extent const& widgetExtent, Extent const& textExtent) noexcept {
		Math::Vec2Int out = {};
		for (int i = 0; i < 2; i++) {
			auto temp = (i32)Math::Round((f32)widgetExtent[i] * 0.5f - (f32)textExtent[i] * 0.5f);
			temp = Math::Max(0, temp);
			out[i] = temp;
		}
		return out;
	}
}

using namespace DEngine;
using namespace DEngine::Gui;

class [[maybe_unused]] Button::Impl
{
public:
	// This data is only available when rendering.
	struct [[maybe_unused]] CustomData
	{
		explicit CustomData(RectCollection::AllocRefT const& alloc) : glyphRects{ alloc } {}

		Extent titleTextOuterExtent = {};
		FontFaceId fontFaceId;
		Std::Vec<Rect, RectCollection::AllocRefT> glyphRects;
	};

	// Return true if the event is consumed.
	[[nodiscard]] static bool PointerPress(impl::PointerPress_Params const& params);

	[[nodiscard]] static bool PointerMove(impl::PointerMove_Params const& params);
};

bool Gui::Button::Impl::PointerPress(impl::PointerPress_Params const& params)
{
	auto& widget = params.btn;
	auto const& widgetRect = params.widgetRect;
	auto const& visibleRect = params.visibleRect;
	auto const& pointer = params.pointer;

	bool returnVal = pointer.consumed;

	auto const pointerInside =
		widgetRect.PointIsInside(pointer.pos) &&
		visibleRect.PointIsInside(pointer.pos);

	returnVal = returnVal || pointerInside;

	if (pointer.type != impl::PointerType::Primary)
		return returnVal;

	if (widget.heldPointerId.HasValue())
	{
		auto const currPointerId = widget.heldPointerId.Value();
		if (params.pointer.id == currPointerId && !params.pointer.pressed && !params.pointer.consumed)
		{
			widget.heldPointerId = Std::nullOpt;

			if (pointerInside)
				widget.Activate();
		}
	}
	else
	{
		if (pointerInside && params.pointer.pressed && !params.pointer.consumed)
			widget.heldPointerId = params.pointer.id;
	}

	return returnVal;
}

bool Gui::Button::Impl::PointerMove(impl::PointerMove_Params const& params)
{
	auto& widget = params.btn;
	auto const& widgetRect = params.widgetRect;
	auto const& visibleRect = params.visibleRect;
	auto const& pointer	= params.pointer;

	auto const pointerInside =
		widgetRect.PointIsInside(pointer.pos) &&
		visibleRect.PointIsInside(pointer.pos);

	if (params.pointer.id == impl::cursorPointerId)
		widget.hoveredByCursor = pointerInside && !params.pointer.occluded;

	return pointerInside;
}

Button::Button()
{
	text = "Button";
}

void Button::Activate()
{
	if (type == Type::Toggle)
	{
		toggled = !toggled;
	}

	if (activateFn)
		activateFn(*this);
}

void Button::SetToggled(bool toggled)
{
	toggled = toggled;
}

bool Button::GetToggled() const
{
	return toggled;
}

void Button::CursorExit(Context& ctx)
{
	hoveredByCursor = false;
}

bool Button::CursorMove(
	Widget::CursorMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	impl::PointerMove_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)params.event.position.x, (f32)params.event.position.y };
	pointer.occluded = occluded;

	impl::PointerMove_Params temp = {
		*this,
		widgetRect,
		visibleRect,
		pointer };

	return Impl::PointerMove(temp);
}

bool Button::CursorPress2(
	Widget::CursorPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.type = impl::ToPointerType(params.event.button);
	pointer.pos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	pointer.pressed = params.event.pressed;
	pointer.consumed = consumed;

	impl::PointerPress_Params temp {
		*this,
		widgetRect,
		visibleRect,
		pointer };

	return Impl::PointerPress(temp);
}

bool Button::TouchMove2(
	TouchMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	impl::PointerMove_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.pos = params.event.position;
	pointer.occluded = occluded;

	impl::PointerMove_Params temp = {
		*this,
		widgetRect,
		visibleRect,
		pointer };

	return Impl::PointerMove(temp);
}

bool Button::TouchPress2(
	TouchPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = params.event.id;
	pointer.type = impl::PointerType::Primary;
	pointer.pos = params.event.position;
	pointer.pressed = params.event.pressed;
	pointer.consumed = consumed;

	impl::PointerPress_Params temp {
		*this,
		widgetRect,
		visibleRect,
		pointer };

	return Impl::PointerPress(temp);
}

SizeHint Button::GetSizeHint2(
	GetSizeHint2_Params const& params) const
{
	auto const& ctx = params.ctx;
	auto& textMgr = params.textManager;
	auto& pusher = params.pusher;
	auto const& window = params.window;

	SizeHint returnValue = {};

	auto pusherIt = pusher.AddEntry(*this);

	auto textScale = ctx.fontScale * window.contentScale;

	if (pusher.IncludeRendering()) {
		auto customData = Impl::CustomData{ pusher.Alloc() };
		customData.fontFaceId = textMgr.GetFontFaceId(
			textScale,
			window.dpiX,
			window.dpiY);

		customData.glyphRects.Resize(text.size());
		auto const textOuterExtent = textMgr.GetOuterExtent(
			{ text.data(), text.size() },
			textScale,
			window.dpiX,
			window.dpiY,
			customData.glyphRects.Data());
		customData.titleTextOuterExtent = textOuterExtent;
		returnValue.minimum = textOuterExtent;
		pusher.AttachCustomData(pusherIt, Std::Move(customData));
	}
	else
	{
		returnValue.minimum = textMgr.GetOuterExtent(
			{ text.data(), text.size() },
			textScale,
			window.dpiX,
			window.dpiY);
	}

	auto margin = (u32)Math::Round((f32)returnValue.minimum.height * 0.25f);
	returnValue.minimum.AddPadding(margin);

	pusher.SetSizeHint(pusherIt, returnValue);
	return returnValue;
}

void Button::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
}

void Button::Render2(
	Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& rectCollection = params.rectCollection;
	auto& drawInfo = params.drawInfo;

	auto const intersection = Rect::Intersection(widgetRect, visibleRect);
	if (intersection.IsNothing())
		return;

	Math::Vec4 currColor = {};
	Math::Vec4 currTextColor = {};

	if (heldPointerId.HasValue())
	{
		// We have a finger pressing on the button atm
		currColor = colors.pressed;
		currTextColor = colors.pressedText;
	}
	else
	{
		if (type == Type::Toggle)
		{
			if (toggled)
			{
				currColor = colors.toggled;
				currTextColor = colors.toggledText;
			}
			else
			{
				currColor = colors.normal;
				currTextColor = colors.normalText;
			}
		}
		else if (type == Type::Push)
		{
			currColor = colors.normal;
			currTextColor = colors.normalText;
		}
		else
			DENGINE_IMPL_UNREACHABLE();

		if (hoveredByCursor)
		{
			for (auto i = 0; i < 3; i++)
				currColor[i] += hoverOverlayColor[i];
		}
	}

	for (auto i = 0; i < 4; i++)
	{
		currColor[i] = Math::Clamp(currColor[i], 0.f, 1.f);
		currTextColor[i] = Math::Clamp(currTextColor[i], 0.f, 1.f);
	}

	drawInfo.PushFilledQuad(
		widgetRect,
		currColor);

	auto* customDataPtr = rectCollection.GetCustomData2<Impl::CustomData>(*this);
	DENGINE_IMPL_GUI_ASSERT(customDataPtr);
	auto& customData = *customDataPtr;

	Rect textRect = {};
	textRect.position =
		widgetRect.position + impl::BuildTextOffset(widgetRect.extent, customData.titleTextOuterExtent);
	textRect.extent = customData.titleTextOuterExtent;

	auto scissor = DrawInfo::ScopedScissor(drawInfo, textRect, widgetRect);

	drawInfo.PushText(
		(u64)customData.fontFaceId,
		{ text.data(), text.size() },
		customData.glyphRects.Data(),
		textRect.position,
		currTextColor);
}