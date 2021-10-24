#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/Variant.hpp>
#include <DEngine/Std/Utility.hpp>

#include <functional>
#include <string>
#include <vector>

namespace DEngine::Gui::impl { struct MenuButtonImpl; }

namespace DEngine::Gui
{
	class MenuButton : public Widget
	{
	public:
		struct Line;
		struct Submenu
		{
			std::vector<Line> lines;
			Std::Opt<uSize> activeSubmenu;
			struct PressedLine
			{
				uSize lineIndex = 0;
				u8 pointerId = 0;
			};
			Std::Opt<PressedLine> pressedLine;
			Std::Opt<uSize> hoveredLineCursor;
		};
		struct LineButton
		{
			std::function<void(LineButton&)> callback;
			bool toggled = false;
			bool togglable = false;
		};
		struct LineEmpty {};
		struct Line
		{
			Line() = default;
			Line(Line&&) = default;
			Line(LineButton&& button) noexcept : data{ Std::Move(button) } {}

			Line& operator=(Line&&) = default;

			std::string title;
			Std::Variant<Submenu, LineButton, LineEmpty> data = LineEmpty{};
		};

		std::string title;

		Submenu submenu;
		u32 spacing = 0;
		u32 margin = 0;
		struct Colors
		{
			Math::Vec4 normal = { 0.f, 0.f, 0.f, 0.f };
			Math::Vec4 active = { 0.25f, 0.25f, 0.25f, 1.f };
		};
		Colors colors = {};

		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual bool CursorPress(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorPressEvent eventd) override;

		virtual bool TouchPressEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchPressEvent event) override;

	private:
		// Determines whether the button is currently pressed and should be showing the submenu.
		bool active = false;
		Std::Opt<u8> pointerId;

		friend impl::MenuButtonImpl;
	};
}