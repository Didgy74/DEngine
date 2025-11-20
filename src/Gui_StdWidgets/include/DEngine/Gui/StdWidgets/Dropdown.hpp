#pragma once

#include <DEngine/Gui/Widget.hpp>


#include <functional>
#include <string>
#include <vector>

namespace DEngine::Gui
{
	class Dropdown : public Widget {
	public:
		static constexpr Math::Vec4 defaultBoxColor = { 0.25f, 0.25f, 0.25f, 1.f };
		static constexpr u32 defaultTextMargin = 10;

		struct Theme {
			Distance margin;
			Distance cornerRadius;
		};
		struct GetThemeParamsT {
			Dropdown const& widget;
			Std::ConstAnyRef const& appData;
		};
		using GetThemeFnT = Theme(GetThemeParamsT const&);
		std::function<GetThemeFnT> getThemeFn;

		Math::Vec4 boxColor = defaultBoxColor;
		uSize selectedItem = 0;
		std::vector<std::string> items;

		using SelectionChangedCallback = void(Dropdown&);
		std::function<SelectionChangedCallback> selectionChangedCallback;

		Dropdown();
		virtual ~Dropdown() override;

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

		virtual bool CursorMove(
			CursorMoveParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;
		virtual void CursorExit() override;

		virtual bool CursorPress2(
			CursorPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;
		virtual TouchEventConsumption WidgetEvent_TouchPress(
			WidgetEvent_TouchPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;

		virtual void WidgetEvent_Navigation(
			Widget_NavigationParams const& param,
			Std::Opt<RectCollection::Iter> const& rectCollIterParam,
			Widget_NavigationState& event) override
		{
			Widget_DefaultNavigationHandler(*this, param, rectCollIterParam, event);
		}
		virtual bool WidgetEvent_Activate(
			WidgetEvent_ActivateParams const&,
			Std::Opt<RectCollection::Iter> const&) override;

		struct Impl;
		friend Impl;
	protected:
		Std::Opt<u8> heldPointerId;
		bool hoveredByCursor = false;
	};
}