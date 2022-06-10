#pragma once

#include <DEngine/Gui/Events.hpp>
#include <DEngine/Gui/SizeHint.hpp>
#include <DEngine/Gui/RectCollection.hpp>
#include <DEngine/Gui/AllocRef.hpp>
#include <DEngine/Gui/Utility.hpp>

#include <DEngine/Std/FrameAllocator.hpp>

namespace DEngine::Gui
{
	class DrawInfo;
	class Context;
	class TextManager;

	class Widget
	{
	public:
		Widget() = default;
		Widget(Widget const&) = delete;
		Widget(Widget&&) = delete;
		inline virtual ~Widget() = 0;

		Widget& operator=(Widget const&) = delete;
		Widget& operator=(Widget&&) = delete;


		struct GetSizeHint2_Params
		{
			Context const& ctx;
			TextManager& textManager;
			AllocRef const& transientAlloc;
			RectCollection::SizeHintPusher& pusher;
		};
		/*
			The implementation should push its SizeHint onto the pusher,
		 	and also return a copy. This is bad design, I know. It's WIP.
		*/
		virtual SizeHint GetSizeHint2(
			GetSizeHint2_Params const& params) const = 0;

		struct BuildChildRects_Params
		{
			Context const& ctx;
			TextManager& textManager;
			AllocRef const& transientAlloc;
			RectCollection::RectPusher& pusher;
		};
		/*
			A Layout is responsible to assign the Rects of
			immediate children.
		 */
		virtual void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const {}

		struct Render_Params
		{
			Context const& ctx;
			TextManager& textManager;
			RectCollection const& rectCollection;
			Extent const& framebufferExtent;
			AllocRef const& transientAlloc;
			DrawInfo& drawInfo;
		};
		virtual void Render2(
			Render_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const = 0;

		struct CursorMoveParams
		{
			Context& ctx;
			RectCollection const& rectCollection;
			TextManager& textManager;
			AllocRef const& transientAlloc;
			WindowID windowId;
			CursorMoveEvent const& event;
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
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool occluded)
		{
			return false;
		}

		struct CursorPressParams
		{
			Context& ctx;
			RectCollection const& rectCollection;
			TextManager& textManager;
			AllocRef const& transientAlloc;
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
			Rect const& widgetRect,
			Rect const& visibleRect,
			bool consumed)
		{
			return false;
		}

		virtual void TextInput(
			Context& ctx,
			AllocRef const& transientAlloc,
			TextInputEvent const& event) {}
		virtual void EndTextInputSession(
			Context& ctx,
			AllocRef const& transientAlloc,
			EndTextInputSessionEvent const& event) {}

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

		virtual bool CursorPress(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorPressEvent event) { return false; }



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