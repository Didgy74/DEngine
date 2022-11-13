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

	struct TouchPressEvent
	{
		WindowID windowId;
		u8 id;
		Math::Vec2 position;
		bool pressed;
	};

	struct TouchMoveEvent
	{
		WindowID windowId;
		u8 id;
		Math::Vec2 position;
	};

	struct TextInputEvent
	{
		WindowID windowId;

		// The index of the substring that should be replaced
		// This can be the end of the entire string, in which case
		// string needs to grow.
		//
		// It is not legal for it to be more than 1 past the string-size.
		uSize oldIndex;

		// The size of the substring that should be replaced.
		uSize oldCount;

		// The new substring to insert.
		// This may be a nullptr, in which case means the destination
		// substring should be completely removed and replaced with nothing.
		//
		// This substring is NOT null-terminated
		u32 const* newTextData;

		// Size of the new substring to insert.
		uSize newTextSize;
	};

	struct EndTextInputSessionEvent
	{
		WindowID windowId;
	};
}