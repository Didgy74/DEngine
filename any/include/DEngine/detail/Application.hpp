#pragma once

#define DENGINE_APPLICATION_CURSORTYPE_COUNT
#define DENGINE_APPLICATION_BUTTON_COUNT
#include <DEngine/Application.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>

#include <chrono>
#include <vector>

#if defined(__ANDROID__)
#	define DENGINE_APP_MAIN_ENTRYPOINT dengine_app_detail_main
#else
#	define DENGINE_APP_MAIN_ENTRYPOINT main
#endif

namespace DEngine::Application::detail
{
	using LogCallback = void(*)(char const*);

	struct WindowData
	{
		Math::Vec2Int position{};
		Extent size{};
		Math::Vec2Int visiblePosition{};
		Extent visibleSize{};

		bool isMinimized = false;
		bool shouldShutdown = false;

		CursorType currentCursorType = CursorType::Arrow;
	};

	struct AppData
	{
		LogCallback logCallback = nullptr;

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
		std::vector<u32> charInputs{};

		Std::Opt<CursorData> cursorOpt{};

		Std::StackVec<TouchInput, maxTouchEventCount> touchInputs{};
		Std::StackVec<std::chrono::high_resolution_clock::time_point, decltype(touchInputs)::Capacity()> touchInputStartTime{};


		// Time related stuff
		u64 tickCount = 0;
		f32 deltaTime = 1.f / 60.f;
		std::chrono::high_resolution_clock::time_point currentNow{};
		std::chrono::high_resolution_clock::time_point previousNow{};

		GamepadState gamepadState{};
		bool gamepadConnected = false;
		int gamepadID = 0;

		std::vector<EventInterface*> eventCallbacks;
	};

	extern AppData* pAppData;

	bool Initialize() noexcept;
	void ProcessEvents();

	bool Backend_Initialize() noexcept;
	void Backend_ProcessEvents();
	void Backend_Log(char const* msg);
	void Backend_DestroyWindow(AppData::WindowNode& windowNode);

	void SetLogCallback(LogCallback callback);

	AppData::WindowNode* GetWindowNode(WindowID id);
	AppData::WindowNode* GetWindowNode(void* platformHandle);
	void UpdateWindowSize(
		void* platformHandle,
		Extent newSize,
		Math::Vec2Int visiblePos,
		Extent visibleSize);
	void UpdateWindowPosition(
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
		AppData::WindowNode const& windowNode,
		Math::Vec2Int pos,
		Math::Vec2Int delta);
	void UpdateCursor(
		AppData::WindowNode const& windowNode,
		Math::Vec2Int pos);

	void UpdateTouchInput(TouchEventType type, u8 id, f32 x, f32 y);

	void UpdateButton(Button button, bool pressed);
	void PushCharInput(u32 charValue);
	void PushCharEnterEvent();
	void PushCharRemoveEvent();

	

	[[nodiscard]] constexpr bool IsValid(CursorType);
}

constexpr bool DEngine::Application::detail::IsValid(CursorType in)
{
	return 0 <= (u8)in && (u8)in < (u8)CursorType::COUNT;
}