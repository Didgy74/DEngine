#pragma once

#include "DRenderer/VulkanInitInfo.hpp"
#include "RendererData.hpp"

#include <vector>
#include <any>
#include <functional>

namespace DRenderer
{
	namespace Vulkan
	{
		void Initialize(Core::APIDataPointer& apiData, InitInfo& createInfo);
		void Terminate(std::any& apiData);
		void PrepareRenderingEarly(const Core::PrepareRenderingEarlyParams& in);
		void PrepareRenderingLate();
		void Draw();
	}
}