#include <DEngine/Application.hpp>
#include <DEngine/impl/Application.hpp>
#include <DEngine/impl/AppAssert.hpp>

#include <DEngine/Std/Utility.hpp>

#include <chrono>
#include <new>
#include <stdexcept>

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

	static void FlushQueuedEventCallbacks(Context& appCtx);

	void DestroyWindow(
		Context::Impl& implData,
		WindowID id,
		bool useLock);
}

using namespace DEngine;
using namespace DEngine::Application;

auto Application::Context::GetImplData() noexcept -> Impl&
{
	DENGINE_IMPL_APPLICATION_ASSERT(m_implData);
	return *m_implData;
}

auto Application::Context::GetImplData() const noexcept -> Impl const&
{
	DENGINE_IMPL_APPLICATION_ASSERT(m_implData);
	return *m_implData;
}

static void Application::impl::FlushQueuedEventCallbacks(Context& appCtx)
{
	auto& implData = appCtx.GetImplData();
	for (auto const& job : implData.queuedEventCallbacks)
	{
		switch (job.type)
		{
			case EventType::ButtonEvent:
			{
				auto const& event = EventUnion_Get<EventType::ButtonEvent>(job.eventUnion);
				job.ptr->ButtonEvent(
					event.windowId,
					event.btn,
					event.state);
				break;
			}
			case EventType::CursorMoveEvent:
			{
				auto const& event = EventUnion_Get<EventType::CursorMoveEvent>(job.eventUnion);
				job.ptr->CursorMove(
					appCtx,
					event.windowId,
					event.pos,
					event.delta);
				break;
			}

			case EventType::TextInputEvent:
			{
				auto const& event = EventUnion_Get<EventType::TextInputEvent>(job.eventUnion);
				DENGINE_IMPL_APPLICATION_ASSERT(event.newTextOffset + event.newTextSize <= implData.textInputDatas.size());
				job.ptr->TextInputEvent(
					appCtx,
					event.windowId,
					event.oldIndex,
					event.oldCount,
					{ implData.textInputDatas.data() + event.newTextOffset, event.newTextSize });
				break;
			}

			case EventType::EndTextInputSessionEvent:
			{
				auto const& event = EventUnion_Get<EventType::EndTextInputSessionEvent>(job.eventUnion);
				job.ptr->EndTextInputSessionEvent(appCtx, event.windowId);
				break;
			}

			case EventType::TouchEvent:
			{
				auto const& event = EventUnion_Get<EventType::TouchEvent>(job.eventUnion);
				job.ptr->TouchEvent(
					event.windowId,
					event.id,
					event.type,
					{ event.x, event.y });
				break;
			}
			case EventType::WindowCursorEnterEvent:
			{
				auto const& event = EventUnion_Get<EventType::WindowCursorEnterEvent>(job.eventUnion);
				job.ptr->WindowCursorEnter(
					event.window,
					event.entered);
				break;
			}

			case EventType::WindowCloseSignalEvent:
			{
				auto const& event = EventUnion_Get<EventType::WindowCloseSignalEvent>(job.eventUnion);
				bool const destroyWindow = job.ptr->WindowCloseSignal(
					appCtx,
					event.window);
				if (destroyWindow)
				{
					constexpr bool useLock = false;
					DestroyWindow(implData, event.window, useLock);
				}
				break;
			}
			case EventType::WindowFocusEvent:
			{
				auto const& event = EventUnion_Get<EventType::WindowFocusEvent>(job.eventUnion);
				job.ptr->WindowFocus(
					event.window,
					event.focusGained);
				break;
			}
			case EventType::WindowMoveEvent:
			{
				auto const& event = EventUnion_Get<EventType::WindowMoveEvent>(job.eventUnion);
				job.ptr->WindowMove(
					event.window,
					event.position);
				break;
			}
			case EventType::WindowResizeEvent:
			{
				auto const& event = EventUnion_Get<EventType::WindowResizeEvent>(job.eventUnion);
				job.ptr->WindowResize(
					appCtx,
					event.window,
					event.extent,
					event.safeAreaOffset,
					event.safeAreaExtent);
				break;
			}

			default:
				DENGINE_IMPL_APPLICATION_UNREACHABLE();
				break;
		}
	}
	implData.queuedEventCallbacks.clear();

}


void Application::impl::DestroyWindow(
	Context::Impl& implData,
	WindowID id,
	bool useLock)
{
	using MutexT = decltype(implData.windowsLock);
	Std::Opt<std::scoped_lock<MutexT>> lockOpt;
	if (useLock)
		lockOpt.Emplace(implData.windowsLock);

	auto const windowNodeIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[id](auto const& element) { return element.id == id; });
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodeIt != implData.windows.end());

	auto windowNode = Std::Move(*windowNodeIt);
	implData.windows.erase(windowNodeIt);

	impl::Backend::DestroyWindow(implData, implData.backendData, windowNode);
}

auto Application::Context::Impl::Initialize() -> Context
{
	Context ctx;

	ctx.m_implData = new Impl;
	auto& implData = *ctx.m_implData;

	implData.backendData = impl::Backend::Initialize(ctx, implData);

	return ctx;
}

void Application::Context::Impl::ProcessEvents(
	Context& ctx,
	bool waitForEvents,
	u64 timeoutNs,
	bool ignoreWaitOnFirstCall)
{
	auto& implData = ctx.GetImplData();

	if (ignoreWaitOnFirstCall && implData.isFirstCall) {
		waitForEvents = false;
		implData.isFirstCall = false;
	}

	implData.previousNow = implData.currentNow;
	implData.currentNow = std::chrono::high_resolution_clock::now();
	if (implData.tickCount > 0)
		implData.deltaTime = std::chrono::duration<f32>(implData.currentNow - implData.previousNow).count();

	// Window stuff
	// Clear event-style values.
	for (auto& window : implData.windows)
		window.events = {};
	implData.orientationEvent = false;

	// Input stuff
	// Clear event-style values.
	for (auto& item : implData.buttonEvents)
		item = {};
	for (auto& item : implData.gamepadState.keyEvents)
		item = {};
	if (implData.cursorOpt.HasValue())
	{
		CursorData& cursorData = implData.cursorOpt.Value();
		cursorData.positionDelta = {};
		cursorData.scrollDeltaY = 0;
	}
	// Handle touch input stuff
	for (uSize i = 0; i < implData.touchInputs.Size(); i += 1)
	{
		if (implData.touchInputs[i].eventType == TouchEventType::Up)
		{
			implData.touchInputs.EraseUnsorted(i);
			implData.touchInputStartTime.EraseUnsorted(i);
			i -= 1;
		}
		else
			implData.touchInputs[i].eventType = TouchEventType::Unchanged;
	}
	implData.textInputDatas.clear();

	impl::Backend::ProcessEvents(ctx, implData, implData.backendData, waitForEvents, timeoutNs);

	// Calculate duration for each button being held.
	for (uSize i = 0; i < (uSize)Button::COUNT; i += 1)
	{
		if (implData.buttonValues[i])
			implData.buttonHeldDuration[i] = std::chrono::duration<f32>(implData.currentNow - implData.buttonHeldStart[i]).count();
		else
			implData.buttonHeldDuration[i] = 0.f;
	}
	// Calculate duration for each touch input.
	for (uSize i = 0; i < implData.touchInputs.Size(); i += 1)
	{
		if (implData.touchInputs[i].eventType != TouchEventType::Up)
			implData.touchInputs[i].duration = std::chrono::duration<f32>(implData.currentNow - implData.touchInputStartTime[i]).count();
	}

	// Flush queued event callbacks.
	{
		std::scoped_lock lock { implData.windowsLock };
		impl::FlushQueuedEventCallbacks(ctx);
	}

	implData.tickCount += 1;
}

Context::~Context() noexcept
{
	if (m_implData)
	{
		if (m_implData->backendData)
		{
			impl::Backend::Destroy(m_implData->backendData);
			m_implData->backendData = nullptr;
		}
		delete m_implData;
	}

	m_implData = nullptr;
}

u64 Context::TickCount() const noexcept
{
	auto& appData = GetImplData();

	return appData.tickCount;
}

auto Context::NewWindow(
	Std::Span<char const> title,
	Extent extent)
		-> NewWindow_ReturnT
{
	auto& appData = GetImplData();

	WindowID newWindowId = {};
	{
		std::scoped_lock lock { appData.windowsLock };
		newWindowId = (WindowID)appData.windowIdTracker;
		appData.windowIdTracker += 1;
	}

	auto newWindowInfoOpt = impl::Backend::NewWindow(
		*this,
		appData,
		appData.backendData,
		newWindowId,
		title,
		extent);
	if (!newWindowInfoOpt.Has())
		throw std::runtime_error("MakeWindow failed.");
	auto& newWindowInfo = newWindowInfoOpt.Get();

	Impl::WindowNode newNode = {};
	newNode.id = newWindowId;
	newNode.platformHandle = newWindowInfo.platformHandle;
	newNode.windowData = newWindowInfo.windowData;
	{
		std::scoped_lock lock { appData.windowsLock };
		appData.windows.push_back(newNode);
	}

	NewWindow_ReturnT returnVal = {};
	returnVal.windowId = newNode.id;
	returnVal.extent = newNode.windowData.extent;
	returnVal.position = newNode.windowData.position;
	returnVal.visibleExtent = newNode.windowData.visibleExtent;
	returnVal.visibleOffset = newNode.windowData.visibleOffset;

	return returnVal;
}

void Context::DestroyWindow(WindowID id) noexcept
{
	auto& appData = GetImplData();

	constexpr bool useLock = true;
	impl::DestroyWindow(
		appData,
		id,
		useLock);
}

u32 Context::GetWindowCount() const noexcept
{
	auto& appData = GetImplData();

	std::scoped_lock windowsLock { appData.windowsLock };
	return (u32)appData.windows.size();
}

WindowEvents Context::GetWindowEvents(WindowID in) const noexcept
{
	auto& implData = GetImplData();

	std::scoped_lock lock { implData.windowsLock };

	auto const* windowNodePtr = implData.GetWindowNode(in);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto const& windowNode = *windowNodePtr;
	return windowNode.events;
}

auto Context::CreateVkSurface(
	WindowID window,
	uSize vkInstance,
	void const* vkAllocationCallbacks) noexcept
		-> CreateVkSurface_ReturnT
{
	auto& implData = GetImplData();

	std::scoped_lock lock { implData.windowsLock };

	auto windowNodePtr = implData.GetWindowNode(window);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	return impl::Backend::CreateVkSurface(
		implData,
		implData.backendData,
		windowNode.platformHandle,
		vkInstance,
		vkAllocationCallbacks);
}

void Context::LockCursor(bool state)
{

}

void Context::SetCursor(WindowID window, CursorType cursor) noexcept
{

}

Std::Opt<GamepadState> Context::GetGamepad(uSize index) const noexcept
{
	auto& appData = GetImplData();

	return appData.gamepadState;
}

void Context::Log(LogSeverity severity, Std::Span<char const> msg)
{
	auto& implData = GetImplData();
	impl::Backend::Log(implData, severity, msg);
}

void Context::StartTextInputSession(SoftInputFilter inputFilter, Std::Span<char const> text)
{
	auto& implData = GetImplData();

	bool success = impl::Backend::StartTextInputSession(
		implData,
		implData.backendData,
		inputFilter,
		text);

	DENGINE_IMPL_APPLICATION_ASSERT(success);

	if (success)
	{
		implData.textInputSessionActive = true;
		implData.textInputSelectedIndex = text.Size();
	}
}

void Context::StopTextInputSession()
{
	auto& implData = GetImplData();
	impl::Backend::StopTextInputSession(implData, implData.backendData);
}

void Context::InsertEventForwarder(EventForwarder& in)
{
	auto& implData = GetImplData();
	implData.eventForwarders.push_back(&in);
}

void Context::RemoveEventForwarder(EventForwarder& in)
{
	auto& implData = GetImplData();

	auto const eventForwarderIt = Std::FindIf(
		implData.eventForwarders.begin(),
		implData.eventForwarders.end(),
		[&in](auto const& element) { return &in == element; });
	DENGINE_IMPL_APPLICATION_ASSERT(eventForwarderIt != implData.eventForwarders.end());

	implData.eventForwarders.erase(eventForwarderIt);
}

auto Context::Impl::GetWindowNode(WindowID id) -> WindowNode*
{
	auto windowNodeIt = Std::FindIf(
		windows.begin(),
		windows.end(),
		[id](auto const& val) { return val.id == id; });
	if (windowNodeIt == windows.end())
		return nullptr;
	else
		return &(*windowNodeIt);
}

auto Context::Impl::GetWindowNode(WindowID id) const -> WindowNode const*
{
	auto windowNodeIt = Std::FindIf(
		windows.begin(),
		windows.end(),
		[id](auto const& val) { return val.id == id; });
	if (windowNodeIt == windows.end())
		return nullptr;
	else
		return &(*windowNodeIt);
}

WindowID Application::Context::Impl::GetWindowId(void* platformHandle) const
{
	auto windowIt = Std::FindIf(
		windows.begin(),
		windows.end(),
		[platformHandle](auto const& item) { return item.platformHandle == platformHandle; });
	DENGINE_IMPL_APPLICATION_ASSERT(windowIt != windows.end());
	auto const& element = *windowIt;

	return element.id;
}

namespace DEngine::Application::impl
{
	template<class T>
	void PushEvent(Context::Impl& implData, T const& in)
	{
		for (auto const& eventCallback : implData.eventForwarders)
		{
			EventCallbackJob job = {};
			job.ptr = eventCallback;
			job.type = GetEventType<T>();
			auto& jobMember = EventUnion_Get<T>(job.eventUnion);
			jobMember = in;

			implData.queuedEventCallbacks.push_back(job);
		}
	}
}

void Application::impl::BackendInterface::UpdateWindowCursorEnter(
	Context::Impl& implData,
	WindowID id,
	bool entered)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	WindowCursorEnterEvent event = {};
	event.window = id;
	event.entered = entered;

	PushEvent(implData, event);
}

void Application::impl::BackendInterface::PushWindowCloseSignal(
	Context::Impl& implData,
	WindowID id)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	WindowCloseSignalEvent event = {};
	event.window = windowNode.id;
	PushEvent(implData, event);
}

void Application::impl::BackendInterface::UpdateWindowPosition(
	Context::Impl& implData,
	WindowID id,
	Math::Vec2Int newPosition)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	windowNode.windowData.position = newPosition;
	windowNode.events.move = true;

	WindowMoveEvent event = {};
	event.position = newPosition;
	event.window = windowNode.id;
	PushEvent(implData, event);
}

void Application::impl::BackendInterface::UpdateWindowSize(
	Context::Impl& implData,
	WindowID id,
	Extent newSize)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	windowNode.windowData.extent = newSize;
	windowNode.events.resize = true;


	WindowResizeEvent event = {};
	event.window = windowNode.id;
	event.extent = newSize;
	event.safeAreaOffset = {};
	event.safeAreaExtent = newSize;
	PushEvent(implData, event);
}

void Application::impl::BackendInterface::UpdateWindowFocus(
	Context::Impl& implData,
	WindowID id,
	bool focusGained)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	WindowFocusEvent event = {};
	event.window = windowNode.id;
	event.focusGained = focusGained;
	PushEvent(implData, event);
}

namespace DEngine::Application::impl::BackendInterface
{
	static void UpdateCursor(
		Context::Impl& implData,
		Context::Impl::WindowNode& windowNode,
		Math::Vec2Int newRelativePos,
		Math::Vec2Int delta)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(implData.cursorOpt.HasValue());
		CursorData& cursorData = implData.cursorOpt.Value();
		cursorData.position = (newRelativePos + windowNode.windowData.position);
		cursorData.positionDelta += delta;

		CursorMoveEvent event = {};
		event.windowId = windowNode.id;
		event.pos = newRelativePos;
		event.delta = cursorData.positionDelta;
		PushEvent(implData, event);
	}

	static void UpdateCursor(
		Context::Impl& implData,
		Context::Impl::WindowNode& windowNode,
		Math::Vec2Int newRelativePos)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(implData.cursorOpt.HasValue());
		CursorData& cursorData = implData.cursorOpt.Value();

		Math::Vec2Int delta = (newRelativePos + windowNode.windowData.position) - cursorData.position;

		return UpdateCursor(
			implData,
			windowNode,
			newRelativePos,
			delta);
	}
}

void Application::impl::BackendInterface::UpdateCursorPosition(
	Context::Impl& implData,
	WindowID id,
	Math::Vec2Int newRelativePosition,
	Math::Vec2Int delta)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	return UpdateCursor(
		implData,
		windowNode,
		newRelativePosition,
		delta);
}

void Application::impl::BackendInterface::UpdateCursorPosition(
	Context::Impl& implData,
	WindowID id,
	Math::Vec2Int newRelativePosition)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	return UpdateCursor(
		implData,
		windowNode,
		newRelativePosition);
}

void Application::impl::BackendInterface::UpdateTouch(
	Context::Impl& implData,
	WindowID windowId,
	TouchEventType eventType,
	u8 touchId,
	f32 x,
	f32 y)
{
	auto windowNodePtr = implData.GetWindowNode(windowId);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	TouchEvent event = {};
	event.windowId = windowId;
	event.id = touchId;
	event.type = eventType;
	event.x = x;
	event.y = y;
	PushEvent(implData, event);
}

void Application::impl::BackendInterface::UpdateButton(
	Context::Impl& implData,
	WindowID id,
	Button button,
	bool pressed)
{
	DENGINE_IMPL_APPLICATION_ASSERT(IsValid(button));

	implData.buttonValues[(int)button] = pressed;
	implData.buttonEvents[(int)button] = pressed ? KeyEventType::Pressed : KeyEventType::Unpressed;

	if (pressed)
	{
		implData.buttonHeldStart[(int)button] = implData.currentNow;
	}
	else
	{
		implData.buttonHeldStart[(int)button] = std::chrono::high_resolution_clock::time_point();
	}

	ButtonEvent event = {};
	event.windowId = id;
	event.btn = button;
	event.state = pressed;
	PushEvent(implData, event);
}

void Application::impl::BackendInterface::PushTextInputEvent(
	Context::Impl& implData,
	WindowID id,
	uSize oldIndex,
	uSize oldCount,
	Std::Span<u32 const> const& newText)
{
	DENGINE_IMPL_APPLICATION_ASSERT(implData.textInputSessionActive);

	uSize oldTextDataSize = implData.textInputDatas.size();
	implData.textInputDatas.resize(oldTextDataSize + newText.Size());
	for (auto i = 0; i < newText.Size(); i++)
		implData.textInputDatas[i + oldTextDataSize] = newText[i];

	implData.textInputSelectedIndex = oldIndex + newText.Size();

	TextInputEvent event = {};
	event.windowId = id;
	event.oldIndex = oldIndex;
	event.oldCount = oldCount;
	event.newTextOffset = oldTextDataSize;
	event.newTextSize = newText.Size();
	PushEvent(implData, event);
}

[[maybe_unused]] void Application::impl::BackendInterface::PushEndTextInputSessionEvent(
	Context::Impl& implData,
	WindowID id)
{
	EndTextInputSessionEvent event = {};
	event.windowId = id;
	PushEvent(implData, event);
}

bool Application::GamepadState::GetKeyState(GamepadKey btn) const noexcept
{
	DENGINE_IMPL_APPLICATION_ASSERT(impl::IsValid(btn));
	return keyStates[(int)btn];
}

KeyEventType Application::GamepadState::GetKeyEvent(GamepadKey btn) const noexcept
{
	DENGINE_IMPL_APPLICATION_ASSERT(impl::IsValid(btn));
	return keyEvents[(int)btn];
}

f32 Application::GamepadState::GetAxisValue(GamepadAxis axis) const noexcept
{
	DENGINE_IMPL_APPLICATION_ASSERT(impl::IsValid(axis));
	return axisValues[(int)axis];
}