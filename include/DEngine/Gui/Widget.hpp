#pragma once

#include <DEngine/Gui/Events.hpp>
#include <DEngine/Gui/Utility.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

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

		virtual void Render(
			Context& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			DrawInfo& drawInfo) const {}

		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) {}

		virtual void CharRemoveEvent(
			Context& ctx) {}

		virtual void CursorMove(
			Rect widgetRect,
			CursorMoveEvent event) {}

		virtual void CursorClick(
			Rect widgetRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) {}

		virtual void TouchEvent(
			Rect widgetRect,
			TouchEvent touch) {}

		virtual void Tick(
			Context& ctx,
			Rect widgetRect) {}
	};

	inline Widget::~Widget() {}
}