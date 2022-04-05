#pragma once

#include "VulkanIncluder.hpp"

#include <DEngine/FixedWidthTypes.hpp>

#include <array>

namespace DEngine::Gfx::Vk::Constants
{
	constexpr u32 maxInFlightCount = 4;
	constexpr u32 preferredInFlightCount = 3;

	constexpr vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eFifo;
	constexpr u32 maxSwapchainLength = 4;
	constexpr u32 preferredSwapchainLength = 3;

	constexpr char const* khronosLayerName { "VK_LAYER_KHRONOS_validation" };
	constexpr char const* armMaliPerfLayerName { "VK_LAYER_ARM_mali_perf_doc" };

	constexpr std::array preferredLayerNames
	{
		khronosLayerName,
		//armMaliPerfLayerName
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

	constexpr std::array<VkSurfaceFormatKHR, 2> preferredSurfaceFormats = { {
		VkSurfaceFormatKHR{ (VkFormat)vk::Format::eB8G8R8A8Unorm, (VkColorSpaceKHR)vk::ColorSpaceKHR::eSrgbNonlinear },
		VkSurfaceFormatKHR{ (VkFormat)vk::Format::eR8G8B8A8Unorm, (VkColorSpaceKHR)vk::ColorSpaceKHR::eSrgbNonlinear }
	} };

	constexpr u32 invalidIndex = static_cast<u32>(-1);
}

namespace DEngine::Gfx::Vk
{
	namespace Const = Constants;
}