#pragma once

#include <DEngine/Application.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/FrameAllocator.hpp>
#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Containers/Vec.hpp>

#include <chrono>
#include <vector>

#if defined(__ANDROID__)
#	define DENGINE_APP_MAIN_ENTRYPOINT dengine_app_detail_main
#else
#	define DENGINE_APP_MAIN_ENTRYPOINT main
#endif

namespace DEngine::Application::detail
{
	struct EventCallbackJob
	{
		enum class Type : u8
		{
			Button,
			CharEnter,
			Char,
			CharRemove,
			CursorMove,
			Touch,
			WindowClose,
			WindowCursorEnter,
			WindowFocus,
			WindowMinimize,
			WindowMove,
			WindowResize,
		};
		Type type = {};
		EventForwarder* ptr = nullptr;

		struct ButtonEvent
		{
			WindowID windowId;
			Button btn;
			bool state;
		};
		struct CharEnterEvent {};
		struct CharEvent
		{
			u32 utfValue;
		};
		struct CharRemoveEvent {};
		struct CursorMoveEvent
		{
			WindowID windowId;
			Math::Vec2Int pos;
			Math::Vec2Int delta;
		};
		struct TouchEvent
		{
			TouchEventType type;
			u8 id;
			f32 x; 
			f32 y;
		};
		struct WindowCursorEnter
		{
			WindowID window;
			bool entered;
		};
		struct WindowFocusEvent
		{
			WindowID window;
			bool focusGained;
		};
		struct WindowMoveEvent
		{
			WindowID window;
			Math::Vec2Int position;
		};
		union
		{
			ButtonEvent button;
			CharEnterEvent charEnter;
			CharEvent charEvent;
			CharRemoveEvent charRemove;
			CursorMoveEvent cursorMove;
			TouchEvent touch;
			WindowCursorEnter windowCursorEnter;
			WindowMoveEvent windowMove;
			WindowFocusEvent windowFocus;
		};
	};

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

	struct AppData
	{
		void* backendData = nullptr;

		u64 windowIdTracker = 0;
		struct WindowNode
		{
			WindowID id;
			WindowData windowData;
			WindowEvents events;
			void* platformHandle;
		};
		std::vector<WindowNode> windows;

		Orientation currentOrientation = Orientation::Invalid;
		bool orientationEvent = false;

		// Input
		bool buttonValues[(int)Button::COUNT] = {};
		std::chrono::high_resolution_clock::time_point buttonHeldStart[(int)Button::COUNT] = {};
		f32 buttonHeldDuration[(int)Button::COUNT] = {};
		KeyEventType buttonEvents[(int)Button::COUNT] = {};

		Std::Opt<CursorData> cursorOpt{};

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
		std::vector<EventForwarder*> registeredEventCallbacks;


		std::vector<EventCallbackJob> queuedEventCallbacks;
	};

	extern AppData* pAppData;

	enum class PollMode
	{
		Immediate,
		Wait,
		COUNT,
	};
	void ProcessEvents(PollMode pollMode, Std::Opt<u64> timeoutNs = Std::nullOpt);

	bool Backend_Initialize() noexcept;
	void Backend_ProcessEvents(PollMode pollMode, Std::Opt<u64> timeoutNs);
	void Backend_Log(char const* msg);
	void Backend_DestroyWindow(AppData::WindowNode& windowNode);

	AppData::WindowNode* GetWindowNode(AppData& appData, WindowID id);
	AppData::WindowNode const* GetWindowNode(AppData const& appData, WindowID id);
	AppData::WindowNode* GetWindowNode(AppData& appData, void* platformHandle);
	AppData::WindowNode const* GetWindowNode(AppData const& appData, void* platformHandle);
	void UpdateWindowSize(
		AppData::WindowNode& windowNode,
		Extent newSize,
		Math::Vec2Int visiblePos,
		Extent visibleSize);
	void UpdateWindowPosition(
		AppData& appData,
		void* platformHandle,
		Math::Vec2Int newPosition);
	void UpdateWindowFocus(
		void* platformHandle,
		bool focused);
	void UpdateWindowMinimized(
		AppData::WindowNode& windowNode,
		bool minimized);
	void UpdateWindowClose(
		AppData::WindowNode& windowNode);
	void UpdateWindowCursorEnter(
		void* platformHandle,
		bool entered);
	void UpdateOrientation(Orientation newOrient);

	void UpdateCursor(
		AppData& appData,
		WindowID id,
		Math::Vec2Int pos,
		Math::Vec2Int delta);
	void UpdateCursor(
		AppData& appData,
		WindowID id,
		Math::Vec2Int pos);
	void UpdateCursor(
		AppData& appData,
		void* platformHandle,
		Math::Vec2Int pos,
		Math::Vec2Int delta);
	void UpdateCursor(
		AppData& appData,
		void* platformHandle,
		Math::Vec2Int pos);

	void UpdateTouchInput(
		AppData& appData,
		TouchEventType type, 
		u8 id, 
		f32 x, 
		f32 y);

	[[maybe_unused]] void UpdateButton(
		AppData& appData,
		Button button, 
		bool pressed);

	void UpdateGamepadButton(
		AppData& appData,
		GamepadKey button,
		bool pressed);
	void UpdateGamepadAxis(
		AppData& appData,
		GamepadAxis axis,
		f32 newValue);

	void PushCharInput(u32 charValue);
	void PushCharEnterEvent();
	void PushCharRemoveEvent();

	[[nodiscard]] constexpr bool IsValid(CursorType);
}

namespace DEngine::Application::impl
{
	using EventCallbackJob = detail::EventCallbackJob;
	using AppData = detail::AppData;
	using WindowData = detail::WindowData;
	using PollMode = detail::PollMode;
}

struct [[maybe_unused]] DEngine::Application::impl::AppImpl
{
	[[nodiscard]] static Context Initialize();
	static void ProcessEvents(Context& ctx, PollMode pollMode, Std::Opt<u64> timeoutNs);
};

namespace DEngine::Application::impl
{
	[[nodiscard]] inline auto Initialize() { return AppImpl::Initialize(); }
	inline void ProcessEvents(Context& ctx, PollMode pollMode, Std::Opt<u64> timeoutNs = Std::nullOpt)
	{
		return AppImpl::ProcessEvents(ctx, pollMode, timeoutNs);
	}

	[[nodiscard]] void* Backend_Initialize(AppData& appData);
	void Backend_ProcessEvents(AppData& appData, PollMode pollMode, Std::Opt<u64> timeoutNs);
	void Backend_Destroy(void* data);
	struct Backend_NewWindow_ReturnT
	{
		WindowData windowData;
		void* platormHandle;
	};
	[[nodiscard]] Backend_NewWindow_ReturnT Backend_NewWindow(
		AppData& appData,
		Std::Span<char const> title,
		Extent extent);
	[[nodiscard]] Context::CreateVkSurface_ReturnT Backend_CreateVkSurface(
		void* platformHandle,
		uSize vkInstance,
		void const* vkAllocationCallbacks) noexcept;

	void Backend_Log(LogSeverity severity, Std::Span<char const> msg);

	[[nodiscard]] AppData::WindowNode* GetWindowNode(AppData& appData, WindowID id);
	[[nodiscard]] AppData::WindowNode const* GetWindowNode(AppData const& appData, WindowID id);

	[[nodiscard]] WindowID GetWindowId(AppData& appData, void* platformHandle);

	// Should be called by the backend.
	[[maybe_unused]] void UpdateWindowCursorEnter(
		AppData& appData,
		WindowID id,
		bool entered);

	// Should be called by the backend.
	[[maybe_unused]] void UpdateWindowFocus(
		AppData& appData,
		WindowID id,
		bool focusGained);

	// Should be called by the backend.
	[[maybe_unused]] void UpdateWindowPosition(
		AppData& appData,
		WindowID id,
		Math::Vec2Int newPosition);

	// Should be called by the backend.
	[[maybe_unused]] void UpdateCursorPosition(
		AppData& appData,
		WindowID id,
		Math::Vec2Int newRelativePosition,
		Math::Vec2Int delta);

	// Should be called by the backend.
	[[maybe_unused]] void UpdateCursorPosition(
		AppData& appData,
		WindowID id,
		Math::Vec2Int newRelativePosition);

	[[maybe_unused]] void UpdateButton(
		AppData& appData,
		WindowID id,
		Button button,
		bool pressed);
}

constexpr bool DEngine::Application::detail::IsValid(CursorType in)
{
	return 0 <= (u8)in && (u8)in < (u8)CursorType::COUNT;
}
