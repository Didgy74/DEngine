#pragma once

#include <future>

import DEngine.FixedWidthTypes;
import DEngine.Math.Vector;
import DEngine.Std.Array;
import DEngine.Std.Opt;
import DEngine.Std.RangeFnRef;
import DEngine.Std.Span;
import DEngine.Std.StackVec;

namespace DEngine::Platform {
	enum class WindowID : u64 {
		Invalid = (u64)-1,
	};
	struct Extent {
		u32 width;
		u32 height;

        [[nodiscard]] auto operator==(Extent const& other) const noexcept {
            return this->width == other.width && this->height == other.height;
        }
        [[nodiscard]] auto operator!=(Extent const& other) const noexcept { return !(*this == other); }
	};
	enum class WindowEventFlag : u64 {
		// TODO: This one can be a bit problematic because we are changing WindowInsets and
		// size as a single event. But sometimes only the WindowInsets changed, and that
		// should not trigger us sending a window-resize signal to the renderer.
		Resize = (1 << 1),
		Move = (1 << 2),
		Restore = (1 << 3),
		Focus = (1 << 4),
		Orientation = (1 << 5),
		ContentScale = (1 << 6),
		Dpi = (1 << 7),
		CursorMove = (1 << 8),
		TouchEvent = (1 << 9),
	};
	[[nodiscard]] inline WindowEventFlag operator&(WindowEventFlag const& a, WindowEventFlag const& b) {
		return WindowEventFlag((u64)a & (u64)b);
	}
	[[nodiscard]] inline bool Contains(WindowEventFlag const& a, WindowEventFlag const& b) {
		return ((u64)a & (u64)b) != 0;
	}

	struct WindowEvents {
		bool resize;
		bool move;
		bool focus;
		bool unfocus;
		bool cursorEnter;
		bool cursorExit;
		bool minimize;
		bool restore;
		bool orientation;
	};

	// The insets can differ based on what is causing the inset.
	// I.e the system bar on Android will not cause as much inset as the touchscreen keyboard will.
	enum class WindowInsetSource {
		SystemBars,
		TextInputSystem,
		// Don't use!
		COUNT,
	};

	// Describes distances from each side of the Window. Should be used to layout UI correctly.
	struct WindowInsets {
		u32 left;
		u32 right;
		u32 top;
		u32 bottom;

		[[nodiscard]] constexpr bool operator==(WindowInsets const& other) const noexcept {
			return
				this->left == other.left
				&& this->right == other.right
				&& this->top == other.top
				&& this->bottom == other.bottom;
		}
		[[nodiscard]] constexpr bool operator!=(WindowInsets const& other) const noexcept { return !(*this == other); }
	};

	// TODO: For text? Need more documentation here
	struct SelectionRange {
		u64 index;
		u64 count;
	};

	Std::StackVec<char const*, 5> GetRequiredVkInstanceExtensions() noexcept;

	enum class CursorType : u8;

	enum class Orientation : u8;

	enum class OS : u8 { Windows, Linux, Android };
	// If this fails to compile, it's likely a build script issue.
	[[maybe_unused]] constexpr OS activeOS = (OS)DENGINE_PLATFORM_OS;
	//enum class Platform : u8;

	// Rename to keyboard button?
	enum class Button : u16;
	enum class CursorButton {
		Primary,
		Secondary,
		COUNT
	};
	enum class KeyEventType : u8 { Unchanged, Unpressed, Pressed, };

	struct CursorData;

	constexpr uSize maxTouchEventCount = 10;
	enum class TouchEventType : u8;
	struct TouchInput;
	Std::StackVec<TouchInput, maxTouchEventCount> TouchInputs();

	// It's unclear whether this will work long-term or if we need
	// a byte-array.
	enum class GamepadDeviceId : u64 { Invalid = (u64)-1, };
	enum class GamepadKey : u8 {
		Invalid,
		A,
		B,
		X,
		Y,
		L1,
		L2,
		L3,
		R1,
		R2,
		R3,
		COUNT,
	};
	enum class GamepadAxis : u8 {
		Invalid,
		// Left joystick X direction
		LeftX, // [-1, 1]
		// Left joystick Y direction
		LeftY, // [-1, 1]
		// Right joystick X direction
		RightX, // [-1, 1]
		// Right joystick Y direction
		RightY, // [-1, 1]
		COUNT,
	};
	struct GamepadState {
		Std::Array<bool, (int)GamepadKey::COUNT> keyStates = {};
		[[nodiscard]] bool GetKeyState(GamepadKey btn) const noexcept;

		Std::Array<KeyEventType, (int)GamepadKey::COUNT> keyEvents = {};
		[[nodiscard]] KeyEventType GetKeyEvent(GamepadKey btn) const noexcept;

		Std::Array<f32, (int)GamepadAxis::COUNT> axisValues = {};
		[[nodiscard]] f32 GetAxisValue(GamepadAxis axis) const noexcept;

		f32 stickDeadzone = 0.1f;
	};

	enum class LogSeverity {
		Debug,
		Warning,
		Error,
	};
	class EventForwarder;
	enum class SoftInputFilter : u8;
	class FileInputStream;

	struct AccessibilityUpdateElement {
		u64 widgetId;
		i32 posX;
		i32 posY;
		u32 width;
		u32 height;
		u64 textStart;
		u64 textCount;
		bool isClickable;
	};

	class Context {
	public:
		Context(Context const&) = delete;
		Context(Context&& other) noexcept :
			m_implData{ other.m_implData }
		{
			other.m_implData = nullptr;
		}
		~Context() noexcept;

		Context& operator=(Context const&) = delete;
		Context& operator=(Context&&) noexcept;

		// Thread safe within a frame
		// TODO: Tick-count for what? I think it's the number of time we have run PollEvents.
		[[nodiscard]] u64 TickCount() const noexcept;

		struct NewWindow_ReturnT {
			WindowID windowId = {};
			Extent extent = {};
			Math::Vec2Int position = {};
			Extent visibleExtent = {};
			Math::Vec2UInt visibleOffset = {};
			f32 dpiX = 0.f;
			f32 dpiY = 0.f;
			f32 contentScale = 1.f;
			Std::Opt<u64> touchScrollSlopPx;
			Std::Opt<u64> scrollBarWidthPx;

			// TODO: I don't like exposing an array like this, but whatever.
			// Ideally we would have an Array-type that just encapsulates the enum stuff.
			Std::Array<WindowInsets, (int)WindowInsetSource::COUNT> insets = {};
		};
		// Thread-safe
		// TODO: Make it return a future. This can be a bit hard because
		// the implementation also uses futures and we need to transform them.
		[[nodiscard]] Std::Opt<NewWindow_ReturnT> NewWindow(
			Std::Span<char const> title,
			Extent extent);
		[[nodiscard]] bool CanCreateNewWindow() const noexcept;
		void DestroyWindow(WindowID) noexcept;
		[[nodiscard]] u32 GetWindowCount() const noexcept;
		[[nodiscard]] Extent GetWindowExtent(WindowID) const noexcept;
		[[nodiscard]] Math::Vec2Int GetWindowPosition(WindowID) const noexcept;
		[[nodiscard]] Extent GetWindowVisibleExtent(WindowID) const noexcept;
		[[nodiscard]] Math::Vec2Int GetWindowVisibleOffset(WindowID) const noexcept;
		[[nodiscard]] WindowEvents GetWindowEvents(WindowID) const noexcept;
		[[nodiscard]] WindowEventFlag GetWindowEventFlags(WindowID) const noexcept;
		[[nodiscard]] bool IsWindowMinimized(WindowID) const noexcept;

		void UpdateAccessibilityData(
			WindowID windowId,
			Std::RangeFnRef<AccessibilityUpdateElement> const& range,
			Std::Span<char const> textData) noexcept;

		struct CreateVkSurface_ReturnT {
			u32 vkResult;
			u64 vkSurface;
		};
		// Thread-safe
		[[nodiscard]] CreateVkSurface_ReturnT CreateVkSurface_ThreadSafe(
			WindowID window,
			void const* vkGetInstanceProcAddrIn,
			uSize vkInstance,
			void const* vkAllocationCallbacks) noexcept;

		[[nodiscard]] Std::Opt<CursorData> Cursor() noexcept;
		void LockCursor(bool state);
		void SetCursor(WindowID window, CursorType cursor) noexcept;

		// Thread safe within a frame
		[[nodiscard]] Std::Opt<GamepadState> GetGamepad(uSize index) const noexcept;

		void Log(LogSeverity severity, Std::Span<char const> msg);

		void StartTextInputSession(
			WindowID windowId,
			SoftInputFilter inputFilter,
			Std::Span<char const> text,
			u64 selectionStart,
			u64 selectionCount);
		void UpdateTextInputConnection(
			WindowID windowId,
			u64 selIndex,
			u64 selCount,
			Std::Span<u32 const> text);
		void UpdateTextInputConnectionSelection(u64 selIndex, u64 selCount);
		void StopTextInputSession();

		// Internal stuff, don't use it.
		struct Impl;
		friend Impl;
		[[nodiscard]] Impl& GetImplData() noexcept;
		[[nodiscard]] Impl const& GetImplData() const noexcept;

	protected:
		Context() = default;

		struct Backend;
		friend Backend;
		struct BackendInterface;
		friend BackendInterface;

		Impl* m_implData = nullptr;
	};


}

enum class DEngine::Platform::CursorType : DEngine::u8
{
	Arrow,
	HorizontalResize,
	VerticalResize,
	COUNT,
};

enum class DEngine::Platform::Orientation : DEngine::u8
{
	Invalid,
	Landscape,
	Portrait
};

enum class DEngine::Platform::Button : DEngine::u16
{
	Undefined,
	Zero, One, Two, Three, Four, Five, Six, Seven, Eight, Nine,
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
	Up, Down, Left, Right,
	Backspace,
	Delete,
	End,
	Enter,
	Escape,
	Home,
	Insert,
	PageUp,
	PageDown,
	Space,
	Tab,
	LeftCtrl,

	Back,

	COUNT
};

struct DEngine::Platform::CursorData {
	// TODO: Add which window it belongs to?
	Math::Vec2Int position;
	Math::Vec2Int positionDelta;
	f32 scrollDeltaY;

	Std::Array<bool, (int)CursorButton::COUNT> buttonPressedArray;
	[[nodiscard]] bool ButtonPressed(CursorButton button) const noexcept
	{
		return buttonPressedArray[(int)button];
	}
};

enum class DEngine::Platform::TouchEventType : DEngine::u8
{
	Unchanged,
	Down,
	Moved,
	Up
};

struct DEngine::Platform::TouchInput {
	static constexpr u8 invalidID = static_cast<u8>(-1);

	u8 id = invalidID;
	TouchEventType eventType = TouchEventType::Unchanged;
	f32 x = 0.f;
	f32 y = 0.f;
	f32 duration = 0.f;
};

enum class DEngine::Platform::SoftInputFilter : DEngine::u8 {
	Text,
	Integer,
	UnsignedInteger,
	Float,
	UnsignedFloat
};

class DEngine::Platform::FileInputStream
{
public:
	FileInputStream();
	explicit FileInputStream(char const* path);
	explicit FileInputStream(Std::Span<char const> path);
	FileInputStream(FileInputStream const&) = delete;
	FileInputStream(FileInputStream&&) noexcept;
	~FileInputStream();

	FileInputStream& operator=(FileInputStream const&) = delete;
	FileInputStream& operator=(FileInputStream&&) noexcept;

	enum class SeekOrigin
	{
		Current,
		Start,
		End
	};
	bool Seek(
		i64 offset,
		SeekOrigin origin = SeekOrigin::Current);
	bool Read(
		char* output, 
		u64 size);
	[[nodiscard]] Std::Opt<u64> Tell() const;
	[[nodiscard]] bool IsOpen() const;
	bool Open(char const* path);
	void Close();

private:
	// Might not be safe, dunno yet
	alignas(8) char m_buffer[16] = {};
};

// Modifications to the App::Context should generally not happen during the event-callbacks
//
// The Context reference passed to the callback is going to reflect the most up to date
// state of the Context, not the state it had when the event happened.
//
// The EventForwarder design is a bit weak because everything is happening deferredly.
// The events are called on the caller only after they have been processed on the backend side.
// This is problematic if you need to read other values from Context and not just exactly what
// changed. Ideally we should have a design that makes it easy to keep your own "snapshot" of the
// Platform::Context in between events, so that you can track them.
//
// Some events are also just naturally blocking, such as accessibility querying stuff...
class DEngine::Platform::EventForwarder
{
public:
	inline virtual ~EventForwarder() = 0;

	virtual void ButtonEvent(
		Platform::Context& appCtx,
		WindowID windowId,
		Button button,
		bool state) {}
	virtual void WindowCursorEnter(
		WindowID window,
		bool entered) {}
	virtual void CursorMove(
		Context& appCtx,
		WindowID windowId,
		Math::Vec2Int position,
		Math::Vec2Int positionDelta,
		CursorData cursorData) {}
	virtual void CursorPress(
		Context& appCtx,
		WindowID windowId,
		CursorButton button,
		bool pressed,
		CursorData cursorData) {}
	struct TextInputEventJob {
		u64 oldStart;
		u64 oldCount;
	};
	virtual void TextInputEvent(
		Context& ctx,
		WindowID windowId,
		u64 start,
		u64 count,
		Std::Span<u32 const> newString) {}
	virtual void TextSelectionEvent(
		Context& ctx,
		WindowID windowId,
		u64 start,
		u64 count) {}
	virtual void TextDeleteEvent(
		Context& ctx,
		WindowID windowId) {}
	virtual void EndTextInputSessionEvent(
		Context& ctx,
		WindowID windowId) {}
	virtual void TouchEvent(
		WindowID windowId,
		u8 id,
		TouchEventType type,
		Math::Vec2 position) {}

	// Probably should forward the previous state?
	// I.e how to determine if we crossed a threshold
	virtual void GamepadAxisEvent(
		Context& ctx,
		WindowID windowId,
		GamepadDeviceId deviceId,
		GamepadAxis axis,
		f32 value) {}

	virtual void GamepadKeyEvent(
		Context& ctx,
		WindowID windowId,
		GamepadDeviceId deviceId,
		GamepadKey key,
		bool pressed) {}

	virtual void WindowContentScale(
		Context& appCtx,
		WindowID window,
		f32 newScale) {}
	virtual void WindowDpi(
		Context& appCtx,
		WindowID window,
		f32 dpiX,
		f32 dpiY) {}
	// Return true if window should be closed.
	[[nodiscard]] virtual bool WindowCloseSignal(
		Context& appCtx,
		WindowID window) { return false; }
	virtual void WindowFocus(
		WindowID window,
		bool focused) {}
	virtual void WindowMinimize(
		WindowID window,
		bool wasMinimized) {}
	virtual void WindowResize(
		Context& appCtx,
		WindowID window,
		Extent extent,
		Std::Array<WindowInsets, (int)WindowInsetSource::COUNT> windowInsets) {}
};

inline DEngine::Platform::EventForwarder::~EventForwarder() {}