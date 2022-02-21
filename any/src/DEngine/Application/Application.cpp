#include <DEngine/Application.hpp>
#include <DEngine/impl/Application.hpp>
#include <DEngine/impl/AppAssert.hpp>

#include <DEngine/Std/Utility.hpp>

#include <chrono>
#include <new>

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
		using Type = EventCallbackJob::Type;
		switch (job.type)
		{
			case Type::ButtonEvent:
			{
				auto const& event = job.buttonEvent;
				job.ptr->ButtonEvent(
					event.windowId,
					event.btn,
					event.state);
				break;
			}
			case Type::CharEvent:
			{
				auto const& event = job.charEvent;
				job.ptr->CharEvent(job.charEvent.utfValue);
				break;
			}
			case Type::CharEnterEvent:
			{
				job.ptr->CharEnterEvent();
				break;
			}
			case Type::CharRemoveEvent:
			{
				job.ptr->CharRemoveEvent(job.charRemoveEvent.windowId);
				break;
			}
			case Type::CursorMoveEvent:
			{
				auto const& event = job.cursorMoveEvent;
				job.ptr->CursorMove(
					appCtx,
					event.windowId,
					event.pos,
					event.delta);
				break;
			}

			case Type::TextInputEvent:
			{
				auto const& event = job.textInputEvent;
				DENGINE_IMPL_APPLICATION_ASSERT(event.newTextOffset + event.newTextSize <= implData.textInputDatas.size());
				job.ptr->TextInputEvent(
					appCtx,
					event.windowId,
					event.oldIndex,
					event.oldCount,
					{ implData.textInputDatas.data() + event.newTextOffset, event.newTextSize });
				break;
			}

			case Type::TouchEvent:
			{
				auto const& event = job.touchEvent;
				job.ptr->TouchEvent(
					event.id,
					event.type,
					{ event.x, event.y });
				break;
			}
			case Type::WindowCursorEnterEvent:
			{
				auto const& event = job.windowCursorEnterEvent;
				job.ptr->WindowCursorEnter(
					event.window,
					event.entered);
				break;
			}


			case Type::WindowCloseSignalEvent:
			{
				auto const& event = job.windowCloseSignalEvent;
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
			case Type::WindowFocusEvent:
			{
				auto const& event = job.windowFocusEvent;
				job.ptr->WindowFocus(
					event.window,
					event.focusGained);
				break;
			}
			case Type::WindowMoveEvent:
			{
				auto const& event = job.windowMoveEvent;
				job.ptr->WindowMove(
					event.window,
					event.position);
				break;
			}
			case Type::WindowResizeEvent:
			{
				auto const& event = job.windowResizeEvent;
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

	auto windowNode = Std::Move(*windowNodeIt);

	implData.windows.erase(windowNodeIt);

	impl::Backend::DestroyWindow(implData, windowNode);
}

auto Application::Context::Impl::Initialize() -> Context
{
	Context ctx;

	auto* appData = new Impl;
	ctx.m_implData = appData;

	appData->backendData = impl::Backend::Initialize(ctx);

	return ctx;
}

void Application::Context::Impl::ProcessEvents(
	Context& ctx,
	impl::PollMode pollMode,
	Std::Opt<u64> const& timeoutNs)
{
	DENGINE_IMPL_APPLICATION_ASSERT(impl::IsValid(pollMode));

	auto& implData = ctx.GetImplData();

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

	impl::Backend::ProcessEvents(ctx, pollMode, timeoutNs);

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

	auto newWindowInfo = impl::Backend::NewWindow(*this, title, extent);

	Impl::WindowNode newNode = {};
	newNode.id = (WindowID)appData.windowIdTracker;
	newNode.platformHandle = newWindowInfo.platormHandle;
	newNode.windowData = newWindowInfo.windowData;

	{
		std::scoped_lock lock { appData.windowsLock };
		appData.windowIdTracker += 1;
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

	bool success = impl::Backend::StartTextInputSession(*this, inputFilter, text);

	DENGINE_IMPL_APPLICATION_ASSERT(success);

	if (success)
	{
		implData.textInputSessionActive = true;
		implData.textInputSelectedIndex = text.Size();
	}
}

void Context::StopTextInputSession()
{
	impl::Backend::StopTextInputSession(*this);
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
			job.type = EventCallbackJob::GetEventType<T>();
			auto& jobMember = job.Get<T>();
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

	EventCallbackJob::WindowCursorEnterEvent event = {};
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

	EventCallbackJob::WindowCloseSignalEvent event = {};
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

	EventCallbackJob::WindowMoveEvent event = {};
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


	EventCallbackJob::WindowResizeEvent event = {};
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

	EventCallbackJob::WindowFocusEvent event = {};
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

		EventCallbackJob::CursorMoveEvent event = {};
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

	EventCallbackJob::ButtonEvent event = {};
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
	std::string temp;
	temp += std::to_string(implData.textInputSelectedIndex);
	Backend::Log(implData, LogSeverity::Debug, { temp.data(), temp.size() });

	EventCallbackJob::TextInputEvent event = {};
	event.windowId = id;
	event.oldIndex = oldIndex;
	event.oldCount = oldCount;
	event.newTextOffset = oldTextDataSize;
	event.newTextSize = newText.Size();
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