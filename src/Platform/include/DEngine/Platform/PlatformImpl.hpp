#pragma once

#include <DEngine/Platform/PlatformAssert.hpp>
#include <DEngine/Platform/Platform.hpp>

#include <DEngine/Std/Containers/FnScratchList.hpp>

#include <vector>
#include <mutex>
#include <chrono>

import DEngine.Std.Box;

#if !defined(DENGINE_PLATFORM_OS)
#error "Error: DEngine-define 'DENGINE_OS' has not been defined. Likely an error in build-script."
#elif DENGINE_PLATFORM_OS == 0 // Windows
#define DENGINE_MAIN_ENTRYPOINT dengine_impl_main
#elif DENGINE_PLATFORM_OS == 2 // Android
#define DENGINE_MAIN_ENTRYPOINT dengine_impl_main
#elif DENGINE_PLATFORM_OS == 3 // macOS
#define DENGINE_MAIN_ENTRYPOINT dengine_impl_main
#else
#error "Unrecognized OS. Likely an error in the build-script"
#define DENGINE_MAIN_ENTRYPOINT main
#endif

namespace DEngine::Platform::impl {
	struct WindowPlatformBackendBase {
		[[nodiscard]] virtual void* GetRawHandle() = 0;
		[[nodiscard]] virtual void const* GetRawHandle() const = 0;
		virtual ~WindowPlatformBackendBase() = default;;
	};
}

struct DEngine::Platform::Context::Impl
{
	[[nodiscard]] static Context Initialize();
	static void ProcessEvents(
		Context& ctx,
		bool waitForEvents,
		u64 timeoutNs,
		bool returnImmediateOnFirstCall,
		EventForwarder* eventForwarder);

	Impl() = default;
	Impl(Impl const&) = delete;
	Impl(Impl&&) = delete;

	bool isFirstCall = true;
	class PlatformBackendBase {
	public:
		PlatformBackendBase() = default;
		PlatformBackendBase(PlatformBackendBase const&) = delete;
		virtual ~PlatformBackendBase() = 0;
	};
	PlatformBackendBase* mBackendData = nullptr;
	[[nodiscard]] PlatformBackendBase& GetBackendData() noexcept { 
		DENGINE_IMPL_APPLICATION_ASSERT(mBackendData != nullptr);
		return *mBackendData; 
	}
	[[nodiscard]] PlatformBackendBase const& GetBackendData() const noexcept {
		DENGINE_IMPL_APPLICATION_ASSERT(mBackendData != nullptr);
		return *mBackendData;
	}


	// Window data start
	struct WindowData {
		Extent extent = {};
		Std::Array<WindowInsets, (int)WindowInsetSource::COUNT> insets = {};
		Orientation orientation = {};

		f32 dpiX = 0.f;
		f32 dpiY = 0.f;
		f32 contentScale = 1.f;

		// Holds the preferred touch scroll slop in scaled pixels (don't apply DPI).
		//
		// TODO: We need to write events that allows these values
		// to change, such as when our Android Activity moves to another display.
		Std::Opt<u64> touchScrollSlopPx;
		Std::Opt<u64> scrollBarWidthPx;

		bool isMinimized = false;
		bool shouldShutdown = false;
		//Orientation currentOrientation = Orientation::Invalid;

		CursorType currentCursorType = CursorType::Arrow;

		u64 currTickEventFlags = 0;
		void SetEventFlag(WindowEventFlag in) {
			if (in == WindowEventFlag::Resize) {
				int i = 0;
			}
			this->currTickEventFlags |= (u64)in;
		}
	};

	mutable std::mutex windowsLock;
	u64 windowIdTracker = 0;
	struct WindowNode {
		WindowID id = {};
		WindowData windowData = {};
		WindowEvents events = {};
		Std::Box<impl::WindowPlatformBackendBase> backendData;
	};
	std::vector<WindowNode> windows;
	[[nodiscard]] impl::WindowPlatformBackendBase& GetWindowBackend(void* platformHandle);
	[[nodiscard]] WindowNode* GetWindowNode(WindowID id);
	[[nodiscard]] WindowNode const* GetWindowNode(WindowID id) const;
	[[nodiscard]] Std::Opt<WindowData> GetWindowData(WindowID) const;
	[[nodiscard]] Std::Opt<WindowID> GetWindowId(void* platformHandle) const;
	// Window data end

	Orientation currentOrientation = Orientation::Invalid;
	bool orientationEvent = false;

	// Input
	bool buttonValues[(int)Button::COUNT] = {};
	std::chrono::high_resolution_clock::time_point buttonHeldStart[(int)Button::COUNT] = {};
	f32 buttonHeldDuration[(int)Button::COUNT] = {};
	KeyEventType buttonEvents[(int)Button::COUNT] = {};

	// TODO: We should store which window the cursor is currently in.
	Std::Opt<CursorData> cursorOpt = {};

	Std::StackVec<TouchInput, maxTouchEventCount> touchInputs = {};
	Std::StackVec<std::chrono::high_resolution_clock::time_point, maxTouchEventCount> touchInputStartTime{};

	// Time related stuff
	u64 tickCount = 0;
	f32 deltaTime = 1.f / 60.f;
	std::chrono::high_resolution_clock::time_point currentNow{};
	std::chrono::high_resolution_clock::time_point previousNow{};

	GamepadState gamepadState = {};

	Std::FnScratchList<Context&, Context::Impl&, EventForwarder&> queuedEventCallbacks;
	// USE ONLY WITH THE queuedEventCallbacks vector!!!!
	//Std::FrameAlloc queuedEvents_InnerBuffer = Std::FrameAlloc::PreAllocate(1024).Get()
};

inline DEngine::Platform::Context::Impl::PlatformBackendBase::~PlatformBackendBase() {}

// The XPlatformToBackend contains functions that need to be implemented by the actual
// backend, that the cross-platform layer is allowed to call.
namespace DEngine::Platform::impl::XPlatformToBackend
{
	// Must be called from main game-thread.
	[[nodiscard]] Context::Impl::PlatformBackendBase* Initialize(Context& ctx, Context::Impl& implData);
	// Must be called from main game-thread.
	void ProcessEvents(
		Context& ctx,
		Context::Impl& implData,
		Context::Impl::PlatformBackendBase& backendData,
		bool waitForEvents,
		u64 timeoutNs);
	// Must be called from main game-thread.
	void Destroy(Context::Impl::PlatformBackendBase* data);

	struct NewWindow_ReturnT {
		WindowID windowId = {};
		Context::Impl::WindowData windowData = {};
	};
	// This function must also insert the window into the Context::Impl
	// window-list.
	//
	// Thread-safe. Blocking.
	// 
	// TODO: In the future we should have a non-blocking variant
	// that returns a future.
	[[nodiscard]] Std::Opt<NewWindow_ReturnT> NewWindow_Blocking(
		Context::Impl& implData,
		Context::Impl::PlatformBackendBase& backendData,
		Std::Span<char const> const& title,
		Extent extent);

	// Thread-safe.
	void DestroyWindow(
		Context::Impl& implData,
		Context::Impl::PlatformBackendBase& backendData,
		Context::Impl::WindowNode const& windowNode);
	// Must be thread-safe
	[[nodiscard]] Context::CreateVkSurface_ReturnT CreateVkSurface(
		Context::Impl& implData,
		Context::Impl::PlatformBackendBase& backendData,
		WindowPlatformBackendBase& window,
		void const* vkGetInstanceProcAddrFn,
		uSize vkInstance,
		void const* vkAllocationCallbacks) noexcept;

	void Log(
		Context::Impl& implData,
		Context::Impl::PlatformBackendBase& backendData,
		LogSeverity severity,
		Std::Span<char const> const& msg);

	[[nodiscard]] bool StartTextInputSession(
		Context::Impl& implData,
		WindowID windowId,
		Context::Impl::PlatformBackendBase& backendData,
		SoftInputFilter inputFilter,
		Std::Span<char const> const& text,
		u64 selectionStart,
		u64 selectionCount);
	bool UpdateTextInputConnection(
		Context::Impl& implData,
		Context::Impl::PlatformBackendBase& backendData,
		WindowID windowId,
		u64 selIndex,
		u64 selCount,
		Std::Span<u32 const> text);
	void UpdateTextInputConnectionSelection(
		Context::Impl::PlatformBackendBase& backendData,
		u64 selIndex,
		u64 selCount);
	void StopTextInputSession(
		Context::Impl& implData,
		Context::Impl::PlatformBackendBase& backendData);

	void UpdateAccessibility(
		Context::Impl& implData,
		Context::Impl::PlatformBackendBase& backendData,
		WindowID windowId,
		Std::RangeFnRef<AccessibilityUpdateElement> const& range,
		Std::Span<char const> textData);
}

namespace DEngine::Platform::impl
{
	[[nodiscard]] inline auto Initialize() { return Context::Impl::Initialize(); }
	// If waitForEvents is set to false, the thread will return as soon as possible.
	//
	// timeoutNs determines the maximum amount of time to wait if waiting for events.
	// If timeoutNs is set to 0, we wait indefinitely.
	// timeoutNs is only taken into consideration if waitForEvents is set to true.
	inline void ProcessEvents(
		Context& ctx,
		bool waitForEvents,
		u64 timeoutNs,
		bool ignoreWaitOnFirstCall,
		EventForwarder* eventForwarder)
	{
		return Context::Impl::ProcessEvents(
			ctx,
			waitForEvents,
			timeoutNs,
			ignoreWaitOnFirstCall,
			eventForwarder);
	}
}

// The BackendInterface namespace contains functions that the backend may call
// to update the cross-platform layer.
namespace DEngine::Platform::impl::BackendInterface
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
	[[maybe_unused]] void WindowContentScale(
		Context::Impl& implData,
		WindowID id,
		float scale);
	[[maybe_unused]] void WindowDpi(
		Context::Impl& implData,
		WindowID id,
		float dpiX,
		float dpiY);
	[[maybe_unused]] void UpdateWindowFocus(
		Context::Impl& implData,
		WindowID id,
		bool focusGained);
	[[maybe_unused]] void UpdateWindowSize(
		Context::Impl& implData,
		WindowID id,
		Extent windowExtent,
		Std::Array<WindowInsets, (int)WindowInsetSource::COUNT> windowInsets);

	[[maybe_unused]] void UpdateCursorPosition(
		Context::Impl& implData,
		WindowID id,
		Math::Vec2Int newRelativePosition,
		Math::Vec2Int delta);
	[[maybe_unused]] void UpdateCursorPosition(
		Context::Impl& implData,
		WindowID id,
		Math::Vec2Int newRelativePosition);
	[[maybe_unused]] void UpdateCursorButton(
		Context::Impl& implData,
		WindowID id,
		CursorButton button,
		bool pressed);
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

	// Raw normalized value in [-1, 1]. Callers should pass the raw axis value;
	// deadzone is applied internally based on GamepadState::stickDeadzone.
	// Only enqueues an event if the effective (post-deadzone) value changed.
	[[maybe_unused]] void UpdateGamepadAxis(
		Context::Impl& implData,
		GamepadDeviceId deviceId,
		GamepadAxis axis,
		f32 rawValue);

	[[maybe_unused]] void UpdateGamepadKey(
		Context::Impl& implData,
		GamepadDeviceId deviceId,
		GamepadKey key,
		bool pressed);

	[[maybe_unused]] void PushTextInputEvent(
		Context::Impl& implData,
		WindowID id,
		u64 start,
		u64 count,
		Std::Span<u32 const> const& newText);
	[[maybe_unused]] void PushTextSelectionEvent(
		Context::Impl& implData,
		WindowID id,
		u64 start,
		u64 count);

	[[maybe_unused]] void PushTextDeleteEvent(
		Context::Impl& implData,
		WindowID id);

	[[maybe_unused]] void PushEndTextInputSessionEvent(
		Context::Impl& implData,
		WindowID id);
}