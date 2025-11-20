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