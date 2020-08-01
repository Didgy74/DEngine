#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/Pair.hpp"
#include "DEngine/Containers/StaticVector.hpp"
#include "DEngine/Containers/Span.hpp"
#include "DEngine/Gfx/Gfx.hpp"

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
		Std::StaticVector<vk::CommandBuffer, Constants::maxInFlightCount> cmdBuffers{};

		vk::DescriptorPool cameraDescrPool{};
		Std::StaticVector<vk::DescriptorSet, Constants::maxInFlightCount> camDataDescrSets{};
		VmaAllocation camVmaAllocation{};
		vk::Buffer camDataBuffer{};
		void* camDataMappedMem = nullptr;
		uSize camElementSize = 0;

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
		std::vector<Node> viewportData{};

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

		void NewViewport(ViewportID& viewportID);
		void DeleteViewport(ViewportID id);

		// Making it static made it more explicit.
		// Easier to identify in the main loop
		static void HandleEvents(
			ViewportManager& viewportManager,
			GlobUtils const& globUtils,
			Std::Span<ViewportUpdate const> viewportUpdates,
			GuiResourceManager const& guiResourceManager);
	};
}