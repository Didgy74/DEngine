#pragma once

#include "VulkanIncluder.hpp"

namespace DEngine::Gfx::Vk
{
	PFN_vkGetInstanceProcAddr loadInstanceProcAddressPFN();

	struct BaseDispatchRaw {
		[[nodiscard]] static BaseDispatchRaw Build(PFN_vkGetInstanceProcAddr getInstanceProcAddr);

		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
		PFN_vkCreateInstance vkCreateInstance;
		PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
		PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
		PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion;
	};

	struct InstanceDispatchRaw {
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

	struct DeviceDispatchRaw {
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
		PFN_vkCmdSetStencilReference vkCmdSetStencilReference;
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

		template<class T>
		auto GetDestroyFn() const = delete;
	};

	template<>
	[[nodiscard]] inline auto DeviceDispatchRaw::GetDestroyFn<vk::CommandPool>() const { return vkDestroyCommandPool; }
	template<>
	[[nodiscard]] inline auto DeviceDispatchRaw::GetDestroyFn<vk::DescriptorSetLayout>() const { return vkDestroyDescriptorSetLayout; }
	template<>
	[[nodiscard]] inline auto DeviceDispatchRaw::GetDestroyFn<vk::DescriptorPool>() const { return vkDestroyDescriptorPool; }
	template<>
	[[nodiscard]] inline auto DeviceDispatchRaw::GetDestroyFn<vk::Fence>() const { return vkDestroyFence; }
	template<>
	[[nodiscard]] inline auto DeviceDispatchRaw::GetDestroyFn<vk::Framebuffer>() const { return vkDestroyFramebuffer; }
	template<>
	[[nodiscard]] inline auto DeviceDispatchRaw::GetDestroyFn<vk::ImageView>() const { return vkDestroyImageView; }
	template<>
	[[nodiscard]] inline auto DeviceDispatchRaw::GetDestroyFn<vk::Pipeline>() const { return vkDestroyPipeline; }
	template<>
	[[nodiscard]] inline auto DeviceDispatchRaw::GetDestroyFn<vk::PipelineLayout>() const { return vkDestroyPipelineLayout; }
	template<>
	[[nodiscard]] inline auto DeviceDispatchRaw::GetDestroyFn<vk::Sampler>() const { return vkDestroySampler; }
	template<>
	[[nodiscard]] inline auto DeviceDispatchRaw::GetDestroyFn<vk::Semaphore>() const { return vkDestroySemaphore; }
	template<>
	[[nodiscard]] inline auto DeviceDispatchRaw::GetDestroyFn<vk::ShaderModule>() const { return vkDestroyShaderModule; }

	struct EXT_DebugUtilsDispatchRaw {
		[[nodiscard]] static EXT_DebugUtilsDispatchRaw Build(
			vk::Instance instance, 
			PFN_vkGetInstanceProcAddr instanceProcAddr);

		PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
	};

	struct KHR_SurfaceDispatchRaw {
		[[nodiscard]] static KHR_SurfaceDispatchRaw Build(
			vk::Instance instance, 
			PFN_vkGetInstanceProcAddr instanceProcAddr);

		PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
		PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
		PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
		PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
		PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
	};

	struct KHR_SwapchainDispatchRaw {
		[[nodiscard]] static KHR_SwapchainDispatchRaw Build(
			vk::Device vkDevice, 
			PFN_vkGetDeviceProcAddr getProcAddr);

		PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
		PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
		PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
		PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
		PFN_vkQueuePresentKHR vkQueuePresentKHR;
	};
}