#pragma once

#include "../APIDataBase.hpp"

#include "Constants.hpp"
#include "DeletionQueue.hpp"
#include "DynamicDispatch.hpp"
#include "GlobUtils.hpp"
#include "ObjectDataManager.hpp"
#include "QueueData.hpp"
#include "TextureManager.hpp"
#include "ViewportManager.hpp"
#include "VMAIncluder.hpp"
#include "VulkanIncluder.hpp"


#include "DEngine/Gfx/Gfx.hpp"

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/StaticVector.hpp"
#include "DEngine/Containers/Array.hpp"
#include "DEngine/Containers/Pair.hpp"

#include <vector>

namespace DEngine::Gfx::Vk
{
	constexpr u32 invalidIndex = static_cast<u32>(-1);
	
	template<typename T>
	[[nodiscard]] inline constexpr bool IsValidIndex(T in) = delete;
	template<>
	[[nodiscard]] inline constexpr bool IsValidIndex<u32>(u32 in) { return in != static_cast<u32>(-1); }
	template<>
	[[nodiscard]] inline constexpr bool IsValidIndex<u64>(u64 in) { return in != static_cast<u64>(-1); }

	struct SurfaceInfo
	{
	public:
		vk::SurfaceKHR handle{};
		vk::SurfaceCapabilitiesKHR capabilities{};
		vk::CompositeAlphaFlagBitsKHR compositeAlphaFlag = {};

		std::vector<vk::PresentModeKHR> supportedPresentModes;
		std::vector<vk::SurfaceFormatKHR> supportedSurfaceFormats;
	};

	struct SwapchainSettings
	{
		vk::SurfaceKHR surface{};
		vk::PresentModeKHR presentMode{};
		vk::SurfaceFormatKHR surfaceFormat{};
		vk::SurfaceTransformFlagBitsKHR transform = {};
		vk::CompositeAlphaFlagBitsKHR compositeAlphaFlag = {};
		vk::Extent2D extents{};
		u32 numImages{};
	};

	struct SwapchainData
	{
		vk::SwapchainKHR handle{};
		u8 uid = 0;

		vk::Extent2D extents{};
		vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
		vk::SurfaceFormatKHR surfaceFormat{};

		Std::StaticVector<vk::Image, Constants::maxSwapchainLength> images{};

		vk::CommandPool cmdPool{};
		Std::StaticVector<vk::CommandBuffer, Constants::maxSwapchainLength> cmdBuffers{};
		vk::Semaphore imageAvailableSemaphore{};
	};

	struct Device
	{
		vk::Device handle{};
		vk::PhysicalDevice physDeviceHandle{};
	};

	struct GUIRenderTarget
	{
		vk::Extent2D extent{};
		vk::Format format{};
		VmaAllocation vmaAllocation{};
		vk::Image img{};
		vk::ImageView imgView{};
		vk::Framebuffer framebuffer{};
	};

	struct GUIData
	{
		GUIRenderTarget renderTarget{};
		vk::CommandPool cmdPool{};
		// Has length of resource sets
		Std::StaticVector<vk::CommandBuffer, Constants::maxResourceSets> cmdBuffers{};
	};

	struct VMA_MemoryTrackingData
	{
		DebugUtilsDispatch const* debugUtils = nullptr;
		vk::Device deviceHandle{};
		std::mutex vma_idTracker_lock{};
		u64 vma_idTracker = 0;
	};

	struct APIData final : public APIDataBase
	{
		APIData();
		virtual ~APIData() override;
		virtual void Draw(Data& gfxData, DrawParams const& drawParams) override;
		// Thread safe
		virtual void NewViewport(uSize& viewportID, void*& imguiTexID) override;
		// Thread safe
		virtual void DeleteViewport(uSize id) override;

		Gfx::ILog* logger = nullptr;
		Gfx::IWsi* wsiInterface = nullptr;
		Gfx::TextureAssetInterface const* test_textureAssetInterface = nullptr;

		SurfaceInfo surface{};
		SwapchainData swapchain{};

		u8 currentInFlightFrame = 0;

		// Do not touch this.
		VMA_MemoryTrackingData vma_trackingData{};

		Std::StaticVector<vk::Fence, Constants::maxResourceSets> mainFences{};

		GUIData guiData{};

		GlobUtils globUtils{};

		ViewportManager viewportManager{};
		ObjectDataManager objectDataManager{};
		TextureManager textureManager{};

		vk::PipelineLayout testPipelineLayout{};
		vk::Pipeline testPipeline{};
	};
}