#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace DEngine::Gfx::Vk
{
	void initStaticDispatcher(VkInstance instance, PFN_vkGetInstanceProcAddr getProcAddr);
}