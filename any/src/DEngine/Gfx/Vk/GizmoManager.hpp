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
		vk::Buffer arrowVtxBuffer{};
		VmaAllocation arrowVtxVmaAlloc{};
		u32 arrowVtxCount = 0;

		vk::PipelineLayout pipelineLayout{};
		vk::Pipeline arrowPipeline{};
		vk::Pipeline quadPipeline{};

		static constexpr uSize lineVtxMinCapacity = 128;
		static constexpr uSize lineVtxElementSize = sizeof(Math::Vec3);
		vk::Pipeline linePipeline{};
		vk::Buffer lineVtxBuffer{};
		VmaAllocation lineVtxVmaAlloc{};
		uSize lineVtxBufferCapacity = 0;
		Std::Span<u8> lineVtxBufferMappedMem{};

		static void Initialize(
			GizmoManager& manager,
			u8 inFlightCount,
			DeviceDispatch const& device,
			QueueData const& queues,
			VmaAllocator const& vma,
			DeletionQueue const& deletionQueue,
			DebugUtilsDispatch const* debugUtils,
			APIData const& apiData,
			Std::Span<Math::Vec3 const> arrowMesh);

		static void UpdateLineVtxBuffer(
			GizmoManager& manager,
			GlobUtils const& globUtils,
			u8 inFlightIndex,
			Std::Span<Math::Vec3 const> vertices);
	};
}