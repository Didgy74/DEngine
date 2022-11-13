#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/Variant.hpp>
#include <DEngine/Std/Utility.hpp>

#include <functional>
#include <string>
#include <vector>

namespace DEngine::Gui::impl { struct MenuBtnImpl; }

namespace DEngine::Gui
{
	class MenuButton : public Widget
	{
	public:
		struct Line {
			std::string title;
			std::function<void(Line&, Context*)> callback;
			bool toggled = false;
			bool togglable = false;
		};
		struct LineAny {
			Std::Box<Widget> widget;
		};
		struct Submenu {
			std::vector<Std::Variant<Line, LineAny>> lines;
			Std::Opt<uSize> activeSubmenu;
			struct PressedLineData {
				u8 pointerId = 0;
				uSize lineIndex = 0;
			};
			Std::Opt<PressedLineData> pressedLineOpt;
			Std::Opt<uSize> cursorHoveredLineOpt;
		};

		std::string title;

		Submenu submenu;
		u32 spacing = 0;
		u32 margin = 0;
		struct Colors
		{
			Math::Vec4 normal = { 0.f, 0.f, 0.f, 0.f };
			Math::Vec4 hovered = { 1.f, 1.f, 1.f, 0.25f };
			Math::Vec4 active = { 0.25f, 0.25f, 0.25f, 1.f };
		};
		Colors colors = {};

		virtual SizeHint GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;
		virtual void Render2(
			Render_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;

		virtual void CursorExit(
			Context& ctx) override;

		virtual bool CursorMove(
			CursorMoveParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool occluded) override;

		virtual bool CursorPress2(
			CursorPressParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool consumed) override;

		virtual bool TouchMove2(
			TouchMoveParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool occluded) override;

		virtual bool TouchPress2(
			TouchPressParams const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool consumed) override;

	private:
		// Determines whether the button is currently pressed and should be showing the submenu.
		bool active = false;
		bool hoveredByCursor = false;

		friend impl::MenuBtnImpl;
	};
}