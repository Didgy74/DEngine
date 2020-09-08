#pragma once

#define DENGINE_APPLICATION_CURSORTYPE_COUNT
#define DENGINE_APPLICATION_BUTTON_COUNT
#include "DEngine/Application.hpp"

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/Opt.hpp"
#include "DEngine/Containers/StackVec.hpp"

#include <chrono>
#include <vector>

#if defined(__ANDROID__)
#	define DENGINE_APP_MAIN_ENTRYPOINT dengine_app_detail_main
#else
#	define DENGINE_APP_MAIN_ENTRYPOINT main
#endif

namespace DEngine::Application::detail
{
	bool Initialize();
	void ProcessEvents();

	bool Backend_Initialize();
	void Backend_ProcessEvents();
	void Backend_Log(char const* msg);

	using LogCallback = void(*)(char const*);
	void SetLogCallback(LogCallback callback);

	void UpdateWindowSize(
		void* platformHandle,
		Extent newSize);
	void UpdateWindowPosition(
		void* platformHandle,
		Math::Vec2Int newPosition);
	void UpdateWindowFocus(
		void* platformHandle,
		bool focused);
	void UpdateWindowMinimized(
		void* platformHandle,
		bool minimized);
	void UpdateWindowCursorEnter(
		void* platformHandle,
		bool entered);
	void UpdateOrientation(Orientation newOrient);

	void UpdateCursor(
		void* platformHandle,
		Math::Vec2Int pos,
		Math::Vec2Int delta);
	void UpdateCursor(
		void* platformHandle,
		Math::Vec2Int pos);

	void UpdateTouchInput(TouchEventType type, u8 id, f32 x, f32 y);

	void UpdateButton(Button button, bool pressed);
	void PushCharInput(u32 charValue);
	void PushCharRemoveEvent();

	struct WindowData
	{
		Extent size;
		Math::Vec2Int position;
		bool isMinimized = false;
		bool shouldShutdown = false;
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

	[[nodiscard]] constexpr bool IsValid(CursorType);
}

constexpr bool DEngine::Application::detail::IsValid(CursorType in)
{
	return 0 <= (u8)in && (u8)in < (u8)CursorType::COUNT;
}