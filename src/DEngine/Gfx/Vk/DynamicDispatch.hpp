#pragma once

#include "VulkanIncluder.hpp"

namespace DEngine::Gfx::Vk
{
	struct BaseDispatchRaw
	{
		static BaseDispatchRaw Build(PFN_vkGetInstanceProcAddr getInstanceProcAddr);

		PFN_vkCreateInstance vkCreateInstance = nullptr;
		PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties = nullptr;
		PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties = nullptr;
		PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion = nullptr;
	};

	struct InstanceDispatchRaw
	{
		static InstanceDispatchRaw Build(vk::Instance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr);

		PFN_vkDestroyInstance vkDestroyInstance = nullptr;
		PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = nullptr;
		PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties = nullptr;
		PFN_vkEnumerateDeviceLayerProperties vkEnumerateDeviceLayerProperties = nullptr;
		PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures = nullptr;
		PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2 = nullptr;
		PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties = nullptr;
		PFN_vkGetPhysicalDeviceMemoryProperties2 vkGetPhysicalDeviceMemoryProperties2 = nullptr;
		PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = nullptr;
		PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2 = nullptr;

		PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = nullptr;
		PFN_vkGetPhysicalDeviceQueueFamilyProperties2 vkGetPhysicalDeviceQueueFamilyProperties2 = nullptr;

		PFN_vkCreateDevice vkCreateDevice = nullptr;
	};

	struct DeviceDispatchRaw
	{
		static DeviceDispatchRaw Build(vk::Device vkDevice, PFN_vkGetDeviceProcAddr getProcAddr);

		PFN_vkBeginCommandBuffer vkBeginCommandBuffer = nullptr;
		PFN_vkCmdBeginQuery vkCmdBeginQuery = nullptr;
		PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass = nullptr;
		PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets = nullptr;
		PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer = nullptr;
		PFN_vkCmdBindPipeline vkCmdBindPipeline = nullptr;
		PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers = nullptr;
		PFN_vkCmdBlitImage vkCmdBlitImage = nullptr;
		PFN_vkCmdClearAttachments vkCmdClearAttachments = nullptr;
		PFN_vkCmdClearColorImage vkCmdClearColorImage = nullptr;
		PFN_vkCmdClearDepthStencilImage vkCmdClearDepthStencilImage = nullptr;
		PFN_vkCmdCopyBuffer vkCmdCopyBuffer = nullptr;
		PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage = nullptr;
		PFN_vkCmdCopyImage vkCmdCopyImage = nullptr;
		PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer = nullptr;
		PFN_vkCmdCopyQueryPoolResults vkCmdCopyQueryPoolResults = nullptr;
		PFN_vkCmdDispatch vkCmdDispatch = nullptr;
		PFN_vkCmdDispatchBase vkCmdDispatchBase = nullptr;
		PFN_vkCmdDispatchIndirect vkCmdDispatchIndirect = nullptr;
		PFN_vkCmdDraw vkCmdDraw = nullptr;
		PFN_vkCmdDrawIndexed vkCmdDrawIndexed = nullptr;
		PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect = nullptr;
		PFN_vkCmdDrawIndirect vkCmdDrawIndirect = nullptr;
		PFN_vkCmdEndQuery vkCmdEndQuery = nullptr;
		PFN_vkCmdEndRenderPass vkCmdEndRenderPass = nullptr;
		PFN_vkCmdExecuteCommands vkCmdExecuteCommands = nullptr;
		PFN_vkCmdFillBuffer vkCmdFillBuffer = nullptr;
		PFN_vkCmdNextSubpass vkCmdNextSubpass = nullptr;
		PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = nullptr;
		PFN_vkCmdPushConstants vkCmdPushConstants = nullptr;
		PFN_vkCmdResetEvent vkCmdResetEvent = nullptr;
		PFN_vkCmdResetQueryPool vkCmdResetQueryPool = nullptr;
		PFN_vkCmdResolveImage vkCmdResolveImage = nullptr;
		PFN_vkCmdSetBlendConstants vkCmdSetBlendConstants = nullptr;
		PFN_vkCmdSetDepthBias vkCmdSetDepthBias = nullptr;
		PFN_vkCmdSetDepthBounds vkCmdSetDepthBounds = nullptr;
		PFN_vkCmdSetDeviceMask vkCmdSetDeviceMask = nullptr;
		PFN_vkCmdSetEvent vkCmdSetEvent = nullptr;
		PFN_vkCmdSetLineWidth vkCmdSetLineWidth = nullptr;
		PFN_vkCmdSetScissor vkCmdSetScissor = nullptr;
		PFN_vkCmdSetStencilCompareMask vkCmdSetStencilCompareMask = nullptr;
		PFN_vkCmdSetStencilReference vkCmdSetStencilReference = nullptr;
		PFN_vkCmdSetStencilWriteMask vkCmdSetStencilWriteMask = nullptr;
		PFN_vkCmdSetViewport vkCmdSetViewport = nullptr;
		PFN_vkCmdUpdateBuffer vkCmdUpdateBuffer = nullptr;
		PFN_vkCmdWaitEvents vkCmdWaitEvents = nullptr;
		PFN_vkCmdWriteTimestamp vkCmdWriteTimestamp = nullptr;
		PFN_vkEndCommandBuffer vkEndCommandBuffer = nullptr;
		PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers = nullptr;
		PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets = nullptr;
		PFN_vkAllocateMemory vkAllocateMemory = nullptr;
		PFN_vkBindBufferMemory vkBindBufferMemory = nullptr;
		PFN_vkBindBufferMemory2 vkBindBufferMemory2 = nullptr;
		PFN_vkBindImageMemory vkBindImageMemory = nullptr;
		PFN_vkBindImageMemory2 vkBindImageMemory2 = nullptr;
		PFN_vkCreateBuffer vkCreateBuffer = nullptr;
		PFN_vkCreateBufferView vkCreateBufferView = nullptr;
		PFN_vkCreateCommandPool vkCreateCommandPool = nullptr;
		PFN_vkCreateComputePipelines vkCreateComputePipelines = nullptr;
		PFN_vkCreateDescriptorPool vkCreateDescriptorPool = nullptr;
		PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout = nullptr;
		PFN_vkCreateDescriptorUpdateTemplate vkCreateDescriptorUpdateTemplate = nullptr;
		PFN_vkCreateEvent vkCreateEvent = nullptr;
		PFN_vkCreateFence vkCreateFence = nullptr;
		PFN_vkCreateFramebuffer vkCreateFramebuffer = nullptr;
		PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines = nullptr;
		PFN_vkCreateImage vkCreateImage = nullptr;
		PFN_vkCreateImageView vkCreateImageView = nullptr;
		PFN_vkCreatePipelineCache vkCreatePipelineCache = nullptr;
		PFN_vkCreatePipelineLayout vkCreatePipelineLayout = nullptr;
		PFN_vkCreateQueryPool vkCreateQueryPool = nullptr;
		PFN_vkCreateRenderPass vkCreateRenderPass = nullptr;
		PFN_vkCreateSampler vkCreateSampler = nullptr;
		PFN_vkCreateSamplerYcbcrConversion vkCreateSamplerYcbcrConversion = nullptr;
		PFN_vkCreateSemaphore vkCreateSemaphore = nullptr;
		PFN_vkCreateShaderModule vkCreateShaderModule = nullptr;
		PFN_vkDestroyBuffer vkDestroyBuffer = nullptr;
		PFN_vkDestroyBufferView vkDestroyBufferView = nullptr;
		PFN_vkDestroyCommandPool vkDestroyCommandPool = nullptr;
		PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool = nullptr;
		PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout = nullptr;
		PFN_vkDestroyDescriptorUpdateTemplate vkDestroyDescriptorUpdateTemplate = nullptr;
		PFN_vkDestroyDevice vkDestroyDevice = nullptr;
		PFN_vkDestroyEvent vkDestroyEvent = nullptr;
		PFN_vkDestroyFence vkDestroyFence = nullptr;
		PFN_vkDestroyFramebuffer vkDestroyFramebuffer = nullptr;
		PFN_vkDestroyImage vkDestroyImage = nullptr;
		PFN_vkDestroyImageView vkDestroyImageView = nullptr;
		PFN_vkDestroyPipeline vkDestroyPipeline = nullptr;
		PFN_vkDestroyPipelineCache vkDestroyPipelineCache = nullptr;
		PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout = nullptr;
		PFN_vkDestroyQueryPool vkDestroyQueryPool = nullptr;
		PFN_vkDestroyRenderPass vkDestroyRenderPass = nullptr;
		PFN_vkDestroySampler vkDestroySampler = nullptr;
		PFN_vkDestroySamplerYcbcrConversion vkDestroySamplerYcbcrConversion = nullptr;
		PFN_vkDestroySemaphore vkDestroySemaphore = nullptr;
		PFN_vkDestroyShaderModule vkDestroyShaderModule = nullptr;
		PFN_vkDeviceWaitIdle vkDeviceWaitIdle = nullptr;
		PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges = nullptr;
		PFN_vkFreeCommandBuffers vkFreeCommandBuffers = nullptr;
		PFN_vkFreeDescriptorSets vkFreeDescriptorSets = nullptr;
		PFN_vkFreeMemory vkFreeMemory = nullptr;
		PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements = nullptr;
		PFN_vkGetBufferMemoryRequirements2 vkGetBufferMemoryRequirements2 = nullptr;
		PFN_vkGetDescriptorSetLayoutSupport vkGetDescriptorSetLayoutSupport = nullptr;
		PFN_vkGetDeviceGroupPeerMemoryFeatures vkGetDeviceGroupPeerMemoryFeatures = nullptr;
		PFN_vkGetDeviceMemoryCommitment vkGetDeviceMemoryCommitment = nullptr;
		PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = nullptr;
		PFN_vkGetDeviceQueue vkGetDeviceQueue = nullptr;
		PFN_vkGetDeviceQueue2 vkGetDeviceQueue2 = nullptr;
		PFN_vkGetEventStatus vkGetEventStatus = nullptr;
		PFN_vkGetFenceStatus vkGetFenceStatus = nullptr;
		PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements = nullptr;
		PFN_vkGetImageMemoryRequirements2 vkGetImageMemoryRequirements2 = nullptr;
		PFN_vkGetImageSparseMemoryRequirements vkGetImageSparseMemoryRequirements = nullptr;
		PFN_vkGetImageSparseMemoryRequirements2 vkGetImageSparseMemoryRequirements2 = nullptr;
		PFN_vkGetImageSubresourceLayout vkGetImageSubresourceLayout = nullptr;
		PFN_vkGetPipelineCacheData vkGetPipelineCacheData = nullptr;
		PFN_vkGetQueryPoolResults vkGetQueryPoolResults = nullptr;
		PFN_vkGetRenderAreaGranularity vkGetRenderAreaGranularity = nullptr;
		PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = nullptr;
		PFN_vkGetSwapchainStatusKHR vkGetSwapchainStatusKHR = nullptr;
		PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges = nullptr;
		PFN_vkMapMemory vkMapMemory = nullptr;
		PFN_vkMergePipelineCaches vkMergePipelineCaches = nullptr;
		PFN_vkResetCommandBuffer vkResetCommandBuffer = nullptr;
		PFN_vkResetCommandPool vkResetCommandPool = nullptr;
		PFN_vkResetDescriptorPool vkResetDescriptorPool = nullptr;
		PFN_vkResetEvent vkResetEvent = nullptr;
		PFN_vkResetFences vkResetFences = nullptr;
		PFN_vkSetEvent vkSetEvent = nullptr;
		PFN_vkTrimCommandPool vkTrimCommandPool = nullptr;
		PFN_vkUnmapMemory vkUnmapMemory = nullptr;
		PFN_vkUpdateDescriptorSetWithTemplate vkUpdateDescriptorSetWithTemplate = nullptr;
		PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets = nullptr;
		PFN_vkWaitForFences vkWaitForFences = nullptr;
		PFN_vkQueueBindSparse vkQueueBindSparse = nullptr;
		PFN_vkQueuePresentKHR vkQueuePresentKHR = nullptr;
		PFN_vkQueueSubmit vkQueueSubmit = nullptr;
		PFN_vkQueueWaitIdle vkQueueWaitIdle = nullptr;
	};

	struct EXT_DebugUtilsDispatchRaw
	{
		static EXT_DebugUtilsDispatchRaw Build(vk::Instance instance, PFN_vkGetInstanceProcAddr instanceProcAddr);

		PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = nullptr;
		PFN_vkSetDebugUtilsObjectTagEXT vkSetDebugUtilsObjectTagEXT = nullptr;
		PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXT = nullptr;
		PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabelEXT = nullptr;
		PFN_vkQueueInsertDebugUtilsLabelEXT vkQueueInsertDebugUtilsLabelEXT = nullptr;
		PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = nullptr;
		PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = nullptr;
		PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT = nullptr;
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = nullptr;
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;
		PFN_vkSubmitDebugUtilsMessageEXT vkSubmitDebugUtilsMessageEXT = nullptr;
	};

	struct KHR_SurfaceDispatchRaw
	{
		static KHR_SurfaceDispatchRaw Build(vk::Instance instance, PFN_vkGetInstanceProcAddr instanceProcAddr);

		PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = nullptr;
		PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR = nullptr;
		PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
		PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
		PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR = nullptr;
	};

	struct KHR_SwapchainDispatchRaw
	{
		static KHR_SwapchainDispatchRaw Build(vk::Device vkDevice, PFN_vkGetDeviceProcAddr getProcAddr);

		PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = nullptr;
		PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = nullptr;
		PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = nullptr;
		PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = nullptr;
		PFN_vkQueuePresentKHR vkQueuePresentKHR = nullptr;
		PFN_vkGetDeviceGroupPresentCapabilitiesKHR vkGetDeviceGroupPresentCapabilitiesKHR = nullptr;
		PFN_vkGetDeviceGroupSurfacePresentModesKHR vkGetDeviceGroupSurfacePresentModesKHR = nullptr;
		PFN_vkGetPhysicalDevicePresentRectanglesKHR vkGetPhysicalDevicePresentRectanglesKHR = nullptr;
		PFN_vkAcquireNextImage2KHR vkAcquireNextImage2KHR = nullptr;
	};

	struct BaseDispatch
	{
		static BaseDispatch Build(PFN_vkGetInstanceProcAddr procAddr);

		BaseDispatchRaw raw{};
		std::uint32_t enumerateInstanceVersion() const
		{
			if (raw.vkEnumerateInstanceVersion == nullptr)
				return VK_MAKE_VERSION(1, 0, 0);
			else
				return vk::enumerateInstanceVersion(raw);
		}
		vk::Result enumerateInstanceExtensionProperties(
			const char* pLayerName,
			std::uint32_t* pPropertyCount,
			vk::ExtensionProperties* pProperties) const
		{
			return vk::enumerateInstanceExtensionProperties(pLayerName, pPropertyCount, pProperties, raw);
		}
		vk::Result enumerateInstanceLayerProperties(std::uint32_t* pPropertyCount, vk::LayerProperties* pProperties) const
		{
			return vk::enumerateInstanceLayerProperties(pPropertyCount, pProperties, raw);
		}
		vk::Instance createInstance(vk::InstanceCreateInfo const& createInfo, vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return vk::createInstance(createInfo, allocator, raw);
		}
	};

	struct DebugUtilsDispatch
	{
		EXT_DebugUtilsDispatchRaw raw{};

		[[nodiscard]] static DebugUtilsDispatch Build(vk::Instance instance, PFN_vkGetInstanceProcAddr instanceProcAddr);

		[[nodiscard]] vk::DebugUtilsMessengerEXT createDebugUtilsMessengerEXT(
			vk::Instance instance,
			vk::DebugUtilsMessengerCreateInfoEXT const& createInfo,
			vk::Optional<const vk::AllocationCallbacks> allocator = nullptr) const
		{
			return instance.createDebugUtilsMessengerEXT(createInfo, allocator, raw);
		}

		void destroyDebugUtilsMessengerEXT(
			vk::Instance instance,
			vk::DebugUtilsMessengerEXT messenger,
			vk::Optional<const vk::AllocationCallbacks> allocator = nullptr) const
		{
			return instance.destroyDebugUtilsMessengerEXT(messenger, allocator, raw);
		}

		void setDebugUtilsObjectNameEXT(
			vk::Device device,
			vk::DebugUtilsObjectNameInfoEXT const& nameInfo) const
		{
			return device.setDebugUtilsObjectNameEXT(nameInfo, raw);
		}
	};

	struct InstanceDispatch
	{
		static InstanceDispatch Build(vk::Instance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr);

		vk::Instance handle{};
		InstanceDispatchRaw raw{};

		vk::Device createDevice(
			vk::PhysicalDevice physDevice,
			vk::DeviceCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return physDevice.createDevice(createInfo, allocator, raw);
		}

		void destroy(vk::Optional<vk::AllocationCallbacks const> allocator = nullptr)
		{
			return handle.destroy(allocator, raw);
		}

		vk::Result enumeratePhysicalDevices(std::uint32_t* pPhysicalDeviceCount, vk::PhysicalDevice* pPhysicalDevices) const
		{
			return handle.enumeratePhysicalDevices(pPhysicalDeviceCount, pPhysicalDevices, raw);
		}
		vk::Result enumeratePhysicalDeviceExtensionProperties(
			vk::PhysicalDevice physDevice,
			std::uint32_t* pPropertyCount,
			vk::ExtensionProperties* pProperties) const
		{
			return physDevice.enumerateDeviceExtensionProperties(nullptr, pPropertyCount, pProperties, raw);
		}

		vk::PhysicalDeviceFeatures getPhysicalDeviceFeatures(vk::PhysicalDevice physDevice) const
		{
			return physDevice.getFeatures(raw);
		}
		vk::PhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties(vk::PhysicalDevice physDevice) const
		{
			return physDevice.getMemoryProperties(raw);
		}
		vk::PhysicalDeviceProperties getPhysicalDeviceProperties(vk::PhysicalDevice physDevice) const
		{
			return physDevice.getProperties(raw);
		}
		void getPhysicalDeviceQueueFamilyProperties(
			vk::PhysicalDevice physDevice,
			std::uint32_t* pQueueFamilyPropertyCount,
			vk::QueueFamilyProperties* pQueueFamilyProperties) const
		{
			return physDevice.getQueueFamilyProperties(pQueueFamilyPropertyCount, pQueueFamilyProperties, raw);
		}

		KHR_SurfaceDispatchRaw surface_raw{};
		void destroySurface(vk::SurfaceKHR surface, vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.destroySurfaceKHR(surface, allocator, surface_raw);
		}
		vk::SurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilitiesKHR(vk::PhysicalDevice physDevice, vk::SurfaceKHR surface) const
		{
			return physDevice.getSurfaceCapabilitiesKHR(surface, surface_raw);
		}
		vk::Result getPhysicalDeviceSurfaceFormatsKHR(
			vk::PhysicalDevice physDevice,
			vk::SurfaceKHR surface,
			std::uint32_t* pSurfaceFormatCount,
			vk::SurfaceFormatKHR* pSurfaceFormats) const
		{
			return physDevice.getSurfaceFormatsKHR(surface, pSurfaceFormatCount, pSurfaceFormats, surface_raw);
		}
		bool getPhysicalDeviceSurfaceSupportKHR(vk::PhysicalDevice physDevice, std::uint32_t queueFamilyIndex, vk::SurfaceKHR surface) const
		{
			return static_cast<bool>(physDevice.getSurfaceSupportKHR(queueFamilyIndex, surface, surface_raw));
		}
		vk::Result getPhysicalDeviceSurfacePresentModesKHR(
			vk::PhysicalDevice physDevice,
			vk::SurfaceKHR surface,
			std::uint32_t* pPresentModeCount,
			vk::PresentModeKHR* pPresentModes) const
		{
			return physDevice.getSurfacePresentModesKHR(surface, pPresentModeCount, pPresentModes, surface_raw);
		}

	};

	class DeviceDispatch
	{
	private:
		DeviceDispatch(const DeviceDispatch&) = default;

		DeviceDispatch& operator=(const DeviceDispatch&) = default;

	public:
		DeviceDispatch() = default;

		void copy(const DeviceDispatch& in);

		static DeviceDispatch Build(vk::Device vkDevice, PFN_vkGetDeviceProcAddr getProcAddr);

		vk::Device handle{};
		DeviceDispatchRaw raw{};

		[[nodiscard]] vk::Result allocateCommandBuffers(
			vk::CommandBufferAllocateInfo const& pAllocateInfo,
			vk::CommandBuffer* pCommandBuffers) const
		{
			return handle.allocateCommandBuffers(&pAllocateInfo, pCommandBuffers, raw);
		}

		[[nodiscard]] vk::Result allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo const& allocInfo,
			vk::DescriptorSet* pDescriptorSets) const
		{
			return handle.allocateDescriptorSets(&allocInfo, pDescriptorSets, raw);
		}

		[[nodiscard]] vk::DeviceMemory allocateMemory(
			vk::MemoryAllocateInfo const& allocInfo,
			vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.allocateMemory(allocInfo, allocator, raw);
		}

		void beginCommandBuffer(vk::CommandBuffer cmdBuffer, vk::CommandBufferBeginInfo const& beginInfo) const
		{
			return cmdBuffer.begin(beginInfo, raw);
		}

		void bindBufferMemory(
			vk::Buffer buffer,
			vk::DeviceMemory memory,
			vk::DeviceSize memoryOffset) const
		{
			return handle.bindBufferMemory(buffer, memory, memoryOffset, raw);
		}

		void bindImageMemory(
			vk::Image image,
			vk::DeviceMemory memory,
			vk::DeviceSize memoryOffset) const
		{
			return handle.bindImageMemory(image, memory, memoryOffset, raw);
		}

		void cmdBeginRenderPass(
			vk::CommandBuffer commandBuffer, 
			vk::RenderPassBeginInfo const& renderPassBegin, 
			vk::SubpassContents contents) const
		{
			return commandBuffer.beginRenderPass(renderPassBegin, contents, raw);
		}

		void cmdBindDescriptorSets(
			vk::CommandBuffer commandBuffer,
			vk::PipelineBindPoint pipelineBindPoint, 
			vk::PipelineLayout layout, 
			std::uint32_t firstSet, 
			vk::ArrayProxy<vk::DescriptorSet const> descriptorSets, 
			vk::ArrayProxy<uint32_t const> dynamicOffsets) const
		{
			return commandBuffer.bindDescriptorSets(
				pipelineBindPoint,
				layout,
				firstSet,
				descriptorSets,
				dynamicOffsets,
				raw);
		}

		void cmdBindPipeline(vk::CommandBuffer commandBuffer, vk::PipelineBindPoint pipelineBindPoint, vk::Pipeline pipeline) const
		{
			return commandBuffer.bindPipeline(pipelineBindPoint, pipeline, raw);
		}

		void cmdCopyImage(
			vk::CommandBuffer commandBuffer,
			vk::Image srcImage,
			vk::ImageLayout srcImageLayout,
			vk::Image dstImage,
			vk::ImageLayout dstImageLayout,
			vk::ArrayProxy<vk::ImageCopy const> regions) const
		{
			return commandBuffer.copyImage(
				srcImage,
				srcImageLayout,
				dstImage,
				dstImageLayout,
				regions,
				raw);
		}

		void cmdDraw(
			vk::CommandBuffer commandBuffer,
			std::uint32_t vertexCount,
			std::uint32_t instanceCount,
			std::uint32_t firstVertex,
			std::uint32_t firstInstance) const
		{
			return commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance, raw);
		}

		void cmdDrawIndexed(
			vk::CommandBuffer commandBuffer,
			std::uint32_t indexCount,
			std::uint32_t instanceCount,
			std::uint32_t firstIndex,
			std::int32_t vertexOffset,
			std::uint32_t firstInstance) const
		{
			return commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance, raw);
		}

		void cmdEndRenderPass(vk::CommandBuffer commandBuffer) const
		{
			return commandBuffer.endRenderPass(raw);
		}

		void cmdPipelineBarrier(
			vk::CommandBuffer commandBuffer,
			vk::PipelineStageFlags srcStageMask,
			vk::PipelineStageFlags dstStageMask,
			vk::DependencyFlags dependencyFlags,
			std::uint32_t memoryBarrierCount,
			vk::MemoryBarrier const* pMemoryBarriers,
			std::uint32_t bufferMemoryBarrierCount,
			vk::BufferMemoryBarrier const* pBufferMemoryBarriers,
			std::uint32_t imageMemoryBarrierCount,
			vk::ImageMemoryBarrier const* pImageMemoryBarriers) const
		{
			return commandBuffer.pipelineBarrier(
				srcStageMask,
				dstStageMask,
				dependencyFlags,
				memoryBarrierCount,
				pMemoryBarriers,
				bufferMemoryBarrierCount,
				pBufferMemoryBarriers,
				imageMemoryBarrierCount,
				pImageMemoryBarriers,
				raw);
		}

		void cmdPipelineBarrier(
			vk::CommandBuffer commandBuffer,
			vk::PipelineStageFlags srcStageMask,
			vk::PipelineStageFlags dstStageMask,
			vk::DependencyFlags dependencyFlags,
			vk::ArrayProxy<vk::MemoryBarrier const> memoryBarriers,
			vk::ArrayProxy<vk::BufferMemoryBarrier const> bufferMemoryBarriers,
			vk::ArrayProxy<vk::ImageMemoryBarrier const> imageMemoryBarriers) const
		{
			return commandBuffer.pipelineBarrier(
				srcStageMask,
				dstStageMask,
				dependencyFlags,
				memoryBarriers,
				bufferMemoryBarriers,
				imageMemoryBarriers,
				raw);
		}

		void cmdSetViewport(
			vk::CommandBuffer commandBuffer, 
			std::uint32_t firstViewport, 
			vk::ArrayProxy<vk::Viewport const> viewports) const
		{
			return commandBuffer.setViewport(firstViewport, viewports, raw);
		}

		[[nodiscard]] vk::Buffer createBuffer(
			vk::BufferCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.createBuffer(createInfo, allocator, raw);
		}

		[[nodiscard]] vk::CommandPool createCommandPool(
			vk::CommandPoolCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.createCommandPool(createInfo, allocator, raw);
		}

		[[nodiscard]] vk::DescriptorPool createDescriptorPool(
			vk::DescriptorPoolCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.createDescriptorPool(createInfo, allocator, raw);
		}

		[[nodiscard]] vk::DescriptorSetLayout createDescriptorSetLayout(
			vk::DescriptorSetLayoutCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.createDescriptorSetLayout(createInfo, allocator, raw);
		}

		[[nodiscard]] vk::Fence createFence(
			vk::FenceCreateInfo const& createInfo, 
			vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.createFence(createInfo, allocator, raw);
		}

		[[nodiscard]] vk::Framebuffer createFramebuffer(
			vk::FramebufferCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.createFramebuffer(createInfo, allocator, raw);
		}

		[[nodiscard]] vk::PipelineLayout createPipelineLayout(
			vk::PipelineLayoutCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.createPipelineLayout(createInfo, allocator, raw);
		}

		[[nodiscard]] vk::Sampler createSampler(
			vk::SamplerCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.createSampler(createInfo, allocator, raw);
		}

		[[nodiscard]] vk::ShaderModule createShaderModule(
			vk::ShaderModuleCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.createShaderModule(createInfo, allocator, raw);
		}

		void destroyCommandPool(vk::CommandPool commandPool, vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.destroyCommandPool(commandPool, allocator, raw);
		}

		void destroyFramebuffer(
			vk::Framebuffer framebuffer,
			vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.destroyFramebuffer(framebuffer, allocator, raw);
		}

		void destroyShaderModule(
			vk::ShaderModule shaderModule,
			vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.destroyShaderModule(shaderModule, allocator, raw);
		}

		void endCommandBuffer(vk::CommandBuffer cmdBuffer) const
		{
			return cmdBuffer.end(raw);
		}

		void freeCommandBuffers(
			vk::CommandPool commandPool,
			vk::ArrayProxy<vk::CommandBuffer const> commandBuffers) const
		{
			return handle.freeCommandBuffers(commandPool, commandBuffers, raw);
		}

		void freeMemory(
			vk::DeviceMemory memory,
			vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.freeMemory(memory, allocator, raw);
		}

		[[nodiscard]] vk::Queue getQueue(std::uint32_t familyIndex, std::uint32_t queueIndex) const
		{
			return handle.getQueue(familyIndex, queueIndex, raw);
		}

		[[nodiscard]] void* mapMemory(
			vk::DeviceMemory memory,
			vk::DeviceSize offset, 
			vk::DeviceSize size,
			vk::MemoryMapFlags flags) const
		{
			return handle.mapMemory(memory, offset, size, flags, raw);
		}

		void queueSubmit(vk::Queue queue, vk::ArrayProxy<vk::SubmitInfo const> submits, vk::Fence fence) const
		{
			return queue.submit(submits, fence, raw);
		}

		void updateDescriptorSets(
			vk::ArrayProxy<vk::WriteDescriptorSet const> descriptorWrites,
			vk::ArrayProxy<vk::CopyDescriptorSet const> descriptorCopies) const
		{
			return handle.updateDescriptorSets(descriptorWrites, descriptorCopies, raw);
		}

		[[nodiscard]] vk::Result waitForFences(
			vk::ArrayProxy<vk::Fence const> fences, 
			bool waitAll, 
			std::uint64_t timeout) const
		{
			return handle.waitForFences(fences, static_cast<VkBool32>(waitAll), timeout, raw);
		}

		void waitQueueIdle(vk::Queue queue) const
		{
			return queue.waitIdle(raw);
		}

		// Pipelines
		[[nodiscard]] vk::Pipeline createGraphicsPipeline(
			vk::PipelineCache pipelineCache,
			vk::GraphicsPipelineCreateInfo const& createInfo,
			vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			vk::ResultValue<vk::Pipeline> temp = handle.createGraphicsPipeline(
				pipelineCache, 
				createInfo, 
				allocator,
				raw);
			if (temp.result == vk::Result::eSuccess)
			{
				return temp.value;
			}
			else
			{
				throw std::runtime_error("DEngine, Vulkan: Unable to create graphics pipeline.");
				return {};
			}
		}
		[[nodiscard]] vk::Result createGraphicsPipelines(
			vk::PipelineCache pipelineCache,
			std::uint32_t createInfoCount,
			vk::GraphicsPipelineCreateInfo const* pCreateInfos,
			vk::Optional<vk::AllocationCallbacks const> allocator,
			vk::Pipeline* pPipelines) const
		{
			return handle.createGraphicsPipelines(pipelineCache, createInfoCount, pCreateInfos, allocator, pPipelines, raw);
		}

		// Memory requirements
		[[nodiscard]] vk::MemoryRequirements getBufferMemoryRequirements(vk::Buffer buffer) const
		{
			return handle.getBufferMemoryRequirements(buffer, raw);
		}
		[[nodiscard]] vk::MemoryRequirements getImageMemoryRequirements(vk::Image image) const
		{
			return handle.getImageMemoryRequirements(image, raw);
		}


		// Fences

		void destroyFence(vk::Fence fence, vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.destroyFence(fence, allocator, raw);
		}

		void resetFences(vk::ArrayProxy<vk::Fence const> fences) const
		{
			return handle.resetFences(fences, raw);
		}



		// Images
		[[nodiscard]] vk::Image createImage(vk::ImageCreateInfo const& createInfo, vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.createImage(createInfo, allocator, raw);
		}
		void destroyImage(vk::Image image, vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.destroyImage(image, allocator, raw);
		}
		vk::ImageView createImageView(vk::ImageViewCreateInfo const& createInfo, vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.createImageView(createInfo, allocator, raw);
		}
		void destroyImageView(vk::ImageView image, vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.destroyImageView(image, allocator, raw);
		}





		// Renderpass
		[[nodiscard]] vk::RenderPass createRenderPass(vk::RenderPassCreateInfo const& createInfo, vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.createRenderPass(createInfo, allocator, raw);
		}
		void destroyRenderPass(vk::RenderPass renderPass, vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.destroyRenderPass(renderPass, allocator, raw);
		}




		// Semaphore
		vk::Semaphore createSemaphore(vk::SemaphoreCreateInfo const& createInfo, vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.createSemaphore(createInfo, allocator, raw);
		}
		void destroySemaphore(vk::Semaphore semaphore, vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.destroySemaphore(semaphore, allocator, raw);
		}

		void waitIdle() const
		{
			return handle.waitIdle(raw);
		}



		KHR_SwapchainDispatchRaw swapchain_raw{};
		vk::SwapchainKHR createSwapchainKHR(vk::SwapchainCreateInfoKHR const& info, vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.createSwapchainKHR(info, allocator, swapchain_raw);
		}
		void destroySwapchainKHR(vk::SwapchainKHR swapchain, vk::Optional<vk::AllocationCallbacks const> allocator = nullptr) const
		{
			return handle.destroySwapchainKHR(swapchain, allocator, swapchain_raw);
		}
		vk::Result getSwapchainImagesKHR(vk::SwapchainKHR swapchain, std::uint32_t* pSwapchainImageCount, vk::Image* pSwapchainImages) const
		{
			return handle.getSwapchainImagesKHR(swapchain, pSwapchainImageCount, pSwapchainImages, swapchain_raw);
		}
		vk::ResultValue<std::uint32_t> acquireNextImageKHR(vk::SwapchainKHR swapchain, std::uint64_t timeout, vk::Semaphore semaphore, vk::Fence fence) const
		{
			return handle.acquireNextImageKHR(swapchain, timeout, semaphore, fence, swapchain_raw);
		}
		vk::ResultValue<std::uint32_t> acquireNextImage2KHR(vk::AcquireNextImageInfoKHR const& acquireInfo) const
		{
			return handle.acquireNextImage2KHR(acquireInfo, swapchain_raw);
		}
		vk::Result queuePresentKHR(vk::Queue queue, vk::PresentInfoKHR const& presentInfo) const
		{
			return queue.presentKHR(&presentInfo, swapchain_raw);
		}

	};

	using DevDispatch = DeviceDispatch;

	PFN_vkGetInstanceProcAddr loadInstanceProcAddressPFN();
}