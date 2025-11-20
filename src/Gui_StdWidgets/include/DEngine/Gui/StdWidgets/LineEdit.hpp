#pragma once

#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/StdWidgets/Utilities/LineEditUtils.hpp>

#include <functional>
#include <string>

namespace DEngine::Gui
{
	// TODO: Implement the rendering solution where we scroll horizontally according to the text selection
	class LineEdit : public Widget
	{
	public:
		Math::Vec4 backgroundColor = { 0.3f, 0.3f, 0.3f, 1.f };

		struct Theme {
			Distance margin;
			Distance cornerRadius;
		};
		struct GetThemeParamsT {
			LineEdit const& widget;
			Std::ConstAnyRef const& appData;
		};
		using GetThemeFnT = Theme(GetThemeParamsT const&);
		std::function<GetThemeFnT> getThemeFn;

		using TextChangedFn = void(LineEdit& widget);
		std::function<TextChangedFn> textChangedFn = nullptr;

		std::string text;

		virtual ~LineEdit();

		[[nodiscard]] GetSizeHint2_ReturnT GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void Render2(
			Render_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;
		virtual bool CursorPress2(
			CursorPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;
		bool CursorMove(
			CursorMoveParams const &params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;
		virtual TouchEventConsumption WidgetEvent_TouchPress(
			WidgetEvent_TouchPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;
		TouchEventConsumption WidgetEvent_TouchMove(
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
		virtual void AccessibilityTest(
			AccessibilityTest_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;

		virtual void WidgetEvent_Navigation(
			Widget_NavigationParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIterOpt,
			Widget_NavigationState& event) override
		{
			Widget::Widget_DefaultNavigationHandler(
				*this,
				params,
				rectCollIterOpt,
				event);
		}
		[[nodiscard]] virtual bool WidgetEvent_Activate(
			WidgetEvent_ActivateParams const&,
			Std::Opt<RectCollection::Iter> const&) override;

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

		void ClearInputConnection();

		struct Impl;
		friend Impl;
	};
}