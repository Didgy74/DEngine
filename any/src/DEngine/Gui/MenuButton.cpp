#include <DEngine/Gui/MenuButton.hpp>

#include <DEngine/Gui/Context.hpp>
#include "ImplData.hpp"

using namespace DEngine;
using namespace DEngine::Gui;

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

	struct PointerPress_Pointer
	{
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
	};
	[[nodiscard]] static bool PointerPress(
		MenuButton& widget,
		Context& ctx,
		WindowID windowId,
		Rect const& widgetRect,
		Rect const& visibleRect,
		PointerPress_Pointer const& pointer)
	{
		bool pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);
		if (pointer.pressed && !pointerInside)
			return false;

		if (pointer.type == PointerType::Primary)
		{
			if (pointer.pressed && pointerInside)
			{
				widget.pointerId = pointer.id;
				return pointerInside;
			}
			else if (!pointer.pressed && widget.pointerId.HasValue() && widget.pointerId.Value() == pointer.id)
			{
				if (pointerInside)
				{
					// We are inside the widget, we were in pressed state,
					// and we unpressed the pointerId that was pressing down.
					
					widget.Test(ctx, windowId, widgetRect);

					widget.pointerId = Std::nullOpt;
					return pointerInside;
				}
				else
				{
					widget.pointerId = Std::nullOpt;
					return pointerInside;
				}
			}
		}

		// We are inside the button, so we always want to consume the event.
		return pointerInside;
	}
}

#include <iostream>

void MenuButton::Test(Context& ctx, WindowID windowId, Rect widgetRect)
{
	Math::Vec2Int position = widgetRect.position;
	position.y += widgetRect.extent.height;

	std::vector<Test_Menu::Line> entries;

	Test_Menu::Line line = {};
	line.title = "First";
	line.type = Test_Menu::Line::Type::Button;
	line.callback = []() {
		std::cout << "First button activated" << std::endl;
	};
	entries.push_back(line);
	
	line.title = "Second";
	line.type = Test_Menu::Line::Type::Submenu;
	{
		Test_Menu::Line subline = {};
		subline.title = "Sub First";
		subline.type = Test_Menu::Line::Type::Label;
		line.subLines.push_back(subline);

		subline.title = "Sub second";
		subline.type = Test_Menu::Line::Type::Button;
		subline.callback = line.callback = []() {
			std::cout << "Sub second button activated" << std::endl;
		};
		line.subLines.push_back(subline);
	}
	entries.push_back(line);

	line.title = "Third";
	line.type = Test_Menu::Line::Type::Button;
	line.callback = []() {
		std::cout << "Third button activated" << std::endl;
	};

	ctx.Test_AddMenu(
		windowId,
		position,
		widgetRect.extent.width,
		entries);
}

SizeHint MenuButton::GetSizeHint(
	Context const& ctx) const
{
	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	return impl::TextManager::GetSizeHint(
		implData.textManager,
		{ title.data(), title.size() });
}

void MenuButton::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	auto& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	impl::TextManager::RenderText(
		implData.textManager,
		{ title.data(), title.size() },
		{ 1.f, 1.f, 1.f, 1.f },
		widgetRect,
		drawInfo);
}

bool MenuButton::CursorPress(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	impl::PointerPress_Pointer pointer = {};
	pointer.id = impl::cursorPointerId;
	pointer.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
	pointer.pressed = event.clicked;
	pointer.type = impl::ToPointerType(event.button);
	return impl::PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		pointer);
}