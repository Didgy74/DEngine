#pragma once

#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/StdWidgets/StackLayout.hpp>


#include <functional>
#include <string>

namespace DEngine::Gui {
	class CollapsingHeader : public Widget {
	public:
		CollapsingHeader() = default;
		CollapsingHeader(CollapsingHeader&&) noexcept = delete;
		CollapsingHeader(CollapsingHeader const&) noexcept = delete;

		Std::Box<Widget> child;
		bool collapsed = true;
		std::string title = "Title";

		Math::Vec4 collapsedColor = { 0.3f, 0.3f, 0.3f, 1.f };
		Math::Vec4 expandedColor = { 0.5f, 0.5f, 0.5f, 1.f };
		// Color that will be displayed behind child content
		Math::Vec4 contentBackgroundColor = { 1.f, 1.f, 1.f, 0.1f };

		struct Theme {
			Distance textMargin;
			Distance cornerRadius;
			Distance contentMargin;
			Distance contentShadowHeight;
		};
		struct GetThemeParamsT {
			CollapsingHeader const& widget;
			Std::ConstAnyRef const& appData;
		};
		using GetThemeFnT = Theme(GetThemeParamsT const&);
		std::function<GetThemeFnT> getThemeFn;

		f32 contentShadowAlpha = 0.2;

		using CollapseFnT = void(CollapsingHeader& widget);
		std::function<CollapseFnT> collapseFn;

		virtual GetSizeHint2_ReturnT GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;
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
		virtual void WidgetEvent_TextInput(
			AllocRef const& transientAlloc,
			WidgetEvent_TextInputParams const& event) override;
		virtual void WidgetEvent_EndTextInputSession(
			AllocRef const& transientAlloc,
			WidgetEvent_EndTextInputSessionParams const& event) override;

		virtual void Render2(
			Render_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;
		virtual void AccessibilityTest(
			AccessibilityTest_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;

		virtual void WidgetEvent_Navigation(
			Widget_NavigationParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			Widget_NavigationState&) override;
		virtual bool WidgetEvent_Activate(
			WidgetEvent_ActivateParams const&,
			Std::Opt<RectCollection::Iter> const&) override;

	protected:
		struct Impl;
		friend Impl;
		static constexpr uSize headerBtnSecondaryItemIndex = 0;

		// Describes which pointer-id is currently pressed on the header.
		Std::Opt<u8> headerPointerId;
		bool hoveredByCursor = false;
	};
}