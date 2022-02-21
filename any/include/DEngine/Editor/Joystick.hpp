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
			return {0.f, 0.f};
		}

		virtual Gui::SizeHint GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void Render2(
			Render_Params const& params,
			Gui::Rect const& widgetRect,
			Gui::Rect const& visibleRect) const override;

		virtual bool CursorPress(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Math::Vec2Int cursorPos,
			Gui::CursorPressEvent event) override;

		virtual bool CursorMove(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Gui::CursorMoveEvent event,
			bool occluded) override;

		virtual bool TouchMoveEvent(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Gui::TouchMoveEvent event,
			bool occluded) override;

		virtual bool TouchPressEvent(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Gui::TouchPressEvent event) override;

		[[nodiscard]] virtual Gui::SizeHint GetSizeHint(
			Gui::Context const& ctx) const override;

		virtual void Render(
			Gui::Context const& ctx,
			Gui::Extent framebufferExtent,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Gui::DrawInfo& drawInfo) const override;
	};
}