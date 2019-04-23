#pragma once

#include <functional>

namespace DRenderer
{
	namespace Vulkan
	{
		struct InitInfo
		{
			std::function<void(*())(void)> test = nullptr;
		};
	}
}