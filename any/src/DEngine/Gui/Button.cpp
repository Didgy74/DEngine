#include <DEngine/Gui/Button.hpp>

#include <DEngine/Gui/Context.hpp>
#include "ImplData.hpp"

namespace DEngine::Gui::impl
{
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

	static constexpr u8 cursorPointerId = (u8)-1;

	struct BtnImpl;
}

using namespace DEngine;
using namespace DEngine::Gui;

class impl::BtnImpl 
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
		bool pointerInWidget = widgetRect.PointIsInside(pointerPos) && visibleRect.PointIsInside(pointerPos);
		if (!pointerInWidget)
			return false;

		bool returnVal = true;
		
		if (pointerType == PointerType::Primary && pointerInWidget)
		{
			if (pointerPressed)
			{
				widget.state = Button::State::Pressed;
				widget.pointerId = pointerId;
				return true;
			}
			else
			{
				// We are in pressed state, we need to have a pointer id.
				DENGINE_IMPL_GUI_ASSERT(widget.state != Button::State::Pressed || widget.pointerId.HasValue());

				if (widget.state == Button::State::Pressed)
				{
					// We are in pressed state, we need to have a pointer id.
					DENGINE_IMPL_GUI_ASSERT(widget.pointerId.HasValue());
					if (widget.pointerId.Value() == pointerId)
					{
						widget.Activate(ctx);
						widget.state = Button::State::Hovered;
						return true;
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
		bool returnVal = false;

		bool pointerInsideWidget = widgetRect.PointIsInside(pointerPos) && visibleRect.PointIsInside(pointerPos);
		if (pointerInsideWidget)
			returnVal = true;

		// If we are in hovered mode and cursor is either occluded or moved outside,
		// we exit hover mode.
		if (pointerId == cursorPointerId && widget.state == Button::State::Hovered)
		{
			if (!pointerInsideWidget || (pointerInsideWidget && pointerOccluded))
				widget.state = Button::State::Normal;
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
	impl::TextManager::RenderText(
		implData.textManager,
		{ text.data(), text.length() },
		currentTextColor,
		widgetRect,
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

void Button::TouchEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event,
	bool cursorOccluded)
{
	/*
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
	*/
}
