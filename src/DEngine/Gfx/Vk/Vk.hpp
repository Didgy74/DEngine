#pragma once

#include "../APIDataBase.hpp"

#include "VulkanIncluder.hpp"
#include "DynamicDispatch.hpp"
#include "Constants.hpp"
#include "DeletionQueue.hpp"
#include "QueueData.hpp"

#include "VMAIncluder.hpp"

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
		std::vector<Std::Pair<uSize, ViewportVkData>> viewportData{};

		static constexpr uSize minimumCamDataCapacity = 8;
		// Thread safe to access
		vk::DescriptorSetLayout cameraDescrLayout{};
		// Thread safe to access
		uSize camElementSize = 0;
	};

	struct VMA_MemoryTrackingData
	{
		DebugUtilsDispatch const* debugUtils = nullptr;
		vk::Device deviceHandle{};
		std::mutex vma_idTracker_lock{};
		u64 vma_idTracker = 0;
	};

	// Everything here is thread-safe to use and access!!
	struct GlobUtils
	{
		GlobUtils();
		GlobUtils(GlobUtils const&) = delete;
		GlobUtils(GlobUtils&&) = delete;

		Gfx::ILog* logger = nullptr;

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

		DeletionQueue deletionQueue;

		u8 resourceSetCount = 0;
		u8 CurrentResourceSetIndex_Async();

		bool useEditorPipeline = false;
		vk::RenderPass guiRenderPass{};

		vk::RenderPass gfxRenderPass{};

	private:
		void SetCurrentResourceSetIndex(u8 index);
		std::mutex currentResourceSetIndex_Lock{};
		u8 currentResourceSetIndex_Var{};

		friend struct APIData;
	};

	struct APIData : public APIDataBase
	{
		APIData();
		virtual ~APIData() override;
		virtual void Draw(Data& gfxData, Draw_Params const& drawParams) override;
		// Thread safe
		virtual void NewViewport(uSize& viewportID, void*& imguiTexID) override;
		// Thread safe
		virtual void DeleteViewport(uSize id) override;

		Gfx::ILog* logger = nullptr;
		Gfx::IWsi* wsiInterface = nullptr;

		SurfaceInfo surface{};
		SwapchainData swapchain{};

		u8 currentInFlightFrame = 0;

		// Do not touch this.
		VMA_MemoryTrackingData vma_trackingData{};

		Std::StaticVector<vk::Fence, Constants::maxResourceSets> mainFences{};

		GUIData guiData{};

		GlobUtils globUtils{};

		ViewportManager viewportManager{};
	

		vk::PipelineLayout testPipelineLayout{};
		vk::Pipeline testPipeline{};
	};
}