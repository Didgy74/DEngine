#pragma once

#include <cstdint>

namespace DRenderer::Vulkan::Constants
{
	constexpr uint32_t maxResourceSets = 3;
	constexpr uint32_t maxSwapchainImages = 4;

	constexpr std::array requiredValidLayers
	{
		"VK_LAYER_KHRONOS_validation",
		//"VK_LAYER_RENDERDOC_Capture"
	};

	constexpr std::array requiredDeviceExtensions
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	constexpr std::array requiredDeviceLayers
	{
		"VK_LAYER_LUNARG_standard_validation"
	};

	constexpr uint32_t invalidIndex = std::numeric_limits<uint32_t>::max();
}