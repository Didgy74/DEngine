#pragma once

#include <DEngine/Gui/Events.hpp>
#include <DEngine/Gui/SizeHint.hpp>
#include <DEngine/Gui/SizeHintCollection.hpp>
#include <DEngine/Gui/Utility.hpp>

#include <DEngine/Std/FrameAllocator.hpp>

namespace DEngine::Gui
{
	class DrawInfo;
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

		struct GetSizeHint2_Params
		{
			Context const& ctx;
			Std::FrameAllocator& transientAlloc;
			SizeHintCollection::Pusher& pusher;
		};
		/*
			The implementation should push its SizeHint onto the pusher,
		 	and also return a copy.
		*/
		virtual SizeHint GetSizeHint2(
			GetSizeHint2_Params const& params) const = 0;

		struct BuildChildRects_Params
		{
			Context const& ctx;
			SizeHintCollection::Pusher2& builder;
			Std::FrameAllocator& transientAlloc;
		};
		virtual void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const {};

		struct Render_Params
		{
			Context const& ctx;
			RectCollection const& rectCollection;
			Extent const& framebufferExtent;
			DrawInfo& drawInfo;
		};
		virtual void Render2(
			Render_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const = 0;

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

		virtual void CursorExit(
			Context& ctx) {}

		virtual bool CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event,
			bool occluded)
		{
			return false;
		}

		struct CursorMoveParams
		{
			Context& ctx;
			RectCollection const& rectCollection;
			WindowID windowId;
			CursorMoveEvent event;
		};
		/*
			Returns true if the widget occludes the move event cursor point.
			That means widgets that are behind/under this widget in Z axis
			should not react to this move event for i.e highlights.

		 	The cursor-point is not guaranteed to be within any of the Widgets rects.

			Transparent overlays should return false.

			The change is reflected in the "occluded" parameter for all
			subsequent widget this event dispatch.

			If the occluded parameter is already true,
			the return value should ordinarily be ignored.

			If the return value is true, it does NOT ordinarily mean
			you can end event dispatching early. That is to say
			this event should ordinarily always be passed to every widget
			in the hierarchy regardless if a widget occluded it.
		*/
		virtual bool CursorMove(
			CursorMoveParams const& params,
			bool occluded)
		{
			return false;
		}

		virtual bool CursorPress(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorPressEvent event) { return false; }

		struct CursorPressParams
		{
			Context& ctx;
			RectCollection const& rectCollection;
			WindowID windowId;
			Math::Vec2Int cursorPos;
			CursorPressEvent event;
		};
		/*
			Return true if event has been consumed.

		 	If the event was consumed already before this event was called,
		 	you can disregard the return-value.

		 	You can not end dispatching early. Pass all presses everywhere.
		*/
		virtual bool CursorPress2(
			CursorPressParams const& params,
			bool consumed)
		{
			return false;
		}

		// Returns true if the widget occludes the move event point.
		// That means widgets that are behind/under this one in terms of Z axis,
		// should not react to this move event for i.e highlights.
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
		virtual bool TouchMoveEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			TouchMoveEvent event,
			bool occluded) { return false; }

		// Return true if event has been consumed.
		//
		// You can end dispatching ONLY if the event is down-pressed.
		// Unpressed events should ordinarily be dispatched everywhere.
		virtual bool TouchPressEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			TouchPressEvent event) { return false; }
	};

	inline Widget::~Widget() {}
}