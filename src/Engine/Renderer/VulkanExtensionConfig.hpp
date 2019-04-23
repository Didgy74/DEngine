#pragma once

#include "DRenderer/DebugConfig.hpp"

#include <cstdint>
#include <cstddef>
#include <array>
#include <type_traits>

namespace DRenderer::Vulkan
{
	constexpr std::array requiredInstanceExtensions
	{
		"VK_KHR_surface"
	};

	constexpr std::array requiredDebugExtensions
	{
		"VK_EXT_debug_utils"
	};

	namespace Utility
	{
		constexpr auto RequiredExtensionCombiner()
		{
			if constexpr (debugConfig == true)
			{
				constexpr size_t count = requiredInstanceExtensions.size() + requiredDebugExtensions.size();
				std::array<const char*, count> returnArr{};
				for (size_t i = 0; i < returnArr.size(); i++)
				{
					if (i < requiredInstanceExtensions.size())
						returnArr[i] = requiredInstanceExtensions[i];
					else
						returnArr[i] = requiredDebugExtensions[i - requiredInstanceExtensions.size()];
				}
				return returnArr;
			}
			else
				return requiredInstanceExtensions;
		}
	}
	
	constexpr auto requiredExtensions = Utility::RequiredExtensionCombiner();
}