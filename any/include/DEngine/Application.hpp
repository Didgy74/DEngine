#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Array.hpp>
#include <DEngine/Std/Containers/RangeFnRef.hpp>
#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Containers/Str.hpp>

#include <DEngine/Math/Vector.hpp>

#include <DEngine/Std/Defines.hpp>

namespace DEngine::Application
{
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

	struct SelectionRange {
		u64 index;
		u64 count;
	};

	Std::StackVec<char const*, 5> GetRequiredVkInstanceExtensions() noexcept;

	enum class CursorType : u8;

	enum class Orientation : u8;

	enum class OS : u8 { Windows, Linux, Android };
	// If this fails to compile, it's likely a build script issue.
	[[maybe_unused]] constexpr OS activeOS = (OS)DENGINE_OS;
	enum class Platform : u8;

	enum class Button : u16;
	enum class KeyEventType : u8 {
		Unchanged,
		Unpressed,
		Pressed,
	};

	struct CursorData;

	constexpr uSize maxTouchEventCount = 10;
	enum class TouchEventType : u8;
	struct TouchInput;
	Std::StackVec<TouchInput, maxTouchEventCount> TouchInputs();

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
	enum class GamepadAxis : u8
	{
		Invalid,
		LeftX, // [-1, 1]
		LeftY, // [-1, 1]
		COUNT,
	};
	struct GamepadState
	{
	public:
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
		int posX;
		int posY;
		int width;
		int height;
		int textStart;
		int textCount;
		bool isClickable;
	};

	class Context
	{
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
		[[nodiscard]] u64 TickCount() const noexcept;

		struct NewWindow_ReturnT
		{
			WindowID windowId = {};
			Extent extent = {};
			Math::Vec2Int position = {};
			Extent visibleExtent = {};
			Math::Vec2UInt visibleOffset = {};
			f32 dpiX = 0.f;
			f32 dpiY = 0.f;
			f32 contentScale = 1.f;
		};
		// Thread-safe
		[[nodiscard]] NewWindow_ReturnT NewWindow(
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
			Std::ConstByteSpan textData) noexcept;

		struct CreateVkSurface_ReturnT {
			u32 vkResult;
			u64 vkSurface;
		};
		// Thread-safe
		[[nodiscard]] CreateVkSurface_ReturnT CreateVkSurface_ThreadSafe(
			WindowID window,
			uSize vkInstance,
			void const* vkAllocationCallbacks) noexcept;


		[[nodiscard]] Std::Opt<CursorData> Cursor() noexcept;
		void LockCursor(bool state);
		void SetCursor(WindowID window, CursorType cursor) noexcept;

		// Thread safe within a frame
		[[nodiscard]] Std::Opt<GamepadState> GetGamepad(uSize index) const noexcept;

		void Log(LogSeverity severity, Std::Span<char const> msg);

		void StartTextInputSession(WindowID windowId, SoftInputFilter inputFilter, Std::Span<char const> text);
		void UpdateTextInputConnection(u64 selIndex, u64 selCount, Std::Span<u32 const> text);
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

namespace DEngine
{
	namespace App = Application;
	namespace Platform = App;
}

enum class DEngine::Application::CursorType : DEngine::u8
{
	Arrow,
	HorizontalResize,
	VerticalResize,
	COUNT,
};

enum class DEngine::Application::Orientation : DEngine::u8
{
	Invalid,
	Landscape,
	Portrait
};

enum class DEngine::Application::Button : DEngine::u16
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
	LeftMouse,
	RightMouse,

	Back,

	COUNT
};

struct DEngine::Application::CursorData
{
	Math::Vec2Int position;
	Math::Vec2Int positionDelta;
	
	f32 scrollDeltaY;
};

enum class DEngine::Application::TouchEventType : DEngine::u8
{
	Unchanged,
	Down,
	Moved,
	Up
};

struct DEngine::Application::TouchInput
{
	static constexpr u8 invalidID = static_cast<u8>(-1);

	u8 id = invalidID;
	TouchEventType eventType = TouchEventType::Unchanged;
	f32 x = 0.f;
	f32 y = 0.f;
	f32 duration = 0.f;
};

enum class DEngine::Application::SoftInputFilter : DEngine::u8
{
	Text,
	Integer,
	UnsignedInteger,
	Float,
	UnsignedFloat
};

class DEngine::Application::FileInputStream
{
public:
	FileInputStream();
	explicit FileInputStream(char const* path);
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
class DEngine::Application::EventForwarder
{
public:
	inline virtual ~EventForwarder() = 0;

	virtual void ButtonEvent(
		WindowID windowId,
		Button button,
		bool state) {}
	virtual void CursorMove(
		Context& appCtx,
		WindowID windowId,
		Math::Vec2Int position,
		Math::Vec2Int positionDelta) {}
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
	virtual void WindowCursorEnter(
		WindowID window,
		bool entered) {}
	virtual void WindowFocus(
		WindowID window,
		bool focused) {}
	virtual void WindowMinimize(
		WindowID window,
		bool wasMinimized) {}
	virtual void WindowMove(
		WindowID window, 
		Math::Vec2Int position) {}
	virtual void WindowResize(
		Context& appCtx,
		WindowID window,
		Extent extent,
		Math::Vec2UInt visibleOffset,
		Extent visibleExtent) {}
};

inline DEngine::Application::EventForwarder::~EventForwarder() {}