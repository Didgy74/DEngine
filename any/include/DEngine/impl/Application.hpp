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
	struct EventCallbackJob
	{
		enum class Type : u8
		{
			ButtonEvent,
			CharEnterEvent,
			CharEvent,
			CharRemoveEvent,
			CursorMoveEvent,
			TextInputEvent,
			TouchEvent,
			WindowCloseSignalEvent,
			WindowCursorEnterEvent,
			WindowFocusEvent,
			WindowMinimizeEvent,
			WindowMoveEvent,
			WindowResizeEvent,
		};
		template<class T>
		static constexpr Type GetEventType();

		Type type = {};
		EventForwarder* ptr = nullptr;

		struct ButtonEvent
		{
			WindowID windowId;
			Button btn;
			bool state;
		};
		template<>
		Type GetEventType<ButtonEvent>() { return Type::ButtonEvent; }

		struct CharEnterEvent {};
		template<>
		Type GetEventType<CharEnterEvent>() { return Type::CharEnterEvent; }

		struct CharEvent
		{
			u32 utfValue;
		};
		template<>
		Type GetEventType<CharEvent>() { return Type::CharEvent; }

		struct CharRemoveEvent
		{
			WindowID windowId;
		};
		template<>
		Type GetEventType<CharRemoveEvent>() { return Type::CharRemoveEvent; }

		struct CursorMoveEvent
		{
			WindowID windowId;
			Math::Vec2Int pos;
			Math::Vec2Int delta;
		};
		template<>
		Type GetEventType<CursorMoveEvent>() { return Type::CursorMoveEvent; }

		struct TextInputEvent
		{
			WindowID windowId;
			uSize oldIndex;
			uSize oldCount;
			uSize newTextOffset;
			uSize newTextSize;
		};
		template<>
		Type GetEventType<TextInputEvent>() { return Type::TextInputEvent; }

		struct TouchEvent
		{
			TouchEventType type;
			u8 id;
			f32 x; 
			f32 y;
		};
		template<>
		Type GetEventType<TouchEvent>() { return Type::TouchEvent; }

		struct WindowCursorEnterEvent
		{
			WindowID window;
			bool entered;
		};
		template<>
		Type GetEventType<WindowCursorEnterEvent>() { return Type::WindowCursorEnterEvent; }

		struct WindowCloseSignalEvent
		{
			WindowID window;
		};
		template<>
		Type GetEventType<WindowCloseSignalEvent>() { return Type::WindowCloseSignalEvent; }

		struct WindowFocusEvent
		{
			WindowID window;
			bool focusGained;
		};
		template<>
		Type GetEventType<WindowFocusEvent>() { return Type::WindowFocusEvent; }

		struct WindowMoveEvent
		{
			WindowID window;
			Math::Vec2Int position;
		};
		template<>
		Type GetEventType<WindowMoveEvent>() { return Type::WindowMoveEvent; }

		struct WindowResizeEvent
		{
			WindowID window;
			Extent extent;
			Math::Vec2UInt safeAreaOffset;
			Extent safeAreaExtent;
		};
		template<>
		Type GetEventType<WindowResizeEvent>() { return Type::WindowResizeEvent; }

		union
		{
			ButtonEvent buttonEvent;
			CharEnterEvent charEnterEvent;
			CharEvent charEvent;
			CharRemoveEvent charRemoveEvent;
			CursorMoveEvent cursorMoveEvent;
			TextInputEvent textInputEvent;
			TouchEvent touchEvent;
			WindowCloseSignalEvent windowCloseSignalEvent;
			WindowCursorEnterEvent windowCursorEnterEvent;
			WindowFocusEvent windowFocusEvent;
			WindowMoveEvent windowMoveEvent;
			WindowResizeEvent windowResizeEvent;
		};
		template<class T>
		constexpr auto& Get();
		template<>
		constexpr auto& Get<ButtonEvent>() { return buttonEvent; }
		template<>
		constexpr auto& Get<CharEnterEvent>() { return charEnterEvent; }
		template<>
		constexpr auto& Get<CharEvent>() { return charEvent; }
		template<>
		constexpr auto& Get<CharRemoveEvent>() { return charRemoveEvent; }
		template<>
		constexpr auto& Get<CursorMoveEvent>() { return cursorMoveEvent; }
		template<>
		constexpr auto& Get<TextInputEvent>() { return textInputEvent; }
		template<>
		constexpr auto& Get<WindowCloseSignalEvent>() { return windowCloseSignalEvent; }
		template<>
		constexpr auto& Get<WindowCursorEnterEvent>() { return windowCursorEnterEvent; }
		template<>
		constexpr auto& Get<WindowFocusEvent>() { return windowFocusEvent; }
		template<>
		constexpr auto& Get<WindowMoveEvent>() { return windowMoveEvent; }
		template<>
		constexpr auto& Get<WindowResizeEvent>() { return windowResizeEvent; }
	};

	enum class PollMode
	{
		Immediate,
		Wait,
		COUNT,
	};
}

struct DEngine::Application::Context::Impl
{
	[[nodiscard]] static Context Initialize();
	static void ProcessEvents(Context& ctx, impl::PollMode pollMode, Std::Opt<u64> const& timeoutNs);


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
	[[nodiscard]] void* Initialize(Context& ctx);
	void ProcessEvents(Context& ctx, impl::PollMode pollMode, Std::Opt<u64> const& timeoutNs);
	void Destroy(void* data);
	struct NewWindow_ReturnT
	{
		Context::Impl::WindowData windowData = {};
		void* platormHandle {};
	};
	[[nodiscard]] NewWindow_ReturnT NewWindow(
		Context& ctx,
		Std::Span<char const> title,
		Extent extent);
	void DestroyWindow(
		Context::Impl& implData,
		Context::Impl::WindowNode const& windowNode);
	[[nodiscard]] Context::CreateVkSurface_ReturnT CreateVkSurface(
		void* platformHandle,
		uSize vkInstance,
		void const* vkAllocationCallbacks) noexcept;

	void Log(
		Context::Impl& implData,
		LogSeverity severity,
		Std::Span<char const> const& msg);

	bool StartTextInputSession(
		Context& ctx,
		SoftInputFilter inputFilter,
		Std::Span<char const> const& text);
	void StopTextInputSession(Context& ctx);
}

namespace DEngine::Application::impl
{
	[[nodiscard]] inline auto Initialize() { return Context::Impl::Initialize(); }
	inline void ProcessEvents(Context& ctx, PollMode pollMode, Std::Opt<u64> const& timeoutNs = Std::nullOpt)
	{
		return Context::Impl::ProcessEvents(ctx, pollMode, timeoutNs);
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
}