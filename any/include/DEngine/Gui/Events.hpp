#pragma once

// This file contains the structs that may be sent into
// the GUI context.

#include <DEngine/Gui/WindowID.hpp>
#include <DEngine/Gui/Utility.hpp>

namespace DEngine::Gui
{
	struct WindowContentScaleEvent {
		WindowID windowId;
		f32 scale;
	};

	struct WindowResizeEvent
	{
		WindowID windowId;
		Extent extent;
		Math::Vec2UInt safeAreaOffset;
		Extent safeAreaExtent;
	};

	struct WindowFocusEvent
	{
		WindowID windowId;
		bool gainedFocus;
	};

	struct WindowMoveEvent
	{
		WindowID windowId;
		Math::Vec2Int position;
	};

	struct WindowMinimizeEvent
	{
		WindowID windowId;
		bool wasMinimized;
	};

	struct WindowCloseEvent
	{
		WindowID windowId;
	};

	struct WindowCursorExitEvent
	{
		WindowID windowId;
	};

	struct CursorMoveEvent
	{
		WindowID windowId;
		Math::Vec2Int position;
		Math::Vec2Int positionDelta;
	};

	enum class CursorButton
	{
		Primary,
		Secondary,
		COUNT
	};
	struct CursorPressEvent
	{
		WindowID windowId;
		CursorButton button;
		bool pressed;
	};

	struct TouchPressEvent {
		WindowID windowId;
		u8 id;
		Math::Vec2 position;
		bool pressed;
	};

	struct TouchMoveEvent {
		WindowID windowId;
		u8 id;
		Math::Vec2 position;
	};

	struct TextInputEvent {
		WindowID windowId = WindowID::Invalid;
		u64 start = 0;
		u64 count = 0;

		// The new substring to insert.
		// This may be a nullptr, in which case means the destination
		// substring should be completely removed and replaced with nothing.
		//
		// This substring is NOT null-terminated
		Std::Span<u32 const> newText;
	};

	struct TextSelectionEvent {
		WindowID windowId = WindowID::Invalid;
		u64 start = 0;
		u64 count = 0;
	};

	struct TextDeleteEvent {
		WindowID windowId = WindowID::Invalid;
	};

	struct EndTextInputSessionEvent {
		WindowID windowId;
	};
}