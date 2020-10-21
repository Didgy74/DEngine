#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Pair.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Gfx/Gfx.hpp>

#include "Constants.hpp"
#include "DynamicDispatch.hpp"
#include "VulkanIncluder.hpp"
#include "VMAIncluder.hpp"

#include <vector>
#include <mutex>

namespace DEngine::Gfx::Vk
{
	class GlobUtils;
	class GuiResourceManager;

	struct GfxRenderTarget
	{
		vk::Extent2D extent{};
		VmaAllocation vmaAllocation{};
		vk::Image img{};
		vk::ImageView imgView{};
		vk::Framebuffer framebuffer{};
	};

	struct ViewportData
	{
		GfxRenderTarget renderTarget{};
		vk::CommandPool cmdPool{};
		Std::StackVec<vk::CommandBuffer, Constants::maxInFlightCount> cmdBuffers{};

		vk::DescriptorPool cameraDescrPool{};
		Std::StackVec<vk::DescriptorSet, Constants::maxInFlightCount> camDataDescrSets{};
		VmaAllocation camVmaAllocation{};
		vk::Buffer camDataBuffer{};
		Std::Span<u8> camDataMappedMem;

		// This is temporary and shit
		// But it works. This is controlled by the GuiResourceManager
		vk::DescriptorSet descrSet{};
	};

	// This variable should basically not be accessed anywhere except from APIData.
	struct ViewportManager
	{
		// Controls the entire structure.
		std::mutex mutexLock{};

		struct CreateJob
		{
			ViewportID id = ViewportID::Invalid;
		};
		uSize viewportIDTracker = 0;
		std::vector<CreateJob> createQueue{};

		std::vector<ViewportID> deleteQueue{};

		// Unsorted vector holding viewport-data and their ID.
		struct Node
		{
			ViewportID id;
			ViewportData viewport;
		};
		std::vector<Node> viewportNodes{};

		static constexpr uSize minimumCamDataCapacity = 8;
		// Thread safe to access
		vk::DescriptorSetLayout cameraDescrLayout{};
		// Thread safe to access
		uSize camElementSize = 0;

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

		// Making it static made it more explicit.
		// Easier to identify in the main loop
		static void HandleEvents(
			ViewportManager& viewportManager,
			GlobUtils const& globUtils,
			Std::Span<ViewportUpdate const> viewportUpdates,
			GuiResourceManager const& guiResourceManager);

		static void UpdateCameras(
			ViewportManager& viewportManager,
			GlobUtils const& globUtils,
			Std::Span<ViewportUpdate const> viewportUpdates,
			u8 inFlightIndex);
	};
}