#pragma once

#include "DEngine/FixedWidthTypes.hpp"

#include "VulkanIncluder.hpp"
#include "QueueData.hpp"

namespace DEngine::Gfx::Vk
{
	struct MemoryTypes
	{
		static constexpr u32 invalidIndex = static_cast<u32>(-1);

		u32 deviceLocal = invalidIndex;
		u32 hostVisible = invalidIndex;
		u32 deviceLocalAndHostVisible = invalidIndex;

		inline bool unifiedMemory() const { return deviceLocal == hostVisible && deviceLocal != invalidIndex && hostVisible != invalidIndex; }
	};

	struct PhysDeviceInfo
	{
		vk::PhysicalDevice handle = nullptr;
		vk::PhysicalDeviceProperties properties{};
		vk::PhysicalDeviceMemoryProperties memProperties{};
		vk::SampleCountFlagBits maxFramebufferSamples{};
		QueueIndices queueIndices{};
		MemoryTypes memInfo{};
	};
}