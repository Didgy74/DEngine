#pragma once

#include "DEngine/FixedWidthTypes.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include <array>
#include <string_view>

namespace DEngine::Gfx::Vk::Constants
{
	constexpr u32 maxResourceSets = 3;
	constexpr u32 preferredResourceSetCount = 2;

	constexpr u32 maxRequiredInstanceExtensions = 20;
	constexpr u32 maxAvailableInstanceExtensions = 20;
	constexpr u32 maxAvailableInstanceLayers = 20;
	constexpr u32 maxAvailableLayerExtensions = 10;
	constexpr u32 maxAvailablePhysicalDevices = 20;
	constexpr u32 maxAvailableQueueFamilies = 10;

	constexpr u32 maxAvailablePresentModes = 10;
	constexpr vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eFifo;
	constexpr u32 maxAvailableSurfaceFormats = 25;
	constexpr u32 maxSwapchainLength = 4;
	constexpr u32 preferredSwapchainLength = 2;

	constexpr std::string_view preferredValidationLayer
	{
		"VK_LAYER_KHRONOS_validation"
	};

	constexpr std::array<const char*, 1> requiredInstanceExtensions
	{
		"VK_KHR_surface"
	};

	constexpr bool enableDebugUtils = true;
	constexpr std::string_view debugUtilsExtensionName{ "VK_EXT_debug_utils" };

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