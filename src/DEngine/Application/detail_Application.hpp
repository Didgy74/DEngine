#pragma once
#define DENGINE_APPLICATION_BUTTON_COUNT

#include "DEngine/Application.hpp"
#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/StaticVector.hpp"

#include <chrono>

#if defined(__ANDROID__)
#	define DENGINE_APP_MAIN_ENTRYPOINT dengine_app_detail_main
#else
#	define DENGINE_APP_MAIN_ENTRYPOINT main
#endif

namespace DEngine::Application::detail
{
	bool Initialize();
	bool Backend_Initialize();

	void ProcessEvents();
	void Backend_ProcessEvents();

	bool ShouldShutdown();
	bool IsMinimized();
	bool IsRestored();
	bool MainWindowRestoreEvent();
	bool ResizeEvent();
	bool MainWindowSurfaceInitializeEvent();

	void ImgGui_Initialize();
	void ImGui_NewFrame();

	Std::StaticVector<char const*, 5> GetRequiredVulkanInstanceExtensions();
	bool CreateVkSurface(uSize vkInstance, void const* vkAllocationCallbacks, void* userData, u64& vkSurface);

	// Input
	extern bool buttonValues[(int)Button::COUNT];
	extern std::chrono::high_resolution_clock::time_point buttonHeldStart[(int)Button::COUNT];
	extern f32 buttonHeldDuration[(int)Button::COUNT];
	extern KeyEventType buttonEvents[(int)Button::COUNT];
	void UpdateButton(Button button, bool pressed);

	extern bool hasMouse;
	extern u32 mousePosition[2];
	extern i32 mouseDelta[2];
	extern Std::StaticVector<TouchInput, 10> touchInputs;
	void UpdateMouse(u32 posX, u32 posY, i32 deltaX, i32 deltaY);
	void UpdateMouse(u32 posX, u32 posY);
	void UpdateTouchInput_Down(u8 id, f32 x, f32 y);
	void UpdateTouchInput_Move(u8 id, f32 x, f32 y);
	void UpdateTouchInput_Up(u8 id, f32 x, f32 y);

	extern u32 displaySize[2];
	extern i32 mainWindowPos[2];
	extern u32 mainWindowSize[2];
	extern u32 mainWindowFramebufferSize[2];
	extern bool mainWindowIsInFocus;
	extern bool mainWindowIsMinimized;
	extern bool mainWindowRestoreEvent;
	extern bool mainWindowResizeEvent;
	extern bool shouldShutdown;

	// Time related stuff
	extern u64 tickCount;
	extern f32 deltaTime;
	extern std::chrono::high_resolution_clock::time_point currentNow;
	extern std::chrono::high_resolution_clock::time_point previousNow;

	extern bool mainWindowSurfaceInitialized;
	extern bool mainWindowSurfaceInitializeEvent;
	extern bool mainWindowSurfaceTerminateEvent;
}