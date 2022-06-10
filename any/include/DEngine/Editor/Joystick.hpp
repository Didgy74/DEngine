#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Math/Vector.hpp>

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

		virtual Gui::SizeHint GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void Render2(
			Render_Params const& params,
			Gui::Rect const& widgetRect,
			Gui::Rect const& visibleRect) const override;
		virtual bool CursorPress2(
			CursorPressParams const& params,
			Gui::Rect const& widgetRect,
			Gui::Rect const& visibleRect,
			bool consumed) override;
		virtual bool CursorMove(
			CursorMoveParams const& params,
			Gui::Rect const& widgetRect,
			Gui::Rect const& visibleRect,
			bool occluded) override;
	};
}