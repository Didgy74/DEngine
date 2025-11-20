#pragma once

#include <DEngine/Gfx/Gfx.hpp>

#include "VulkanIncluder.hpp"
#include "VMAIncluder.hpp"
#include "ForwardDeclarations.hpp"
#include "RaiiHandles.hpp"

import DEngine.Std.BumpAllocator;

namespace DEngine::Gfx::Vk
{
	class GizmoManager {
	public:
		static constexpr f32 gizmoTransparency = 0.5f;

		BoxVmaBuffer arrowVtxBuffer = {};
		u32 arrowVtxCount = 0;

		BoxVmaBuffer circleVtxBuffer = {};
		u32 circleVtxCount = 0;

		BoxVmaBuffer scaleArrow2d_VtxBuffer = {};
		u32 scaleArrow2d_VtxCount = 0;

		struct PushConstant {
			Math::Mat4 objectMatrix;
			Math::Vec4 color;
		};
		// All shaders share the same layout
		BoxVkPipelineLayout pipelineLayout = {};

		BoxVkPipeline arrowPipeline = {};
		BoxVkPipeline quadPipeline = {};

		static constexpr uSize lineVtxMinCapacity = 128;
		static constexpr uSize lineVtxElementSize = sizeof(Math::Vec3);
		BoxVkPipeline linePipeline = {};
		BoxVmaBuffer lineVtxBuffer = {};
		VmaAllocationInfo lineVtxVmaAllocInfo = {};
		// Holds the number of points we can hold. The actual buffer will be this number * amount
		// of in flight frames.
		uSize lineVtxBufferCapacity = 0;

		struct InitInfo {
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