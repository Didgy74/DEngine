#pragma once

#include <DEngine/Gui/SizeHint.hpp>
#include <DEngine/Gui/DrawInfo.hpp>
#include <DEngine/Gui/Events.hpp>
#include <DEngine/Gui/Utility.hpp>

namespace DEngine::Gui
{
	class Context;

	class Layout
	{
	public:
		Layout() noexcept = default;
		Layout(Layout const&) = delete;
		Layout(Layout&&) noexcept = default;
		inline virtual ~Layout() = 0;

		[[nodiscard]] virtual Gui::SizeHint SizeHint(
			Context const& ctx) const = 0;

		[[nodiscard]] virtual Gui::SizeHint SizeHint_Tick(
			Context const& ctx) = 0;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const {}

		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) {}

		virtual void CharRemoveEvent(
			Context& ctx) {}

		virtual void CursorMove(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event) {}

		virtual void CursorClick(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) {}

		virtual void TouchEvent(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect,
			TouchEvent event) {}

		virtual void Tick(
			Context& ctx,
			Rect widgetRect,
			Rect visibleRect) {}
	};

	inline Layout::~Layout() {}
}