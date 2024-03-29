#pragma once

#include "VulkanIncluder.hpp"

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Gfx/impl/Assert.hpp>

#include <limits>

namespace DEngine::Gfx::Vk
{
	PFN_vkGetInstanceProcAddr loadInstanceProcAddressPFN();

	struct BaseDispatchRaw
	{
		[[nodiscard]] static BaseDispatchRaw Build(PFN_vkGetInstanceProcAddr getInstanceProcAddr);

		PFN_vkCreateInstance vkCreateInstance;
		PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
		PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
		PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion;
	};

	struct InstanceDispatchRaw
	{
		[[nodiscard]] static InstanceDispatchRaw Build(
			vk::Instance instance, 
			PFN_vkGetInstanceProcAddr getInstanceProcAddr);

		PFN_vkCreateDevice vkCreateDevice;
		PFN_vkDestroyInstance vkDestroyInstance;
		PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
		PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
		PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
		PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
		PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
		PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;		
	};

	struct DeviceDispatchRaw
	{
		[[nodiscard]] static DeviceDispatchRaw Build(
			vk::Device vkDevice, 
			PFN_vkGetDeviceProcAddr getProcAddr);

		PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
		PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
		PFN_vkAllocateMemory vkAllocateMemory;
		PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
		PFN_vkBindBufferMemory vkBindBufferMemory;
		PFN_vkBindImageMemory vkBindImageMemory;
		PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
		PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
		PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
		PFN_vkCmdBindPipeline vkCmdBindPipeline;
		PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
		PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
		PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
		PFN_vkCmdCopyImage vkCmdCopyImage;
		PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer;
		PFN_vkCmdDraw vkCmdDraw;
		PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
		PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
		PFN_vkCmdNextSubpass vkCmdNextSubpass;
		PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
		PFN_vkCmdPushConstants vkCmdPushConstants;
		PFN_vkCmdSetScissor vkCmdSetScissor;
		PFN_vkCmdSetViewport vkCmdSetViewport;
		PFN_vkCreateBuffer vkCreateBuffer;
		PFN_vkCreateBufferView vkCreateBufferView;
		PFN_vkCreateCommandPool vkCreateCommandPool;
		PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
		PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
		PFN_vkCreateFence vkCreateFence;
		PFN_vkCreateFramebuffer vkCreateFramebuffer;
		PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
		PFN_vkCreateImage vkCreateImage;
		PFN_vkCreateImageView vkCreateImageView;
		PFN_vkCreatePipelineCache vkCreatePipelineCache;
		PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
		PFN_vkCreateRenderPass vkCreateRenderPass;
		PFN_vkCreateSampler vkCreateSampler;
		PFN_vkCreateSemaphore vkCreateSemaphore;
		PFN_vkCreateShaderModule vkCreateShaderModule;
		PFN_vkDestroyBuffer vkDestroyBuffer;
		PFN_vkDestroyBufferView vkDestroyBufferView;
		PFN_vkDestroyCommandPool vkDestroyCommandPool;
		PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
		PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
		PFN_vkDestroyDevice vkDestroyDevice;
		PFN_vkDestroyFence vkDestroyFence;
		PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
		PFN_vkDestroyImage vkDestroyImage;
		PFN_vkDestroyImageView vkDestroyImageView;
		PFN_vkDestroyPipeline vkDestroyPipeline;
		PFN_vkDestroyPipelineCache vkDestroyPipelineCache;
		PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
		PFN_vkDestroyRenderPass vkDestroyRenderPass;
		PFN_vkDestroySampler vkDestroySampler;
		PFN_vkDestroySemaphore vkDestroySemaphore;
		PFN_vkDestroyShaderModule vkDestroyShaderModule;
		PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
		PFN_vkEndCommandBuffer vkEndCommandBuffer;
		PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
		PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
		PFN_vkFreeDescriptorSets vkFreeDescriptorSets;
		PFN_vkFreeMemory vkFreeMemory;
		PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
		PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
		PFN_vkGetDeviceQueue vkGetDeviceQueue;
		PFN_vkGetFenceStatus vkGetFenceStatus;
		PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
		PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges;
		PFN_vkMapMemory vkMapMemory;
		PFN_vkResetCommandBuffer vkResetCommandBuffer;
		PFN_vkResetCommandPool vkResetCommandPool;
		PFN_vkResetDescriptorPool vkResetDescriptorPool;
		PFN_vkResetFences vkResetFences;
		PFN_vkTrimCommandPool vkTrimCommandPool;
		PFN_vkUnmapMemory vkUnmapMemory;
		PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
		PFN_vkWaitForFences vkWaitForFences;
		PFN_vkQueueSubmit vkQueueSubmit;
		PFN_vkQueueWaitIdle vkQueueWaitIdle;
	};

	struct EXT_DebugUtilsDispatchRaw
	{
		[[nodiscard]] static EXT_DebugUtilsDispatchRaw Build(
			vk::Instance instance, 
			PFN_vkGetInstanceProcAddr instanceProcAddr);

		PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
	};

	struct KHR_SurfaceDispatchRaw
	{
		[[nodiscard]] static KHR_SurfaceDispatchRaw Build(
			vk::Instance instance, 
			PFN_vkGetInstanceProcAddr instanceProcAddr);

		PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
		PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
		PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
		PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
		PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
	};

	struct KHR_SwapchainDispatchRaw
	{
		[[nodiscard]] static KHR_SwapchainDispatchRaw Build(
			vk::Device vkDevice, 
			PFN_vkGetDeviceProcAddr getProcAddr);

		PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
		PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
		PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
		PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
		PFN_vkQueuePresentKHR vkQueuePresentKHR;
	};

	class BaseDispatch
	{
	public:
		BaseDispatch() noexcept = default;
		BaseDispatch(BaseDispatch const&) noexcept = delete;
		BaseDispatch(BaseDispatch&&) noexcept = delete;

		BaseDispatch& operator=(BaseDispatch const&) noexcept = delete;
		BaseDispatch& operator=(BaseDispatch&&) noexcept = delete;

		static void BuildInPlace(
			BaseDispatch& dispatcher,
			PFN_vkGetInstanceProcAddr procAddr);

		BaseDispatchRaw raw;
		[[nodiscard]] vk::Instance CreateInstance(
			vk::InstanceCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		[[nodiscard]] vk::Result EnumerateInstanceExtensionProperties(
			char const* pLayerName,
			std::uint32_t* pPropertyCount,
			vk::ExtensionProperties* pProperties) const;

		[[nodiscard]] vk::Result EnumerateInstanceLayerProperties(
			std::uint32_t* pPropertyCount,
			vk::LayerProperties* pProperties) const;

		[[nodiscard]] std::uint32_t EnumerateInstanceVersion() const;
	};

	class DebugUtilsDispatch
	{
	public:
		EXT_DebugUtilsDispatchRaw raw;

		DebugUtilsDispatch() noexcept = default;
		DebugUtilsDispatch(DebugUtilsDispatch const&) noexcept = delete;
		DebugUtilsDispatch(DebugUtilsDispatch&&) noexcept = delete;

		DebugUtilsDispatch& operator=(DebugUtilsDispatch const&) noexcept = delete;
		DebugUtilsDispatch& operator=(DebugUtilsDispatch&&) noexcept = delete;

		static void BuildInPlace(
			DebugUtilsDispatch& dispatcher,
			vk::Instance instance, 
			PFN_vkGetInstanceProcAddr instanceProcAddr);

		[[nodiscard]] vk::DebugUtilsMessengerEXT createDebugUtilsMessengerEXT(
			vk::Instance instance,
			vk::DebugUtilsMessengerCreateInfoEXT const& createInfo,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		void Destroy(
			vk::Instance instance,
			vk::DebugUtilsMessengerEXT messenger,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		void setDebugUtilsObjectNameEXT(
			vk::Device device,
			vk::DebugUtilsObjectNameInfoEXT const& nameInfo) const;

		template<typename T>
		void Helper_SetObjectName(vk::Device device, T handle, char const* name) const
		{
			using BaseType = typename T::CType;
			DENGINE_IMPL_GFX_ASSERT(raw.vkSetDebugUtilsObjectNameEXT != nullptr);
			vk::DebugUtilsObjectNameInfoEXT nameInfo = {};
			nameInfo.objectHandle = uint64_t(BaseType(handle));
			nameInfo.objectType = T::objectType;
			nameInfo.pObjectName = name;
			setDebugUtilsObjectNameEXT(device, nameInfo);
		}
	};

	class InstanceDispatch
	{
	public:
		InstanceDispatch() noexcept = default;
		InstanceDispatch(InstanceDispatch const&) noexcept = delete;
		InstanceDispatch(InstanceDispatch&&) noexcept = delete;
		InstanceDispatch& operator=(InstanceDispatch const&) noexcept = delete;
		InstanceDispatch& operator=(InstanceDispatch&&) noexcept = delete;

		static void BuildInPlace(
			InstanceDispatch& dispatcher,
			vk::Instance instance, 
			PFN_vkGetInstanceProcAddr getInstanceProcAddr);

		vk::Instance handle;
		InstanceDispatchRaw raw;

		[[nodiscard]] vk::Device createDevice(
			vk::PhysicalDevice physDevice,
			vk::DeviceCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		void Destroy(vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		[[nodiscard]] vk::Result enumeratePhysicalDeviceExtensionProperties(
			vk::PhysicalDevice physDevice,
			std::uint32_t* pPropertyCount,
			vk::ExtensionProperties* pProperties) const;

		[[nodiscard]] vk::Result enumeratePhysicalDevices(
			std::uint32_t* pPhysicalDeviceCount, 
			vk::PhysicalDevice* pPhysicalDevices) const;

		[[nodiscard]] vk::PhysicalDeviceFeatures getPhysicalDeviceFeatures(vk::PhysicalDevice physDevice) const;

		[[nodiscard]] vk::PhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties(vk::PhysicalDevice physDevice) const;

		[[nodiscard]] vk::PhysicalDeviceProperties getPhysicalDeviceProperties(vk::PhysicalDevice physDevice) const;

		void getPhysicalDeviceQueueFamilyProperties(
			vk::PhysicalDevice physDevice,
			std::uint32_t* pQueueFamilyPropertyCount,
			vk::QueueFamilyProperties* pQueueFamilyProperties) const;


		KHR_SurfaceDispatchRaw surfaceRaw{};
		void Destroy(
			vk::SurfaceKHR in,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		[[nodiscard]] vk::SurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilitiesKHR(
			vk::PhysicalDevice physDevice, 
			vk::SurfaceKHR surface) const;

		[[nodiscard]] vk::Result getPhysicalDeviceSurfaceFormatsKHR(
			vk::PhysicalDevice physDevice,
			vk::SurfaceKHR surface,
			std::uint32_t* pSurfaceFormatCount,
			vk::SurfaceFormatKHR* pSurfaceFormats) const;

		[[nodiscard]] vk::Result getPhysicalDeviceSurfacePresentModesKHR(
			vk::PhysicalDevice physDevice,
			vk::SurfaceKHR surface,
			std::uint32_t* pPresentModeCount,
			vk::PresentModeKHR* pPresentModes) const;

		[[nodiscard]] bool getPhysicalDeviceSurfaceSupportKHR(
			vk::PhysicalDevice physDevice,
			std::uint32_t queueFamilyIndex,
			vk::SurfaceKHR surface) const;
	};

	class QueueData;
	class DeviceDispatch
	{
	public:
		DeviceDispatch() noexcept = default;
		DeviceDispatch(DeviceDispatch const&) noexcept = delete;
		DeviceDispatch(DeviceDispatch&&) noexcept = delete;

		DeviceDispatch& operator=(DeviceDispatch const&) noexcept = delete;
		DeviceDispatch& operator=(DeviceDispatch&&) noexcept = delete;

		static void BuildInPlace(
			DeviceDispatch& dispatcher,
			vk::Device vkDevice,
			vk::PhysicalDeviceLimits physDeviceLimits,
			PFN_vkGetDeviceProcAddr getProcAddr);

		vk::Device handle{};
		DeviceDispatchRaw raw{};
		vk::PhysicalDeviceLimits physDeviceLimits{};
		[[nodiscard]] auto const& PhysDeviceLimits() const { return physDeviceLimits; }

		[[nodiscard]] vk::Result allocateCommandBuffers(
			vk::CommandBufferAllocateInfo const& allocateInfo,
			vk::CommandBuffer* pCommandBuffers) const noexcept;

		[[nodiscard]] vk::Result AllocateDescriptorSets(
			vk::DescriptorSetAllocateInfo const& info,
			vk::DescriptorSet* pSets) const noexcept;
		[[nodiscard]] auto Alloc(
			vk::DescriptorSetAllocateInfo const& info,
			vk::DescriptorSet* pSets) const noexcept {
			return AllocateDescriptorSets(info, pSets);
		}

		[[nodiscard]] vk::DeviceMemory allocateMemory(
			vk::MemoryAllocateInfo const& allocInfo,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		void beginCommandBuffer(
			vk::CommandBuffer cmdBuffer, 
			vk::CommandBufferBeginInfo const& beginInfo) const;

		void bindBufferMemory(
			vk::Buffer buffer,
			vk::DeviceMemory memory,
			vk::DeviceSize memoryOffset) const;

		void bindImageMemory(
			vk::Image image,
			vk::DeviceMemory memory,
			vk::DeviceSize memoryOffset) const;

		void cmdBeginRenderPass(
			vk::CommandBuffer commandBuffer,
			vk::RenderPassBeginInfo const& renderPassBegin,
			vk::SubpassContents contents) const noexcept;

		void cmdBindDescriptorSets(
			vk::CommandBuffer commandBuffer,
			vk::PipelineBindPoint pipelineBindPoint,
			vk::PipelineLayout layout,
			std::uint32_t firstSet,
			vk::ArrayProxy<vk::DescriptorSet const> descriptorSets,
			vk::ArrayProxy<uint32_t const> dynamicOffsets) const noexcept;

		void cmdBindIndexBuffer(
			vk::CommandBuffer commandBuffer,
			vk::Buffer buffer, 
			vk::DeviceSize offset, 
			vk::IndexType indexType) const noexcept;

		void cmdBindPipeline(
			vk::CommandBuffer commandBuffer,
			vk::PipelineBindPoint pipelineBindPoint,
			vk::Pipeline pipeline) const noexcept;

		void cmdBindVertexBuffers(
			vk::CommandBuffer commandBuffer,
			uint32_t firstBinding,
			vk::ArrayProxy<vk::Buffer const> buffers,
			vk::ArrayProxy<vk::DeviceSize const> offsets) const noexcept;

		void cmdCopyBuffer(
			vk::CommandBuffer commandBuffer,
			vk::Buffer srcBuffer,
			vk::Buffer dstBuffer,
			vk::ArrayProxy<vk::BufferCopy const> regions) const noexcept;
		
		void cmdCopyBufferToImage(
			vk::CommandBuffer commandBuffer,
			vk::Buffer srcBuffer,
			vk::Image dstImage,
			vk::ImageLayout dstImageLayout,
			vk::ArrayProxy<vk::BufferImageCopy const> regions) const noexcept;

		void cmdCopyImage(
			vk::CommandBuffer commandBuffer,
			vk::Image srcImage,
			vk::ImageLayout srcImageLayout,
			vk::Image dstImage,
			vk::ImageLayout dstImageLayout,
			vk::ArrayProxy<vk::ImageCopy const> regions) const noexcept;

		void cmdDraw(
			vk::CommandBuffer commandBuffer,
			std::uint32_t vertexCount,
			std::uint32_t instanceCount,
			std::uint32_t firstVertex,
			std::uint32_t firstInstance) const noexcept;

		void cmdDrawIndexed(
			vk::CommandBuffer commandBuffer,
			std::uint32_t indexCount,
			std::uint32_t instanceCount,
			std::uint32_t firstIndex,
			std::int32_t vertexOffset,
			std::uint32_t firstInstance) const noexcept;

		void cmdEndRenderPass(vk::CommandBuffer commandBuffer) const noexcept;

		void cmdPipelineBarrier(
			vk::CommandBuffer commandBuffer,
			vk::PipelineStageFlags srcStageMask,
			vk::PipelineStageFlags dstStageMask,
			vk::DependencyFlags dependencyFlags,
			vk::ArrayProxy<vk::MemoryBarrier const> memoryBarriers,
			vk::ArrayProxy<vk::BufferMemoryBarrier const> bufferMemoryBarriers,
			vk::ArrayProxy<vk::ImageMemoryBarrier const> imageMemoryBarriers) const noexcept;

		void cmdPushConstants(
			vk::CommandBuffer commandBuffer,
			vk::PipelineLayout layout,
			vk::ShaderStageFlags stageFlags,
			std::uint32_t offset,
			std::uint32_t size,
			void const* pValues) const noexcept;

		void cmdSetScissor(
			vk::CommandBuffer commandBuffer,
			std::uint32_t firstScissor,
			vk::ArrayProxy<vk::Rect2D const> scissors) const noexcept;

		void cmdSetViewport(
			vk::CommandBuffer commandBuffer,
			std::uint32_t firstViewport,
			vk::ArrayProxy<vk::Viewport const> viewports) const noexcept;

		[[nodiscard]] vk::Buffer createBuffer(
			vk::BufferCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		[[nodiscard]] vk::ResultValue<vk::CommandPool> CreateCommandPool(
			vk::CommandPoolCreateInfo const& info,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const noexcept;
		[[nodiscard]] auto Create(
			vk::CommandPoolCreateInfo const& info,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const noexcept {
			return CreateCommandPool(info, allocator);
		}

		[[nodiscard]] vk::DescriptorPool CreateDescriptorPool(
			vk::DescriptorPoolCreateInfo const& info,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;
		[[nodiscard]] auto Create(
			vk::DescriptorPoolCreateInfo const& info,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const {
			return CreateDescriptorPool(info, allocator);
		}

		[[nodiscard]] vk::DescriptorSetLayout CreateDescriptorSetLayout(
			vk::DescriptorSetLayoutCreateInfo const& info,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;
		[[nodiscard]] auto Create(
			vk::DescriptorSetLayoutCreateInfo const& info,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const {
			return CreateDescriptorSetLayout(info, allocator);
		}

		[[nodiscard]] vk::Fence createFence(
			vk::FenceCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		[[nodiscard]] vk::Framebuffer createFramebuffer(
			vk::FramebufferCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		[[nodiscard]] vk::Result CreateGraphicsPipelines(
			vk::PipelineCache cache,
			vk::ArrayProxy<vk::GraphicsPipelineCreateInfo const> infos,
			vk::Optional<vk::AllocationCallbacks> allocator,
			vk::Pipeline* pPipelines) const;
		[[nodiscard]] auto Create(
			vk::PipelineCache cache,
			vk::ArrayProxy<vk::GraphicsPipelineCreateInfo const> infos,
			vk::Optional<vk::AllocationCallbacks> allocator,
			vk::Pipeline* pPipelines) const {
			return CreateGraphicsPipelines(
				cache,
				infos,
				allocator,
				pPipelines);
		}
		[[nodiscard]] auto Create(
			vk::PipelineCache cache,
			vk::ArrayProxy<vk::GraphicsPipelineCreateInfo const> infos,
			vk::Pipeline* pPipelines) const {
			return CreateGraphicsPipelines(
				cache,
				infos,
				nullptr,
				pPipelines);
		}
		[[nodiscard]] auto Create(
			vk::ArrayProxy<vk::GraphicsPipelineCreateInfo const> infos,
			vk::Pipeline* pPipelines) const {
			return CreateGraphicsPipelines(
				{},
				infos,
				nullptr,
				pPipelines);
		}

		[[nodiscard]] vk::Image createImage(
			vk::ImageCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		[[nodiscard]] vk::ImageView createImageView(
			vk::ImageViewCreateInfo const& createInfo, 
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		[[nodiscard]] vk::PipelineLayout CreatePipelineLayout(
			vk::PipelineLayoutCreateInfo const& info,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;
		[[nodiscard]] auto Create(
			vk::PipelineLayoutCreateInfo const& info,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const {
			return CreatePipelineLayout(info, allocator);
		}

		[[nodiscard]] vk::RenderPass createRenderPass(
			vk::RenderPassCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;


		[[nodiscard]] vk::Sampler CreateSampler(
			vk::SamplerCreateInfo const& info,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;
		[[nodiscard]] auto Create(
			vk::SamplerCreateInfo const& info,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const {
			return CreateSampler(info, allocator);
		}

		[[nodiscard]] vk::ResultValue<vk::Semaphore> createSemaphore(
			vk::SemaphoreCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		[[nodiscard]] vk::ShaderModule createShaderModule(
			vk::ShaderModuleCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		void Destroy() const;
		void Destroy(
			vk::CommandPool in,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;
		void Destroy(
			vk::DescriptorPool in,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;
		void Destroy(
			vk::Framebuffer in,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;
		void Destroy(
			vk::Fence in,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;
		void Destroy(
			vk::Image in,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;
		void Destroy(
			vk::ImageView in,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;
		void Destroy(
			vk::RenderPass in,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;
		void Destroy(
			vk::Semaphore in,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;
		void Destroy(
			vk::ShaderModule in,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		void endCommandBuffer(vk::CommandBuffer cmdBuffer) const;

		[[nodiscard]] vk::Result FlushMappedMemoryRanges(vk::ArrayProxy<vk::MappedMemoryRange const> const& ranges) const;

		void FreeCommandBuffers(
			vk::CommandPool pool,
			vk::ArrayProxy<vk::CommandBuffer const> cmdBuffers) const;
		void Free(
			vk::CommandPool pool,
			vk::ArrayProxy<vk::CommandBuffer const> cmdBuffers) const {
			FreeCommandBuffers(pool, cmdBuffers);
		}

		void FreeDescriptorSets(
			vk::DescriptorPool pool,
			vk::ArrayProxy<vk::DescriptorSet const> sets) const;
		void Free(
			vk::DescriptorPool pool,
			vk::ArrayProxy<vk::DescriptorSet const> sets) const {
			FreeDescriptorSets(pool, sets);
		}

		void freeMemory(
			vk::DeviceMemory memory,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		[[nodiscard]] vk::Result getFenceStatus(vk::Fence fence) const noexcept;

		[[nodiscard]] vk::Queue getQueue(
			std::uint32_t familyIndex, 
			std::uint32_t queueIndex) const;

		[[nodiscard]] void* mapMemory(
			vk::DeviceMemory memory,
			vk::DeviceSize offset,
			vk::DeviceSize size,
			vk::MemoryMapFlags flags) const;

		void resetCommandPool(
			vk::CommandPool commandPool,
			vk::CommandPoolResetFlags flags = vk::CommandPoolResetFlags()) const;
		void resetFences(vk::ArrayProxy<vk::Fence const> fences) const;

		void updateDescriptorSets(
			vk::ArrayProxy<vk::WriteDescriptorSet const> descriptorWrites,
			vk::ArrayProxy<vk::CopyDescriptorSet const> descriptorCopies) const;

		[[nodiscard]] vk::Result waitForFences(
			vk::ArrayProxy<vk::Fence const> fences,
			bool waitAll,
			std::uint64_t timeout = std::numeric_limits<std::uint64_t>::max()) const noexcept;

		QueueData const* m_queueDataPtr = nullptr;
		void waitIdle() const;



		KHR_SwapchainDispatchRaw swapchainRaw{};
		[[nodiscard]] vk::ResultValue<std::uint32_t> acquireNextImageKHR(
			vk::SwapchainKHR swapchain,
			std::uint64_t timeout,
			vk::Semaphore semaphore,
			vk::Fence fence) const noexcept;

		[[nodiscard]] vk::SwapchainKHR createSwapchainKHR(
			vk::SwapchainCreateInfoKHR const& createInfo,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		void Destroy(
			vk::SwapchainKHR in,
			vk::Optional<vk::AllocationCallbacks> allocator = nullptr) const;

		[[nodiscard]] vk::Result getSwapchainImagesKHR(
			vk::SwapchainKHR swapchain, 
			std::uint32_t* pSwapchainImageCount,
			vk::Image* pSwapchainImages) const noexcept;

		[[nodiscard]] vk::Result queuePresentKHR(
			vk::Queue queue,
			vk::PresentInfoKHR const& presentInfo) const noexcept;
	};

	using DevDispatch = DeviceDispatch;

	
}