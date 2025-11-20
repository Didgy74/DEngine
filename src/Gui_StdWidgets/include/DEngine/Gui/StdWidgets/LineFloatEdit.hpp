#pragma once

#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/StdWidgets/Utilities/LineEditUtils.hpp>

#include <DEngine/Std/Limits.hpp>

#include <string>
#include <functional>

namespace DEngine::Gui
{
	class LineFloatEdit : public Widget
	{
	public:
		static constexpr f64 defaultMin = Std::Limits<f64>::lowest;
		static constexpr f64 defaultMax = Std::Limits<f64>::highest;
		// Set minimum to 0.0 or higher to get an unsigned text input session.
		f64 min = defaultMin;
		f64 max = defaultMax;

		struct Theme {
			Distance margin;
			Distance cornerRadius;
		};
		struct GetThemeParamsT {
			LineFloatEdit const& widget;
			Std::ConstAnyRef const& appData;
		};
		using GetThemeFnT = Theme(GetThemeParamsT const&);
		std::function<GetThemeFnT> getThemeFn;

		// TODO: This should probably return true/false so the user can decide to cancel or accept the new value.
		using ValueChangedFnT = void(LineFloatEdit& widget, f64 newValue);
		std::function<ValueChangedFnT> valueChangedFn;

		Math::Vec4 backgroundColor = { 0.3f, 0.3f, 0.3f, 1.f };

		static constexpr u8 defaultDecimalPoints = 2;
		u8 decimalPoints = defaultDecimalPoints;

		void SetValue(f64 in);
		[[nodiscard]] bool HasInputSession() const { return textEditingSessionOpt.Has(); }

		~LineFloatEdit() override;

		GetSizeHint2_ReturnT GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;
		bool CursorPress2(
			CursorPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;
		bool CursorMove(
			CursorMoveParams const &params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;
		TouchEventConsumption WidgetEvent_TouchPress(
			WidgetEvent_TouchPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;
		TouchEventConsumption WidgetEvent_TouchMove(
			WidgetEvent_TouchMoveParams const &params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;
		void WidgetEvent_TextInput(
			AllocRef const& transientAlloc,
			WidgetEvent_TextInputParams const& event) override;
		void WidgetEvent_EndTextInputSession(
			AllocRef const& transientAlloc,
			WidgetEvent_EndTextInputSessionParams const& event) override;
		void Render2(
			Render_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;
		void AccessibilityTest(
			AccessibilityTest_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;

		struct Impl;
		friend Impl;

	protected:
		// Holds the pointer ID if the widget is currently pressed.
		Std::Opt<u8> pointerId;

		struct TextEditingSession {
			// We should remember to always clamp the selection during events to the string we are
			// working on, and secretly update it during mutable events.
			LineEditUtils::SelectionRange selectionRange;
			Std::Box<Widget_TextInputSession> textInputSession;
		};
		Std::Opt<TextEditingSession> textEditingSessionOpt;

		std::string text = "0.0";
		f64 value = 0.0;

	};
}