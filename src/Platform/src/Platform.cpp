#include <DEngine/Platform/Platform.hpp>
#include <DEngine/Platform/PlatformAssert.hpp>
#include <DEngine/Platform/PlatformImpl.hpp>

#include <chrono>
#include <new>
#include <stdexcept>
#include <future>

namespace DEngine::Platform::impl
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

	static void FlushQueuedEventCallbacks(Context& appCtx, EventForwarder* eventForwarder);

	void DestroyWindow(
		Context::Impl& implData,
		WindowID id,
		bool useLock);
}

using namespace DEngine;
using namespace DEngine::Platform;
namespace XPlatformToBackend = DEngine::Platform::impl::XPlatformToBackend;

auto Context::GetImplData() noexcept -> Impl&
{
	DENGINE_IMPL_APPLICATION_ASSERT(m_implData);
	return *m_implData;
}

auto Context::GetImplData() const noexcept -> Impl const&
{
	DENGINE_IMPL_APPLICATION_ASSERT(m_implData);
	return *m_implData;
}

static void Platform::impl::FlushQueuedEventCallbacks(Context& ctx, EventForwarder* eventForwarder)
{
	auto& implData = ctx.GetImplData();

	if (eventForwarder != nullptr) {
		implData.queuedEventCallbacks.Consume(ctx, implData, *eventForwarder);
	} else {
		implData.queuedEventCallbacks.Clear();
	}
}

void Platform::impl::DestroyWindow(
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

	XPlatformToBackend::DestroyWindow(implData, implData.GetBackendData(), windowNode);
}

auto Platform::Context::Impl::Initialize() -> Context
{
	Context ctx;

	ctx.m_implData = new Impl;
	auto& implData = *ctx.m_implData;

	implData.mBackendData = impl::XPlatformToBackend::Initialize(ctx, implData);

	return ctx;
}

void Platform::Context::Impl::ProcessEvents(
	Context& ctx,
	bool waitForEvents,
	u64 timeoutNs,
	bool ignoreWaitOnFirstCall,
	EventForwarder* eventForwarder)
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
	for (auto& window : implData.windows) {
		window.events = {};
		window.windowData.currTickEventFlags = 0;
	}

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
	for (uSize i = 0; i < implData.touchInputs.Size(); i += 1) {
		if (implData.touchInputs[i].eventType == TouchEventType::Up) {
			implData.touchInputs.EraseUnsorted(i);
			implData.touchInputStartTime.EraseUnsorted(i);
			i -= 1;
		}
		else
			implData.touchInputs[i].eventType = TouchEventType::Unchanged;
	}

	XPlatformToBackend::ProcessEvents(
		ctx, 
		implData, 
		implData.GetBackendData(),
		waitForEvents, 
		timeoutNs);

	// Calculate duration for each button being held.
	for (uSize i = 0; i < (uSize)Button::COUNT; i += 1) {
		if (implData.buttonValues[i])
			implData.buttonHeldDuration[i] = std::chrono::duration<f32>(implData.currentNow - implData.buttonHeldStart[i]).count();
		else
			implData.buttonHeldDuration[i] = 0.f;
	}
	// Calculate duration for each touch input.
	for (uSize i = 0; i < implData.touchInputs.Size(); i += 1) {
		if (implData.touchInputs[i].eventType != TouchEventType::Up)
			implData.touchInputs[i].duration = std::chrono::duration<f32>(implData.currentNow - implData.touchInputStartTime[i]).count();
	}

	// Flush queued event callbacks.
	{
		std::scoped_lock lock { implData.windowsLock };
		impl::FlushQueuedEventCallbacks(ctx, eventForwarder);
	}

	implData.tickCount += 1;
}

Context::~Context() noexcept
{
	if (m_implData) {
		if (m_implData->mBackendData)
			XPlatformToBackend::Destroy(m_implData->mBackendData);
		m_implData->mBackendData = nullptr;
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
		-> Std::Opt<NewWindow_ReturnT>
{
	auto& appData = GetImplData();

	// This code needs to be run as an uninterrupted operation on the backend
	// polling thread
	// This is a requirement on Win32 because a window can only receive
	// events on the thread on which it was created but we want
	// to have all events happening on a single thread.
	auto innerWindowResultOpt = XPlatformToBackend::NewWindow_Blocking(
		appData,
		appData.GetBackendData(),
		title,
		extent);
	if (!innerWindowResultOpt.Has()) {
		return Std::nullOpt;
	}
	auto& innerWindowResult = innerWindowResultOpt.Get();
	auto const& windowData = innerWindowResult.windowData;

	NewWindow_ReturnT returnVal = {};
	returnVal.windowId = innerWindowResult.windowId;
	returnVal.extent = windowData.extent;
	returnVal.insets = windowData.insets;
	returnVal.dpiX = windowData.dpiX;
	returnVal.dpiY = windowData.dpiY;
	returnVal.contentScale = windowData.contentScale;
	returnVal.touchScrollSlopPx = windowData.touchScrollSlopPx;
	returnVal.scrollBarWidthPx = windowData.scrollBarWidthPx;

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

WindowEventFlag Context::GetWindowEventFlags(WindowID in) const noexcept
{
	auto& implData = GetImplData();

	std::scoped_lock lock { implData.windowsLock };

	auto const* windowNodePtr = implData.GetWindowNode(in);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto const& windowNode = *windowNodePtr;
	return (WindowEventFlag)windowNode.windowData.currTickEventFlags;
}

bool Context::IsWindowMinimized(WindowID in) const noexcept {
	auto& implData = GetImplData();

	std::scoped_lock lock { implData.windowsLock };

	auto const* windowNodePtr = implData.GetWindowNode(in);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto const& windowNode = *windowNodePtr;
	return windowNode.windowData.isMinimized;
}

void Context::UpdateAccessibilityData(
	WindowID windowId,
	Std::RangeFnRef<AccessibilityUpdateElement> const& range,
	Std::Span<char const> textData) noexcept
{
	auto& implData = GetImplData();

	XPlatformToBackend::UpdateAccessibility(
		implData,
		implData.GetBackendData(),
		windowId,
		range,
		textData);
}

auto Context::CreateVkSurface_ThreadSafe(
	WindowID window,
	void const* vkGetInstanceProcAddrIn,
	uSize vkInstance,
	void const* vkAllocationCallbacks) noexcept
		-> CreateVkSurface_ReturnT
{
	auto& implData = GetImplData();

	std::scoped_lock lock { implData.windowsLock };

	auto windowNodePtr = implData.GetWindowNode(window);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	return XPlatformToBackend::CreateVkSurface(
		implData,
		implData.GetBackendData(),
		*windowNode.backendData,
		vkGetInstanceProcAddrIn,
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
	XPlatformToBackend::Log(
		implData, 
		implData.GetBackendData(),
		severity, 
		msg);
}

void Context::StartTextInputSession(
	WindowID windowId,
	SoftInputFilter inputFilter,
	Std::Span<char const> text,
	u64 selectionStart,
	u64 selectionCount)
{
	auto& implData = GetImplData();

	bool success = XPlatformToBackend::StartTextInputSession(
		implData,
		windowId,
		implData.GetBackendData(),
		inputFilter,
		text,
		selectionStart,
		selectionCount);
	DENGINE_IMPL_APPLICATION_ASSERT(success);
}

void Context::UpdateTextInputConnection(
	WindowID windowId,
	u64 selIndex,
	u64 selCount,
	Std::Span<u32 const> text)
{
	DENGINE_IMPL_APPLICATION_ASSERT(selIndex + selCount <= text.Size());
	auto& implData = GetImplData();
	XPlatformToBackend::UpdateTextInputConnection(
		implData,
		implData.GetBackendData(),
		windowId,
		selIndex,
		selCount,
		text);
}

void Context::UpdateTextInputConnectionSelection(u64 selIndex, u64 selCount) {
	auto& implData = GetImplData();
	XPlatformToBackend::UpdateTextInputConnectionSelection(
		implData.GetBackendData(),
		selIndex,
		selCount);
}

void Context::StopTextInputSession()
{
	auto& implData = GetImplData();
	XPlatformToBackend::StopTextInputSession(implData, implData.GetBackendData());
}

auto Context::Impl::GetWindowBackend(void* platformHandle) -> impl::WindowPlatformBackendBase& {
	auto temp = this->GetWindowNode(GetWindowId(platformHandle).Get());
	return *temp->backendData.Get();
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

auto Context::Impl::GetWindowData(WindowID id) const -> Std::Opt<WindowData> {
	auto node = this->GetWindowNode(id);
	if (node != nullptr) {
		return node->windowData;
	} else {
		return Std::nullOpt;
	}
}

Std::Opt<WindowID> Platform::Context::Impl::GetWindowId(void* platformHandle) const
{
	auto windowIt = Std::FindIf(
		windows.begin(),
		windows.end(),
		[platformHandle](auto const& item) { return item.backendData->GetRawHandle() == platformHandle; });
	if (windowIt != windows.end()) {
		auto const& element = *windowIt;
		return element.id;
	} else {
		return Std::nullOpt;
	}
}
namespace DEngine::Platform::impl
{
	template<class Callable> requires (!Std::Trait::isRef<Callable>)
	void EnqueueEvent2(Context::Impl& implData, Callable&& in) {
		auto temp = Std::Move(in);
		implData.queuedEventCallbacks.Push(
			[=](Context& ctx, Context::Impl& implData, EventForwarder& eventForwarder) {
				temp(ctx, implData, eventForwarder);
			});
	}
}

using namespace DEngine::Platform::impl;

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
	windowNode.windowData.orientation = newOrientation;
	windowNode.windowData.SetEventFlag(WindowEventFlag::Orientation);

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
	windowNode.windowData.SetEventFlag(WindowEventFlag::ContentScale);

	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			forwarder.WindowContentScale(ctx, id, scale);
		});
}

void BackendInterface::WindowDpi(
	Context::Impl& implData,
	WindowID id,
	float dpiX,
	float dpiY)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	if (windowNode.windowData.dpiX == dpiX && windowNode.windowData.dpiY == dpiY)
		return;

	windowNode.windowData.dpiX = dpiX;
	windowNode.windowData.dpiY = dpiY;
	windowNode.windowData.SetEventFlag(WindowEventFlag::Dpi);

	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			forwarder.WindowDpi(ctx, id, dpiX, dpiY);
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

	// Only apply if any change happened.
	if (windowNode.windowData.isMinimized == minimize)
		return;

	windowNode.windowData.isMinimized = minimize;

	if (minimize)
		windowNode.events.minimize = true;
	else {

		windowNode.events.restore = true;
		windowNode.windowData.SetEventFlag(WindowEventFlag::Restore);
	}


	EnqueueEvent2(
		implData,
		[=](Context&, Context::Impl&, EventForwarder& forwarder) {
			forwarder.WindowMinimize(id, minimize);
		});
}

void BackendInterface::UpdateWindowSize(
	Context::Impl& implData,
	WindowID id,
	Extent windowExtent,
	Std::Array<WindowInsets, (int)WindowInsetSource::COUNT> windowInsets)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;
	auto& windowData = windowNode.windowData;

	bool isDifferent =
		windowData.extent != windowExtent
		|| windowData.insets != windowInsets;
	if (!isDifferent) {
		return;
	}

	if (windowExtent != windowNode.windowData.extent) {
		windowNode.windowData.extent = windowExtent;
		windowNode.windowData.SetEventFlag(WindowEventFlag::Resize);
		windowNode.events.resize = true;
	}

	windowNode.windowData.insets = windowInsets;

	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			forwarder.WindowResize(
				ctx,
				id,
				windowExtent,
				windowInsets);
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

	windowNode.windowData.SetEventFlag(WindowEventFlag::Focus);

	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			forwarder.WindowFocus(id, focusGained);
		});
}

namespace DEngine::Platform::impl::BackendInterface
{
	static void UpdateCursorPos(
		Context::Impl& implData,
		Context::Impl::WindowNode& windowNode,
		Math::Vec2Int newRelativePos,
		Math::Vec2Int delta)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(implData.cursorOpt.HasValue());
		CursorData& cursorData = implData.cursorOpt.Value();
		cursorData.position = newRelativePos;
		cursorData.positionDelta += delta;

		EnqueueEvent2(
			implData,
			[=, windowId = windowNode.id] (
				Context& ctx,
				Context::Impl&,
				EventForwarder& forwarder)
			{
				forwarder.CursorMove(
					ctx,
					windowId,
					cursorData.position,
					cursorData.positionDelta,
					cursorData);
			});
	}

	static void UpdateCursorPos(
		Context::Impl& implData,
		Context::Impl::WindowNode& windowNode,
		Math::Vec2Int newRelativePos)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(implData.cursorOpt.HasValue());
		CursorData& cursorData = implData.cursorOpt.Value();

		Math::Vec2Int delta = (newRelativePos) - cursorData.position;

		return UpdateCursorPos(
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

	return UpdateCursorPos(
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

	return UpdateCursorPos(
		implData,
		windowNode,
		newRelativePosition);
}

void BackendInterface::UpdateCursorButton(
	Context::Impl& implData,
	WindowID windowId,
	CursorButton button,
	bool pressed)
{
	DENGINE_IMPL_APPLICATION_ASSERT(implData.cursorOpt.HasValue());
	CursorData& cursorData = implData.cursorOpt.Value();
	cursorData.buttonPressedArray[(int)button] = pressed;

	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			forwarder.CursorPress(
				ctx,
				windowId,
				button,
				pressed,
				cursorData);
		});
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
			forwarder.ButtonEvent(ctx, id, button, pressed);
		});
}

void BackendInterface::PushTextInputEvent(
	Context::Impl& implData,
	WindowID id,
	u64 start,
	u64 count,
	Std::Span<u32 const> const& newText)
{
	std::vector<u32> inputText;
	inputText.resize(newText.Size());
	for (int i = 0; i < newText.Size(); i++) {
		inputText[i] = newText[i];
	}

	EnqueueEvent2(
		implData,
		[=, text = Std::Move(inputText)](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			forwarder.TextInputEvent(
				ctx,
				id,
				start,
				count,
				{ text.data(), text.size() });
		});
}

void BackendInterface::PushTextSelectionEvent(
	Context::Impl& implData,
	WindowID id,
	u64 start,
	u64 count)
{
	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			forwarder.TextSelectionEvent(
				ctx,
				id,
				start,
				count);
		});
}

void BackendInterface::PushTextDeleteEvent(
	Context::Impl& implData,
	WindowID windowId)
{
	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl& implData, EventForwarder& forwarder) {
			forwarder.TextDeleteEvent(ctx, windowId);
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

void BackendInterface::UpdateGamepadAxis(
	Context::Impl& implData,
	GamepadDeviceId deviceId,
	GamepadAxis axis,
	f32 rawValue)
{
	DENGINE_IMPL_APPLICATION_ASSERT(IsValid(axis));

	f32 const deadzone = implData.gamepadState.stickDeadzone;
	f32 const effectiveValue = (rawValue > -deadzone && rawValue < deadzone) ? 0.f : rawValue;

	f32& stored = implData.gamepadState.axisValues[(int)axis];
	if (stored == effectiveValue)
		return;
	stored = effectiveValue;

	WindowID windowId = WindowID::Invalid;
	if (!implData.windows.empty())
		windowId = implData.windows.front().id;

	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl&, EventForwarder& forwarder) {
			forwarder.GamepadAxisEvent(ctx, windowId, deviceId, axis, effectiveValue);
		});
}

void BackendInterface::UpdateGamepadKey(
	Context::Impl& implData,
	GamepadDeviceId deviceId,
	GamepadKey key,
	bool pressed)
{
	DENGINE_IMPL_APPLICATION_ASSERT(IsValid(key));

	implData.gamepadState.keyStates[(int)key] = pressed;
	implData.gamepadState.keyEvents[(int)key] = pressed ? KeyEventType::Pressed : KeyEventType::Unpressed;

	WindowID windowId = WindowID::Invalid;
	if (!implData.windows.empty())
		windowId = implData.windows.front().id;

	EnqueueEvent2(
		implData,
		[=](Context& ctx, Context::Impl&, EventForwarder& forwarder) {
			forwarder.GamepadKeyEvent(ctx, windowId, deviceId, key, pressed);
		});
}

bool Platform::GamepadState::GetKeyState(GamepadKey btn) const noexcept
{
	DENGINE_IMPL_APPLICATION_ASSERT(impl::IsValid(btn));
	return keyStates[(int)btn];
}

KeyEventType Platform::GamepadState::GetKeyEvent(GamepadKey btn) const noexcept
{
	DENGINE_IMPL_APPLICATION_ASSERT(impl::IsValid(btn));
	return keyEvents[(int)btn];
}

f32 Platform::GamepadState::GetAxisValue(GamepadAxis axis) const noexcept
{
	DENGINE_IMPL_APPLICATION_ASSERT(impl::IsValid(axis));
	return axisValues[(int)axis];
}