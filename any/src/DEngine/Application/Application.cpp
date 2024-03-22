#include <DEngine/Application.hpp>
#include <DEngine/impl/Application.hpp>
#include <DEngine/impl/AppAssert.hpp>

#include <DEngine/Std/Utility.hpp>

#include <chrono>
#include <new>
#include <stdexcept>
#include <future>

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

	static void FlushQueuedEventCallbacks(Context& appCtx, EventForwarder* eventForwarder);

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

static void Application::impl::FlushQueuedEventCallbacks(Context& ctx, EventForwarder* eventForwarder)
{
	auto& implData = ctx.GetImplData();

	if (eventForwarder != nullptr) {
		implData.queuedEventCallbacks.Consume(ctx, implData, *eventForwarder);
	} else {
		implData.queuedEventCallbacks.Clear();
	}
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

	impl::Backend::ProcessEvents(ctx, implData, implData.backendData, waitForEvents, timeoutNs);

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
		if (m_implData->backendData) {
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
	using namespace impl::Backend;

	auto& appData = GetImplData();

	// This code needs to be run as an uninterrupted operation on the backend
	auto windowDataFuture = impl::Backend::RunOnBackendThread2(
		appData.backendData,
		[&](Context::Impl& implData) {
			auto newWindowInfoOpt = impl::Backend::NewWindow_NotAsync(
				implData,
				implData.backendData,
				title,
				extent);
			return newWindowInfoOpt;
		});
	auto newWindowOpt = windowDataFuture.get();
	if (!newWindowOpt.Has())
		throw std::runtime_error("Failed to make window");
	auto const& newWindow = newWindowOpt.Get();
    auto const& windowData = newWindow.windowData;

	NewWindow_ReturnT returnVal = {};
	returnVal.windowId = newWindow.windowId;
	returnVal.extent = windowData.extent;
	returnVal.position = windowData.position;
	returnVal.visibleExtent = windowData.visibleExtent;
	returnVal.visibleOffset = windowData.visibleOffset;
	returnVal.dpiX = windowData.dpiX;
	returnVal.dpiY = windowData.dpiY;
	returnVal.contentScale = windowData.contentScale;

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
	Std::ConstByteSpan textData) noexcept
{
	auto& implData = GetImplData();

	impl::Backend::UpdateAccessibility(
		implData,
		implData.backendData,
		windowId,
		range,
		textData);
}

auto Context::CreateVkSurface_ThreadSafe(
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
		*windowNode.backendData,
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

void Context::StartTextInputSession(WindowID windowId, SoftInputFilter inputFilter, Std::Span<char const> text)
{
	auto& implData = GetImplData();

	bool success = impl::Backend::StartTextInputSession(
		implData,
		windowId,
		implData.backendData,
		inputFilter,
		text);
	DENGINE_IMPL_APPLICATION_ASSERT(success);
}

void Context::UpdateTextInputConnection(u64 selIndex, u64 selCount, Std::Span<u32 const> text)
{
	auto& implData = GetImplData();
	impl::Backend::UpdateTextInputConnection(
		implData.backendData,
		selIndex,
		selCount,
		text);
}

void Context::UpdateTextInputConnectionSelection(u64 selIndex, u64 selCount) {
	auto& implData = GetImplData();
	impl::Backend::UpdateTextInputConnectionSelection(
		implData.backendData,
		selIndex,
		selCount);
}

void Context::StopTextInputSession()
{
	auto& implData = GetImplData();
	impl::Backend::StopTextInputSession(implData, implData.backendData);
}

auto Context::Impl::GetWindowBackend(void* platformHandle) -> impl::WindowBackendData& {
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

Std::Opt<WindowID> Application::Context::Impl::GetWindowId(void* platformHandle) const
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
namespace DEngine::Application::impl
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

void BackendInterface::UpdateWindowPosition(
	Context::Impl& implData,
	WindowID id,
	Math::Vec2Int newPosition)
{
	auto windowNodePtr = implData.GetWindowNode(id);
	DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	windowNode.windowData.position = newPosition;
	windowNode.windowData.SetEventFlag(WindowEventFlag::Move);
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
    auto& windowData = windowNode.windowData;

    bool isDifferent = windowData.extent != windowExtent ||
    windowData.visibleOffset.x != safeAreaOffsetX ||
    windowData.visibleOffset.y != safeAreaOffsetY ||
    windowData.visibleExtent != safeAreaExtent;
    if (!isDifferent) {
        return;
    }

	windowNode.windowData.extent = windowExtent;
	windowNode.windowData.visibleOffset = { safeAreaOffsetX, safeAreaOffsetY };
	windowNode.windowData.visibleExtent = safeAreaExtent;
	windowNode.windowData.SetEventFlag(WindowEventFlag::Resize);
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

	windowNode.windowData.SetEventFlag(WindowEventFlag::Focus);

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
			[
				windowId = windowNode.id,
			    newRelativePos,
			    positionDelta = cursorData.positionDelta] (
				Context& ctx,
				Context::Impl& implData,
				EventForwarder& forwarder)
			{
				forwarder.CursorMove(
					ctx,
					windowId,
					newRelativePos,
					positionDelta);
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