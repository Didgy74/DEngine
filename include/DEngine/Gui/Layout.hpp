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

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const = 0;

		virtual void CharEnterEvent(
			Context& ctx) = 0;

		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) = 0;

		virtual void CharRemoveEvent(
			Context& ctx) = 0;

		virtual void CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event) = 0;

		virtual void CursorClick(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) = 0;

		virtual void TouchEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchEvent event) = 0;
	};
}

inline DEngine::Gui::Layout::~Layout() {}

inline void DEngine::Gui::Layout::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const {}

inline void DEngine::Gui::Layout::CharEnterEvent(
	Context& ctx) {}

inline void DEngine::Gui::Layout::CharEvent(
	Context& ctx,
	u32 utfValue) {}

inline void DEngine::Gui::Layout::CharRemoveEvent(
	Context& ctx) {}

inline void DEngine::Gui::Layout::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event) {}

inline void DEngine::Gui::Layout::CursorClick(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event) {}

inline void DEngine::Gui::Layout::TouchEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event) {}