#include <DEngine/Gui/Button.hpp>

#include <DEngine/Gui/Context.hpp>
#include "ImplData.hpp"

using namespace DEngine;
using namespace DEngine::Gui;

Button::Button()
{
	textWidget.String_Set("Button");
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

	Gfx::GuiDrawCmd cmd{};
	cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
	cmd.filledMesh.color = currentColor;
	cmd.filledMesh.mesh = drawInfo.GetQuadMesh();
	cmd.rectPosition.x = (f32)widgetRect.position.x / framebufferExtent.width;
	cmd.rectPosition.y = (f32)widgetRect.position.y / framebufferExtent.height;
	cmd.rectExtent.x = (f32)widgetRect.extent.width / framebufferExtent.width;
	cmd.rectExtent.y = (f32)widgetRect.extent.height / framebufferExtent.height;
	drawInfo.drawCmds.push_back(cmd);


	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
	impl::TextManager::RenderText(
		implData.textManager,
		textWidget.StringView(),
		currentTextColor,
		widgetRect,
		drawInfo);
}

void Button::SetState(State newState)
{
	if (newState == state)
		return;

	switch (newState)
	{
	case State::Normal:
		textWidget.color = normalTextColor;
		break;
	case State::Hovered:
		textWidget.color = hoverTextColor;
		break;
	case State::Pressed:
		textWidget.color = pressedTextColor;
		break;
	case State::Toggled:
		textWidget.color = toggledTextColor;
		break;
	}

	state = newState;
}

void Button::Activate(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect)
{
	if (type == Type::Toggle)
	{
		toggled = !toggled;
	}

	if (activatePfn)
		activatePfn(
			*this,
			ctx,
			windowId,
			widgetRect,
			visibleRect);
}

void Button::SetToggled(bool toggled)
{
	if (toggled)
	{
		textWidget.color = toggledTextColor;
	}
	else
	{
		textWidget.color = normalTextColor;
	}

	this->toggled = toggled;
}

bool Button::GetToggled() const
{
	return toggled;
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
			SetState(State::Hovered);
	}
	else
	{
		if (toggled)
			SetState(State::Toggled);
		else
			SetState(State::Normal);
	}
}

void Button::CursorClick(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	if (event.button == CursorButton::Left)
	{
		bool temp = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);
			
		if (temp)
		{
			if (event.clicked)
				SetState(State::Pressed);
			else
			{
				if (state == State::Pressed)
				{
					Activate(
						ctx,
						windowId,
						widgetRect,
						visibleRect);
				}
				SetState(State::Hovered);
			}
		}
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
				SetState(State::Pressed);
			else if (event.type == TouchEventType::Up)
			{
				if (state == State::Pressed)
					Activate(
						ctx,
						(WindowID)0,
						widgetRect,
						visibleRect);
				SetState(State::Normal);
			}
		}
		else
		{
			SetState(State::Normal);
		}
	}
}

SizeHint Button::SizeHint(
	Context const& ctx) const
{
	return textWidget.SizeHint(ctx);
}
