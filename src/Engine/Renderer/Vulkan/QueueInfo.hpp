#pragma once

#include "volk/volk.hpp"
#include <vulkan/vulkan.hpp>

namespace DRenderer::Vulkan
{
	struct QueueInfo
	{
		static constexpr uint32_t invalidIndex = std::numeric_limits<uint32_t>::max();

		struct Queue
		{
			vk::Queue handle = nullptr;
			uint32_t familyIndex = invalidIndex;
			uint32_t queueIndex = invalidIndex;
		};

		Queue graphics{};
		Queue transfer{};

		inline bool TransferIsSeparate() const { return transfer.familyIndex != graphics.familyIndex; }
	};
}