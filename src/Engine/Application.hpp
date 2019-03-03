#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Utility/ImgDim.hpp"

namespace Engine
{
	namespace Application
	{
		constexpr Utility::ImgDim defaultWindowSize = { 1280, 720 };
		constexpr Utility::ImgDim minimumWindowSize = { 800, 600 };
		constexpr float narrowestAspectRatio = minimumWindowSize.GetAspectRatio();
		constexpr std::string_view defaultApplicationName = "VulkanProject1";

		enum class API3D
		{
			Vulkan,
			OpenGL
		};

		Utility::ImgDim GetWindowSize();
		Utility::ImgDim GetViewportSize();
		uint16_t GetRefreshRate();

		bool IsMinimized();

		std::string GetApplicationName();

		std::string GetAbsolutePath();

		bool IsRunning();

		namespace Core
		{
			void* GetSDLWindowHandle();
			void GL_SwapWindow(void* sdlWindow);
			std::vector<const char*> Vulkan_GetInstanceExtensions(void* windowHandle);
			void Vulkan_CreateSurface(void* windowHandle, void* vkInstance, void* surfaceHandle);

			void UpdateEvents();

			void Initialize(API3D api);
			void Terminate();
		}
	};
}


