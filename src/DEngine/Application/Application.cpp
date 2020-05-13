#include "detail_Application.hpp"
#include "DEngine/Containers/StaticVector.hpp"
#include "Assert.hpp"

#include <iostream>
#include <cstring>
#include <chrono>
#include <stdexcept>

namespace DEngine::Application::detail
{
	// Input
	bool buttonValues[(int)Button::COUNT] = {};
	std::chrono::high_resolution_clock::time_point buttonHeldStart[(int)Button::COUNT] = {};
	f32 buttonHeldDuration[(int)Button::COUNT] = {};
	KeyEventType buttonEvents[(int)Button::COUNT] = {};
	Std::StaticVector<char, maxCharInputCount> charInputs{};
	Std::Opt<CursorData> cursorOpt{};
	Std::StaticVector<TouchInput, 10> touchInputs{};
	Std::StaticVector<std::chrono::high_resolution_clock::time_point, touchInputs.Capacity()> touchInputStartTime{};

	u32 displaySize[2] = {};
	i32 mainWindowPos[2] = {};
	u32 mainWindowSize[2] = {};
	u32 mainWindowFramebufferSize[2] = {};
	bool mainWindowIsInFocus = false;
	bool mainWindowIsMinimized = false;
	bool mainWindowRestoreEvent = false;
	bool mainWindowResizeEvent = false;
	bool shouldShutdown = false;

	// Time related stuff
	u64 tickCount = 0;
	f32 deltaTime = 1.f / 60.f;
	std::chrono::high_resolution_clock::time_point currentNow{};
	std::chrono::high_resolution_clock::time_point previousNow{};

	bool mainWindowSurfaceInitialized = false;
	bool mainWindowSurfaceInitializeEvent = false;
	bool mainWindowSurfaceTerminateEvent = false;

	GamepadState gamepadState{};
	bool gamepadConnected = false;
	int gamepadID = 0;

	static bool IsValid(Button in);
}

static bool DEngine::Application::detail::IsValid(Button in)
{
	return static_cast<u64>(in) < static_cast<u64>(Button::COUNT);
}

DEngine::u64 DEngine::Application::TickCount()
{
	return detail::tickCount;
}

bool DEngine::Application::ButtonValue(Button input)
{
	if (!detail::IsValid(input))
		throw std::runtime_error("Called DEngine::Application::ButtonValue with invalid Button value.");
	return detail::buttonValues[(int)input];
}

void DEngine::Application::detail::UpdateButton(
	Button button, 
	bool pressed)
{
	DENGINE_DETAIL_APPLICATION_ASSERT(IsValid(button));

	detail::buttonValues[(int)button] = pressed;
	detail::buttonEvents[(int)button] = pressed ? KeyEventType::Pressed : KeyEventType::Unpressed;

	if (pressed)
	{
		detail::buttonHeldStart[(int)button] = detail::currentNow;
	}
	else
	{
		detail::buttonHeldStart[(int)button] = std::chrono::high_resolution_clock::time_point();
	}
}

DEngine::Application::KeyEventType DEngine::Application::ButtonEvent(Button input)
{
	if (!detail::IsValid(input))
		throw std::runtime_error("Called DEngine::Application::ButtonEvent with invalid Button value.");
	return detail::buttonEvents[(int)input];
}

DEngine::f32 DEngine::Application::ButtonDuration(Button input)
{
	if (!detail::IsValid(input))
		throw std::runtime_error("Called DEngine::Application::ButtonDuration with invalid Button value.");
	return detail::buttonHeldDuration[(int)input];
}

DEngine::Std::StaticVector<char, DEngine::Application::maxCharInputCount> DEngine::Application::CharInputs()
{
	return detail::charInputs;
}

DEngine::Std::Opt<DEngine::Application::CursorData> DEngine::Application::Cursor()
{
	return detail::cursorOpt;
}

void DEngine::Application::detail::UpdateCursor(u32 posX, u32 posY, i32 deltaX, i32 deltaY)
{
	CursorData& cursorData = detail::cursorOpt.Value();
	cursorData.posDeltaX = deltaX;
	cursorData.posDeltaY = deltaY;

	cursorData.posX = posX;
	cursorData.posY = posY;
}

void DEngine::Application::detail::UpdateCursor(u32 posX, u32 posY)
{
	CursorData& cursorData = detail::cursorOpt.Value();

	// We compare it with the previous values to get delta.
	cursorData.posDeltaX = (i32)posX - (i32)cursorData.posX;
	cursorData.posDeltaY = (i32)posY - (i32)cursorData.posY;

	cursorData.posX = posX;
	cursorData.posY = posY;
}

namespace DEngine::Application::detail
{
	static bool TouchInputIDExists(u8 id)
	{
		for (uSize i = 0; i < detail::touchInputs.Size(); i += 1)
		{
			if (detail::touchInputs[i].id == id)
				return true;
		}
		return false;
	}
}

void DEngine::Application::detail::UpdateTouchInput_Down(u8 id, f32 x, f32 y)
{
	DENGINE_DETAIL_APPLICATION_ASSERT(!TouchInputIDExists(id));

	TouchInput newValue{};
	newValue.eventType = TouchEventType::Down;
	newValue.id = id;
	newValue.x = x;
	newValue.y = y;

	detail::touchInputs.PushBack(newValue);
	detail::touchInputStartTime.PushBack(detail::currentNow);
}

void DEngine::Application::detail::UpdateTouchInput_Move(u8 id, f32 x, f32 y)
{
	DENGINE_DETAIL_APPLICATION_ASSERT(TouchInputIDExists(id));
	
	for (auto& item : detail::touchInputs)
	{
		if (item.id == id)
		{
			item.eventType = TouchEventType::Moved;
			item.x = x;
			item.y = y;

			break;
		}
	}
}

void DEngine::Application::detail::UpdateTouchInput_Up(u8 id, f32 x, f32 y)
{
	DENGINE_DETAIL_APPLICATION_ASSERT(TouchInputIDExists(id));

	for (auto& item : detail::touchInputs)
	{
		if (item.id == id)
		{
			item.eventType = TouchEventType::Up;
			item.x = x;
			item.y = y;

			break;
		}
	}
}

DEngine::Std::StaticVector<DEngine::Application::TouchInput, 10> DEngine::Application::TouchInputs()
{
	return detail::touchInputs;
}

bool DEngine::Application::detail::Initialize()
{
	Backend_Initialize();

	return true;
}

void DEngine::Application::detail::ImgGui_Initialize()
{
}

void DEngine::Application::detail::ProcessEvents()
{
	detail::previousNow = detail::currentNow;
	detail::currentNow = std::chrono::high_resolution_clock::now();
	if (detail::tickCount > 0)
		detail::deltaTime = std::chrono::duration<f32>(detail::currentNow - detail::previousNow).count();

	// Input stuff
	// Clear event-style values.
	for (auto& item : detail::buttonEvents)
		item = KeyEventType::Unchanged;
	detail::charInputs.Clear();
	if (detail::cursorOpt.HasValue())
	{
		CursorData& cursorData = detail::cursorOpt.Value();
		cursorData.posDeltaX = 0;
		cursorData.posDeltaY = 0;
		cursorData.scrollDeltaY = 0;
	}

	for (uSize i = 0; i < detail::touchInputs.Size(); i += 1)
	{
		if (detail::touchInputs[i].eventType == TouchEventType::Up)
		{
			detail::touchInputs.Erase(i);
			detail::touchInputStartTime.Erase(i);
			i -= 1;
		}
		else
			detail::touchInputs[i].eventType = TouchEventType::Unchanged;
	}
	// Window stuff
	detail::mainWindowSurfaceInitializeEvent = false;
	detail::mainWindowSurfaceTerminateEvent = false;
	detail::mainWindowRestoreEvent = false;
	detail::mainWindowResizeEvent = false;

	Backend_ProcessEvents();

	// Calculate duration for each button being held.
	for (uSize i = 0; i < (uSize)Button::COUNT; i += 1)
	{
		if (detail::buttonValues[i])
			detail::buttonHeldDuration[i] = std::chrono::duration<f32>(detail::currentNow - detail::buttonHeldStart[i]).count();
		else
			detail::buttonHeldDuration[i] = 0.f;
	}
	// Calculate duration for each touch input.
	for (uSize i = 0; i < detail::touchInputs.Size(); i += 1)
	{
		if (detail::touchInputs[i].eventType != TouchEventType::Up)
			detail::touchInputs[i].duration = std::chrono::duration<f32>(detail::currentNow - detail::touchInputStartTime[i]).count();
	}

	detail::tickCount += 1;
}

bool DEngine::Application::detail::ShouldShutdown()
{
	return shouldShutdown;
}

bool DEngine::Application::detail::IsMinimized()
{
	return mainWindowIsMinimized;
}

bool DEngine::Application::detail::IsRestored()
{
	return mainWindowRestoreEvent;
}

bool DEngine::Application::detail::MainWindowRestoreEvent()
{
	return detail::mainWindowRestoreEvent;
}

bool DEngine::Application::detail::ResizeEvent()
{
	return mainWindowResizeEvent;
}

bool DEngine::Application::detail::MainWindowSurfaceInitializeEvent()
{
	return mainWindowSurfaceInitializeEvent;
}

void DEngine::Application::SetRelativeMouseMode(bool enabled)
{

}

DEngine::Std::Optional<DEngine::Application::GamepadState> DEngine::Application::GetGamepad()
{
	if (detail::gamepadConnected)
		return detail::gamepadState;
	else
		return {};
}

bool DEngine::Application::MainWindowMinimized()
{
	return detail::mainWindowIsMinimized;
}