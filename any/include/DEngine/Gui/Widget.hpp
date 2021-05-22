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

		// Returns true if the widget occludes the cursor.
		// That means widgets that are behind/under this one in terms of Z axis,
		// should not react to this cursor-move for i.e highlights.
		// 
		// Transparent overlays should always return false.
		//
		// The change is reflected in the "occluded" parameter for all
		// subsequent widget this event dispatch.
		// 
		// If the occluded parameter is already true,
		// the return value should ordinarily be ignored.
		//
		// If the return value is true, it does NOT ordinarily mean
		// you can end event dispatching early. That is to say
		// this event should ordinarily always be passed to every widget
		// in the hierarchy.
		virtual bool CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event,
			bool occluded) { return false; }

		// Return true if event has been consumed.
		//
		// You can end dispatching ONLY if the button is down-pressed.
		// Unpressed button events should ordinarily be dispatched everywhere.
		virtual bool CursorPress(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) { return false; }

		virtual void TouchEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			TouchEvent event,
			bool occluded) {}
	};

	inline Widget::~Widget() {}
}