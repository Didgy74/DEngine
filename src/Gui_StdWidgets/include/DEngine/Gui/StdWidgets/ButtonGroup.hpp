#pragma once

#include <DEngine/Gui/Widget.hpp>


#include <vector>
#include <string>
#include <functional>

namespace DEngine::Gui
{
	class ButtonGroup : public Widget {
	public:
		u32 activeIndex = 0;
		struct InternalButton {
			std::string title;
		};
		std::vector<InternalButton> buttons;

		struct Theme {
			Distance margin;
			Distance cornerRadius;
		};
		struct GetThemeParamsT {
			ButtonGroup const& widget;
			Std::ConstAnyRef const& appData;
		};
		using GetThemeFnT = Theme(GetThemeParamsT const&);
		std::function<GetThemeFnT> getThemeFn;

		using ActiveChangedCallbackT = void(ButtonGroup& widget, u32 newIndex);
		std::function<ActiveChangedCallbackT> activeChangedCallback;

		struct Colors
		{
			Math::Vec4 inactiveColor = { 0.3f, 0.3f, 0.3f, 1.f };
			Math::Vec4 hoveredColor = { 0.4f, 0.4f, 0.4f, 1.f };
			Math::Vec4 activeColor = { 0.6f, 0.6f, 0.6f, 1.f };
		};
		Colors colors = {};

		void AddButton(std::string const& title);
		[[nodiscard]] u32 GetButtonCount() const;
		[[nodiscard]] u32 GetActiveButtonIndex() const;

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

		virtual void AccessibilityTest(
			AccessibilityTest_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;

		virtual void WidgetEvent_Navigation(
			Widget_NavigationParams const&,
			Std::Opt<RectCollection::Iter> const&,
			Widget_NavigationState&) override;
		[[nodiscard]] virtual bool WidgetEvent_Activate(
			WidgetEvent_ActivateParams const&,
			Std::Opt<RectCollection::Iter> const&) override;

		struct Impl;

	protected:
		struct HeldPointerData {
			uSize buttonIndex = 0;
			u8 pointerId = 0;
		};
		Std::Opt<HeldPointerData> heldPointerData;
		Std::Opt<uSize> cursorHoverIndex;


		friend Impl;
	};
}