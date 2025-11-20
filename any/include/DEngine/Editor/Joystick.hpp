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

		// Returns a vector in range [-1, 1].
		// Y axis points down!
		[[nodiscard]] Math::Vec2 GetVector() const noexcept
		{
			if (pressedData.HasValue())
				return pressedData.Value().currPos;
			return { 0.f, 0.f };
		}

		virtual GetSizeHint2_ReturnT GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void Render2(
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