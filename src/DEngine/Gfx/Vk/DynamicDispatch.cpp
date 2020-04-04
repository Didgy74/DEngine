#include "DynamicDispatch.hpp"

namespace DEngine::Gfx::Vk
{
    BaseDispatchRaw BaseDispatchRaw::Build(PFN_vkGetInstanceProcAddr getInstanceProcAddr)
    {
        BaseDispatchRaw returnVal{};

        returnVal.vkCreateInstance = (PFN_vkCreateInstance)getInstanceProcAddr(nullptr, "vkCreateInstance");
        returnVal.vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)getInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties");
        returnVal.vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)getInstanceProcAddr(nullptr, "vkEnumerateInstanceLayerProperties");
        returnVal.vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)getInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion");

        return returnVal;
    }

	DeviceDispatchRaw DeviceDispatchRaw::Build(vk::Device inDevice, PFN_vkGetDeviceProcAddr getDeviceProcAddr)
	{
		DeviceDispatchRaw returnVal{};

        VkDevice device = static_cast<VkDevice>(inDevice);

        returnVal.vkBeginCommandBuffer = PFN_vkBeginCommandBuffer(getDeviceProcAddr((VkDevice)device, "vkBeginCommandBuffer"));
        returnVal.vkCmdBeginQuery = PFN_vkCmdBeginQuery(getDeviceProcAddr((VkDevice)device, "vkCmdBeginQuery"));
        
        returnVal.vkCmdBeginRenderPass = PFN_vkCmdBeginRenderPass(getDeviceProcAddr(device, "vkCmdBeginRenderPass"));
        returnVal.vkCmdBindDescriptorSets = PFN_vkCmdBindDescriptorSets(getDeviceProcAddr(device, "vkCmdBindDescriptorSets"));
        returnVal.vkCmdBindIndexBuffer = PFN_vkCmdBindIndexBuffer(getDeviceProcAddr(device, "vkCmdBindIndexBuffer"));
        returnVal.vkCmdBindPipeline = PFN_vkCmdBindPipeline(getDeviceProcAddr(device, "vkCmdBindPipeline"));
        returnVal.vkCmdBindVertexBuffers = PFN_vkCmdBindVertexBuffers(getDeviceProcAddr(device, "vkCmdBindVertexBuffers"));
        returnVal.vkCmdBlitImage = PFN_vkCmdBlitImage(getDeviceProcAddr(device, "vkCmdBlitImage"));
        returnVal.vkCmdClearAttachments = PFN_vkCmdClearAttachments(getDeviceProcAddr(device, "vkCmdClearAttachments"));
        returnVal.vkCmdClearColorImage = PFN_vkCmdClearColorImage(getDeviceProcAddr(device, "vkCmdClearColorImage"));
        returnVal.vkCmdClearDepthStencilImage = PFN_vkCmdClearDepthStencilImage(getDeviceProcAddr(device, "vkCmdClearDepthStencilImage"));
        returnVal.vkCmdCopyBuffer = PFN_vkCmdCopyBuffer(getDeviceProcAddr(device, "vkCmdCopyBuffer"));
        returnVal.vkCmdCopyBufferToImage = PFN_vkCmdCopyBufferToImage(getDeviceProcAddr(device, "vkCmdCopyBufferToImage"));
        returnVal.vkCmdCopyImage = PFN_vkCmdCopyImage(getDeviceProcAddr(device, "vkCmdCopyImage"));
        returnVal.vkCmdCopyImageToBuffer = PFN_vkCmdCopyImageToBuffer(getDeviceProcAddr(device, "vkCmdCopyImageToBuffer"));
        returnVal.vkCmdCopyQueryPoolResults = PFN_vkCmdCopyQueryPoolResults(getDeviceProcAddr(device, "vkCmdCopyQueryPoolResults"));
        returnVal.vkCmdDispatch = PFN_vkCmdDispatch(getDeviceProcAddr(device, "vkCmdDispatch"));
        returnVal.vkCmdDispatchBase = PFN_vkCmdDispatchBase(getDeviceProcAddr(device, "vkCmdDispatchBase"));
        returnVal.vkCmdDispatchIndirect = PFN_vkCmdDispatchIndirect(getDeviceProcAddr(device, "vkCmdDispatchIndirect"));
        returnVal.vkCmdDraw = PFN_vkCmdDraw(getDeviceProcAddr(device, "vkCmdDraw"));
        returnVal.vkCmdDrawIndexed = PFN_vkCmdDrawIndexed(getDeviceProcAddr(device, "vkCmdDrawIndexed"));
        returnVal.vkCmdDrawIndexedIndirect = PFN_vkCmdDrawIndexedIndirect(getDeviceProcAddr(device, "vkCmdDrawIndexedIndirect"));
        returnVal.vkCmdDrawIndirect = PFN_vkCmdDrawIndirect(getDeviceProcAddr(device, "vkCmdDrawIndirect"));
        returnVal.vkCmdEndQuery = PFN_vkCmdEndQuery(getDeviceProcAddr(device, "vkCmdEndQuery"));
        returnVal.vkCmdEndRenderPass = PFN_vkCmdEndRenderPass(getDeviceProcAddr(device, "vkCmdEndRenderPass"));
        returnVal.vkCmdExecuteCommands = PFN_vkCmdExecuteCommands(getDeviceProcAddr(device, "vkCmdExecuteCommands"));
        returnVal.vkCmdFillBuffer = PFN_vkCmdFillBuffer(getDeviceProcAddr(device, "vkCmdFillBuffer"));
        returnVal.vkCmdNextSubpass = PFN_vkCmdNextSubpass(getDeviceProcAddr(device, "vkCmdNextSubpass"));
        returnVal.vkCmdPipelineBarrier = PFN_vkCmdPipelineBarrier(getDeviceProcAddr(device, "vkCmdPipelineBarrier"));
        returnVal.vkCmdPushConstants = PFN_vkCmdPushConstants(getDeviceProcAddr(device, "vkCmdPushConstants"));
        returnVal.vkCmdResetEvent = PFN_vkCmdResetEvent(getDeviceProcAddr(device, "vkCmdResetEvent"));
        returnVal.vkCmdResetQueryPool = PFN_vkCmdResetQueryPool(getDeviceProcAddr(device, "vkCmdResetQueryPool"));
        returnVal.vkCmdResolveImage = PFN_vkCmdResolveImage(getDeviceProcAddr(device, "vkCmdResolveImage"));
        returnVal.vkCmdSetBlendConstants = PFN_vkCmdSetBlendConstants(getDeviceProcAddr(device, "vkCmdSetBlendConstants"));
        returnVal.vkCmdSetDepthBias = PFN_vkCmdSetDepthBias(getDeviceProcAddr(device, "vkCmdSetDepthBias"));
        returnVal.vkCmdSetDepthBounds = PFN_vkCmdSetDepthBounds(getDeviceProcAddr(device, "vkCmdSetDepthBounds"));
        returnVal.vkCmdSetDeviceMask = PFN_vkCmdSetDeviceMask(getDeviceProcAddr(device, "vkCmdSetDeviceMask"));
        returnVal.vkCmdSetEvent = PFN_vkCmdSetEvent(getDeviceProcAddr(device, "vkCmdSetEvent"));
        returnVal.vkCmdSetLineWidth = PFN_vkCmdSetLineWidth(getDeviceProcAddr(device, "vkCmdSetLineWidth"));
        returnVal.vkCmdSetScissor = PFN_vkCmdSetScissor(getDeviceProcAddr(device, "vkCmdSetScissor"));
        returnVal.vkCmdSetStencilCompareMask = PFN_vkCmdSetStencilCompareMask(getDeviceProcAddr(device, "vkCmdSetStencilCompareMask"));
        returnVal.vkCmdSetStencilReference = PFN_vkCmdSetStencilReference(getDeviceProcAddr(device, "vkCmdSetStencilReference"));
        returnVal.vkCmdSetStencilWriteMask = PFN_vkCmdSetStencilWriteMask(getDeviceProcAddr(device, "vkCmdSetStencilWriteMask"));
        returnVal.vkCmdSetViewport = PFN_vkCmdSetViewport(getDeviceProcAddr(device, "vkCmdSetViewport"));
        returnVal.vkCmdUpdateBuffer = PFN_vkCmdUpdateBuffer(getDeviceProcAddr(device, "vkCmdUpdateBuffer"));
        returnVal.vkCmdWaitEvents = PFN_vkCmdWaitEvents(getDeviceProcAddr(device, "vkCmdWaitEvents"));
        returnVal.vkCmdWriteTimestamp = PFN_vkCmdWriteTimestamp(getDeviceProcAddr(device, "vkCmdWriteTimestamp"));
        returnVal.vkEndCommandBuffer = PFN_vkEndCommandBuffer(getDeviceProcAddr(device, "vkEndCommandBuffer"));
        returnVal.vkResetCommandBuffer = PFN_vkResetCommandBuffer(getDeviceProcAddr(device, "vkResetCommandBuffer"));
        returnVal.vkAllocateCommandBuffers = PFN_vkAllocateCommandBuffers(getDeviceProcAddr(device, "vkAllocateCommandBuffers"));
        returnVal.vkAllocateDescriptorSets = PFN_vkAllocateDescriptorSets(getDeviceProcAddr(device, "vkAllocateDescriptorSets"));
        returnVal.vkAllocateMemory = PFN_vkAllocateMemory(getDeviceProcAddr(device, "vkAllocateMemory"));
        returnVal.vkBindBufferMemory = PFN_vkBindBufferMemory(getDeviceProcAddr(device, "vkBindBufferMemory"));
        returnVal.vkBindBufferMemory2 = PFN_vkBindBufferMemory2(getDeviceProcAddr(device, "vkBindBufferMemory2"));
        returnVal.vkBindImageMemory = PFN_vkBindImageMemory(getDeviceProcAddr(device, "vkBindImageMemory"));
        returnVal.vkBindImageMemory2 = PFN_vkBindImageMemory2(getDeviceProcAddr(device, "vkBindImageMemory2"));
        returnVal.vkCreateBuffer = PFN_vkCreateBuffer(getDeviceProcAddr(device, "vkCreateBuffer"));
        returnVal.vkCreateBufferView = PFN_vkCreateBufferView(getDeviceProcAddr(device, "vkCreateBufferView"));
        returnVal.vkCreateCommandPool = PFN_vkCreateCommandPool(getDeviceProcAddr(device, "vkCreateCommandPool"));
        returnVal.vkCreateComputePipelines = PFN_vkCreateComputePipelines(getDeviceProcAddr(device, "vkCreateComputePipelines"));
        returnVal.vkCreateDescriptorPool = PFN_vkCreateDescriptorPool(getDeviceProcAddr(device, "vkCreateDescriptorPool"));
        returnVal.vkCreateDescriptorSetLayout = PFN_vkCreateDescriptorSetLayout(getDeviceProcAddr(device, "vkCreateDescriptorSetLayout"));
        returnVal.vkCreateDescriptorUpdateTemplate = PFN_vkCreateDescriptorUpdateTemplate(getDeviceProcAddr(device, "vkCreateDescriptorUpdateTemplate"));
        returnVal.vkCreateEvent = PFN_vkCreateEvent(getDeviceProcAddr(device, "vkCreateEvent"));
        returnVal.vkCreateFence = PFN_vkCreateFence(getDeviceProcAddr(device, "vkCreateFence"));
        returnVal.vkCreateFramebuffer = PFN_vkCreateFramebuffer(getDeviceProcAddr(device, "vkCreateFramebuffer"));
        returnVal.vkCreateGraphicsPipelines = PFN_vkCreateGraphicsPipelines(getDeviceProcAddr(device, "vkCreateGraphicsPipelines"));
        returnVal.vkCreateImage = PFN_vkCreateImage(getDeviceProcAddr(device, "vkCreateImage"));
        returnVal.vkCreateImageView = PFN_vkCreateImageView(getDeviceProcAddr(device, "vkCreateImageView"));
        returnVal.vkCreatePipelineCache = PFN_vkCreatePipelineCache(getDeviceProcAddr(device, "vkCreatePipelineCache"));
        returnVal.vkCreatePipelineLayout = PFN_vkCreatePipelineLayout(getDeviceProcAddr(device, "vkCreatePipelineLayout"));
        returnVal.vkCreateQueryPool = PFN_vkCreateQueryPool(getDeviceProcAddr(device, "vkCreateQueryPool"));
        returnVal.vkCreateRenderPass = PFN_vkCreateRenderPass(getDeviceProcAddr(device, "vkCreateRenderPass"));
        returnVal.vkCreateSampler = PFN_vkCreateSampler(getDeviceProcAddr(device, "vkCreateSampler"));
        returnVal.vkCreateSamplerYcbcrConversion = PFN_vkCreateSamplerYcbcrConversion(getDeviceProcAddr(device, "vkCreateSamplerYcbcrConversion"));
        returnVal.vkCreateSemaphore = PFN_vkCreateSemaphore(getDeviceProcAddr(device, "vkCreateSemaphore"));
        returnVal.vkCreateShaderModule = PFN_vkCreateShaderModule(getDeviceProcAddr(device, "vkCreateShaderModule"));
        returnVal.vkDestroyBuffer = PFN_vkDestroyBuffer(getDeviceProcAddr(device, "vkDestroyBuffer"));
        returnVal.vkDestroyBufferView = PFN_vkDestroyBufferView(getDeviceProcAddr(device, "vkDestroyBufferView"));
        returnVal.vkDestroyCommandPool = PFN_vkDestroyCommandPool(getDeviceProcAddr(device, "vkDestroyCommandPool"));
        returnVal.vkDestroyDescriptorPool = PFN_vkDestroyDescriptorPool(getDeviceProcAddr(device, "vkDestroyDescriptorPool"));
        returnVal.vkDestroyDescriptorSetLayout = PFN_vkDestroyDescriptorSetLayout(getDeviceProcAddr(device, "vkDestroyDescriptorSetLayout"));
        returnVal.vkDestroyDescriptorUpdateTemplate = PFN_vkDestroyDescriptorUpdateTemplate(getDeviceProcAddr(device, "vkDestroyDescriptorUpdateTemplate"));
        returnVal.vkDestroyDevice = PFN_vkDestroyDevice(getDeviceProcAddr(device, "vkDestroyDevice"));
        returnVal.vkDestroyEvent = PFN_vkDestroyEvent(getDeviceProcAddr(device, "vkDestroyEvent"));
        returnVal.vkDestroyFence = PFN_vkDestroyFence(getDeviceProcAddr(device, "vkDestroyFence"));
        returnVal.vkDestroyFramebuffer = PFN_vkDestroyFramebuffer(getDeviceProcAddr(device, "vkDestroyFramebuffer"));
        returnVal.vkDestroyImage = PFN_vkDestroyImage(getDeviceProcAddr(device, "vkDestroyImage"));
        returnVal.vkDestroyImageView = PFN_vkDestroyImageView(getDeviceProcAddr(device, "vkDestroyImageView"));
        returnVal.vkDestroyPipeline = PFN_vkDestroyPipeline(getDeviceProcAddr(device, "vkDestroyPipeline"));
        returnVal.vkDestroyPipelineCache = PFN_vkDestroyPipelineCache(getDeviceProcAddr(device, "vkDestroyPipelineCache"));
        returnVal.vkDestroyPipelineLayout = PFN_vkDestroyPipelineLayout(getDeviceProcAddr(device, "vkDestroyPipelineLayout"));
        returnVal.vkDestroyQueryPool = PFN_vkDestroyQueryPool(getDeviceProcAddr(device, "vkDestroyQueryPool"));
        returnVal.vkDestroyRenderPass = PFN_vkDestroyRenderPass(getDeviceProcAddr(device, "vkDestroyRenderPass"));
        returnVal.vkDestroySampler = PFN_vkDestroySampler(getDeviceProcAddr(device, "vkDestroySampler"));
        returnVal.vkDestroySamplerYcbcrConversion = PFN_vkDestroySamplerYcbcrConversion(getDeviceProcAddr(device, "vkDestroySamplerYcbcrConversion"));
        returnVal.vkDestroySemaphore = PFN_vkDestroySemaphore(getDeviceProcAddr(device, "vkDestroySemaphore"));
        returnVal.vkDestroyShaderModule = PFN_vkDestroyShaderModule(getDeviceProcAddr(device, "vkDestroyShaderModule"));
        returnVal.vkDeviceWaitIdle = PFN_vkDeviceWaitIdle(getDeviceProcAddr(device, "vkDeviceWaitIdle"));
        returnVal.vkFlushMappedMemoryRanges = PFN_vkFlushMappedMemoryRanges(getDeviceProcAddr(device, "vkFlushMappedMemoryRanges"));
        returnVal.vkFreeCommandBuffers = PFN_vkFreeCommandBuffers(getDeviceProcAddr(device, "vkFreeCommandBuffers"));
        returnVal.vkFreeDescriptorSets = PFN_vkFreeDescriptorSets(getDeviceProcAddr(device, "vkFreeDescriptorSets"));
        returnVal.vkFreeMemory = PFN_vkFreeMemory(getDeviceProcAddr(device, "vkFreeMemory"));
        returnVal.vkGetBufferMemoryRequirements = PFN_vkGetBufferMemoryRequirements(getDeviceProcAddr(device, "vkGetBufferMemoryRequirements"));
        returnVal.vkGetBufferMemoryRequirements2 = PFN_vkGetBufferMemoryRequirements2(getDeviceProcAddr(device, "vkGetBufferMemoryRequirements2"));
        returnVal.vkGetDescriptorSetLayoutSupport = PFN_vkGetDescriptorSetLayoutSupport(getDeviceProcAddr(device, "vkGetDescriptorSetLayoutSupport"));
        returnVal.vkGetDeviceGroupPeerMemoryFeatures = PFN_vkGetDeviceGroupPeerMemoryFeatures(getDeviceProcAddr(device, "vkGetDeviceGroupPeerMemoryFeatures"));
        returnVal.vkGetDeviceMemoryCommitment = PFN_vkGetDeviceMemoryCommitment(getDeviceProcAddr(device, "vkGetDeviceMemoryCommitment"));
        returnVal.vkGetDeviceProcAddr = PFN_vkGetDeviceProcAddr(getDeviceProcAddr(device, "getDeviceProcAddr"));
        returnVal.vkGetDeviceQueue = PFN_vkGetDeviceQueue(getDeviceProcAddr(device, "vkGetDeviceQueue"));
        returnVal.vkGetDeviceQueue2 = PFN_vkGetDeviceQueue2(getDeviceProcAddr(device, "vkGetDeviceQueue2"));
        returnVal.vkGetEventStatus = PFN_vkGetEventStatus(getDeviceProcAddr(device, "vkGetEventStatus"));
        returnVal.vkGetFenceStatus = PFN_vkGetFenceStatus(getDeviceProcAddr(device, "vkGetFenceStatus"));
        returnVal.vkGetImageMemoryRequirements = PFN_vkGetImageMemoryRequirements(getDeviceProcAddr(device, "vkGetImageMemoryRequirements"));
        returnVal.vkGetImageMemoryRequirements2 = PFN_vkGetImageMemoryRequirements2(getDeviceProcAddr(device, "vkGetImageMemoryRequirements2"));
        returnVal.vkGetImageSparseMemoryRequirements = PFN_vkGetImageSparseMemoryRequirements(getDeviceProcAddr(device, "vkGetImageSparseMemoryRequirements"));
        returnVal.vkGetImageSparseMemoryRequirements2 = PFN_vkGetImageSparseMemoryRequirements2(getDeviceProcAddr(device, "vkGetImageSparseMemoryRequirements2"));
        returnVal.vkGetImageSubresourceLayout = PFN_vkGetImageSubresourceLayout(getDeviceProcAddr(device, "vkGetImageSubresourceLayout"));
        returnVal.vkGetPipelineCacheData = PFN_vkGetPipelineCacheData(getDeviceProcAddr(device, "vkGetPipelineCacheData"));
        returnVal.vkGetQueryPoolResults = PFN_vkGetQueryPoolResults(getDeviceProcAddr(device, "vkGetQueryPoolResults"));
        returnVal.vkGetRenderAreaGranularity = PFN_vkGetRenderAreaGranularity(getDeviceProcAddr(device, "vkGetRenderAreaGranularity"));
        returnVal.vkInvalidateMappedMemoryRanges = PFN_vkInvalidateMappedMemoryRanges(getDeviceProcAddr(device, "vkInvalidateMappedMemoryRanges"));
        returnVal.vkMapMemory = PFN_vkMapMemory(getDeviceProcAddr(device, "vkMapMemory"));
        returnVal.vkMergePipelineCaches = PFN_vkMergePipelineCaches(getDeviceProcAddr(device, "vkMergePipelineCaches"));
        returnVal.vkResetCommandPool = PFN_vkResetCommandPool(getDeviceProcAddr(device, "vkResetCommandPool"));
        returnVal.vkResetDescriptorPool = PFN_vkResetDescriptorPool(getDeviceProcAddr(device, "vkResetDescriptorPool"));
        returnVal.vkResetEvent = PFN_vkResetEvent(getDeviceProcAddr(device, "vkResetEvent"));
        returnVal.vkResetFences = PFN_vkResetFences(getDeviceProcAddr(device, "vkResetFences"));
        returnVal.vkSetEvent = PFN_vkSetEvent(getDeviceProcAddr(device, "vkSetEvent"));
        returnVal.vkTrimCommandPool = PFN_vkTrimCommandPool(getDeviceProcAddr(device, "vkTrimCommandPool"));
        returnVal.vkUnmapMemory = PFN_vkUnmapMemory(getDeviceProcAddr(device, "vkUnmapMemory"));
        returnVal.vkUpdateDescriptorSetWithTemplate = PFN_vkUpdateDescriptorSetWithTemplate(getDeviceProcAddr(device, "vkUpdateDescriptorSetWithTemplate"));
        returnVal.vkUpdateDescriptorSets = PFN_vkUpdateDescriptorSets(getDeviceProcAddr(device, "vkUpdateDescriptorSets"));
        returnVal.vkWaitForFences = PFN_vkWaitForFences(getDeviceProcAddr(device, "vkWaitForFences"));
        returnVal.vkQueueBindSparse = PFN_vkQueueBindSparse(getDeviceProcAddr(device, "vkQueueBindSparse"));
        returnVal.vkQueuePresentKHR = PFN_vkQueuePresentKHR(getDeviceProcAddr(device, "vkQueuePresentKHR"));
        returnVal.vkQueueSubmit = PFN_vkQueueSubmit(getDeviceProcAddr(device, "vkQueueSubmit"));
        returnVal.vkQueueWaitIdle = PFN_vkQueueWaitIdle(getDeviceProcAddr(device, "vkQueueWaitIdle"));

		return returnVal;
	}

    KHR_SurfaceDispatchRaw KHR_SurfaceDispatchRaw::Build(vk::Instance inInstance, PFN_vkGetInstanceProcAddr instanceProcAddr)
    {
        KHR_SurfaceDispatchRaw returnVal{};

        VkInstance instance = static_cast<VkInstance>(inInstance);

        returnVal.vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)instanceProcAddr(instance, "vkDestroySurfaceKHR");
        returnVal.vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)instanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
        returnVal.vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)instanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
        returnVal.vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)instanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
        returnVal.vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)instanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");

        return returnVal;
    }

	KHR_SwapchainDispatchRaw KHR_SwapchainDispatchRaw::Build(vk::Device device, PFN_vkGetDeviceProcAddr getProcAddr)
	{
        VkDevice vkDevice = static_cast<VkDevice>(device);

		KHR_SwapchainDispatchRaw returnVal{};
		returnVal.vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)getProcAddr(vkDevice, "vkCreateSwapchainKHR");
		returnVal.vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)getProcAddr(vkDevice, "vkDestroySwapchainKHR");
		returnVal.vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)getProcAddr(vkDevice, "vkGetSwapchainImagesKHR");
		returnVal.vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)getProcAddr(vkDevice, "vkAcquireNextImageKHR");
		returnVal.vkQueuePresentKHR = (PFN_vkQueuePresentKHR)getProcAddr(vkDevice, "vkQueuePresentKHR");
		returnVal.vkGetDeviceGroupPresentCapabilitiesKHR = (PFN_vkGetDeviceGroupPresentCapabilitiesKHR)getProcAddr(vkDevice, "vkGetDeviceGroupPresentCapabilitiesKHR");
		returnVal.vkGetDeviceGroupSurfacePresentModesKHR = (PFN_vkGetDeviceGroupSurfacePresentModesKHR)getProcAddr(vkDevice, "vkGetDeviceGroupSurfacePresentModesKHR");
		returnVal.vkGetPhysicalDevicePresentRectanglesKHR = (PFN_vkGetPhysicalDevicePresentRectanglesKHR)getProcAddr(vkDevice, "vkGetPhysicalDevicePresentRectanglesKHR");
		returnVal.vkAcquireNextImage2KHR = (PFN_vkAcquireNextImage2KHR)getProcAddr(vkDevice, "vkAcquireNextImage2KHR");

		if (returnVal.vkCreateSwapchainKHR == nullptr)
			throw std::runtime_error("Vulkan: Unable to load VK_KHR_swapchain function pointers.");

		return returnVal;
	}

	EXT_DebugUtilsDispatchRaw EXT_DebugUtilsDispatchRaw::Build(vk::Instance inInstance, PFN_vkGetInstanceProcAddr instanceProcAddr)
	{
        VkInstance instance = static_cast<VkInstance>(inInstance);

		EXT_DebugUtilsDispatchRaw returnVal{};
		returnVal.vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)instanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
		returnVal.vkSetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)instanceProcAddr(instance, "vkSetDebugUtilsObjectTagEXT");
		returnVal.vkQueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT)instanceProcAddr(instance, "vkQueueBeginDebugUtilsLabelEXT");
		returnVal.vkQueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT)instanceProcAddr(instance, "vkQueueEndDebugUtilsLabelEXT");
		returnVal.vkQueueInsertDebugUtilsLabelEXT = (PFN_vkQueueInsertDebugUtilsLabelEXT)instanceProcAddr(instance, "vkQueueInsertDebugUtilsLabelEXT");
		returnVal.vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)instanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
		returnVal.vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)instanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
		returnVal.vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)instanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT");
		returnVal.vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)instanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		returnVal.vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)instanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		returnVal.vkSubmitDebugUtilsMessageEXT = (PFN_vkSubmitDebugUtilsMessageEXT)instanceProcAddr(instance, "vkSubmitDebugUtilsMessageEXT");
		if (returnVal.vkSetDebugUtilsObjectNameEXT == nullptr ||
			returnVal.vkQueueBeginDebugUtilsLabelEXT == nullptr ||
			returnVal.vkCreateDebugUtilsMessengerEXT == nullptr)
		{
			throw std::runtime_error("Vulkan: Unable to load Debug Utils function pointers.");
		}

		return returnVal;
	}

    BaseDispatch BaseDispatch::Build(PFN_vkGetInstanceProcAddr procAddr)
    {
        BaseDispatch returnVal{};

        returnVal.raw = BaseDispatchRaw::Build(procAddr);

        return returnVal;
    }

    InstanceDispatchRaw InstanceDispatchRaw::Build(vk::Instance inInstance, PFN_vkGetInstanceProcAddr getInstanceProcAddr)
    {
        VkInstance instance = static_cast<VkInstance>(inInstance);

        InstanceDispatchRaw returnVal{};

        returnVal.vkDestroyInstance = (PFN_vkDestroyInstance)getInstanceProcAddr(instance, "vkDestroyInstance");
        returnVal.vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)getInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
        returnVal.vkCreateDevice = (PFN_vkCreateDevice)getInstanceProcAddr(instance, "vkCreateDevice");
        returnVal.vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)getInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties");
        returnVal.vkEnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties)getInstanceProcAddr(instance, "vkEnumerateDeviceLayerProperties");
        returnVal.vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)getInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures");
        returnVal.vkGetPhysicalDeviceFeatures2 = (PFN_vkGetPhysicalDeviceFeatures2)getInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2");
        returnVal.vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)getInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties");
        returnVal.vkGetPhysicalDeviceMemoryProperties2 = (PFN_vkGetPhysicalDeviceMemoryProperties2)getInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties2");
        returnVal.vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)getInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties");
        returnVal.vkGetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2)getInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2");
        returnVal.vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)getInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties");
        returnVal.vkGetPhysicalDeviceQueueFamilyProperties2 = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2)getInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties2");

        return returnVal;
    }

    InstanceDispatch InstanceDispatch::Build(vk::Instance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr)
    {
        InstanceDispatch returnVal{};

        returnVal.handle = instance;
        
        returnVal.raw = InstanceDispatchRaw::Build(instance, getInstanceProcAddr);
        returnVal.surface_raw = KHR_SurfaceDispatchRaw::Build(instance, getInstanceProcAddr);

        return returnVal;
    }

	DebugUtilsDispatch DebugUtilsDispatch::Build(vk::Instance instance, PFN_vkGetInstanceProcAddr instanceProcAddr)
	{
		DebugUtilsDispatch returnVal{};
		returnVal.raw = EXT_DebugUtilsDispatchRaw::Build(instance, instanceProcAddr);
		return returnVal;
	}

	DeviceDispatch DeviceDispatch::Build(vk::Device vkDevice, PFN_vkGetDeviceProcAddr getProcAddr)
	{
		DeviceDispatch returnVal{};
		returnVal.handle = vkDevice;

		returnVal.raw = DeviceDispatchRaw::Build(vkDevice, getProcAddr);
		returnVal.swapchain_raw = KHR_SwapchainDispatchRaw::Build(vkDevice, getProcAddr);

		return returnVal;
	}

	void DeviceDispatch::copy(const DeviceDispatch& in)
	{
		if (this == &in)
			return;
		*this = in;
	}

    PFN_vkGetInstanceProcAddr loadInstanceProcAddressPFN()
    {
        PFN_vkGetInstanceProcAddr procAddr = nullptr;

#if defined(__linux__)
        void* m_library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
        if (m_library == nullptr)
            throw std::runtime_error("Vulkan: Unable to load the system libvulkan.so for dynamic dispatching");
        procAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(m_library, "vkGetInstanceProcAddr"));
        dlclose(m_library);
#elif defined(_WIN32)
        HMODULE m_library = LoadLibrary(TEXT("vulkan-1.dll"));
        if (m_library == nullptr)
            throw std::runtime_error("Vulkan: Unable to load the system vulkan-1.dll for dynamic dispatching");
        procAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(m_library, "vkGetInstanceProcAddr"));
        FreeLibrary(m_library);
#else
#error Unsupported platform
#endif

        if (procAddr == nullptr)
            throw std::runtime_error("Vulkan: Unable to load the vkGetInstanceProcAddr function pointer.");

        return procAddr;
    }
}

void DEngine::Gfx::Vk::InstanceDispatch::Destroy(
    vk::SurfaceKHR in, 
    vk::Optional<vk::AllocationCallbacks> allocator) const
{
    surface_raw.vkDestroySurfaceKHR(
        static_cast<VkInstance>(handle),
        static_cast<VkSurfaceKHR>(in),
        reinterpret_cast<VkAllocationCallbacks const*>(static_cast<vk::AllocationCallbacks const*>(allocator)));
}

vk::ResultValue<vk::Semaphore> DEngine::Gfx::Vk::DeviceDispatch::createSemaphore(
    vk::SemaphoreCreateInfo const& createInfo,
    vk::Optional<vk::AllocationCallbacks const> allocator) const
{
    vk::Semaphore temp{};
    vk::Result result = static_cast<vk::Result>(raw.vkCreateSemaphore(
        static_cast<VkDevice>(handle),
        reinterpret_cast<VkSemaphoreCreateInfo const*>(&createInfo),
        reinterpret_cast<VkAllocationCallbacks const*>(static_cast<vk::AllocationCallbacks const*>(allocator)),
        reinterpret_cast<VkSemaphore*>(&temp)));
    return { result, temp };
}

vk::Result DEngine::Gfx::Vk::DeviceDispatch::createGraphicsPipelines(
    vk::PipelineCache pipelineCache,
    vk::ArrayProxy<vk::GraphicsPipelineCreateInfo const> createInfos,
    vk::Optional<vk::AllocationCallbacks const> allocator,
    vk::Pipeline* pPipelines) const
{
    return static_cast<vk::Result>(raw.vkCreateGraphicsPipelines(
        static_cast<VkDevice>(handle),
        static_cast<VkPipelineCache>(pipelineCache),
        createInfos.size(),
        reinterpret_cast<VkGraphicsPipelineCreateInfo const*>(createInfos.data()),
        reinterpret_cast<VkAllocationCallbacks const*>(static_cast<vk::AllocationCallbacks const*>(allocator)),
        reinterpret_cast<VkPipeline*>(pPipelines)));
}

#define DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(typeName) \
    raw.vkDestroy ##typeName( \
        static_cast<VkDevice>(handle), \
        static_cast<Vk##typeName>(in), \
        reinterpret_cast<VkAllocationCallbacks const*>(static_cast<vk::AllocationCallbacks const*>(allocator))); \
        

void DEngine::Gfx::Vk::DeviceDispatch::Destroy(
    vk::CommandPool in,
    vk::Optional<vk::AllocationCallbacks> allocator) const
{
    DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(CommandPool)
}

void DEngine::Gfx::Vk::DeviceDispatch::Destroy(vk::DescriptorPool in, vk::Optional<vk::AllocationCallbacks> allocator) const
{
    DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(DescriptorPool)
}

void DEngine::Gfx::Vk::DeviceDispatch::Destroy(
    vk::Fence in, 
    vk::Optional<vk::AllocationCallbacks> allocator) const
{
    DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(Fence)
}

void DEngine::Gfx::Vk::DeviceDispatch::Destroy(
    vk::Framebuffer in, 
    vk::Optional<vk::AllocationCallbacks> allocator) const
{
    DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(Framebuffer)
}

void DEngine::Gfx::Vk::DeviceDispatch::Destroy(
    vk::Image in, 
    vk::Optional<vk::AllocationCallbacks> allocator) const
{
    DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(Image)
}

void DEngine::Gfx::Vk::DeviceDispatch::Destroy(
    vk::ImageView in, 
    vk::Optional<vk::AllocationCallbacks> allocator) const
{
    DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(ImageView)
}

void DEngine::Gfx::Vk::DeviceDispatch::Destroy(
    vk::RenderPass in, 
    vk::Optional<vk::AllocationCallbacks> allocator) const
{
    DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(RenderPass)
}

void DEngine::Gfx::Vk::DeviceDispatch::Destroy(
    vk::ShaderModule in, 
    vk::Optional<vk::AllocationCallbacks> allocator) const
{
    DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(ShaderModule)
}

void DEngine::Gfx::Vk::DeviceDispatch::Destroy(
    vk::SwapchainKHR in, 
    vk::Optional<vk::AllocationCallbacks> allocator) const
{
    swapchain_raw.vkDestroySwapchainKHR(
        static_cast<VkDevice>(handle),
        static_cast<VkSwapchainKHR>(in),
        reinterpret_cast<VkAllocationCallbacks const*>(static_cast<vk::AllocationCallbacks const*>(allocator)));
}

void DEngine::Gfx::Vk::DeviceDispatch::FreeCommandBuffers(
    vk::CommandPool commandPool, 
    vk::ArrayProxy<vk::CommandBuffer const> commandBuffers) const
{
    raw.vkFreeCommandBuffers(
        static_cast<VkDevice>(handle),
        static_cast<VkCommandPool>(commandPool),
        commandBuffers.size(),
        reinterpret_cast<VkCommandBuffer const*>(commandBuffers.data()));
}

void DEngine::Gfx::Vk::DeviceDispatch::FreeMemory(
    vk::DeviceMemory memory, 
    vk::Optional<vk::AllocationCallbacks const> allocator) const
{
    raw.vkFreeMemory(
        static_cast<VkDevice>(handle),
        static_cast<VkDeviceMemory>(memory),
        reinterpret_cast<VkAllocationCallbacks const*>(static_cast<vk::AllocationCallbacks const*>(allocator)));
}

vk::Result DEngine::Gfx::Vk::DeviceDispatch::GetFenceStatus(vk::Fence fence) const
{
    return static_cast<vk::Result>(raw.vkGetFenceStatus(
        static_cast<VkDevice>(handle),
        static_cast<VkFence>(fence)));
}

#include "QueueData.hpp"
void DEngine::Gfx::Vk::DeviceDispatch::WaitIdle() const
{
    // Go through all queues and lock their mutexes.
    std::lock_guard graphicsLock{ m_queueDataPtr->graphics.m_lock };
    std::lock_guard transferLock{ m_queueDataPtr->transfer.m_lock };
    std::lock_guard computeLock{ m_queueDataPtr->compute.m_lock };

    raw.vkDeviceWaitIdle(static_cast<VkDevice>(handle));
}
