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
	enum class OS : u8;
	enum class Platform : u8;

	enum class Button : u16;
	enum class KeyEventType : u8;
	bool ButtonValue(Button input);
	KeyEventType ButtonEvent(Button input);
	f32 ButtonDuration(Button input);

	struct MouseData;
	Std::Opt<MouseData> Mouse();

	enum class TouchEventType : u8;
	struct TouchInput;
	Std::StaticVector<TouchInput, 10> TouchInputs();

	void Log(char const* msg);

	bool MainWindowMinimized();
	void SetRelativeMouseMode(bool enabled);

	class FileStream;
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

struct DEngine::Application::MouseData
{
	Std::Array<u32, 2> pos{};
	Std::Array<i32, 2> delta{};
};

enum class DEngine::Application::TouchEventType : DEngine::u8
{
	Unchanged,
	Down,
	Up,
	Cancelled,
	Moved
};

struct DEngine::Application::TouchInput
{
	uSize id = 0;
	TouchEventType eventType = TouchEventType::Unchanged;
	f32 x = 0.f;
	f32 y = 0.f;
};

class DEngine::Application::FileStream
{
public:
	static Std::Opt<FileStream> OpenPath(char const* path);

	enum class SeekOrigin
	{
		Current,
		Start,
		End
	};
	bool Seek(i64 offset, SeekOrigin origin = SeekOrigin::Current);
	bool Read(char* output, u64 size);

	u64 Tell();

	FileStream(FileStream const&) = delete;
	FileStream(FileStream&&) noexcept;
	~FileStream();
	FileStream& operator=(FileStream const&) = delete;
	FileStream& operator=(FileStream&&);
private:
	// Might not be safe, dunno yet
	alignas(8) char m_buffer[16] = {};

	constexpr FileStream() noexcept = default;
};