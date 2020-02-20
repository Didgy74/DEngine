#pragma once

#include "DEngine/Application/Application.hpp"
#include "DEngine/Int.hpp"
#include "Dengine/Containers/FixedVector.hpp"

namespace DEngine::Application::detail
{
	bool Initialize();

	void ProcessEvents();

	bool ShouldShutdown();
	bool IsMinimized();

	void ImgGui_Initialize();
	void ImGui_NewFrame();

	Cont::FixedVector<char const*, 5> GetRequiredVulkanInstanceExtensions();
	bool CreateVkSurface(u64 vkInstance, void const* vkAllocationCallbacks, void* userData, u64* vkSurface);
}