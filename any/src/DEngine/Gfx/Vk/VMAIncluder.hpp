#pragma once

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#include "vk_mem_alloc.h"

#include <vulkan/vulkan.hpp>

#include "DEngine/Gfx/impl/Assert.hpp"

namespace DEngine::Gfx::Vk {
	// For a given VkFormat, there is an alignment requirement when transferring
	[[nodiscard]] inline unsigned int bufferOffsetAlignmentForFormat(vk::Format format)
	{
		switch (format) {
			case vk::Format::eR8G8B8A8Snorm:
			case vk::Format::eR8G8B8A8Unorm:
			case vk::Format::eB8G8R8A8Unorm:
				return 4;
			default:
				DENGINE_IMPL_GFX_UNREACHABLE();
				return 1;
		}

		return 1;
	}
}