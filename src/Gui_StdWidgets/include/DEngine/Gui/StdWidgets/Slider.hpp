#pragma once

#include <DEngine/Gui/Widget.hpp>


#include <functional>
#include <string>

namespace DEngine::Gui
{
	class Slider : public Widget {
	public:
		using ParentType = Widget;

		Slider();
		virtual ~Slider() override {}

		// Parameter returns 0 to 1
		struct ValueChangedFnParamsT {
			Std::AnyRef appData;
			f32 newValue;
		};
		using CallbackT = void(ValueChangedFnParamsT const&);
		std::function<CallbackT> valueChangedFn;

		// TODO: Need to support steps

		virtual WidgetEvent_Measure_ReturnT WidgetEvent_Measure(
			WidgetEvent_Measure_Params const& params) const override;
		virtual void WidgetEvent_Layout(
			WidgetEvent_Layout_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;
		virtual void WidgetEvent_Render(
			Render_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;
		virtual void AccessibilityTest(
			AccessibilityTest_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const override;

		virtual bool CursorMove(
			CursorMoveParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;

		virtual bool CursorPress2(
			CursorPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;

		virtual void CursorExit() override;

		virtual TouchEventConsumption WidgetEvent_TouchMove(
			WidgetEvent_TouchMoveParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded) override;

		virtual TouchEventConsumption WidgetEvent_TouchPress(
			WidgetEvent_TouchPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed) override;

	protected:

		f32 xPosNorm = 0.f;
		Std::Opt<u8> heldPointerId;

		class Impl;
		friend Impl;
	};
}