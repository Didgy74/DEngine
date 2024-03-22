#pragma once

#include "VulkanIncluder.hpp"
#include "ForwardDeclarations.hpp"

#include <vector>

namespace DEngine::Gfx::Vk
{
	// Holds information we assume to be equal
	// across all surfaces.
	struct SurfaceInfo
	{
	public:
		vk::CompositeAlphaFlagBitsKHR compositeAlphaToUse = {};

		std::vector<vk::PresentModeKHR> supportedPresentModes;
		vk::PresentModeKHR presentModeToUse;

		std::vector<vk::SurfaceFormatKHR> supportedSurfaceFormats;
		vk::SurfaceFormatKHR surfaceFormatToUse;

		static void BuildInPlace(
			SurfaceInfo& surfaceInfo,
			vk::SurfaceKHR initialSurface,
			InstanceDispatch const& instance,
			vk::PhysicalDevice physDevice);
	};
}