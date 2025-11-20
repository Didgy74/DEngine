#pragma once

#include <DEngine/Gui/Widget.hpp>


#include <functional>

namespace DEngine::Gui::impl { class SA_Impl; }

namespace DEngine::Gui {
	class ScrollArea : public Widget {
	public:
		using ParentType = Widget;

		Std::Box<Widget> child;
		static constexpr bool defaultExpandChild = true;
		bool expandChild = defaultExpandChild;

		struct Theme {
			Distance scrollbarPadding;
			Distance childPadding;
		};
		struct GetThemeParamsT {
			ScrollArea const& widget;
			Std::ConstAnyRef const& appData;
		};
		using GetThemeFnT = Theme(GetThemeParamsT const&);
		std::function<GetThemeFnT> getThemeFn;

		int defaultTouchScrollSlopPx = 20;

		Math::Vec4 scrollbarInactiveColor = { 1, 1, 1, 0.5f };

		// The continuous-input control this ScrollArea scrolls on. It only scrolls while it is the
		// focused widget. Defaults to Primary, which is the control the host currently dispatches
		// (the right stick). See src/Gui/Navigation.md.
		AxisId scrollAxisId = AxisId::Primary;
		// Pixels scrolled per second at full stick deflection, before content-scale. Frame-rate
		// independent: analog is scaled by the source's deltaTime.
		f32 axisScrollSpeed = 600.f;

		virtual WidgetEvent_Measure_ReturnT WidgetEvent_Measure(
			WidgetEvent_Measure_Params const& params) const override;

		virtual void WidgetEvent_Layout(
			WidgetEvent_Layout_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
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

		virtual void WidgetEvent_TextInput(
			AllocRef const& transientAlloc,
			WidgetEvent_TextInputParams const& event) override;
		virtual void WidgetEvent_TextSelection(
			AllocRef const& transientAlloc,
			WidgetEvent_TextSelectionParams const& event) override;
		virtual void WidgetEvent_EndTextInputSession(
			AllocRef const& transientAlloc,
			WidgetEvent_EndTextInputSessionParams const& event) override;

		virtual void WidgetEvent_Render(
			Render_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;

		virtual void WidgetEvent_Navigation(
			Widget_NavigationParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			Widget_NavigationState&) override;
		virtual void WidgetEvent_AxisInput(
			WidgetEvent_AxisInputParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			WidgetEvent_AxisInputEventState&) override;

		virtual void AccessibilityTest(
			AccessibilityTest_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;

	private:
		friend impl::SA_Impl;

		// The scrollbar position should always be clamped
		u32 currScrollbarPos = 0;

		struct Scrollbar_Pressed_T {
			u8 pointerId;
			f32 pointerRelativePosY;
		};
		Std::Opt<Scrollbar_Pressed_T> scrollbarHoldData;

		// Tracks automatic scrolling for touch
		struct TouchScroll_Pressed_T {
			u8 pointerId;
			// Relative to ScrollArea WidgetRect
			f32 pointerRelativeStartPosY;
			// Holds the Y location point relative to the child Widget Rect where the finger was,
			// when we began touch scrolling. If this is nullOpt, we haven't started scrolling.
			// This is set once we move a certain distance from the initial press-point.
			// Taking this into account allows to not have any snapping once touch-scrolling
			// kicks in.
			Std::Opt<f32> widgetRelativePosYAtStart;
		};
		Std::Opt<TouchScroll_Pressed_T> touchScrollHoldData;

		bool scrollbarHoveredByCursor = false;
	};
}