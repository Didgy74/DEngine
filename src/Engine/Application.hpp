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
		uint16_t GetRefreshRate();

		bool IsMinimized();

		std::string GetApplicationName();

		std::string GetAbsolutePath();

		bool IsRunning();

		namespace Core
		{
			void* GetMainWindowHandle();
			void GL_SwapWindow(void* windowHandle);

			void UpdateEvents();

			void Initialize(API3D api);
			void Terminate();
		}
	};
}


