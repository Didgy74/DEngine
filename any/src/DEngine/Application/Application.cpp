#include <DEngine/impl/Application.hpp>
#include <DEngine/impl/AppAssert.hpp>

#include <DEngine/Std/Utility.hpp>

#include <iostream>
#include <chrono>
#include <new>
#include <DEngine/Application.hpp>

namespace DEngine::Application::detail
{
	AppData* pAppData = nullptr;
	[[nodiscard]] static constexpr bool IsValid(Button in) noexcept;
	[[nodiscard]] static constexpr bool IsValid(GamepadKey in) noexcept;
	[[nodiscard]] static constexpr bool IsValid(GamepadAxis in) noexcept;
	[[nodiscard]] static constexpr bool IsValid(PollMode in) noexcept;
}

namespace DEngine::Application::impl
{
	[[nodiscard]] static constexpr bool IsValid(Button in) noexcept
	{
		return static_cast<unsigned int>(in) < static_cast<unsigned int>(Button::COUNT);
	}

	[[nodiscard]] static constexpr bool IsValid(GamepadKey in) noexcept
	{
		return static_cast<unsigned int>(in) < static_cast<unsigned int>(GamepadKey::COUNT);
	}
	[[nodiscard]] static constexpr bool IsValid(GamepadAxis in) noexcept
	{
		return static_cast<unsigned int>(in) < static_cast<unsigned int>(GamepadAxis::COUNT);
	}
	[[nodiscard]] static constexpr bool IsValid(PollMode in) noexcept
	{
		return static_cast<unsigned int>(in) < static_cast<unsigned int>(PollMode::COUNT);
	}

	static void FlushQueuedEventCallbacks(AppData& appData);
}

using namespace DEngine;
using namespace DEngine::Application;

static void Application::impl::FlushQueuedEventCallbacks(AppData& appData)
{
	for (auto const& job : appData.queuedEventCallbacks)
	{
		using Type = EventCallbackJob::Type;
		switch (job.type)
		{
			case Type::Button:
			{
				job.ptr->ButtonEvent(
					job.button.windowId,
					job.button.btn,
					job.button.state);
				break;
			}
			case Type::Char:
			{
				job.ptr->CharEvent(job.charEvent.utfValue);
				break;
			}
			case Type::CharEnter:
			{
				job.ptr->CharEnterEvent();
				break;
			}
			case Type::CharRemove:
			{
				job.ptr->CharRemoveEvent();
				break;
			}
			case Type::CursorMove:
			{
				job.ptr->CursorMove(
					job.cursorMove.windowId,
					job.cursorMove.pos,
					job.cursorMove.delta);
				break;
			}
			case Type::Touch:
			{
				job.ptr->TouchEvent(
					job.touch.id,
					job.touch.type,
					{ job.touch.x, job.touch.y });
				break;
			}
			case Type::WindowCursorEnter:
			{
				job.ptr->WindowCursorEnter(
					job.windowCursorEnter.window,
					job.windowCursorEnter.entered);
				break;
			}
			case Type::WindowFocus:
			{
				job.ptr->WindowFocus(job.windowFocus.window, true);
				break;
			}
			case Type::WindowMove:
			{
				job.ptr->WindowMove(job.windowMove.window, job.windowMove.position);
				break;
			}
			default:
				DENGINE_IMPL_APPLICATION_UNREACHABLE();
				break;
		}
	}
	appData.queuedEventCallbacks.clear();
}


void Application::DestroyWindow(WindowID id) noexcept
{
	auto& appData = *detail::pAppData;
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[id](detail::AppData::WindowNode const& val) {
			return id == val.id; });
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodeIt != appData.windows.end());
	
	detail::Backend_DestroyWindow(*windowNodeIt);

	appData.windows.erase(windowNodeIt);
}

u32 Application::GetWindowCount() noexcept
{
	auto const& appData = *detail::pAppData;
	return (u32)appData.windows.size();
}

Extent Application::GetWindowExtent(WindowID window) noexcept
{
	auto const& appData = *detail::pAppData;
	auto const windowNodePtr = detail::GetWindowNode(appData, window);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto const& windowNode = *windowNodePtr;
	return windowNode.windowData.extent;
}

Extent Application::GetWindowVisibleExtent(WindowID window) noexcept
{
	auto const& appData = *detail::pAppData;
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[window](auto const& val) -> bool { return window == val.id; });
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodeIt != appData.windows.end());
	auto const& windowNode = *windowNodeIt;
	return windowNode.windowData.visibleExtent;
}

Math::Vec2Int Application::GetWindowPosition(WindowID window) noexcept
{
	auto const& appData = *detail::pAppData;
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[window](detail::AppData::WindowNode const& val) -> bool {
			return window == val.id; });
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodeIt != appData.windows.end());
	auto const& windowNode = *windowNodeIt;
	return windowNode.windowData.position;
}

/*
Math::Vec2Int Application::GetWindowVisibleOffset(WindowID window) noexcept
{
	auto const& appData = *detail::pAppData;
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[window](auto const& val) {
			return window == val.id; });
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodeIt != appData.windows.end());
	auto const& windowNode = *windowNodeIt;
	return 	windowNode.windowData.visibleOffset;
}
*/

bool Application::GetWindowMinimized(WindowID window) noexcept
{
	auto const& appData = *detail::pAppData;
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[window](detail::AppData::WindowNode const& val) -> bool { 
			return window == val.id; });
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodeIt != appData.windows.end());
	auto const& windowNode = *windowNodeIt;
	return windowNode.windowData.isMinimized;
}

WindowEvents Application::GetWindowEvents(WindowID window) noexcept
{
	auto const& appData = *detail::pAppData;
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[window](detail::AppData::WindowNode const& val) -> bool {
			return window == val.id; });
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodeIt != appData.windows.end());
	auto const& windowNode = *windowNodeIt;
	return windowNode.events;
}

static constexpr bool Application::detail::IsValid(Button in) noexcept
{
	return static_cast<unsigned int>(in) < static_cast<unsigned int>(Button::COUNT);
}

static constexpr bool Application::detail::IsValid(GamepadKey in) noexcept
{
	return static_cast<unsigned int>(in) < static_cast<unsigned int>(GamepadKey::COUNT);
}

static constexpr bool Application::detail::IsValid(GamepadAxis in) noexcept
{
	return static_cast<unsigned int>(in) < static_cast<unsigned int>(GamepadAxis::COUNT);
}

constexpr bool detail::IsValid(PollMode in) noexcept
{
	return static_cast<unsigned int>(in) < static_cast<unsigned int>(PollMode::COUNT);
}

u64 Application::TickCount()
{
	return detail::pAppData->tickCount;
}

bool Application::ButtonValue(Button input) noexcept
{
	DENGINE_IMPL_APPLICATION_ASSERT(detail::pAppData);
	DENGINE_IMPL_APPLICATION_ASSERT(detail::IsValid(input));
	return detail::pAppData->buttonValues[(int)input];
}

void Application::detail::UpdateButton(
	AppData& appData,
	Button button,
	bool pressed)
{
	DENGINE_IMPL_APPLICATION_ASSERT(IsValid(button));

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

	for (auto const& registeredCallback : appData.registeredEventCallbacks)
	{
		EventCallbackJob job = {};
		job.type = EventCallbackJob::Type::Button;
		job.ptr = registeredCallback;

		EventCallbackJob::ButtonEvent event = {};
		event.btn = button;
		event.state = pressed;
		job.button = event;

		appData.queuedEventCallbacks.push_back(job);
	}
}

void Application::detail::UpdateGamepadButton(
	AppData& appData,
	GamepadKey button,
	bool pressed)
{
	DENGINE_IMPL_APPLICATION_ASSERT(IsValid(button));

	auto& gamepadState = appData.gamepadState;

	auto& keyState = gamepadState.keyStates[(int)button];
	auto const oldKeyState = keyState;
	keyState = pressed;

	auto& keyEvent = gamepadState.keyEvents[(int)button];
	if (oldKeyState && !pressed)
		keyEvent = KeyEventType::Unpressed;
	else if (!oldKeyState && pressed)
		keyEvent = KeyEventType::Pressed;
	else
		keyEvent = KeyEventType::Unchanged;
}

void Application::detail::UpdateGamepadAxis(
	AppData& appData,
	GamepadAxis axis,
	f32 newValue)
{
	DENGINE_IMPL_APPLICATION_ASSERT(IsValid(axis));

	auto& gamepadState = appData.gamepadState;

	gamepadState.axisValues[(int)axis] = newValue;
}

void Application::detail::PushCharInput(u32 charValue)
{
	DENGINE_IMPL_APPLICATION_ASSERT(detail::pAppData);
	auto& appData = *detail::pAppData;

	for (auto const& registeredCallback : appData.registeredEventCallbacks)
	{
		EventCallbackJob job = {};
		job.type = EventCallbackJob::Type::Char;
		job.ptr = registeredCallback;

		EventCallbackJob::CharEvent event = {};
		event.utfValue = (u8)charValue;
		job.charEvent = event;

		appData.queuedEventCallbacks.push_back(job);
	}
}

void DEngine::Application::detail::PushCharEnterEvent()
{
	DENGINE_IMPL_APPLICATION_ASSERT(detail::pAppData);
	auto& appData = *detail::pAppData;

	for (auto const& registeredCallback : appData.registeredEventCallbacks)
	{
		EventCallbackJob job = {};
		job.type = EventCallbackJob::Type::CharEnter;
		job.ptr = registeredCallback;

		EventCallbackJob::CharEnterEvent event = {};
		job.charEnter = event;

		appData.queuedEventCallbacks.push_back(job);
	}
}

void Application::detail::PushCharRemoveEvent()
{
	DENGINE_IMPL_APPLICATION_ASSERT(detail::pAppData);
	auto& appData = *detail::pAppData;

	for (auto const& registeredCallback : appData.registeredEventCallbacks)
	{
		EventCallbackJob job = {};
		job.type = EventCallbackJob::Type::CharRemove;
		job.ptr = registeredCallback;

		EventCallbackJob::CharRemoveEvent event = {};
		job.charRemove = event;

		appData.queuedEventCallbacks.push_back(job);
	}
}

Application::KeyEventType Application::ButtonEvent(Button input) noexcept
{
	DENGINE_IMPL_APPLICATION_ASSERT(detail::pAppData);
	DENGINE_IMPL_APPLICATION_ASSERT(detail::IsValid(input));
	return detail::pAppData->buttonEvents[(int)input];
}

f32 Application::ButtonDuration(Button input) noexcept
{
	DENGINE_IMPL_APPLICATION_ASSERT(detail::pAppData);
	DENGINE_IMPL_APPLICATION_ASSERT(detail::IsValid(input));
	return detail::pAppData->buttonHeldDuration[(int)input];
}

Std::Opt<Application::CursorData> Application::Cursor() noexcept
{
	DENGINE_IMPL_APPLICATION_ASSERT(detail::pAppData);
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

auto Application::impl::AppImpl::Initialize() -> Context
{
	Context ctx;

	auto* appData = new AppData;
	ctx.data = appData;

	appData->backendData = Backend_Initialize(*appData);

	return ctx;
}

void Application::impl::AppImpl::ProcessEvents(Context& ctx, PollMode pollMode, Std::Opt<u64> timeoutNs)
{
	DENGINE_IMPL_APPLICATION_ASSERT(impl::IsValid(pollMode));

	auto& appData = *static_cast<AppData*>(ctx.data);

	appData.previousNow = appData.currentNow;
	appData.currentNow = std::chrono::high_resolution_clock::now();
	if (appData.tickCount > 0)
		appData.deltaTime = std::chrono::duration<f32>(appData.currentNow - appData.previousNow).count();

	// Window stuff
	// Clear event-style values.
	for (auto& window : appData.windows)
		window.events = {};
	appData.orientationEvent = false;

	// Input stuff
	// Clear event-style values.
	for (auto& item : appData.buttonEvents)
		item = {};
	for (auto& item : appData.gamepadState.keyEvents)
		item = {};
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

	Backend_ProcessEvents(appData, pollMode, timeoutNs);

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
	// Flush queued event callbacks.
	impl::FlushQueuedEventCallbacks(appData);

	appData.tickCount += 1;
}

Context::~Context() noexcept
{
	if (data)
	{
		auto* const appData = static_cast<impl::AppData*>(data);
		if (appData->backendData)
		{
			impl::Backend_Destroy(appData->backendData);
			appData->backendData = nullptr;
		}

		delete appData;
	}

	data = nullptr;
}

auto Context::NewWindow(
	Std::Span<char const> title,
	Extent extent) -> NewWindow_ReturnT
{
	auto& appData = *static_cast<impl::AppData*>(data);

	auto newWindowInfo = impl::Backend_NewWindow(appData, title, extent);

	impl::AppData::WindowNode newNode = {};

	newNode.id = (WindowID)appData.windowIdTracker;
	appData.windowIdTracker += 1;

	newNode.platformHandle = newWindowInfo.platormHandle;
	newNode.windowData = newWindowInfo.windowData;

	appData.windows.push_back(newNode);

	NewWindow_ReturnT returnVal = {};
	returnVal.windowId = newNode.id;
	returnVal.extent = newNode.windowData.extent;
	returnVal.position = newNode.windowData.position;
	returnVal.visibleExtent = newNode.windowData.visibleExtent;
	returnVal.visibleOffset = newNode.windowData.visibleOffset;

	return returnVal;
}

u32 Context::GetWindowCount() const noexcept
{
	auto& appData = *static_cast<impl::AppData*>(data);

	return (u32)appData.windows.size();
}

auto Context::CreateVkSurface(
	WindowID window,
	uSize vkInstance,
	void const* vkAllocationCallbacks) noexcept
-> Context::CreateVkSurface_ReturnT
{
	auto& appData = *static_cast<impl::AppData*>(data);

	auto windowNodePtr = impl::GetWindowNode(appData, window);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	return impl::Backend_CreateVkSurface(
		windowNode.platformHandle,
		vkInstance,
		vkAllocationCallbacks);
}

void Context::InsertEventInterface(EventForwarder& in)
{
	auto& appData = *static_cast<impl::AppData*>(data);

	appData.registeredEventCallbacks.push_back(&in);
}

void Context::Log(LogSeverity severity, Std::Span<char const> msg)
{
	impl::Backend_Log(severity, msg);
}

auto Application::impl::GetWindowNode(
	AppData& appData,
	WindowID id) -> AppData::WindowNode*
{
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[id](auto const& val) { return val.id == id; });
	if (windowNodeIt == appData.windows.end())
		return nullptr;
	else
		return &(*windowNodeIt);
}

auto Application::impl::GetWindowNode(
	AppData const& appData,
	WindowID id) -> AppData::WindowNode const*
{
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[id](auto const& val) { return val.id == id; });
	if (windowNodeIt == appData.windows.end())
		return nullptr;
	else
		return &(*windowNodeIt);
}

WindowID Application::impl::GetWindowId(impl::AppData& appData, void* platformHandle)
{
	auto windowIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[platformHandle](auto const& item) { return item.platformHandle == platformHandle; });
	DENGINE_IMPL_APPLICATION_ASSERT(windowIt != appData.windows.end());
	auto const& element = *windowIt;

	return element.id;
}

void Application::impl::UpdateWindowCursorEnter(
	AppData& appData,
	WindowID id,
	bool entered)
{
	auto windowNodePtr = impl::GetWindowNode(appData, id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	for (auto const& eventCallback : appData.registeredEventCallbacks)
	{
		using T = EventCallbackJob;
		T job = {};
		job.ptr = eventCallback;
		job.type = T::Type::WindowCursorEnter;
		T::WindowCursorEnter event = {};
		event.window = id;
		event.entered = entered;
		job.windowCursorEnter = event;
		appData.queuedEventCallbacks.push_back(job);
	}
}

// Should be called by the backend.
void Application::impl::UpdateWindowPosition(
	AppData& appData,
	WindowID id,
	Math::Vec2Int newPosition)
{
	auto windowNodePtr = impl::GetWindowNode(appData, id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	windowNode.windowData.position = newPosition;
	windowNode.events.move = true;

	for (auto const& eventCallback : appData.registeredEventCallbacks)
	{
		using T = EventCallbackJob;
		T job = {};
		job.ptr = eventCallback;
		job.type = T::Type::WindowMove;
		T::WindowMoveEvent event = {};
		event.position = newPosition;
		event.window = windowNode.id;
		job.windowMove = event;
		appData.queuedEventCallbacks.push_back(job);
	}
}

void Application::impl::UpdateWindowFocus(
	AppData& appData,
	WindowID id,
	bool focusGained)
{
	auto windowNodePtr = impl::GetWindowNode(appData, id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	for (auto const& eventCallback : appData.registeredEventCallbacks)
	{
		using T = EventCallbackJob;
		T job = {};
		job.ptr = eventCallback;
		job.type = T::Type::WindowFocus;
		T::WindowFocusEvent event = {};
		event.window = windowNode.id;
		event.focusGained = focusGained;
		job.windowFocus = event;
		appData.queuedEventCallbacks.push_back(job);
	}
}

namespace DEngine::Application::impl
{
	static void UpdateCursor(
		AppData& appData,
		AppData::WindowNode& windowNode,
		Math::Vec2Int newRelativePos,
		Math::Vec2Int delta)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(appData.cursorOpt.HasValue());
		CursorData& cursorData = appData.cursorOpt.Value();
		cursorData.position = (newRelativePos + windowNode.windowData.position);
		cursorData.positionDelta += delta;

		for (auto const& eventCallback : appData.registeredEventCallbacks)
		{
			EventCallbackJob job = {};
			job.type = EventCallbackJob::Type::CursorMove;
			job.ptr = eventCallback;
			EventCallbackJob::CursorMoveEvent event = {};
			event.windowId = windowNode.id;
			event.pos = newRelativePos;
			event.delta = cursorData.positionDelta;
			job.cursorMove = event;
			appData.queuedEventCallbacks.push_back(job);
		}
	}

	static void UpdateCursor(
		AppData& appData,
		AppData::WindowNode& windowNode,
		Math::Vec2Int newRelativePos)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(appData.cursorOpt.HasValue());
		CursorData& cursorData = appData.cursorOpt.Value();

		Math::Vec2Int delta = (newRelativePos + windowNode.windowData.position) - cursorData.position;

		return UpdateCursor(
			appData,
			windowNode,
			newRelativePos,
			delta);
	}
}

void Application::impl::UpdateCursorPosition(
	AppData& appData,
	WindowID id,
	Math::Vec2Int newRelativePosition,
	Math::Vec2Int delta)
{
	auto windowNodePtr = impl::GetWindowNode(appData, id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;
	return UpdateCursor(
		appData,
		windowNode,
		newRelativePosition,
		delta);
}

void Application::impl::UpdateCursorPosition(
	AppData& appData,
	WindowID id,
	Math::Vec2Int newRelativePosition)
{
	auto windowNodePtr = impl::GetWindowNode(appData, id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;
	return UpdateCursor(
		appData,
		windowNode,
		newRelativePosition);
}

void Application::impl::UpdateButton(
	AppData& appData,
	WindowID id,
	Button button,
	bool pressed)
{
	DENGINE_IMPL_APPLICATION_ASSERT(IsValid(button));

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

	for (auto const& registeredCallback : appData.registeredEventCallbacks)
	{
		EventCallbackJob job = {};
		job.type = EventCallbackJob::Type::Button;
		job.ptr = registeredCallback;

		EventCallbackJob::ButtonEvent event = {};
		event.windowId = id;
		event.btn = button;
		event.state = pressed;
		job.button = event;

		appData.queuedEventCallbacks.push_back(job);
	}
}

detail::AppData::WindowNode* Application::detail::GetWindowNode(
	AppData& appData,
	WindowID id)
{
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[id](auto const& val) -> bool { return val.id == id; });
	if (windowNodeIt == appData.windows.end())
		return nullptr;
	else
		return &(*windowNodeIt);
}

detail::AppData::WindowNode const* Application::detail::GetWindowNode(
	AppData const& appData,
	WindowID id)
{
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[id](auto const& val) -> bool { return val.id == id; });
	if (windowNodeIt == appData.windows.end())
		return nullptr;
	else
		return &(*windowNodeIt);
}

detail::AppData::WindowNode* Application::detail::GetWindowNode(
	AppData& appData,
	void* platformHandle)
{
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[platformHandle](auto const& val) -> bool { return val.platformHandle == platformHandle; });
	if (windowNodeIt == appData.windows.end())
		return nullptr;
	else
		return &(*windowNodeIt);
}

detail::AppData::WindowNode const* Application::detail::GetWindowNode(
	AppData const& appData,
	void* platformHandle)
{
	auto windowNodeIt = Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[platformHandle](auto const& val) -> bool { return val.platformHandle == platformHandle; });
	if (windowNodeIt == appData.windows.end())
		return nullptr;
	else
		return &(*windowNodeIt);
}

void Application::detail::UpdateWindowSize(
	AppData::WindowNode& windowNode,
	Extent newSize,
	Math::Vec2Int visibleOffset,
	Extent visibleExtent)
{
	windowNode.windowData.extent = newSize;
	windowNode.windowData.visibleOffset = { (u32)visibleOffset.x, (u32)visibleOffset.y };
	windowNode.windowData.visibleExtent = visibleExtent;
	windowNode.events.resize = true;
	for (EventForwarder* eventCallback : pAppData->registeredEventCallbacks)
		eventCallback->WindowResize(windowNode.id, newSize, visibleOffset, visibleExtent);
}

void Application::detail::UpdateWindowPosition(
	AppData& appData,
	void* platformHandle, 
	Math::Vec2Int newPosition)
{
	auto windowNodePtr = GetWindowNode(appData, platformHandle);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	windowNode.windowData.position = newPosition;
	windowNode.events.move = true;

	for (auto const& eventCallback : appData.registeredEventCallbacks)
	{
		using T = EventCallbackJob;
		T job = {};
		job.ptr = eventCallback;
		job.type = T::Type::WindowMove;
		T::WindowMoveEvent event = {};
		event.position = newPosition;
		event.window = windowNode.id;
		job.windowMove = event;
		appData.queuedEventCallbacks.push_back(job);
	}
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
	for (EventForwarder* eventCallback : pAppData->registeredEventCallbacks)
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
	for (EventForwarder* eventCallback : appData.registeredEventCallbacks)
		eventCallback->WindowMinimize(windowNode.id, minimized);
}

void Application::detail::UpdateWindowClose(
	AppData::WindowNode& windowNode)
{
	auto& appData = *detail::pAppData;

	windowNode.windowData.shouldShutdown = true;
	for (EventForwarder* eventCallback : appData.registeredEventCallbacks)
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
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodeIt != appData.windows.end());
	auto& windowNode = *windowNodeIt;
	if (entered)
		windowNode.events.cursorEnter = true;
	else
		windowNode.events.cursorExit = true;
	for (EventForwarder* eventCallback : pAppData->registeredEventCallbacks)
		eventCallback->WindowCursorEnter(windowNode.id, entered);
}

void Application::detail::UpdateOrientation(Orientation newOrient)
{
	DENGINE_IMPL_APPLICATION_ASSERT(newOrient != Orientation::Invalid);
	auto& appData = *detail::pAppData;
	appData.currentOrientation = newOrient;
	appData.orientationEvent = true;
}

namespace DEngine::Application::detail
{
	static void UpdateCursor(
		AppData& appData,
		AppData::WindowNode& windowNode,
		Math::Vec2Int pos,
		Math::Vec2Int delta)
	{
		Math::Vec2Int newPosition = {
			pos.x + windowNode.windowData.position.x,
			pos.y + windowNode.windowData.position.y };

		DENGINE_IMPL_APPLICATION_ASSERT(appData.cursorOpt.HasValue());
		CursorData& cursorData = appData.cursorOpt.Value();
		cursorData.position = newPosition;
		cursorData.positionDelta += delta;

		for (auto const eventCallback : appData.registeredEventCallbacks)
		{
			EventCallbackJob job = {};
			job.type = EventCallbackJob::Type::CursorMove;
			job.ptr = eventCallback;
			EventCallbackJob::CursorMoveEvent event = {};
			event.pos = cursorData.position;
			event.delta = cursorData.positionDelta;
			job.cursorMove = event;
			appData.queuedEventCallbacks.push_back(job);
		}
	}

	static void UpdateCursor(
		AppData& appData,
		AppData::WindowNode& windowNode,
		Math::Vec2Int pos)
	{
		Math::Vec2Int newPosition = {
			pos.x + windowNode.windowData.position.x,
			pos.y + windowNode.windowData.position.y };

		DENGINE_IMPL_APPLICATION_ASSERT(appData.cursorOpt.HasValue());
		CursorData& cursorData = appData.cursorOpt.Value();

		Math::Vec2Int delta = newPosition - cursorData.position;

		return UpdateCursor(
			appData,
			windowNode,
			pos,
			delta);
	}
}

void Application::detail::UpdateCursor(
	AppData& appData,
	WindowID id,
	Math::Vec2Int pos,
	Math::Vec2Int delta)
{
	auto windowNodePtr = GetWindowNode(appData, id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;
	return UpdateCursor(
		appData,
		windowNode,
		pos,
		delta);
}

void Application::detail::UpdateCursor(
	AppData& appData,
	WindowID id,
	Math::Vec2Int pos)
{
	auto windowNodePtr = GetWindowNode(appData, id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;
	return UpdateCursor(
		appData,
		windowNode,
		pos);
}

void Application::detail::UpdateCursor(
	AppData& appData,
	void* platformHandle,
	Math::Vec2Int pos,
	Math::Vec2Int delta)
{
	auto windowNodePtr = GetWindowNode(appData, platformHandle);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;
	return UpdateCursor(
		appData,
		windowNode,
		pos,
		delta);
}

void Application::detail::UpdateCursor(
	AppData& appData,
	void* platformHandle,
	Math::Vec2Int pos)
{
	auto windowNodePtr = GetWindowNode(appData, platformHandle);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;
	return UpdateCursor(
		appData,
		windowNode,
		pos);
}

void Application::detail::UpdateTouchInput(
	AppData& appData,
	TouchEventType type, 
	u8 id, 
	f32 x, 
	f32 y)
{
	if (type == TouchEventType::Down)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(appData.touchInputs.CanPushBack());
		TouchInput newValue = {};
		newValue.eventType = TouchEventType::Down;
		newValue.id = id;
		newValue.x = x;
		newValue.y = y;
		appData.touchInputs.PushBack(newValue);
		appData.touchInputStartTime.PushBack(appData.currentNow);
	}
	else
	{
		DENGINE_IMPL_APPLICATION_ASSERT(TouchInputIDExists(id));
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

	for (auto const eventCallback : appData.registeredEventCallbacks)
	{
		EventCallbackJob job = {};
		job.type = EventCallbackJob::Type::Touch;
		job.ptr = eventCallback;
		EventCallbackJob::TouchEvent event = {};
		event.id = id;
		event.type = type;
		event.x = x;
		event.y = y;
		job.touch = event;
		appData.queuedEventCallbacks.push_back(job);
	}
}

Std::Opt<GamepadState> Application::GetGamepad()
{
	auto& appData = *detail::pAppData;
	return appData.gamepadState;
}

bool GamepadState::GetKeyState(GamepadKey btn) const noexcept
{
	DENGINE_IMPL_APPLICATION_ASSERT(detail::IsValid(btn));
	return keyStates[(int)btn];
}

KeyEventType GamepadState::GetKeyEvent(GamepadKey btn) const noexcept
{
	DENGINE_IMPL_APPLICATION_ASSERT(detail::IsValid(btn));
	return keyEvents[(int)btn];
}

f32 GamepadState::GetGamepadAxisValue(GamepadAxis axis) const noexcept
{
	DENGINE_IMPL_APPLICATION_ASSERT(detail::IsValid(axis));
	return axisValues[(int)axis];
}

void Application::Log(char const* msg)
{
	detail::Backend_Log(msg);
}

void Application::InsertEventInterface(EventForwarder& in)
{
	detail::pAppData->registeredEventCallbacks.push_back(&in);
}

void Application::RemoveEventInterface(EventForwarder& in)
{
	auto callbackIt = Std::FindIf(
		detail::pAppData->registeredEventCallbacks.begin(),
		detail::pAppData->registeredEventCallbacks.end(),
		[&in](EventForwarder* const val) -> bool {
			return val == &in; });
	detail::pAppData->registeredEventCallbacks.erase(callbackIt);
}