 #pragma once

#include "VulkanIncluder.hpp"

#include <array>

import DEngine.FixedWidthTypes;
import DEngine.Std.Array;

namespace DEngine::Gfx::Vk::Constants {
	constexpr u32 maxInFlightCount = 4;
	constexpr u32 preferredInFlightCount = 3;
	// We need at least two, because one always needs to be available for writing.
	static_assert(preferredInFlightCount >= 2);

	constexpr vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eFifo;
	constexpr u32 maxSwapchainLength = 4;
	constexpr u32 preferredSwapchainLength = 3;

	constexpr char const* khronosLayerName = { "VK_LAYER_KHRONOS_validation" };
	constexpr char const* armMaliPerfLayerName = { "VK_LAYER_ARM_mali_perf_doc" };

	constexpr std::array preferredLayerNames = {
		khronosLayerName,
		//armMaliPerfLayerName
	};

	constexpr std::array<const char*, 1> requiredInstanceExtensions = {
		"VK_KHR_surface"
	};

	constexpr bool enableDebugUtils = true;
	constexpr char const* debugUtilsExtensionName = { "VK_EXT_debug_utils" };

	constexpr std::array requiredDeviceExtensions = {
		"VK_KHR_swapchain"
	};

	constexpr Std::Array<vk::SurfaceFormatKHR, 2> preferredSurfaceFormats = { {
		{ vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear },
		// MoltenVK doesn't provide RGBA.
		{ vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear },
	} };

	constexpr u32 invalidIndex = static_cast<u32>(-1);
}

namespace DEngine::Gfx::Vk
{
	namespace Const = Constants;
}