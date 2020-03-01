#pragma once

#include "VulkanIncluder.hpp"
#include "DynamicDispatch.hpp"
#include "Constants.hpp"
#include "DeletionQueue.hpp"
#include "QueueData.hpp"

#include "vk_mem_alloc.h"

#include "DEngine/Gfx/Gfx.hpp"

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/FixedVector.hpp"
#include "DEngine/Containers/Array.hpp"
#include "DEngine/Containers/Pair.hpp"

namespace DEngine::Gfx::Vk
{
	constexpr u32 invalidIndex = static_cast<std::uint32_t>(-1);
	
	template<typename T>
	[[nodiscard]] inline constexpr bool IsValidIndex(T in) = delete;
	template<>
	[[nodiscard]] inline constexpr bool IsValidIndex<std::uint32_t>(std::uint32_t in) { return in != static_cast<std::uint32_t>(-1); }
	template<>
	[[nodiscard]] inline constexpr bool IsValidIndex<std::uint64_t>(std::uint64_t in) { return in != static_cast<std::uint64_t>(-1); }
   

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

	struct SurfaceInfo
	{
	public:
		vk::SurfaceKHR handle{};
		vk::SurfaceCapabilitiesKHR capabilities{};
		vk::CompositeAlphaFlagBitsKHR compositeAlphaFlag = {};

		Cont::FixedVector<vk::PresentModeKHR, Constants::maxAvailablePresentModes> supportedPresentModes;
		Cont::FixedVector<vk::SurfaceFormatKHR, Constants::maxAvailableSurfaceFormats> supportedSurfaceFormats;
	};

	struct SwapchainSettings
	{
		vk::SurfaceKHR surface{};
		vk::SurfaceCapabilitiesKHR capabilities{};
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
		std::uint8_t uid = 0;

		vk::Extent2D extents{};
		vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
		vk::SurfaceFormatKHR surfaceFormat{};

		Cont::FixedVector<vk::Image, Constants::maxSwapchainLength> images{};

		vk::CommandPool cmdPool{};
		Cont::FixedVector<vk::CommandBuffer, Constants::maxSwapchainLength> cmdBuffers{};
		vk::Semaphore imageAvailableSemaphore{};
	};

	struct Device
	{
		vk::Device handle{};
		vk::PhysicalDevice physDeviceHandle{};
	};

	struct GfxRenderTarget
	{
		VmaAllocation vmaAllocation{};

		vk::Extent2D extent{};
		vk::Image img{};
		vk::ImageView imgView{};
		vk::Framebuffer framebuffer{};
	};

	struct ViewportVkData
	{
		bool inUse = false;
		GfxRenderTarget renderTarget{};
		vk::CommandPool cmdPool{};
		Cont::FixedVector<vk::CommandBuffer, Constants::maxResourceSets> cmdBuffers{};
		void* imguiTextureID = nullptr;
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
		vk::RenderPass renderPass{};
		GUIRenderTarget renderTarget{};
		vk::CommandPool cmdPool{};
		// Has length of resource sets
		Cont::FixedVector<vk::CommandBuffer, Constants::maxResourceSets> cmdBuffers{};
	};

	// Everything here is thread-safe to use and access!!
	struct GlobUtils
	{
		InstanceDispatch instance{};

		DebugUtilsDispatch debugUtils{};
		vk::DebugUtilsMessengerEXT debugMessenger{};
		bool UsingDebugUtils() const {
			return debugUtils.raw.vkCmdBeginDebugUtilsLabelEXT != nullptr;
		}
		DebugUtilsDispatch const* DebugUtilsPtr() const {
			return debugUtils.raw.vkCmdBeginDebugUtilsLabelEXT != nullptr ? &debugUtils : nullptr;
		}

		PhysDeviceInfo physDevice{};

		DeviceDispatch device{};

		QueueData queues{};

		VmaAllocator vma{};

		DeletionQueue deletionQueue{};

		u8 resourceSetCount = 0;

		bool useEditorPipeline = false;
		vk::RenderPass guiRenderPass{};

		vk::RenderPass gfxRenderPass{};
	};

	struct APIData
	{
		Gfx::ILog* logger = nullptr;
		Gfx::IWsi* wsiInterface = nullptr;

		SurfaceInfo surface{};
		SwapchainData swapchain{};

		u8 currentResourceSet = 0;

		Cont::FixedVector<vk::Fence, Constants::maxResourceSets> mainFences{};

		GUIData guiData{};

		// The main renderpass for rendering the 3D stuff
		Cont::Array<ViewportVkData, Gfx::Constants::maxViewportCount> viewportDatas{};

		GlobUtils globUtils{};


		vk::DescriptorSetLayout test_cameraDescrLayout{};
		Cont::FixedVector<vk::DescriptorSet, Constants::maxResourceSets * Gfx::Constants::maxViewportCount> test_cameraDescrSets{};
		void* test_mappedMem = nullptr;
		u32 test_camUboOffset = 0;

		vk::PipelineLayout testPipelineLayout{};
		vk::Pipeline testPipeline{};
	};
}