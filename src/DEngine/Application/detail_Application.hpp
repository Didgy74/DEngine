#pragma once

#include "DEngine/Application.hpp"
#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/StaticVector.hpp"

namespace DEngine::Application::detail
{
	bool Initialize();

	void ProcessEvents();

	bool ShouldShutdown();
	bool IsMinimized();
	bool IsRestored();
	bool ResizeEvent();

	void ImgGui_Initialize();
	void ImGui_NewFrame();

	Std::StaticVector<char const*, 5> GetRequiredVulkanInstanceExtensions();
	bool CreateVkSurface(u64 vkInstance, void const* vkAllocationCallbacks, void* userData, u64* vkSurface);
}