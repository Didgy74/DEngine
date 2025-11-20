#pragma once

#include <DEngine/Gui/Widget.hpp>


namespace DEngine::Editor
{
	class Joystick : public Gui::Widget
	{
	public:
		struct PressedData
		{
			u8 pointerId;
			// Holds relative position [-1, 1] bound to a circle.
			Math::Vec2 currPos = {};
		};
		Std::Opt<PressedData> pressedData = {};

		// When set, the stick only reports vertical movement. Used for the right
		// on-screen stick, which only drives forwards/backwards.
		bool lockX = false;

		// Returns a vector in range [-1, 1].
		// Y axis points down!
		[[nodiscard]] Math::Vec2 GetVector() const noexcept
		{
			if (pressedData.HasValue())
			{
				auto vec = pressedData.Value().currPos;
				if (lockX)
					vec.x = 0.f;
				return vec;
			}
			return { 0.f, 0.f };
		}

		virtual WidgetEvent_Measure_ReturnT WidgetEvent_Measure(
			WidgetEvent_Measure_Params const& params) const override;
		virtual void WidgetEvent_Render(
			Render_Params const& params,
			Std::Opt<Gui::RectCollection::Iter> const& rectCollIter) const override;
		virtual bool CursorPress2(
			CursorPressParams const& params,
			Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
			bool occluded) override;
		virtual bool CursorMove(
			CursorMoveParams const& params,
			Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
			bool consumed) override;
		virtual Gui::TouchEventConsumption WidgetEvent_TouchMove(
			WidgetEvent_TouchMoveParams const& params,
			Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
			bool occluded) override;
		virtual Gui::TouchEventConsumption WidgetEvent_TouchPress(
			WidgetEvent_TouchPressParams const& params,
			Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
			bool consumed) override;
	};
}