#pragma once

#include <DEngine/Gui/WindowID.hpp>
#include <DEngine/Gui/Utility.hpp>

namespace DEngine::Gui
{
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
		u8 id;
		Math::Vec2 position;
		bool pressed;
	};

	struct TouchMoveEvent
	{
		u8 id;
		Math::Vec2 position;
	};

	struct CharEnterEvent {};

	struct CharEvent
	{
		u32 utfValue;
	};

	struct CharRemoveEvent
	{
		WindowID windowId;
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