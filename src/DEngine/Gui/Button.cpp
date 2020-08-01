#include <DEngine/Gui/Button.hpp>

#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/impl/ImplData.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

Button::Button()
{
	textWidget.text = "Button";
}

void Button::Render(
	Context& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
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
		textWidget.text,
		currentTextColor,
		framebufferExtent,
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

void Button::Activate()
{
	if (type == Type::Toggle)
	{
		toggled = !toggled;
	}

	if (activatePfn)
		activatePfn(*this);
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
	Rect widgetRect,
	CursorMoveEvent event)
{
	bool temp = widgetRect.PointIsInside(event.position);
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
	Rect widgetRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	if (event.button == CursorButton::Left)
	{
		bool temp = widgetRect.PointIsInside(cursorPos);
		if (temp)
		{
			if (event.clicked)
				SetState(State::Pressed);
			else
			{
				if (state == State::Pressed)
				{
					Activate();
				}
				SetState(State::Hovered);
			}
		}
	}

}

void Button::TouchEvent(
	Rect widgetRect,
	Gui::TouchEvent touch)
{
	if (touch.id == 0)
	{
		bool temp = widgetRect.PointIsInside(touch.position);
		if (temp)
		{
			if (touch.type == TouchEventType::Down)
				SetState(State::Pressed);
			else if (touch.type == TouchEventType::Up)
			{
				if (state == State::Pressed)
					Activate();
				SetState(State::Normal);
			}
		}
		else
		{
			SetState(State::Normal);
		}
	}
}