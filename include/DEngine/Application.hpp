#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/Array.hpp"
#include "DEngine/Containers/StaticVector.hpp"

namespace DEngine::Application
{
	enum class OS : u8;
	enum class Platform : u8;

	enum class Button : u16;
	enum class KeyEventType : u8;
	bool ButtonValue(Button input);
	KeyEventType ButtonEvent(Button input);
	f32 ButtonDuration(Button input);
	Std::Array<i32, 2> MouseDelta();
	Std::Array<u32, 2> MousePosition();

	enum class TouchEventType : u8
	{
		Unchanged,
		Down,
		Up,
		Cancelled
	};
	struct TouchInput
	{
		uSize id = 0;
		TouchEventType eventType = TouchEventType::Unchanged;
		f32 x = 0.f;
		f32 y = 0.f;
	};
	Std::StaticVector<TouchInput, 10> GetTouchInputs();

	void Log(char const* msg);

	void SetRelativeMouseMode(bool enabled);
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
#if defined(_WIN32) || defined(_WIN64)
	constexpr OS targetOS = OS::Windows;
	constexpr Platform targetOSType = Platform::Desktop;
#elif defined(__ANDROID__)
	constexpr OS targetOS = OS::Android;
	constexpr Platform targetOSType = Platform::Mobile;
#elif defined(__GNUC__)
	constexpr OS targetOS = OS::Linux;
	constexpr Platform targetOSType = Platform::Desktop;
#else
#error Error. DEngine::Application does not support this platform/compiler
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