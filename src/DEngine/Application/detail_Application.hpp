#pragma once

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
	void Backend_ProcessEvents(
		std::chrono::high_resolution_clock::time_point now);

	bool ShouldShutdown();
	bool IsMinimized();
	bool IsRestored();
	bool ResizeEvent();

	void ImgGui_Initialize();
	void ImGui_NewFrame();

	Std::StaticVector<char const*, 5> GetRequiredVulkanInstanceExtensions();
	bool CreateVkSurface(u64 vkInstance, void const* vkAllocationCallbacks, void* userData, u64* vkSurface);

	void UpdateButton(Button button, bool pressed, std::chrono::high_resolution_clock::time_point now);
	void UpdateMouse(u32 posX, u32 posY, i32 deltaX, i32 deltaY);
	void UpdateMouse(u32 posX, u32 posY);


	extern u32 displaySize[2];
	extern i32 mainWindowPos[2];
	extern u32 mainWindowSize[2];
	extern u32 mainWindowFramebufferSize[2];
	extern bool mainWindowIsInFocus;
	extern bool mainWindowIsMinimized;
	extern bool mainWindowRestoreEvent;
	extern bool mainWindowResizeEvent;
	extern bool shouldShutdown;
}