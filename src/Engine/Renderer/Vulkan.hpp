#pragma once

#include "DRenderer/VulkanInitInfo.hpp"

#include <vector>
#include <any>
#include <functional>

namespace DRenderer
{
	namespace Vulkan
	{
		void Initialize(std::any& apiData, InitInfo& createInfo);
		void Terminate(std::any& apiData);
		void PrepareRenderingEarly(const std::vector<size_t>& spriteLoadQueue, const std::vector<size_t>& meshLoadQueue);
		void PrepareRenderingLate();
		void Draw();
	}
}