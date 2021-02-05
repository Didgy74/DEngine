#pragma once

#include <DEngine/Gui/DrawInfo.hpp>
#include <DEngine/Gui/Events.hpp>
#include <DEngine/Gui/SizeHint.hpp>
#include <DEngine/Gui/Utility.hpp>

namespace DEngine::Gui
{
	class Context;

	class Widget
	{
	public:
		Widget() = default;
		Widget(Widget const&) = delete;
		Widget(Widget&&) = delete;
		inline virtual ~Widget() = 0;

		Widget& operator=(Widget const&) = delete;
		Widget& operator=(Widget&&) = delete;

		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const = 0;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const {}

		virtual void CharEnterEvent(
			Context& ctx) {}

		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) {}

		virtual void CharRemoveEvent(
			Context& ctx) {}

		virtual void InputConnectionLost() {}

		virtual void CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event) {}

		virtual void CursorClick(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) {}

		virtual void TouchEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			TouchEvent event) {}
	};

	inline Widget::~Widget() {}
}