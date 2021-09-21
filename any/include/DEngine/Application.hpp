#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Array.hpp>
#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Containers/Str.hpp>

#include <DEngine/Math/Vector.hpp>

#if defined(_WIN32) || defined(_WIN64)
#	define DENGINE_OS_WINDOWS
#elif defined(__ANDROID__)
#	define DENGINE_OS_ANDROID
#elif defined(__GNUC__)
#	define DENGINE_OS_LINUX
#else
#	error Error. DEngine does not support this platform/compiler
#endif

namespace DEngine::Application
{
	enum class WindowID : u64;
	struct Extent
	{
		u32 width;
		u32 height;
	};
	struct WindowEvents
	{
		bool resize;
		bool move;
		bool focus;
		bool unfocus;
		bool cursorEnter;
		bool cursorExit;
		bool minimize;
		bool restore;
	};
	
	WindowID CreateWindow(
		char const* title,
		Extent extents);
	void DestroyWindow(WindowID) noexcept;
	u32 GetWindowCount() noexcept;
	Extent GetWindowExtent(WindowID) noexcept;
	Extent GetWindowVisibleExtent(WindowID) noexcept;
	Math::Vec2Int GetWindowPosition(WindowID) noexcept;
	// Gets the offset of the visible extent, relative to the window's position.
	Math::Vec2Int GetWindowVisibleOffset(WindowID) noexcept;
	bool GetWindowMinimized(WindowID) noexcept;
	WindowEvents GetWindowEvents(WindowID) noexcept;
	Std::StackVec<char const*, 5> RequiredVulkanInstanceExtensions() noexcept;
	Std::Opt<u64> CreateVkSurface(
		WindowID window,
		uSize vkInstance,
		void const* vkAllocationCallbacks);

	enum class CursorType : u8;
	void SetCursor(WindowID window, CursorType cursor) noexcept;

	enum class Orientation : u8;
	Orientation GetOrientation();
	bool GetOrientationEvent();

	u64 TickCount();

	enum class OS : u8;
	enum class Platform : u8;

	enum class Button : u16;
	enum class KeyEventType : u8
	{
		Unchanged,
		Unpressed,
		Pressed,
	};
	bool ButtonValue(Button input) noexcept;
	KeyEventType ButtonEvent(Button input) noexcept;
	f32 ButtonDuration(Button input) noexcept;

	struct CursorData;
	Std::Opt<CursorData> Cursor() noexcept;
	void LockCursor(bool state);

	constexpr uSize maxTouchEventCount = 10;
	enum class TouchEventType : u8;
	struct TouchInput;
	Std::StackVec<TouchInput, maxTouchEventCount> TouchInputs();

	enum class GamepadKey : u8
	{
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
		COUNT, // Invalid use to index with
	};
	enum class GamepadAxis : u8
	{
		Invalid,
		LeftX, // [-1, 1]
		LeftY, // [-1, 1]
		COUNT, // Invalid use to index with
	};
	struct GamepadState
	{
	public:
		Std::Array<bool, (int)GamepadKey::COUNT> keyStates = {};
		[[nodiscard]] bool GetKeyState(GamepadKey btn) const noexcept;

		Std::Array<KeyEventType, (int)GamepadKey::COUNT> keyEvents = {};
		[[nodiscard]] KeyEventType GetKeyEvent(GamepadKey btn) const noexcept;

		Std::Array<f32, (int)GamepadAxis::COUNT> axisValues = {};
		[[nodiscard]] f32 GetGamepadAxisValue(GamepadAxis axis) const noexcept;

		f32 stickDeadzone = 0.1f;
	};
	Std::Opt<GamepadState> GetGamepad();

	void Log(char const* msg);

	class EventInterface;
	void InsertEventInterface(EventInterface&);
	void RemoveEventInterface(EventInterface&);

	enum class SoftInputFilter : u8;
	void OpenSoftInput(Std::Str text, SoftInputFilter inputFilter);
	void UpdateCharInputContext(Std::Str text);
	void HideSoftInput();

	class FileInputStream;
}

namespace DEngine
{
	namespace App = Application;
}

enum class DEngine::Application::CursorType : DEngine::u8
{
	Arrow,
	HorizontalResize,
	VerticalResize,
#ifdef DENGINE_APPLICATION_CURSORTYPE_COUNT
	COUNT,
#endif
};

enum class DEngine::Application::Orientation : DEngine::u8
{
	Invalid,
	Landscape,
	Portrait
};

enum class DEngine::Application::OS : DEngine::u8
{
	Windows,
	Linux,
	Android
};

enum class DEngine::Application::Platform : DEngine::u8
{
	Desktop,
	Mobile
};

namespace DEngine::Application
{
#if defined(DENGINE_OS_WINDOWS)
	constexpr OS targetOS = OS::Windows;
	constexpr Platform targetOSType = Platform::Desktop;
#elif defined(DENGINE_OS_ANDROID)
	constexpr OS targetOS = OS::Android;
	constexpr Platform targetOSType = Platform::Mobile;
#elif defined(DENGINE_OS_LINUX)
	constexpr OS targetOS = OS::Linux;
	constexpr Platform targetOSType = Platform::Desktop;
#endif
}

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
#ifdef DENGINE_APPLICATION_BUTTON_COUNT
	COUNT
#endif
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
	Integer,
	UnsignedInteger,
	Float,
	UnsignedFloat
};

class DEngine::Application::FileInputStream
{
public:
	FileInputStream();
	FileInputStream(char const* path);
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
	Std::Opt<u64> Tell() const;
	bool IsOpen() const;
	bool Open(char const* path);
	void Close();

private:
	// Might not be safe, dunno yet
	alignas(8) char m_buffer[16] = {};
};

class DEngine::Application::EventInterface
{
public:
	inline virtual ~EventInterface() = 0;

	virtual void ButtonEvent(
		Button button,
		bool state) {}
	virtual void CharEnterEvent() {}
	virtual void CharEvent(u32 utfValue) {}
	virtual void CharRemoveEvent() {}
	virtual void CursorMove(
		Math::Vec2Int position,
		Math::Vec2Int positionDelta) {}
	virtual void TouchEvent(
		u8 id,
		TouchEventType type,
		Math::Vec2 position) {}
	virtual void WindowClose(
		WindowID window) {}
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
		WindowID window,
		Extent extent,
		Math::Vec2Int visiblePos,
		Extent visibleExtent) {}
	
	virtual void Log(char const* msg) {};
};

inline DEngine::Application::EventInterface::~EventInterface() {}