#include <DEngine/Application.hpp>
#include <DEngine/impl/Application.hpp>
#include <DEngine/impl/AppAssert.hpp>

#include <DEngine/Std/Utility.hpp>

#include <chrono>
#include <new>
#include <stdexcept>

namespace DEngine::Application::impl
{
	[[nodiscard]] static constexpr bool IsValid(Button in) noexcept {
		return (unsigned int)in < (unsigned int)Button::COUNT;
	}
	[[nodiscard]] static constexpr bool IsValid(GamepadKey in) noexcept {
		return (unsigned int)in < (unsigned int)GamepadKey::COUNT;
	}
	[[nodiscard]] static constexpr bool IsValid(GamepadAxis in) noexcept {
		return (unsigned int)in < (unsigned int)GamepadAxis::COUNT;
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

static void Application::impl::FlushQueuedEventCallbacks(Context& ctx)
{
	auto& implData = ctx.GetImplData();

	for(auto const& job : implData.queuedEventCallbacks) {
		for (auto* eventForwarder : implData.eventForwarders)
			job(ctx, implData, *eventForwarder);
	}
	implData.queuedEventCallbacks.clear();
	implData.queuedEvents_InnerBuffer.Reset(false);
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
	returnVal.dpiX = newNode.windowData.dpiX;
	returnVal.dpiY = newNode.windowData.dpiY;
	returnVal.contentScale = newNode.windowData.contentScale;

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

bool Context::IsWindowMinimized(WindowID in) const noexcept {
	auto& implData = GetImplData();

	std::scoped_lock lock { implData.windowsLock };

	auto const* windowNodePtr = implData.GetWindowNode(in);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto const& windowNode = *windowNodePtr;
	return windowNode.windowData.isMinimized;
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
	template<class Callable>
	void EnqueueEvent2(Context::Impl& implData, Callable&& in) {

		auto* tempPtr = (Callable*)implData.queuedEvents_InnerBuffer.Alloc(sizeof(Callable), alignof(Callable));
		DENGINE_IMPL_APPLICATION_ASSERT(tempPtr != nullptr);
		new(tempPtr) Callable(Std::Move(in));

		implData.queuedEventCallbacks.emplace_back(*tempPtr);
	}
}

using namespace DEngine::Application::impl;

void BackendInterface::UpdateWindowCursorEnter(
	Context::Impl& implData,
	WindowID id,
	bool entered)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	windowNode.events.cursorEnter = true;

	EnqueueEvent2(
		implData,
		[=](Context&, Context::Impl&, EventForwarder& forwarder) {
			forwarder.WindowCursorEnter(id, entered);
		});
}

void BackendInterface::WindowReorientation(
	Context::Impl& implData,
	WindowID id,
	Orientation newOrientation)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	windowNode.events.orientation = true;

	//DENGINE_IMPL_APPLICATION_ASSERT(windowData.orientation != newOrientation);
}

void BackendInterface::WindowContentScale(
	Context::Impl& implData,
	WindowID id,
	float scale)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	windowNode.windowData.contentScale = scale;

	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			forwarder.WindowContentScale(ctx, id, scale);
		});
}

void BackendInterface::PushWindowCloseSignal(Context::Impl& implData, WindowID id){
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			auto destroyWindow = forwarder.WindowCloseSignal(ctx, id);
			if (destroyWindow) {
				constexpr bool useLock = false;
				DestroyWindow(implData, id, useLock);
			}
		});
}

void BackendInterface::PushWindowMinimizeSignal(
	Context::Impl& implData,
	WindowID id,
	bool minimize)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;
	windowNode.windowData.isMinimized = minimize;

	if (minimize)
		windowNode.events.minimize = true;
	else
		windowNode.events.restore = true;

	EnqueueEvent2(
		implData,
		[=](Context&, Context::Impl&, EventForwarder& forwarder) {
			forwarder.WindowMinimize(id, minimize);
		});
}

void BackendInterface::UpdateWindowPosition(
	Context::Impl& implData,
	WindowID id,
	Math::Vec2Int newPosition)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	windowNode.windowData.position = newPosition;
	windowNode.events.move = true;

	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			forwarder.WindowMove(id, newPosition);
		});
}

void BackendInterface::UpdateWindowSize(
	Context::Impl& implData,
	WindowID id,
	Extent windowExtent,
	u32 safeAreaOffsetX,
	u32 safeAreaOffsetY,
	Extent safeAreaExtent)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	windowNode.windowData.extent = windowExtent;
	windowNode.windowData.visibleOffset = { safeAreaOffsetX, safeAreaOffsetY };
	windowNode.windowData.visibleExtent = safeAreaExtent;
	windowNode.events.resize = true;

	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			forwarder.WindowResize(
				ctx,
				id,
				windowExtent,
				{ safeAreaOffsetX, safeAreaOffsetY },
				safeAreaExtent);
		});
}

void BackendInterface::UpdateWindowFocus(
	Context::Impl& implData,
	WindowID id,
	bool focusGained)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			forwarder.WindowFocus(id, focusGained);
		});
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

		EnqueueEvent2(
			implData,
			[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
				forwarder.CursorMove(
					ctx,
					windowNode.id,
					newRelativePos,
					cursorData.positionDelta);
			});
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

void BackendInterface::UpdateCursorPosition(
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

void BackendInterface::UpdateCursorPosition(
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

void BackendInterface::UpdateTouch(
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

	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			forwarder.TouchEvent(
				windowId,
				touchId,
				eventType,
				{ x, y });
		});
}

void BackendInterface::UpdateButton(
	Context::Impl& implData,
	WindowID id,
	Button button,
	bool pressed)
{
	DENGINE_IMPL_APPLICATION_ASSERT(IsValid(button));

	implData.buttonValues[(int)button] = pressed;
	implData.buttonEvents[(int)button] = pressed ? KeyEventType::Pressed : KeyEventType::Unpressed;

	if (pressed)
		implData.buttonHeldStart[(int)button] = implData.currentNow;
	else
		implData.buttonHeldStart[(int)button] = std::chrono::high_resolution_clock::time_point();

	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			forwarder.ButtonEvent(id, button, pressed);
		});
}

void BackendInterface::PushTextInputEvent(
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

	auto newTextOffset = oldTextDataSize;
	auto newTextSize = newText.Size();
	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			DENGINE_IMPL_APPLICATION_ASSERT(newTextOffset + newTextSize <= implData.textInputDatas.size());
			forwarder.TextInputEvent(
				ctx,
				id,
				oldIndex,
				oldCount,
				{ implData.textInputDatas.data() + newTextOffset, newTextSize });
		});
}

void BackendInterface::PushEndTextInputSessionEvent(
	Context::Impl& implData,
	WindowID id)
{
	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			forwarder.EndTextInputSessionEvent(ctx, id);
		});
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