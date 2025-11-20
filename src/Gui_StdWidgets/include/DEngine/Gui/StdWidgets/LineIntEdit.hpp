#pragma once

#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/StdWidgets/Utilities/LineEditUtils.hpp>

#include <DEngine/Std/Limits.hpp>

#include <string>
#include <functional>

namespace DEngine::Gui
{
	class LineIntEdit : public Widget
	{
	public:
		static constexpr i64 defaultMin = Std::Limits<i64>::lowest;
		static constexpr i64 defaultMax = Std::Limits<i64>::highest;
		// Set minimum to 0.0 or higher to get an unsigned text input session.
		i64 min = defaultMin;
		i64 max = defaultMax;

		using ValueChangedFnT = void(LineIntEdit& widget, i64 newValue);
		std::function<ValueChangedFnT> valueChangedFn;

		Math::Vec4 backgroundColor = { 0.3f, 0.3f, 0.3f, 1.f };
		struct Theme {
			Distance margin;
			Distance cornerRadius;
		};
		struct GetThemeParamsT {
			LineIntEdit const& widget;
			Std::ConstAnyRef const& appData;
		};
		using GetThemeFnT = Theme(GetThemeParamsT const&);
		std::function<GetThemeFnT> getThemeFn;

		void SetValue(i64 in);
		[[nodiscard]] bool HasInputSession() const { return textEditingSessionOpt.Has(); }

		virtual ~LineIntEdit() override;

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
		virtual bool CursorPress2(
			CursorPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;
		virtual bool CursorMove(
			CursorMoveParams const &params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;
		virtual TouchEventConsumption WidgetEvent_TouchPress(
			WidgetEvent_TouchPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;
		virtual TouchEventConsumption WidgetEvent_TouchMove(
			WidgetEvent_TouchMoveParams const &params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;

		virtual void WidgetEvent_TextInput(
			AllocRef const& transientAlloc,
			WidgetEvent_TextInputParams const& event) override;
		virtual void WidgetEvent_TextSelection(
			AllocRef const& transientAlloc,
			WidgetEvent_TextSelectionParams const& event) override;
		virtual void WidgetEvent_EndTextInputSession(
			AllocRef const& transientAlloc,
			WidgetEvent_EndTextInputSessionParams const& event) override;
		void AccessibilityTest(
			AccessibilityTest_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;

		struct Impl;
		friend Impl;

	protected:
		Std::Opt<u8> pointerId;
		struct TextEditingSession {
			// We should remember to always clamp the selection during events to the string we are
			// working on, and secretly update it during mutable events.
			LineEditUtils::SelectionRange selectionRange;
			Std::Box<Widget_TextInputSession> textInputSession;
		};
		Std::Opt<TextEditingSession> textEditingSessionOpt;
		std::string text = "0";
		i64 value = 0;
	};
}