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

	struct WindowCursorEnterEvent
	{
		WindowID windowId;
		bool entered;
	};

	struct CursorMoveEvent
	{
		Math::Vec2Int position;
		Math::Vec2Int positionDelta;
	};

	enum class CursorButton
	{
		Left,
		Right,
		COUNT
	};
	struct CursorClickEvent
	{
		CursorButton button;
		bool clicked;
	};

	enum class TouchEventType
	{
		Unchanged,
		Down,
		Up,
		Moved
	};
	struct TouchEvent
	{
		u8 id;
		TouchEventType type;
		Math::Vec2 position;
	};

	struct CharEnterEvent {};

	struct CharEvent
	{
		u32 utfValue;
	};

	struct CharRemoveEvent {};
}