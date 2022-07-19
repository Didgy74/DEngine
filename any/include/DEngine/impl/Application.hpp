#pragma once

#include <DEngine/Application.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Std/Containers/FnRef.hpp>
#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Containers/Vec.hpp>

#include <chrono>
#include <vector>
#include <mutex>

#if defined(__ANDROID__)
#	define DENGINE_APP_MAIN_ENTRYPOINT dengine_app_detail_main
#else
#	define DENGINE_APP_MAIN_ENTRYPOINT main
#endif

struct DEngine::Application::Context::Impl
{
	[[nodiscard]] static Context Initialize();
	static void ProcessEvents(
		Context& ctx,
		bool waitForEvents,
		u64 timeoutNs,
		bool returnImmediateOnFirstCall);

	Impl() = default;
	Impl(Impl const&) = delete;
	Impl(Impl&&) = delete;


	bool isFirstCall = true;
	void* backendData = nullptr;

	// Window data start
	struct WindowData
	{
		Math::Vec2Int position = {};
		Extent extent = {};
		Math::Vec2UInt visibleOffset = {};
		Extent visibleExtent = {};
		Orientation orientation = {};

		bool isMinimized = false;
		bool shouldShutdown = false;

		CursorType currentCursorType = CursorType::Arrow;
	};

	mutable std::mutex windowsLock;
	u64 windowIdTracker = 0;
	struct WindowNode
	{
		WindowID id = {};
		WindowData windowData = {};
		WindowEvents events = {};
		void* platformHandle = nullptr;
	};
	std::vector<WindowNode> windows;
	[[nodiscard]] WindowNode* GetWindowNode(WindowID id);
	[[nodiscard]] WindowNode const* GetWindowNode(WindowID id) const;
	[[nodiscard]] WindowID GetWindowId(void* platformHandle) const;
	// Window data end

	Orientation currentOrientation = Orientation::Invalid;
	bool orientationEvent = false;

	// Input
	bool buttonValues[(int)Button::COUNT] = {};
	std::chrono::high_resolution_clock::time_point buttonHeldStart[(int)Button::COUNT] = {};
	f32 buttonHeldDuration[(int)Button::COUNT] = {};
	KeyEventType buttonEvents[(int)Button::COUNT] = {};

	Std::Opt<CursorData> cursorOpt = {};

	Std::StackVec<TouchInput, maxTouchEventCount> touchInputs = {};
	Std::StackVec<std::chrono::high_resolution_clock::time_point, maxTouchEventCount> touchInputStartTime{};

	// Time related stuff
	u64 tickCount = 0;
	f32 deltaTime = 1.f / 60.f;
	std::chrono::high_resolution_clock::time_point currentNow{};
	std::chrono::high_resolution_clock::time_point previousNow{};

	GamepadState gamepadState = {};

	// Polymorphic callback events.
	// These call specific functions when events happen,
	// in the order they happened in. We can register
	// multiple. They will all be called at the end of
	// the event processing, as to not take up any unnecessary time
	// from OS polling.
	// Pending event callbacks are stored in a vector.
	std::vector<EventForwarder*> eventForwarders;


	std::vector<Std::FnRef<void(Context&, Context::Impl&, EventForwarder&)>> queuedEventCallbacks;
	// USE ONLY WITH THE queuedEventCallbacks vector!!!!
	Std::FrameAlloc queuedEvents_InnerBuffer = Std::FrameAlloc::PreAllocate(1024).Get();


	bool textInputSessionActive = false;
	uSize textInputSelectedIndex = 0;
	std::vector<u32> textInputDatas;
};

namespace DEngine::Application::impl::Backend
{
	[[nodiscard]] void* Initialize(Context& ctx, Context::Impl& implData);
	void ProcessEvents(
		Context& ctx,
		Context::Impl& implData,
		void* pBackendData,
		bool waitForEvents,
		u64 timeoutNs);
	void Destroy(void* data);
	struct NewWindow_ReturnT
	{
		Context::Impl::WindowData windowData = {};
		void* platformHandle {};
	};
	[[nodiscard]] Std::Opt<NewWindow_ReturnT> NewWindow(
		Context& ctx,
		Context::Impl& implData,
		void* pBackendData,
		WindowID windowId,
		Std::Span<char const> const& title,
		Extent extent);
	void DestroyWindow(
		Context::Impl& implData,
		void* backendData,
		Context::Impl::WindowNode const& windowNode);
	[[nodiscard]] Context::CreateVkSurface_ReturnT CreateVkSurface(
		Context::Impl& implData,
		void* backendData,
		void* platformHandle,
		uSize vkInstance,
		void const* vkAllocationCallbacks) noexcept;

	void Log(
		Context::Impl& implData,
		LogSeverity severity,
		Std::Span<char const> const& msg);

	bool StartTextInputSession(
		Context::Impl& implData,
		void* backendData,
		SoftInputFilter inputFilter,
		Std::Span<char const> const& text);
	void StopTextInputSession(
		Context::Impl& implData,
		void* backendData);
}

namespace DEngine::Application::impl
{
	[[nodiscard]] inline auto Initialize() { return Context::Impl::Initialize(); }
	inline void ProcessEvents(
		Context& ctx,
		bool waitForEvents,
		u64 timeoutNs,
		bool ignoreWaitOnFirstCall)
	{
		return Context::Impl::ProcessEvents(ctx, waitForEvents, timeoutNs, ignoreWaitOnFirstCall);
	}
}

namespace DEngine::Application::impl::BackendInterface
{
	[[maybe_unused]] void PushWindowCloseSignal(
		Context::Impl& implData,
		WindowID id);

	// Send false for a restore event, or true for a minimize event.
	[[maybe_unused]] void PushWindowMinimizeSignal(
		Context::Impl& implData,
		WindowID id,
		bool minimized);

	[[maybe_unused]] void UpdateWindowCursorEnter(
		Context::Impl& implData,
		WindowID id,
		bool entered);

	[[maybe_unused]] void WindowReorientation(
		Context::Impl& implData,
		WindowID id,
		Orientation newOrientation);

	[[maybe_unused]] void UpdateWindowFocus(
		Context::Impl& implData,
		WindowID id,
		bool focusGained);

	[[maybe_unused]] void UpdateWindowPosition(
		Context::Impl& implData,
		WindowID id,
		Math::Vec2Int newPosition);

	[[maybe_unused]] void UpdateWindowSize(
		Context::Impl& implData,
		WindowID id,
		Extent windowExtent,
		u32 safeAreaOffsetX,
		u32 safeAreaOffsetY,
		Extent safeAreaExtent);

	[[maybe_unused]] void UpdateCursorPosition(
		Context::Impl& implData,
		WindowID id,
		Math::Vec2Int newRelativePosition,
		Math::Vec2Int delta);
	[[maybe_unused]] void UpdateCursorPosition(
		Context::Impl& implData,
		WindowID id,
		Math::Vec2Int newRelativePosition);
	[[maybe_unused]] void UpdateTouch(
		Context::Impl& implData,
		WindowID windowId,
		TouchEventType eventType,
		u8 touchId,
		f32 x,
		f32 y);
	[[maybe_unused]] void UpdateButton(
		Context::Impl& implData,
		WindowID id,
		Button button,
		bool pressed);

	[[maybe_unused]] void PushTextInputEvent(
		Context::Impl& implData,
		WindowID id,
		uSize oldIndex,
		uSize oldCount,
		Std::Span<u32 const> const& newText);

	[[maybe_unused]] void PushEndTextInputSessionEvent(
		Context::Impl& implData,
		WindowID id);
}