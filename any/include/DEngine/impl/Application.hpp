#pragma once

#include <DEngine/Application.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/FnRef.hpp>
#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Containers/Vec.hpp>
#include <DEngine/Std/Containers/FnScratchList.hpp>

#include <vector>
#include <mutex>
#include <future>


#if !defined(DENGINE_OS)
#error "Error: DEngine-define 'DENGINE_OS' has not been defined. Likely an error in build-script."
#elif DENGINE_OS == DENGINE_OS_VALUE_ANDROID
#define DENGINE_MAIN_ENTRYPOINT dengine_impl_main
#elif DENGINE_OS == DENGINE_OS_VALUE_WINDOWS
#define DENGINE_MAIN_ENTRYPOINT dengine_impl_main
#else
#define DENGINE_MAIN_ENTRYPOINT main
#endif

namespace DEngine::Application::impl {
	struct WindowBackendData {
		[[nodiscard]] virtual void* GetRawHandle() = 0;
		[[nodiscard]] virtual void const* GetRawHandle() const = 0;
		virtual ~WindowBackendData() = default;;
	};
}

struct DEngine::Application::Context::Impl
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
	void* backendData = nullptr;


	// Window data start
	struct WindowData {
		Math::Vec2Int position = {};
		Extent extent = {};
		Math::Vec2UInt visibleOffset = {};
		Extent visibleExtent = {};
		Orientation orientation = {};

		f32 dpiX = 0.f;
		f32 dpiY = 0.f;
		f32 contentScale = 1.f;

		bool isMinimized = false;
		bool shouldShutdown = false;
		Orientation currentOrientation = Orientation::Invalid;

		CursorType currentCursorType = CursorType::Arrow;

		u64 currTickEventFlags = 0;
		void SetEventFlag(WindowEventFlag in) { this->currTickEventFlags |= (u64)in; }
	};

	mutable std::mutex windowsLock;
	u64 windowIdTracker = 0;
	struct WindowNode {
		WindowID id = {};
		WindowData windowData = {};
		WindowEvents events = {};
		Std::Box<impl::WindowBackendData> backendData;
	};
	std::vector<WindowNode> windows;
	[[nodiscard]] impl::WindowBackendData& GetWindowBackend(void* platformHandle);
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
	struct RunOnBackendThread_JobItem {
		void* data = nullptr;
		// This should run the job and do the cleanup.
		using ConsumeFnT = void(*)(void*, Context::Impl&);
		// This should run the job and do the cleanup.
		ConsumeFnT consumeFn = nullptr;
	};
	void RunOnBackendThread(void* pBackendData, RunOnBackendThread_JobItem job);
	// Returns a future with the return value of the lambda
	template<class Callable>
	auto RunOnBackendThread2(void* pBackendData, Callable&& func) {
		using RType = std::invoke_result_t<Callable, Context::Impl&>;
		struct JobData {
			Callable func;
			std::promise<RType> promise;
			explicit JobData(Callable&& func) : func{ Std::Move(func) } {}
		};
		auto* jobData = new JobData(Std::Move(func));
		auto future = jobData->promise.get_future();

		RunOnBackendThread_JobItem jobItem = {};
		jobItem.data = jobData;
		jobItem.consumeFn = [](void* dataIn, Context::Impl& implData) {
			auto* jobData = (JobData*)dataIn;
			if constexpr (Std::Trait::isSame<RType, void>) {
				jobData->func(implData);
				jobData->promise.set_value();
			} else {
				jobData->promise.set_value(jobData->func(implData));
			}
			delete jobData;
		};
		RunOnBackendThread(pBackendData, jobItem);


		return future;
	}

	template<class Callable>
	void RunOnBackendThread_Wait(void* pBackendData, Callable&& func) {
		auto future = RunOnBackendThread2(pBackendData, Std::Move(func));
		future.wait();
	}

	struct NewWindow_ReturnT {
        WindowID windowId = {};
		Context::Impl::WindowData windowData = {};
	};
	[[nodiscard]] Std::Opt<NewWindow_ReturnT> NewWindow_NotAsync(
		Context::Impl& implData,
		void* pBackendData,
		Std::Span<char const> const& title,
		Extent extent);
	void DestroyWindow(
		Context::Impl& implData,
		void* backendData,
		Context::Impl::WindowNode const& windowNode);
	[[nodiscard]] Context::CreateVkSurface_ReturnT CreateVkSurface(
		Context::Impl& implData,
		void* backendData,
		WindowBackendData& platformHandle,
		uSize vkInstance,
		void const* vkAllocationCallbacks) noexcept;

	void Log(
		Context::Impl& implData,
		LogSeverity severity,
		Std::Span<char const> const& msg);

	bool StartTextInputSession(
		Context::Impl& implData,
		WindowID windowId,
		void* backendData,
		SoftInputFilter inputFilter,
		Std::Span<char const> const& text);
	void UpdateTextInputConnection(
		void* backendData,
		u64 selIndex,
		u64 selCount,
		Std::Span<u32 const> text);
	void UpdateTextInputConnectionSelection(
	   void* backendData,
	   u64 selIndex,
	   u64 selCount);
	void StopTextInputSession(
		Context::Impl& implData,
		void* backendData);

	void UpdateAccessibility(
		Context::Impl& implData,
		void* backendData,
		WindowID windowId,
		Std::RangeFnRef<AccessibilityUpdateElement> const& range,
		Std::ConstByteSpan textData);
}

namespace DEngine::Application::impl
{
	[[nodiscard]] inline auto Initialize() { return Context::Impl::Initialize(); }
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