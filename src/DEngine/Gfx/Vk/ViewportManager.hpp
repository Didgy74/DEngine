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
	struct GlobUtils;

	struct GfxRenderTarget
	{
		VmaAllocation vmaAllocation{};

		vk::Extent2D extent{};
		vk::Image img{};
		vk::ImageView imgView{};
		vk::Framebuffer framebuffer{};
	};

	struct ViewportData
	{
		uSize id = static_cast<uSize>(-1);

		GfxRenderTarget renderTarget{};
		vk::CommandPool cmdPool{};
		Std::StaticVector<vk::CommandBuffer, Constants::maxResourceSets> cmdBuffers{};
		void* imguiTextureID = nullptr;

		vk::DescriptorPool cameraDescrPool{};
		Std::StaticVector<vk::DescriptorSet, Constants::maxResourceSets> camDataDescrSets{};
		VmaAllocation camVmaAllocation{};
		vk::Buffer camDataBuffer{};
		void* mappedMem = nullptr;
		uSize camElementSize = 0;
	};

	// This variable should basically not be accessed anywhere except from APIData.
	struct ViewportManager
	{
		struct CreateJob
		{
			uSize id = static_cast<uSize>(-1);
			void* imguiTexID = nullptr;
		};
		uSize viewportIDTracker = 0;
		std::vector<CreateJob> createQueue{};

		std::vector<uSize> deleteQueue{};

		// Controls the entire structure.
		std::mutex mutexLock{};

		// Unsorted vector holding viewport-data and their ID.
		std::vector<Std::Pair<uSize, ViewportData>> viewportData{};

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

		void NewViewport(uSize& viewportID, void*& imguiTexID);
		void DeleteViewport(uSize id);

		// Making it static made it more explicit.
		// Easier to identify in the main loop
		static void HandleEvents(
			ViewportManager& vpManager,
			GlobUtils const& globUtils,
			Std::Span<ViewportUpdateData const> viewportUpdates);
	};
}