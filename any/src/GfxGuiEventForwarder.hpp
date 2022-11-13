#pragma once

// This file is only used in the Gui playground
// It will just directly forward any events fired by the Application layer,
// to the Gfx-context and the Gui-context.

#include <DEngine/Application.hpp>
#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Gui/Context.hpp>

namespace DEngine {

	class GfxGuiEventForwarder : public App::EventForwarder
	{
	public:
		Gui::Context* guiCtx = nullptr;
		Gfx::Context* gfxCtx = nullptr;

		virtual void WindowMinimize(
			App::WindowID window,
			bool wasMinimized) override
		{
			Gui::WindowMinimizeEvent event {};
			event.windowId = (Gui::WindowID)window;
			event.wasMinimized = wasMinimized;
			guiCtx->PushEvent(event);
		}

		virtual void WindowContentScale(
			App::Context& appCtx,
			App::WindowID window,
			f32 scale) override
		{
			Gui::WindowContentScaleEvent event {};
			event.windowId = (Gui::WindowID)window;
			event.scale = scale;
			guiCtx->PushEvent(event);
		}

		virtual bool WindowCloseSignal(
			App::Context& appCtx,
			App::WindowID window) override
		{
			guiCtx->DestroyWindow((Gui::WindowID)window);

			// Then destroy the window in the renderer.
			gfxCtx->DeleteNativeWindow((Gfx::NativeWindowID)window);

			// We return true to tell the appCtx to destroy the window.
			return true;
		}

		virtual void TextInputEvent(
			App::Context& ctx,
			App::WindowID windowId,
			uSize oldIndex,
			uSize oldCount,
			Std::Span<u32 const> newString) override
		{
			Gui::TextInputEvent event = {};
			event.windowId = (Gui::WindowID)windowId;
			event.oldIndex = oldIndex;
			event.oldCount = oldCount;
			event.newTextData = newString.Data();
			event.newTextSize = newString.Size();
			guiCtx->PushEvent(event);
		}

		virtual void WindowCursorEnter(
			App::WindowID window,
			bool entered) override
		{
			if (entered)
				return;
			Gui::WindowCursorExitEvent event = {};
			event.windowId = (Gui::WindowID)window;
			guiCtx->PushEvent(event);
		}

		virtual void WindowFocus(
			App::WindowID window,
			bool focused) override
		{
			if (focused)
			{
				Gui::WindowFocusEvent event = {};
				event.windowId = (Gui::WindowID)window;
				event.gainedFocus = focused;
				guiCtx->PushEvent(event);
			}
		}

		virtual void WindowMove(
			App::WindowID window,
			Math::Vec2Int position) override
		{
			Gui::WindowMoveEvent event = {};
			event.windowId = (Gui::WindowID)window;
			event.position = position;
			guiCtx->PushEvent(event);
		}

		virtual void WindowResize(
			App::Context& ctx,
			App::WindowID window,
			App::Extent extent,
			Math::Vec2UInt visibleOffset,
			App::Extent visibleExtent) override
		{
			Gui::WindowResizeEvent event = {};
			event.windowId = (Gui::WindowID)window;
			event.extent = { extent.width, extent.height };
			event.safeAreaOffset = visibleOffset;
			event.safeAreaExtent = { visibleExtent.width, visibleExtent.height };
			guiCtx->PushEvent(event);
		}

		virtual void ButtonEvent(
			App::WindowID windowId,
			App::Button button,
			bool state) override
		{
			if (button == App::Button::LeftMouse)
			{
				Gui::CursorPressEvent event = {};
				event.windowId = (Gui::WindowID)windowId;
				event.button = Gui::CursorButton::Primary;
				event.pressed = state;
				guiCtx->PushEvent(event);
			}
		}

		virtual void CursorMove(
			App::Context& ctx,
			App::WindowID windowId,
			Math::Vec2Int position,
			Math::Vec2Int positionDelta) override
		{
			Gui::CursorMoveEvent moveEvent = {};
			moveEvent.windowId = (Gui::WindowID)windowId;
			moveEvent.position = position;
			moveEvent.positionDelta = positionDelta;
			guiCtx->PushEvent(moveEvent);
		}

		virtual void TouchEvent(
			App::WindowID windowId,
			u8 id,
			App::TouchEventType type,
			Math::Vec2 position) override
		{
			if (type == App::TouchEventType::Down || type == App::TouchEventType::Up) {
				Gui::TouchPressEvent event = {};
				event.windowId = (Gui::WindowID)windowId;
				event.position = position;
				event.id = id;
				event.pressed = type == App::TouchEventType::Down;
				guiCtx->PushEvent(event);
			}
			else if (type == App::TouchEventType::Moved) {
				Gui::TouchMoveEvent event = {};
				event.windowId = (Gui::WindowID)windowId;
				event.position = position;
				event.id = id;
				guiCtx->PushEvent(event);
			}
		}
	};
}