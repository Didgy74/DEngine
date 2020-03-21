#pragma once

#include "DEngine/Application.hpp"
#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/FixedVector.hpp"

namespace DEngine::Application::detail
{
	[[nodiscard]] bool Initialize();

	void ProcessEvents();

	[[nodiscard]] bool ShouldShutdown();
	[[nodiscard]] bool IsMinimized();
	[[nodiscard]] bool IsRestored();
	[[nodiscard]] bool ResizeEvent();

	void ImgGui_Initialize();
	void ImGui_NewFrame();

	[[nodiscard]] Cont::FixedVector<char const*, 5> GetRequiredVulkanInstanceExtensions();
	[[nodiscard]] bool CreateVkSurface(u64 vkInstance, void const* vkAllocationCallbacks, void* userData, u64* vkSurface);
}