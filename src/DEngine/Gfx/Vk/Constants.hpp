#pragma once

#include "DEngine/FixedWidthTypes.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include <array>
#include <string_view>

namespace DEngine::Gfx::Vk::Constants
{
	constexpr u32 maxResourceSets = 4;
	constexpr u32 preferredResourceSetCount = 3;

	constexpr vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eFifo;
	constexpr u32 maxSwapchainLength = 4;
	constexpr u32 preferredSwapchainLength = 3;

	constexpr char const* preferredValidationLayer
	{
		"VK_LAYER_KHRONOS_validation"
	};

	constexpr std::array<const char*, 1> requiredInstanceExtensions
	{
		"VK_KHR_surface"
	};

	constexpr bool enableDebugUtils = true;
	constexpr char const* debugUtilsExtensionName{ "VK_EXT_debug_utils" };

	constexpr std::array<const char*, 1> requiredDeviceExtensions
	{
		"VK_KHR_swapchain"
	};

	constexpr std::array<VkSurfaceFormatKHR, 2> preferredSurfaceFormats =
	{
		(VkFormat)vk::Format::eB8G8R8A8Unorm, (VkColorSpaceKHR)vk::ColorSpaceKHR::eSrgbNonlinear,
		(VkFormat)vk::Format::eR8G8B8A8Unorm, (VkColorSpaceKHR)vk::ColorSpaceKHR::eSrgbNonlinear,
	};

	constexpr u32 invalidIndex = static_cast<u32>(-1);
}