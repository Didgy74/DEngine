#include <DEngine/Gui/Button.hpp>

#include <DEngine/Gui/Context.hpp>
#include "ImplData.hpp"

using namespace DEngine;
using namespace DEngine::Gui;

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

	if (activatePfn)
		activatePfn(
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
	return impl::TextManager::GetSizeHint(
		implData.textManager,
		{ text.data(), text.size() });
}

void Button::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	Math::Vec4 currentColor{};
	Math::Vec4 currentTextColor{};
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
	impl::TextManager::RenderText(
		implData.textManager,
		{ text.data(), text.length() },
		currentTextColor,
		widgetRect,
		drawInfo);
}

void Button::CursorClick(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	if (event.button == CursorButton::Primary)
	{
		bool temp = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);

		if (temp)
		{
			if (event.clicked)
				state = State::Pressed;
			else
			{
				if (state == State::Pressed)
				{
					Activate(
						ctx);
				}
				state = State::Hovered;
			}
		}
	}
}

void Button::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event)
{
	bool temp = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);
	if (temp)
	{
		if (state != State::Pressed)
			state = State::Hovered;
	}
	else
	{
		if (toggled)
			state = State::Toggled;
		else
			state = State::Normal;
	}
}

void Button::TouchEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event)
{
	if (event.id == 0)
	{
		bool temp = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);
		if (temp)
		{
			if (event.type == TouchEventType::Down)
				state = State::Pressed;
			else if (event.type == TouchEventType::Up)
			{
				if (state == State::Pressed)
					Activate(
						ctx);
				state = State::Normal;
			}
		}
		else
		{
			state = State::Normal;
		}
	}
}
