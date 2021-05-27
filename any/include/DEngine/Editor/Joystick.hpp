#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Math/Vector.hpp>

namespace DEngine::Editor
{
	class Joystick : public Gui::Widget
	{
	public:
		static constexpr u8 cursorPointerId = (u8)-1;

		struct ActiveData
		{
			u8 pointerId;
			// Holds relative position [-1, 1] bound to a circle.
			Math::Vec2 currPos = {};
		};
		Std::Opt<ActiveData> activeData = {};

		// Returns a vector in range [-1, 1].
		// Y axis points down!
		[[nodiscard]] Math::Vec2 GetVector() const noexcept
		{
			if (activeData.HasValue())
				return activeData.Value().currPos;
			return { 0.f, 0.f };
		}

		virtual bool CursorPress(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Math::Vec2Int cursorPos,
			Gui::CursorClickEvent event) override;

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