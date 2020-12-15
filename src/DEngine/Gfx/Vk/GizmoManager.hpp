#pragma once

#include "VulkanIncluder.hpp"
#include "VMAIncluder.hpp"

namespace DEngine::Gfx::Vk
{
	class APIData;
	class DebugUtilsDispatch;
	class DeletionQueue;
	class DeviceDispatch;
	class QueueData;

	class GizmoManager
	{
	public:

		vk::Buffer vtxBuffer{};
		VmaAllocation vtxVmaAlloc{};
		u32 vertexCount = 0;

		vk::PipelineLayout pipelineLayout{};
		vk::Pipeline pipeline{};

		static void Initialize(
			GizmoManager& manager,
			DeviceDispatch const& device,
			QueueData const& queues,
			VmaAllocator const& vma,
			DeletionQueue const& deletionQueue,
			DebugUtilsDispatch const* debugUtils,
			APIData const& apiData);
	};
}