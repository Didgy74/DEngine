#pragma once

#include <DEngine/Application.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/FrameAllocator.hpp>
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

namespace DEngine::Application::impl
{
	enum class EventType : u8
	{
		ButtonEvent,
		CursorMoveEvent,
		TextInputEvent,
		EndTextInputSessionEvent,
		TouchEvent,
		WindowCloseSignalEvent,
		WindowCursorEnterEvent,
		WindowFocusEvent,
		WindowMinimizeEvent,
		WindowMoveEvent,
		WindowResizeEvent,
	};
	template<class T>
	constexpr EventType GetEventType();

	struct ButtonEvent
	{
		WindowID windowId;
		Button btn;
		bool state;
	};
	template<>
	constexpr EventType GetEventType<ButtonEvent>() { return EventType::ButtonEvent; }

	struct EndTextInputSessionEvent
	{
		WindowID windowId;
	};
	template<>
	constexpr EventType GetEventType<EndTextInputSessionEvent>() { return EventType::EndTextInputSessionEvent; }

	struct CursorMoveEvent
	{
		WindowID windowId;
		Math::Vec2Int pos;
		Math::Vec2Int delta;
	};
	template<>
	constexpr EventType GetEventType<CursorMoveEvent>() { return EventType::CursorMoveEvent; }

	struct TextInputEvent
	{
		WindowID windowId;
		uSize oldIndex;
		uSize oldCount;
		uSize newTextOffset;
		uSize newTextSize;
	};
	template<>
	constexpr EventType GetEventType<TextInputEvent>() { return EventType::TextInputEvent; }

	struct TouchEvent
	{
		WindowID windowId;
		TouchEventType type;
		u8 id;
		f32 x;
		f32 y;
	};
	template<>
	constexpr EventType GetEventType<TouchEvent>() { return EventType::TouchEvent; }

	struct WindowCursorEnterEvent
	{
		WindowID window;
		bool entered;
	};
	template<>
	constexpr EventType GetEventType<WindowCursorEnterEvent>() { return EventType::WindowCursorEnterEvent; }

	struct WindowCloseSignalEvent
	{
		WindowID window;
	};
	template<>
	constexpr EventType GetEventType<WindowCloseSignalEvent>() { return EventType::WindowCloseSignalEvent; }

	struct WindowFocusEvent
	{
		WindowID window;
		bool focusGained;
	};
	template<>
	constexpr EventType GetEventType<WindowFocusEvent>() { return EventType::WindowFocusEvent; }

	struct WindowMoveEvent
	{
		WindowID window;
		Math::Vec2Int position;
	};
	template<>
	constexpr EventType GetEventType<WindowMoveEvent>() { return EventType::WindowMoveEvent; }

	struct WindowResizeEvent
	{
		WindowID window;
		Extent extent;
		Math::Vec2UInt safeAreaOffset;
		Extent safeAreaExtent;
	};
	template<>
	constexpr EventType GetEventType<WindowResizeEvent>() { return EventType::WindowResizeEvent; }

	union EventUnion
	{
		ButtonEvent buttonEvent;
		CursorMoveEvent cursorMoveEvent;
		TextInputEvent textInputEvent;
		EndTextInputSessionEvent endTextInputSessionEvent;
		TouchEvent touchEvent;
		WindowCloseSignalEvent windowCloseSignalEvent;
		WindowCursorEnterEvent windowCursorEnterEvent;
		WindowFocusEvent windowFocusEvent;
		WindowMoveEvent windowMoveEvent;
		WindowResizeEvent windowResizeEvent;
	};

	template<class T>
	constexpr auto& EventUnion_Get(EventUnion& in);
	template<EventType type>
	constexpr auto& EventUnion_Get(EventUnion& in);
	template<EventType type>
	constexpr auto const& EventUnion_Get(EventUnion const& in) { return EventUnion_Get<type>(const_cast<EventUnion&>(in)); }

	template<>
	constexpr auto& EventUnion_Get<ButtonEvent>(EventUnion& in) { return in.buttonEvent; }
	template<>
	constexpr auto& EventUnion_Get<EventType::ButtonEvent>(EventUnion& in) { return in.buttonEvent; }
	template<>
	constexpr auto& EventUnion_Get<CursorMoveEvent>(EventUnion& in) { return in.cursorMoveEvent; }
	template<>
	constexpr auto& EventUnion_Get<EventType::CursorMoveEvent>(EventUnion& in) { return in.cursorMoveEvent; }
	template<>
	constexpr auto& EventUnion_Get<TextInputEvent>(EventUnion& in) { return in.textInputEvent; }
	template<>
	constexpr auto& EventUnion_Get<EventType::TextInputEvent>(EventUnion& in) { return in.textInputEvent; }
	template<>
	constexpr auto& EventUnion_Get<EndTextInputSessionEvent>(EventUnion& in) { return in.endTextInputSessionEvent; }
	template<>
	constexpr auto& EventUnion_Get<EventType::EndTextInputSessionEvent>(EventUnion& in) { return in.endTextInputSessionEvent; }
	template<>
	constexpr auto& EventUnion_Get<TouchEvent>(EventUnion& in) { return in.touchEvent; }
	template<>
	constexpr auto& EventUnion_Get<EventType::TouchEvent>(EventUnion& in) { return in.touchEvent; }
	template<>
	constexpr auto& EventUnion_Get<WindowCloseSignalEvent>(EventUnion& in) { return in.windowCloseSignalEvent; }
	template<>
	constexpr auto& EventUnion_Get<EventType::WindowCloseSignalEvent>(EventUnion& in) { return in.windowCloseSignalEvent; }
	template<>
	constexpr auto& EventUnion_Get<WindowCursorEnterEvent>(EventUnion& in) { return in.windowCursorEnterEvent; }
	template<>
	constexpr auto& EventUnion_Get<EventType::WindowCursorEnterEvent>(EventUnion& in) { return in.windowCursorEnterEvent; }
	template<>
	constexpr auto& EventUnion_Get<WindowFocusEvent>(EventUnion& in) { return in.windowFocusEvent; }
	template<>
	constexpr auto& EventUnion_Get<EventType::WindowFocusEvent>(EventUnion& in) { return in.windowFocusEvent; }
	template<>
	constexpr auto& EventUnion_Get<WindowMoveEvent>(EventUnion& in) { return in.windowMoveEvent; }
	template<>
	constexpr auto& EventUnion_Get<EventType::WindowMoveEvent>(EventUnion& in) { return in.windowMoveEvent; }
	template<>
	constexpr auto& EventUnion_Get<WindowResizeEvent>(EventUnion& in) { return in.windowResizeEvent; }
	template<>
	constexpr auto& EventUnion_Get<EventType::WindowResizeEvent>(EventUnion& in) { return in.windowResizeEvent; }

	struct EventCallbackJob
	{
		EventType type = {};
		EventForwarder* ptr = nullptr;
		EventUnion eventUnion = {};
	};
}

struct DEngine::Application::Context::Impl
{
	[[nodiscard]] static Context Initialize();
	static void ProcessEvents(
		Context& ctx,
		bool waitForEvents,
		u64 timeoutNs,
		bool returnImmediateOnFirstCall);

	bool isFirstCall = true;
	void* backendData = nullptr;

	// Window data start
	struct WindowData
	{
		Math::Vec2Int position = {};
		Extent extent = {};
		Math::Vec2UInt visibleOffset = {};
		Extent visibleExtent = {};

		bool isMinimized = false;
		bool shouldShutdown = false;

		CursorType currentCursorType = CursorType::Arrow;
	};

	mutable std::mutex windowsLock;
	u64 windowIdTracker = 0;
	struct WindowNode
	{
		WindowID id;
		WindowData windowData;
		WindowEvents events;
		void* platformHandle;
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

	std::vector<impl::EventCallbackJob> queuedEventCallbacks;

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

	[[maybe_unused]] void UpdateWindowCursorEnter(
		Context::Impl& implData,
		WindowID id,
		bool entered);

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
		Extent newSize);

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