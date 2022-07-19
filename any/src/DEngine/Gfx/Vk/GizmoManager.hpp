#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Math/Vector.hpp>
#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Gfx/Gfx.hpp>

#include "VulkanIncluder.hpp"
#include "VMAIncluder.hpp"
#include "ForwardDeclarations.hpp"

namespace DEngine::Gfx::Vk
{
	class GizmoManager
	{
	public:
		vk::Buffer arrowVtxBuffer = {};
		VmaAllocation arrowVtxVmaAlloc = {};
		u32 arrowVtxCount = 0;

		vk::Buffer circleVtxBuffer = {};
		VmaAllocation circleVtxAlloc = {};
		u32 circleVtxCount = 0;

		vk::Buffer scaleArrow2d_VtxBuffer = {};
		VmaAllocation scaleArrow2d_VtxAlloc = {};
		u32 scaleArrow2d_VtxCount = 0;

		vk::PipelineLayout pipelineLayout = {};
		vk::Pipeline arrowPipeline = {};
		vk::Pipeline quadPipeline = {};

		static constexpr uSize lineVtxMinCapacity = 128;
		static constexpr uSize lineVtxElementSize = sizeof(Math::Vec3);
		vk::Pipeline linePipeline = {};
		vk::Buffer lineVtxBuffer = {};
		VmaAllocation lineVtxVmaAlloc = {};
		uSize lineVtxBufferCapacity = 0;
		Std::Span<u8> lineVtxBufferMappedMem = {};

		struct InitInfo
		{
			u8 inFlightCount;
			DeviceDispatch const* device;
			QueueData const* queues;
			VmaAllocator const* vma;
			DeletionQueue* delQueue;
			Std::BumpAllocator* frameAlloc;
			DebugUtilsDispatch const* debugUtils;
			APIData const* apiData;
			Std::Span<Math::Vec3 const> arrowMesh;
			Std::Span<Math::Vec3 const> circleLineMesh;
			Std::Span<Math::Vec3 const> arrowScaleMesh2d;
		};
		static void Initialize(
			GizmoManager& manager,
			InitInfo const& initInfo);

		static void UpdateLineVtxBuffer(
			GizmoManager& manager,
			GlobUtils const& globUtils,
			u8 inFlightIndex,
			Std::Span<Math::Vec3 const> vertices);

		static void DebugLines_RecordDrawCalls(
			GizmoManager const& manager,
			GlobUtils const& globUtils,
			ViewportMgr_ViewportData const& viewportData,
			Std::Span<LineDrawCmd const> lineDrawCmds,
			vk::CommandBuffer cmdBuffer,
			u8 inFlightIndex) noexcept;

		static void Gizmo_RecordDrawCalls(
			GizmoManager const& manager,
			GlobUtils const& globUtils,
			ViewportMgr_ViewportData const& viewportData,
			ViewportUpdate::Gizmo const& gizmo,
			vk::CommandBuffer cmdBuffer,
			u8 inFlightIndex) noexcept;
	};
}