export module DEngine.Gui.Events;

import DEngine.FixedWidthTypes;
import DEngine.Gui.Utility;
import DEngine.Gui.WindowID;
import DEngine.Gui.WindowInsets;
import DEngine.Math.Vector;
import DEngine.Std.Span;

export namespace DEngine::Gui {
	struct WindowContentScaleEvent {
		WindowID windowId;
		f32 scale;
	};

	struct WindowResizeEvent {
		WindowID windowId;
		Extent extent;
		WindowInsets insets;
	};

	struct WindowFocusEvent {
		WindowID windowId;
		bool gainedFocus;
	};

	struct WindowMoveEvent {
		WindowID windowId;
		Math::Vec2Int position;
	};

	struct WindowMinimizeEvent {
		WindowID windowId;
		bool wasMinimized;
	};

	struct WindowCloseEvent {
		WindowID windowId;
	};

	struct WindowCursorExitEvent {
		WindowID windowId;
	};

	// TODO: To be removed but currently used in Layer.hpp
	struct CursorMoveEvent {
		WindowID windowId;
		Math::Vec2Int position;
		Math::Vec2Int positionDelta;
	};

	enum class CursorButton { Primary, Secondary, COUNT };
	// TODO: To be removed but currently used in Layer.hpp
	struct CursorPressEvent {
		WindowID windowId;
		CursorButton button;
		bool pressed;
	};

	// TODO: To be removed but currently used in Layer.hpp
	struct TouchPressEvent {
		WindowID windowId;
		u8 id;
		Math::Vec2 position;
		bool pressed;
	};

	// TODO: To be removed but currently used in Layer.hpp
	struct TouchMoveEvent {
		WindowID windowId;
		u8 id;
		Math::Vec2 position;
	};
} // namespace DEngine::Gui
