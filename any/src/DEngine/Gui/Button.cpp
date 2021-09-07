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
}

using namespace DEngine;
using namespace DEngine::Gui;

class [[maybe_unused]] impl::BtnImpl
{
public:
	[[nodiscard]] static bool PointerPress(
		Button& widget,
		Context& ctx,
		Rect const& widgetRect,
		Rect const& visibleRect,
		u8 pointerId,
		PointerType pointerType,
		Math::Vec2 pointerPos,
		bool pointerPressed)
	{
		bool pointerInside = widgetRect.PointIsInside(pointerPos) && visibleRect.PointIsInside(pointerPos);
		if (pointerPressed && !pointerInside)
			return false;

		bool returnVal = true;

		if (pointerType == PointerType::Primary)
		{
			if (pointerPressed && pointerInside)
			{
				widget.state = Button::State::Pressed;
				widget.pointerId = pointerId;
				return true;
			}
			else if (!pointerPressed && widget.state == Button::State::Pressed)
			{
				// We are in pressed state, we need to have a pointer id.
				DENGINE_IMPL_GUI_ASSERT(widget.pointerId.HasValue());

				if (widget.pointerId.Value() == pointerId)
				{
					if (pointerInside)
					{
						// We are inside the widget, we were in pressed state,
						// and we unpressed the pointerId that was pressing down.
						widget.Activate(ctx);
						if (pointerId == cursorPointerId)
							widget.state = Button::State::Hovered;
						else
							widget.state = Button::State::Normal;
						widget.pointerId.Clear();
						return true;
					}
					else
					{
						widget.state = Button::State::Normal;
						widget.pointerId.Clear();
						return false;
					}
				}
			}
		}

		// We are inside the button, so we always want to consume the event.
		return true;
	}

	[[nodiscard]] static bool PointerMove(
		Button& widget,
		Context& ctx,
		Rect const& widgetRect,
		Rect const& visibleRect,
		u8 pointerId,
		Math::Vec2 pointerPos,
		bool pointerOccluded)
	{
		bool pointerInside = widgetRect.PointIsInside(pointerPos) && visibleRect.PointIsInside(pointerPos);

		// If we are in hovered mode and cursor is either occluded or moved outside,
		// we exit hover mode.
		if (pointerId == cursorPointerId && widget.state == Button::State::Hovered)
		{
			if (!pointerInside || (pointerInside && pointerOccluded))
			{
				widget.state = Button::State::Normal;
				return pointerInside;
			}
		}

		if (!pointerOccluded && pointerId == cursorPointerId && pointerInside)
		{
			if (widget.state == Button::State::Normal || widget.state == Button::State::Toggled)
			{
				widget.state = Button::State::Hovered;
				return pointerInside;
			}
		}

		return true;
	}
};

Button::Button()
{
	text = "Button";
}

void Button::Activate(
	Context& ctx)
{
	if (type == Type::Toggle)
	{
		toggled = !toggled;
	}

	if (activateFn)
		activateFn(
			*this);
}

void Button::SetToggled(bool toggled)
{
	if (toggled)
	{
		state = State::Toggled;
	}

	this->toggled = toggled;
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
	Math::Vec4 currentColor = {};
	Math::Vec4 currentTextColor = {};

	if (state == State::Normal && toggled)
	{
		currentColor = toggledColor;
		currentTextColor = toggledTextColor;
	}
	else
	{
		switch (state)
		{
			case State::Hovered:
				currentColor = hoverColor;
				currentTextColor = hoverTextColor;
				break;
			case State::Normal:
				currentColor = normalColor;
				currentTextColor = normalTextColor;
				break;
			case State::Pressed:
				currentColor = pressedColor;
				currentTextColor = pressedTextColor;
				break;
			case State::Toggled:
				currentColor = toggledColor;
				currentTextColor = toggledTextColor;
				break;
			default:
				break;
		}
	}
	drawInfo.PushFilledQuad(widgetRect, currentColor);

	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	auto textRect = widgetRect;
	textRect.position.x += textMargin;
	textRect.position.y += textMargin;
	textRect.extent.width -= textMargin * 2;
	textRect.extent.height -= textMargin * 2;

	impl::TextManager::RenderText(
		implData.textManager,
		{ text.data(), text.length() },
		currentTextColor,
		textRect,
		drawInfo);
}

bool Button::CursorPress(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	return impl::BtnImpl::PointerPress(
		*this,
		ctx,
		widgetRect,
		visibleRect,
		impl::cursorPointerId,
		impl::ToPointerType(event.button),
		{ (f32)cursorPos.x, (f32)cursorPos.y },
		event.clicked);
}

bool Button::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event,
	bool cursorOccluded)
{
	return impl::BtnImpl::PointerMove(
		*this,
		ctx,
		widgetRect,
		visibleRect,
		impl::cursorPointerId,
		{ (f32)event.position.x, (f32)event.position.y },
		cursorOccluded);
}

bool Button::TouchPressEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchPressEvent event)
{
	return impl::BtnImpl::PointerPress(
		*this,
		ctx,
		widgetRect,
		visibleRect,
		event.id,
		impl::PointerType::Primary,
		event.position,
		event.pressed);
}

bool Button::TouchMoveEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchMoveEvent event,
	bool occluded)
{
	return impl::BtnImpl::PointerMove(
		*this,
		ctx,
		widgetRect,
		visibleRect,
		event.id,
		event.position,
		occluded);
}
