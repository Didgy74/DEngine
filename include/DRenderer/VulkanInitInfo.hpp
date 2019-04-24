#pragma once

#include <functional>
#include <vector>
#include <string_view>

namespace DRenderer
{
	namespace Vulkan
	{
		struct InitInfo
		{
			std::function<void(*())()> getInstanceProcAddr = nullptr;
			std::function<std::vector<std::string_view>()> getRequiredInstanceExtensions = nullptr;
			std::function<bool(void* instance, void* windowHandle, void* surfaceHandle)> test = nullptr;
		};
	}
}