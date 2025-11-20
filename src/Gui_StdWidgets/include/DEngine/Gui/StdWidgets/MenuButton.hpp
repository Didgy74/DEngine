#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <functional>
#include <string>
#include <vector>

namespace DEngine::Gui
{
	class MenuButton : public Widget
	{
	public:
		struct Line {
			std::string title;
			std::function<void(Line&, Widget::WidgetEvent_DeferredJobQueue&)> callback;
			bool toggled = false;
			bool togglable = false;
		};
		struct Submenu {
			std::vector<Line> lines;
		};

		std::string title;

		Submenu submenu;
		struct Colors {
			Math::Vec4 normal = { 0.f, 0.f, 0.f, 0.f };
			Math::Vec4 hovered = { 1.f, 1.f, 1.f, 0.25f };
			Math::Vec4 active = { 0.25f, 0.25f, 0.25f, 1.f };
		};
		Colors colors = {};

		struct Theme {
			Distance textMargin;
			Distance cornerRadius;
		};
		struct GetThemeParamsT {
			MenuButton const& widget;
			Std::ConstAnyRef const& appData;
		};
		using GetThemeFnT = Theme(GetThemeParamsT const&);
		std::function<GetThemeFnT> getThemeFn;

		virtual GetSizeHint2_ReturnT GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;
		virtual void Render2(
			Render_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;
		virtual void AccessibilityTest(
			AccessibilityTest_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;

		virtual void CursorExit() override;

		virtual bool CursorMove(
			CursorMoveParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;

		virtual bool CursorPress2(
			CursorPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;

		virtual TouchEventConsumption WidgetEvent_TouchMove(
			WidgetEvent_TouchMoveParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;

		virtual TouchEventConsumption WidgetEvent_TouchPress(
			WidgetEvent_TouchPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;

		struct Impl;
		friend Impl;
	private:
		// The pointer currently pressing the button, used to open the submenu on release.
		Std::Opt<u8> heldPointerId;
		bool hoveredByCursor = false;
	};
}
