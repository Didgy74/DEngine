#include <DEngine/detail/Application.hpp>
#include <DEngine/detail/AppAssert.hpp>

#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Utility.hpp>

#include <iostream>
#include <cstring>
#include <chrono>

namespace DEngine::Application::detail
{
	AppData* pAppData = nullptr;
	static constexpr bool IsValid(Button in) noexcept;
}

using namespace DEngine;

void Application::DestroyWindow(WindowID id) noexcept
{
	auto& appData = *detail::pAppData;
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[id](detail::AppData::WindowNode const& val) -> bool {
			return id== val.id; });
	DENGINE_DETAIL_APPLICATION_ASSERT(windowNodeIt != appData.windows.end());
	
	detail::Backend_DestroyWindow(*windowNodeIt);

	appData.windows.erase(windowNodeIt);
}

u32 Application::GetWindowCount() noexcept
{
	auto const& appData = *detail::pAppData;
	return (u32)appData.windows.size();
}

Application::Extent Application::GetWindowSize(WindowID window) noexcept
{
	auto const& appData = *detail::pAppData;
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[window](auto const& val) -> bool { return window == val.id; });
	DENGINE_DETAIL_APPLICATION_ASSERT(windowNodeIt != appData.windows.end());
	auto const& windowNode = *windowNodeIt;
	return windowNode.windowData.size;
}

Application::Extent Application::GetWindowVisibleSize(WindowID window) noexcept
{
	auto const& appData = *detail::pAppData;
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[window](auto const& val) -> bool { return window == val.id; });
	DENGINE_DETAIL_APPLICATION_ASSERT(windowNodeIt != appData.windows.end());
	auto const& windowNode = *windowNodeIt;
	return windowNode.windowData.visibleSize;
}

Math::Vec2Int Application::GetWindowPosition(WindowID window) noexcept
{
	auto const& appData = *detail::pAppData;
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[window](detail::AppData::WindowNode const& val) -> bool {
			return window == val.id; });
	DENGINE_DETAIL_APPLICATION_ASSERT(windowNodeIt != appData.windows.end());
	auto const& windowNode = *windowNodeIt;
	return windowNode.windowData.position;
}

Math::Vec2Int Application::GetWindowVisiblePosition(WindowID window) noexcept
{
	auto const& appData = *detail::pAppData;
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[window](detail::AppData::WindowNode const& val) -> bool {
			return window == val.id; });
	DENGINE_DETAIL_APPLICATION_ASSERT(windowNodeIt != appData.windows.end());
	auto const& windowNode = *windowNodeIt;
	return windowNode.windowData.visiblePosition;
}

bool Application::GetWindowMinimized(WindowID window) noexcept
{
	auto const& appData = *detail::pAppData;
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[window](detail::AppData::WindowNode const& val) -> bool { 
			return window == val.id; });
	DENGINE_DETAIL_APPLICATION_ASSERT(windowNodeIt != appData.windows.end());
	auto const& windowNode = *windowNodeIt;
	return windowNode.windowData.isMinimized;
}

Application::WindowEvents Application::GetWindowEvents(WindowID window) noexcept
{
	auto const& appData = *detail::pAppData;
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[window](detail::AppData::WindowNode const& val) -> bool {
			return window == val.id; });
	DENGINE_DETAIL_APPLICATION_ASSERT(windowNodeIt != appData.windows.end());
	auto const& windowNode = *windowNodeIt;
	return windowNode.events;
}

static constexpr bool Application::detail::IsValid(Button in) noexcept
{
	return static_cast<unsigned int>(in) < static_cast<unsigned int>(Button::COUNT);
}

u64 Application::TickCount()
{
	return detail::pAppData->tickCount;
}

bool Application::ButtonValue(Button input) noexcept
{
	DENGINE_DETAIL_APPLICATION_ASSERT(detail::pAppData);
	DENGINE_DETAIL_APPLICATION_ASSERT(detail::IsValid(input));
	return detail::pAppData->buttonValues[(int)input];
}

void Application::detail::UpdateButton(
	Button button, 
	bool pressed)
{
	DENGINE_DETAIL_APPLICATION_ASSERT(IsValid(button));

	DENGINE_DETAIL_APPLICATION_ASSERT(detail::pAppData);
	auto& appData = *detail::pAppData;

	appData.buttonValues[(int)button] = pressed;
	appData.buttonEvents[(int)button] = pressed ? KeyEventType::Pressed : KeyEventType::Unpressed;

	if (pressed)
	{
		appData.buttonHeldStart[(int)button] = appData.currentNow;
	}
	else
	{
		appData.buttonHeldStart[(int)button] = std::chrono::high_resolution_clock::time_point();
	}

	for (EventInterface* eventCallback : appData.eventCallbacks)
		eventCallback->ButtonEvent(button, pressed);

	if (appData.buttonEvents[(int)button] == KeyEventType::Pressed && (button == Button::Backspace || button == Button::Delete))
	{
		for (EventInterface* eventCallback : appData.eventCallbacks)
			eventCallback->CharRemoveEvent();
	}
	else if (appData.buttonEvents[(int)button] == KeyEventType::Pressed && button == Button::Enter)
	{
		for (EventInterface* eventCallback : appData.eventCallbacks)
			eventCallback->CharEnterEvent();
	}
}

void Application::detail::PushCharInput(u32 charValue)
{
	DENGINE_DETAIL_APPLICATION_ASSERT(detail::pAppData);
	auto& appData = *detail::pAppData;

	appData.charInputs.push_back(charValue);

	for (EventInterface* eventCallback : appData.eventCallbacks)
		eventCallback->CharEvent(charValue);
}

void DEngine::Application::detail::PushCharEnterEvent()
{
	DENGINE_DETAIL_APPLICATION_ASSERT(detail::pAppData);
	auto& appData = *detail::pAppData;
	for (EventInterface* eventCallback : appData.eventCallbacks)
		eventCallback->CharEnterEvent();
}

void Application::detail::PushCharRemoveEvent()
{
	DENGINE_DETAIL_APPLICATION_ASSERT(detail::pAppData);
	auto& appData = *detail::pAppData;
	for (EventInterface* eventCallback : appData.eventCallbacks)
		eventCallback->CharRemoveEvent();
}

Application::KeyEventType Application::ButtonEvent(Button input) noexcept
{
	DENGINE_DETAIL_APPLICATION_ASSERT(detail::pAppData);
	DENGINE_DETAIL_APPLICATION_ASSERT(detail::IsValid(input));
	return detail::pAppData->buttonEvents[(int)input];
}

f32 Application::ButtonDuration(Button input) noexcept
{
	DENGINE_DETAIL_APPLICATION_ASSERT(detail::pAppData);
	DENGINE_DETAIL_APPLICATION_ASSERT(detail::IsValid(input));
	return detail::pAppData->buttonHeldDuration[(int)input];
}

Std::Opt<Application::CursorData> Application::Cursor() noexcept
{
	DENGINE_DETAIL_APPLICATION_ASSERT(detail::pAppData);
	return detail::pAppData->cursorOpt;
}

namespace DEngine::Application::detail
{
	static bool TouchInputIDExists(u8 id) noexcept
	{
		for (auto const& item : detail::pAppData->touchInputs)
		{
			if (item.id == id)
				return true;
		}
		return false;
	}
}

Std::StackVec<Application::TouchInput, 10> Application::TouchInputs()
{
	return detail::pAppData->touchInputs;
}

bool Application::detail::Initialize() noexcept
{
	pAppData = new AppData();

	return Backend_Initialize();
}

void Application::detail::ProcessEvents()
{
	auto& appData = *detail::pAppData;

	appData.previousNow = detail::pAppData->currentNow;
	appData.currentNow = std::chrono::high_resolution_clock::now();
	if (appData.tickCount > 0)
		appData.deltaTime = std::chrono::duration<f32>(appData.currentNow - appData.previousNow).count();

	// Window stuff
	// Clear event-style values.
	for (auto& window : appData.windows)
		window.events = WindowEvents();
	appData.orientationEvent = false;

	// Input stuff
	// Clear event-style values.
	for (auto& item : detail::pAppData->buttonEvents)
		item = KeyEventType::Unchanged;
	appData.charInputs.clear();
	if (appData.cursorOpt.HasValue())
	{
		CursorData& cursorData = appData.cursorOpt.Value();
		cursorData.positionDelta = {};
		cursorData.scrollDeltaY = 0;
	}
	// Handle touch input stuff
	for (uSize i = 0; i < appData.touchInputs.Size(); i += 1)
	{
		if (appData.touchInputs[i].eventType == TouchEventType::Up)
		{
			appData.touchInputs.EraseUnsorted(i);
			appData.touchInputStartTime.EraseUnsorted(i);
			i -= 1;
		}
		else
			appData.touchInputs[i].eventType = TouchEventType::Unchanged;
	}

	Backend_ProcessEvents();

	// Calculate duration for each button being held.
	for (uSize i = 0; i < (uSize)Button::COUNT; i += 1)
	{
		if (appData.buttonValues[i])
			appData.buttonHeldDuration[i] = std::chrono::duration<f32>(appData.currentNow - appData.buttonHeldStart[i]).count();
		else
			appData.buttonHeldDuration[i] = 0.f;
	}
	// Calculate duration for each touch input.
	for (uSize i = 0; i < appData.touchInputs.Size(); i += 1)
	{
		if (appData.touchInputs[i].eventType != TouchEventType::Up)
			appData.touchInputs[i].duration = std::chrono::duration<f32>(appData.currentNow - appData.touchInputStartTime[i]).count();
	}

	appData.tickCount += 1;
}

void Application::detail::SetLogCallback(LogCallback callback)
{
	detail::pAppData->logCallback = callback;
}

Application::detail::AppData::WindowNode* DEngine::Application::detail::GetWindowNode(WindowID id) noexcept
{
	auto& appData = *detail::pAppData;
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[id](AppData::WindowNode const& val) -> bool { return val.id == id; });
	if (windowNodeIt == appData.windows.end())
		return nullptr;
	else
		return &(*windowNodeIt);
}

Application::detail::AppData::WindowNode* DEngine::Application::detail::GetWindowNode(void* platformHandle)
{
	auto& appData = *detail::pAppData;
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[platformHandle](AppData::WindowNode const& val) -> bool { return val.platformHandle == platformHandle; });
	if (windowNodeIt == appData.windows.end())
		return nullptr;
	else
		return &(*windowNodeIt);
}

void Application::detail::UpdateWindowSize(
	void* platformHandle,
	Extent newSize,
	Math::Vec2Int visiblePos,
	Extent visibleSize)
{
	auto& windowNode = *Std::FindIf(
		pAppData->windows.begin(),
		pAppData->windows.end(),
		[platformHandle](AppData::WindowNode const& val) -> bool {
			return val.platformHandle == platformHandle; });

	windowNode.windowData.size = newSize;
	windowNode.windowData.visiblePosition = visiblePos;
	windowNode.windowData.visibleSize = visibleSize;
	windowNode.events.resize = true;
	for (EventInterface* eventCallback : pAppData->eventCallbacks)
		eventCallback->WindowResize(windowNode.id, newSize, visiblePos, visibleSize);
}

void Application::detail::UpdateWindowPosition(
	void* platformHandle, 
	Math::Vec2Int newPosition)
{
	auto& windowNode = *Std::FindIf(
		pAppData->windows.begin(),
		pAppData->windows.end(),
		[platformHandle](AppData::WindowNode const& val) -> bool {
			return val.platformHandle == platformHandle; });

	windowNode.windowData.position = newPosition;
	windowNode.events.move = true;
	for (EventInterface* eventCallback : pAppData->eventCallbacks)
		eventCallback->WindowMove(windowNode.id, newPosition);
}

void Application::detail::UpdateWindowFocus(
	void* platformHandle,
	bool focused)
{
	auto& windowNode = *Std::FindIf(
		pAppData->windows.begin(),
		pAppData->windows.end(),
		[platformHandle](AppData::WindowNode const& val) -> bool {
			return val.platformHandle == platformHandle; });

	windowNode.events.focus = focused;
	if (focused)
		windowNode.events.focus = true;
	else
		windowNode.events.unfocus = true;
	for (EventInterface* eventCallback : pAppData->eventCallbacks)
		eventCallback->WindowFocus(windowNode.id, focused);
}

void Application::detail::UpdateWindowMinimized(
	AppData::WindowNode& windowNode, 
	bool minimized)
{
	auto& appData = *detail::pAppData;

	windowNode.windowData.isMinimized = minimized;
	if (minimized)
		windowNode.events.minimize = true;
	else
		windowNode.events.restore = true;
	for (EventInterface* eventCallback : appData.eventCallbacks)
		eventCallback->WindowMinimize(windowNode.id, minimized);
}

void Application::detail::UpdateWindowClose(
	AppData::WindowNode& windowNode)
{
	auto& appData = *detail::pAppData;

	windowNode.windowData.shouldShutdown = true;
	for (EventInterface* eventCallback : appData.eventCallbacks)
		eventCallback->WindowClose(windowNode.id);
}

void Application::detail::UpdateWindowCursorEnter(
	void* platformHandle, 
	bool entered)
{
	auto& appData = *detail::pAppData;
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[platformHandle](AppData::WindowNode const& val) -> bool {
			return val.platformHandle == platformHandle; });
	DENGINE_DETAIL_APPLICATION_ASSERT(windowNodeIt != appData.windows.end());
	auto& windowNode = *windowNodeIt;
	if (entered)
		windowNode.events.cursorEnter = true;
	else
		windowNode.events.cursorExit = true;
	for (EventInterface* eventCallback : pAppData->eventCallbacks)
		eventCallback->WindowCursorEnter(windowNode.id, entered);
}

void Application::detail::UpdateOrientation(Orientation newOrient)
{
	DENGINE_DETAIL_APPLICATION_ASSERT(newOrient != Orientation::Invalid);
	auto& appData = *detail::pAppData;
	appData.currentOrientation = newOrient;
	appData.orientationEvent = true;
}

void Application::detail::UpdateCursor(
	void* platformHandle,
	Math::Vec2Int pos,
	Math::Vec2Int delta)
{
	auto const& windowNode = *Std::FindIf(
		pAppData->windows.begin(),
		pAppData->windows.end(),
		[platformHandle](AppData::WindowNode const& val) -> bool {
			return val.platformHandle == platformHandle; });

	Math::Vec2Int newPosition = {
		pos.x + windowNode.windowData.position.x,
		pos.y + windowNode.windowData.position.y };

	CursorData& cursorData = pAppData->cursorOpt.Value();
	cursorData.position = newPosition;
	cursorData.positionDelta += delta;
	
	for (EventInterface* eventCallback : pAppData->eventCallbacks)
		eventCallback->CursorMove(cursorData.position, delta);
}

void Application::detail::UpdateCursor(
	void* platformHandle,
	Math::Vec2Int pos)
{
	auto const& windowNode = *Std::FindIf(
		pAppData->windows.begin(),
		pAppData->windows.end(),
		[platformHandle](AppData::WindowNode const& val) -> bool {
			return val.platformHandle == platformHandle; });

	Math::Vec2Int newPosition = {
		pos.x + windowNode.windowData.position.x,
		pos.y + windowNode.windowData.position.y };

	CursorData& cursorData = pAppData->cursorOpt.Value();

	Math::Vec2Int positionDelta = newPosition - cursorData.position;

	cursorData.positionDelta = positionDelta;
	cursorData.position = newPosition;

	for (EventInterface* eventCallback : pAppData->eventCallbacks)
		eventCallback->CursorMove(cursorData.position, positionDelta);
}

void Application::detail::UpdateTouchInput(TouchEventType type, u8 id, f32 x, f32 y)
{
	DENGINE_DETAIL_APPLICATION_ASSERT(type != TouchEventType::Unchanged);
	auto& appData = *detail::pAppData;

	if (type == TouchEventType::Down)
	{
		DENGINE_DETAIL_APPLICATION_ASSERT(appData.touchInputs.CanPushBack());
		TouchInput newValue{};
		newValue.eventType = TouchEventType::Down;
		newValue.id = id;
		newValue.x = x;
		newValue.y = y;
		appData.touchInputs.PushBack(newValue);
		appData.touchInputStartTime.PushBack(appData.currentNow);
	}
	else
	{
		DENGINE_DETAIL_APPLICATION_ASSERT(TouchInputIDExists(id));
		for (auto& item : appData.touchInputs)
		{
			if (item.id == id)
			{
				item.eventType = type;
				item.x = x;
				item.y = y;
				break;
			}
		}
	}

	for (EventInterface* eventCallback : pAppData->eventCallbacks)
		eventCallback->TouchEvent(id, type, { x, y });
}

Std::Opt<Application::GamepadState> Application::GetGamepad()
{
	if (detail::pAppData->gamepadConnected)
		return Std::Opt<Application::GamepadState>{ detail::pAppData->gamepadState };
	else
		return {};
}

void Application::Log(char const* msg)
{
	if (detail::pAppData->logCallback)
		detail::pAppData->logCallback(msg);
	else
		detail::Backend_Log(msg);
}

void Application::InsertEventInterface(EventInterface& in)
{
	detail::pAppData->eventCallbacks.push_back(&in);
}

void Application::RemoveEventInterface(EventInterface& in)
{
	auto callbackIt = Std::FindIf(
		detail::pAppData->eventCallbacks.begin(),
		detail::pAppData->eventCallbacks.end(),
		[&in](EventInterface* const val) -> bool {
			return val == &in; });
	detail::pAppData->eventCallbacks.erase(callbackIt);
}