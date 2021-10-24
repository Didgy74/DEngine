#pragma once

#include <DEngine/Gui/WindowID.hpp>
#include <DEngine/Gui/Utility.hpp>

namespace DEngine::Gui
{
	struct WindowResizeEvent
	{
		WindowID windowId;
		Extent extent;
		Rect visibleRect;
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

	struct CharRemoveEvent {};
}