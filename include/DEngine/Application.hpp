#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/Array.hpp"
#include "DEngine/Containers/StaticVector.hpp"
#include "DEngine/Containers/Optional.hpp"

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
	constexpr uSize maxTouchEventCount = 10;
	constexpr uSize maxCharInputCount = 10;

	u64 TickCount();

	enum class OS : u8;
	enum class Platform : u8;

	enum class Button : u16;
	enum class KeyEventType : u8;
	bool ButtonValue(Button input);
	KeyEventType ButtonEvent(Button input);
	f32 ButtonDuration(Button input);

	Std::StaticVector<char, maxCharInputCount> CharInputs();

	struct CursorData;
	Std::Opt<CursorData> Cursor();

	enum class TouchEventType : u8;
	struct TouchInput;
	Std::StaticVector<TouchInput, maxTouchEventCount> TouchInputs();

	struct GamepadState
	{
		f32 leftStickX = 0.f;
		f32 leftStickY = 0.f;
	};
	Std::Optional<GamepadState> GetGamepad();

	void Log(char const* msg);

	bool MainWindowMinimized();
	void SetRelativeMouseMode(bool enabled);

	void OpenSoftInput();

	class FileInputStream;
}

namespace DEngine
{
	namespace App = Application;
}

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

enum class DEngine::Application::KeyEventType : DEngine::u8
{
	Unchanged,
	Unpressed,
	Pressed,
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
#ifdef DENGINE_APPLICATION_BUTTON_COUNT
	COUNT
#endif
};

struct DEngine::Application::CursorData
{
	u32 posX;
	u32 posY;
	i32 posDeltaX;
	i32 posDeltaY;
	
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
	using IDType = u8;
	static constexpr u8 invalidID = static_cast<u8>(-1);
	u8 id = invalidID;
	TouchEventType eventType = TouchEventType::Unchanged;
	f32 x = 0.f;
	f32 y = 0.f;
	f32 duration = 0.f;
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
	bool Seek(i64 offset, SeekOrigin origin = SeekOrigin::Current);
	bool Read(char* output, u64 size);
	Std::Opt<u64> Tell() const;
	bool IsOpen() const;
	bool Open(char const* path);
	void Close();

private:
	// Might not be safe, dunno yet
	alignas(8) char m_buffer[16] = {};
};