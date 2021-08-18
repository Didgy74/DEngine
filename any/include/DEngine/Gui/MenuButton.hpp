#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/Variant.hpp>
#include <DEngine/Std/Utility.hpp>

#include <functional>
#include <string>
#include <vector>

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
		Std::Opt<u8> pointerId;
		
		Submenu submenu;

		void Test(Context& ctx, WindowID windowId, Rect widgetRect);

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
			CursorClickEvent eventd) override;

		virtual bool TouchPressEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchPressEvent event) override;
	};
}