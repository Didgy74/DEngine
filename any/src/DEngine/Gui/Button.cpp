#include <DEngine/Gui/Button.hpp>

#include <DEngine/Gui/Context.hpp>
#include "ImplData.hpp"

namespace DEngine::Gui::impl
{
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

	struct BtnImpl;

	struct PointerPress_Params
	{
		Button* widget = nullptr;
		Context* ctx = nullptr;
		Rect const* widgetRect = nullptr;
		Rect const* visibleRect = nullptr;
		u8 pointerId = 0;
		PointerType pointerType = {};
		Math::Vec2 pointerPos = {};
		bool pointerPressed = {};
	};

	struct PointerMove_Params
	{
		Button* widget = nullptr;
		Context* ctx = nullptr;
		Rect const* widgetRect = nullptr;
		Rect const* visibleRect = nullptr;
		u8 pointerId = 0;
		Math::Vec2 pointerPos = {};
		bool pointerOccluded = {};
	};
}

using namespace DEngine;
using namespace DEngine::Gui;

class [[maybe_unused]] Gui::impl::BtnImpl
{
public:
	// Return true if the event is consumed.
	[[nodiscard]] static bool PointerPress(PointerPress_Params const& params)
	{
		auto& widget = *params.widget;

		bool returnVal = false;

		auto const pointerInside =
			params.widgetRect->PointIsInside(params.pointerPos) &&
			params.visibleRect->PointIsInside(params.pointerPos);

		if (pointerInside)
			returnVal = true;

		if (params.pointerType != PointerType::Primary)
			return returnVal;

		if (widget.pointerId.HasValue())
		{
			auto const currPointerId = widget.pointerId.Value();
			if (params.pointerId == currPointerId && !params.pointerPressed)
			{
				widget.pointerId = Std::nullOpt;

				if (pointerInside)
					widget.Activate();
			}
		}
		else
		{
			if (pointerInside && params.pointerPressed)
				widget.pointerId = params.pointerId;
		}

		return returnVal;
	}

	[[nodiscard]] static bool PointerMove(PointerMove_Params const& params)
	{
		auto const pointerInside =
			params.widgetRect->PointIsInside(params.pointerPos) &&
			params.visibleRect->PointIsInside(params.pointerPos);

		if (params.pointerId == cursorPointerId)
		{
			params.widget->hoveredByCursor = pointerInside && !params.pointerOccluded;
		}

		return pointerInside;
	}
};

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

SizeHint Button::GetSizeHint(
	Context const& ctx) const
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
	auto returnVal = impl::TextManager::GetSizeHint(
		implData.textManager,
		{ text.data(), text.size() });

	returnVal.preferred.width += textMargin * 2;
	returnVal.preferred.height += textMargin * 2;

	return returnVal;
}

void Button::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	Math::Vec4 currColor = {};
	Math::Vec4 currTextColor = {};

	if (pointerId.HasValue())
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
	drawInfo.PushFilledQuad(widgetRect, currColor);

	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	auto textRect = widgetRect;
	textRect.position.x += textMargin;
	textRect.position.y += textMargin;
	textRect.extent.width -= textMargin * 2;
	textRect.extent.height -= textMargin * 2;

	impl::TextManager::RenderText(
		implData.textManager,
		{ text.data(), text.length() },
		currTextColor,
		textRect,
		drawInfo);
}

bool Button::CursorPress(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorPressEvent event)
{
	impl::PointerPress_Params params = {};
	params.widget = this;
	params.ctx = &ctx;
	params.widgetRect = &widgetRect;
	params.visibleRect = &visibleRect;
	params.pointerId = impl::cursorPointerId;
	params.pointerType = impl::ToPointerType(event.button);
	params.pointerPos = { (f32)cursorPos.x, (f32)cursorPos.y };
	params.pointerPressed = event.pressed;

	return impl::BtnImpl::PointerPress(params);
}

bool Button::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event,
	bool cursorOccluded)
{
	impl::PointerMove_Params params = {};
	params.widget = this;
	params.ctx = &ctx;
	params.widgetRect = &widgetRect;
	params.visibleRect = &visibleRect;
	params.pointerId = impl::cursorPointerId;
	params.pointerPos = { (f32)event.position.x, (f32)event.position.y };
	params.pointerOccluded = cursorOccluded;

	return impl::BtnImpl::PointerMove(params);
}

bool Button::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
	impl::PointerPress_Params params = {};
	params.widget = this;
	params.ctx = &ctx;
	params.widgetRect = &widgetRect;
	params.visibleRect = &visibleRect;
	params.pointerId = event.id;
	params.pointerType = impl::PointerType::Primary;
	params.pointerPos = event.position;
	params.pointerPressed = event.pressed;

	return impl::BtnImpl::PointerPress(params);
}

bool Button::TouchMoveEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchMoveEvent event,
	bool occluded)
{
	impl::PointerMove_Params params = {};
	params.widget = this;
	params.ctx = &ctx;
	params.widgetRect = &widgetRect;
	params.visibleRect = &visibleRect;
	params.pointerId = event.id;
	params.pointerPos = event.position;
	params.pointerOccluded = occluded;

	return impl::BtnImpl::PointerMove(params);
}

SizeHint Button::GetSizeHint2(
	GetSizeHint2_Params const& params) const
{
	auto& implData = *static_cast<impl::ImplData*>(params.ctx.Internal_ImplData());

	auto returnVal = impl::TextManager::GetSizeHint(
		implData.textManager,
		{ text.data(), text.size() });

	returnVal.preferred.width += textMargin * 2;
	returnVal.preferred.height += textMargin * 2;

	params.pusher.PushSizeHint(*this, returnVal);

	return returnVal;
}

void Button::CursorExit(Context& ctx)
{
	hoveredByCursor = false;
	pointerId = Std::nullOpt;
}

bool Button::CursorPress2(Widget::CursorPressParams const& params)
{
	auto const& rects = params.rectCollection.GetRect(*this);

	impl::PointerPress_Params temp = {};
	temp.widget = this;
	temp.ctx = &params.ctx;
	temp.widgetRect = &rects.widgetRect;
	temp.visibleRect = &rects.visibleRect;
	temp.pointerId = impl::cursorPointerId;
	temp.pointerType = impl::ToPointerType(params.event.button);
	temp.pointerPos = { (f32)params.cursorPos.x, (f32)params.cursorPos.y };
	temp.pointerPressed = params.event.pressed;

	return impl::BtnImpl::PointerPress(temp);
}

bool Button::CursorMove(
	Widget::CursorMoveParams const& params,
	bool occluded)
{
	auto const& rects = params.rectCollection.GetRect(*this);

	impl::PointerMove_Params temp = {};
	temp.widget = this;
	temp.ctx = &params.ctx;
	temp.widgetRect = &rects.widgetRect;
	temp.visibleRect = &rects.visibleRect;
	temp.pointerId = impl::cursorPointerId;
	temp.pointerPos = { (f32)params.event.position.x, (f32)params.event.position.y };
	temp.pointerOccluded = occluded;

	return impl::BtnImpl::PointerMove(temp);
}

void Button::Render2(
	Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	Math::Vec4 currColor = {};
	Math::Vec4 currTextColor = {};

	if (pointerId.HasValue())
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
	params.drawInfo.PushFilledQuad(widgetRect, currColor);

	auto& implData = *static_cast<impl::ImplData*>(params.ctx.Internal_ImplData());

	auto textRect = widgetRect;
	textRect.position.x += textMargin;
	textRect.position.y += textMargin;
	textRect.extent.width -= textMargin * 2;
	textRect.extent.height -= textMargin * 2;

	impl::TextManager::RenderText(
		implData.textManager,
		{ text.data(), text.length() },
		currTextColor,
		textRect,
		params.drawInfo);
}