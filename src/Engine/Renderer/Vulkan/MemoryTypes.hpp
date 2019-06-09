#pragma once

#include <cstdint>
#include <limits>

namespace DRenderer::Vulkan
{
	struct MemoryTypes
	{
		static constexpr uint32_t invalidIndex = std::numeric_limits<uint32_t>::max();

		uint32_t deviceLocal = invalidIndex;
		uint32_t hostVisible = invalidIndex;
		uint32_t deviceLocalAndHostVisible = invalidIndex;

		inline bool DeviceLocalIsHostVisible() const { return deviceLocal == hostVisible && deviceLocal != invalidIndex; }
	};
}