#pragma once

namespace DEngine::Gfx::Vk::Constants
{
	constexpr u32 maxResourceSets = 2;
	constexpr u32 preferredResourceSetCount = 2;

	constexpr u32 maxRequiredInstanceExtensions = 20;
	constexpr u32 maxAvailableInstanceExtensions = 20;
	constexpr u32 maxAvailableInstanceLayers = 20;

	constexpr u32 maxAvailablePresentModes = 10;
	constexpr vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eFifo;
	constexpr u32 maxAvailableSurfaceFormats = 25;
	constexpr u32 maxSwapchainLength = 2;
	constexpr u32 preferredSwapchainLength = 2;

	constexpr std::array<const char*, 1> preferredValidationLayers
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

	constexpr std::array<vk::SurfaceFormatKHR, 2> preferredSurfaceFormats =
	{
		vk::SurfaceFormatKHR(vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear),
		vk::SurfaceFormatKHR(vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear),
	};

	constexpr u32 invalidIndex = static_cast<u32>(-1);
}