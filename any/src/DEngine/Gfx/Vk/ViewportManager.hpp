#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Std/Containers/AllocRef.hpp>
#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Gfx/Gfx.hpp>

#include "Constants.hpp"
#include "VulkanIncluder.hpp"
#include "VMAIncluder.hpp"
#include "ForwardDeclarations.hpp"

#include <vector>
#include <mutex>

namespace DEngine::Gfx::Vk
{
	struct ViewportMgr_GfxRenderTarget {
		vk::Extent2D extent = {};
		VmaAllocation vmaAllocation = {};
		vk::Image img = {};
		vk::ImageView imgView = {};
		vk::Framebuffer framebuffer = {};

		[[nodiscard]] bool IsInitialized() const { return img != vk::Image{}; }
	};

	struct ViewportMgr_ViewportData {
		ViewportMgr_GfxRenderTarget renderTarget = {};
		vk::DescriptorPool cameraDescrPool = {};
		Std::StackVec<vk::DescriptorSet, Constants::maxInFlightCount> camDataDescrSets = {};
		VmaAllocation camVmaAllocation = {};
		vk::Buffer camDataBuffer = {};
		Std::ByteSpan camDataMappedMem = {};

		// idk if I like this
		vk::DescriptorPool imgDescrPool = {};
		vk::DescriptorSet imgDescrSet = {};
	};

	// This variable should basically not be accessed anywhere except from APIData.
	struct ViewportManager {
		// Create queue resources start
		std::mutex createQueue_Lock;
		struct CreateJob { ViewportID id = ViewportID::Invalid; };
		uSize viewportIDTracker = 0;
		std::vector<CreateJob> createQueue{};
		// Create queue resources end

		// Delete queue resources start
		std::mutex deleteQueue_Lock;
		std::vector<ViewportID> deleteQueue{};
		// Delete queue resources end

		// Main mutable resources start
		// This doesn't need a mutex because it's only ever accessed by the rendering thread.
		struct Node {
			ViewportID id;
			ViewportMgr_ViewportData viewport;

			[[nodiscard]] bool IsInitialized() const;
		};
		// Unsorted vector holding viewport-data and their ID.
		std::vector<Node> viewportNodes{};
		// Main mutable resources end

		[[nodiscard]] ViewportMgr_ViewportData const& GetViewportData(ViewportID id) const;

		static constexpr uSize minimumCamDataCapacity = 8;
		vk::DescriptorSetLayout cameraDescrLayout{};
		uSize camElementSize = 0;
		vk::Sampler imgSampler = {};
		vk::DescriptorSetLayout imgDescrSetLayout = {};


		[[nodiscard]] static bool Init(
			ViewportManager& manager,
			DeviceDispatch const& device,
			uSize minUniformBufferOffsetAlignment,
			DebugUtilsDispatch const* debugUtils);

		static void NewViewport(
			ViewportManager& viewportManager, 
			ViewportID& viewportID);
		static void DeleteViewport(
			ViewportManager& viewportManager, 
			ViewportID id);

		struct ProcessEvents_Params {
			GlobUtils const& globUtils;
			vk::CommandBuffer cmdBuffer;
			DelQueue& delQueue;
			Std::AllocRef transientAlloc;
			Std::Span<ViewportUpdate const> viewportUpdates;
			int inFlightIndex;
			DebugUtilsDispatch const* debugUtils;
		};
		// Making it static made it more explicit.
		// Easier to identify in the main loop
		static void ProcessEvents(
			ViewportManager& manager,
			ProcessEvents_Params const& params);

		static void UpdateCameras(
			ViewportManager& viewportManager,
			GlobUtils const& globUtils,
			Std::Span<ViewportUpdate const> viewportUpdates,
			u8 inFlightIndex);
	};

	using ViewportMan = ViewportManager;
}