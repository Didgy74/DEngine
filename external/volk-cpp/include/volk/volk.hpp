/**
 * volk
 *
 * Copyright (C) 2018-2019, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
 * Report bugs and download new versions at https://github.com/zeux/volk
 *
 * This library is distributed under the MIT License. See notice at the end of this file.
 */
#ifndef VOLK_H_
#define VOLK_H_

#if defined(VULKAN_H_) && !defined(VK_NO_PROTOTYPES)
#	error To use volk, you need to define VK_NO_PROTOTYPES before including vulkan.h
#endif

/* VOLK_GENERATE_VERSION */
#define VOLK_HEADER_VERSION 107
/* VOLK_GENERATE_VERSION */

#ifndef VK_NO_PROTOTYPES
#	define VK_NO_PROTOTYPES
#endif

#ifndef VULKAN_H_
#	include <vulkan/vulkan.h>
#endif

namespace Volk
{
	struct DeviceTable;

	/*
	* Initialize library by loading Vulkan loader; call this function before creating the Vulkan instance.
	*
	* Returns VK_SUCCESS on success and VK_ERROR_INITIALIZATION_FAILED otherwise.
	*/
	VkResult Initialize();

	/*
	* Initialize library by providing a custom handler to load global symbols.
	*
	* This function can be used instead of volkInitialize.
	* The handler function pointer will be asked to load global Vulkan symbols which require no instance
	* (such as vkCreateInstance, vkEnumerateInstance* and vkEnumerateInstanceVersion if available).
	*/
	void InitializeCustom(PFN_vkGetInstanceProcAddr handler);

	/*
	* Get Vulkan instance version supported by the Vulkan loader, or 0 if Vulkan isn't supported
	*
	* Returns 0 if volkInitialize wasn't called or failed.
	*/
	uint32_t GetInstanceVersion();

	/*
	* Load global function pointers using application-created VkInstance; call this function after creating the Vulkan instance.
	*/
	void LoadInstance(VkInstance instance);

	/**
	* Load global function pointers using application-created VkDevice; call this function after creating the Vulkan device.
	*
	* Note: this is not suitable for applications that want to use multiple VkDevice objects concurrently.
	*/
	void LoadDevice(VkDevice device);

	/**
	* Load function pointers using application-created VkDevice into a table.
	* Application should use function pointers from that table instead of using global function pointers.
	*/
	void LoadDeviceTable(struct VolkDeviceTable* table, VkDevice device);
}

/**
 * Device-specific function pointer table
 */
struct Volk::DeviceTable
{
	/* VOLK_GENERATE_DEVICE_TABLE */
#if defined(VK_VERSION_1_0)
	PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
	PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
	PFN_vkAllocateMemory vkAllocateMemory;
	PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
	PFN_vkBindBufferMemory vkBindBufferMemory;
	PFN_vkBindImageMemory vkBindImageMemory;
	PFN_vkCmdBeginQuery vkCmdBeginQuery;
	PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
	PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
	PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
	PFN_vkCmdBindPipeline vkCmdBindPipeline;
	PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
	PFN_vkCmdBlitImage vkCmdBlitImage;
	PFN_vkCmdClearAttachments vkCmdClearAttachments;
	PFN_vkCmdClearColorImage vkCmdClearColorImage;
	PFN_vkCmdClearDepthStencilImage vkCmdClearDepthStencilImage;
	PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
	PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
	PFN_vkCmdCopyImage vkCmdCopyImage;
	PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer;
	PFN_vkCmdCopyQueryPoolResults vkCmdCopyQueryPoolResults;
	PFN_vkCmdDispatch vkCmdDispatch;
	PFN_vkCmdDispatchIndirect vkCmdDispatchIndirect;
	PFN_vkCmdDraw vkCmdDraw;
	PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
	PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect;
	PFN_vkCmdDrawIndirect vkCmdDrawIndirect;
	PFN_vkCmdEndQuery vkCmdEndQuery;
	PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
	PFN_vkCmdExecuteCommands vkCmdExecuteCommands;
	PFN_vkCmdFillBuffer vkCmdFillBuffer;
	PFN_vkCmdNextSubpass vkCmdNextSubpass;
	PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
	PFN_vkCmdPushConstants vkCmdPushConstants;
	PFN_vkCmdResetEvent vkCmdResetEvent;
	PFN_vkCmdResetQueryPool vkCmdResetQueryPool;
	PFN_vkCmdResolveImage vkCmdResolveImage;
	PFN_vkCmdSetBlendConstants vkCmdSetBlendConstants;
	PFN_vkCmdSetDepthBias vkCmdSetDepthBias;
	PFN_vkCmdSetDepthBounds vkCmdSetDepthBounds;
	PFN_vkCmdSetEvent vkCmdSetEvent;
	PFN_vkCmdSetLineWidth vkCmdSetLineWidth;
	PFN_vkCmdSetScissor vkCmdSetScissor;
	PFN_vkCmdSetStencilCompareMask vkCmdSetStencilCompareMask;
	PFN_vkCmdSetStencilReference vkCmdSetStencilReference;
	PFN_vkCmdSetStencilWriteMask vkCmdSetStencilWriteMask;
	PFN_vkCmdSetViewport vkCmdSetViewport;
	PFN_vkCmdUpdateBuffer vkCmdUpdateBuffer;
	PFN_vkCmdWaitEvents vkCmdWaitEvents;
	PFN_vkCmdWriteTimestamp vkCmdWriteTimestamp;
	PFN_vkCreateBuffer vkCreateBuffer;
	PFN_vkCreateBufferView vkCreateBufferView;
	PFN_vkCreateCommandPool vkCreateCommandPool;
	PFN_vkCreateComputePipelines vkCreateComputePipelines;
	PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
	PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
	PFN_vkCreateEvent vkCreateEvent;
	PFN_vkCreateFence vkCreateFence;
	PFN_vkCreateFramebuffer vkCreateFramebuffer;
	PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
	PFN_vkCreateImage vkCreateImage;
	PFN_vkCreateImageView vkCreateImageView;
	PFN_vkCreatePipelineCache vkCreatePipelineCache;
	PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
	PFN_vkCreateQueryPool vkCreateQueryPool;
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
	PFN_vkDestroyEvent vkDestroyEvent;
	PFN_vkDestroyFence vkDestroyFence;
	PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
	PFN_vkDestroyImage vkDestroyImage;
	PFN_vkDestroyImageView vkDestroyImageView;
	PFN_vkDestroyPipeline vkDestroyPipeline;
	PFN_vkDestroyPipelineCache vkDestroyPipelineCache;
	PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
	PFN_vkDestroyQueryPool vkDestroyQueryPool;
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
	PFN_vkGetDeviceMemoryCommitment vkGetDeviceMemoryCommitment;
	PFN_vkGetDeviceQueue vkGetDeviceQueue;
	PFN_vkGetEventStatus vkGetEventStatus;
	PFN_vkGetFenceStatus vkGetFenceStatus;
	PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
	PFN_vkGetImageSparseMemoryRequirements vkGetImageSparseMemoryRequirements;
	PFN_vkGetImageSubresourceLayout vkGetImageSubresourceLayout;
	PFN_vkGetPipelineCacheData vkGetPipelineCacheData;
	PFN_vkGetQueryPoolResults vkGetQueryPoolResults;
	PFN_vkGetRenderAreaGranularity vkGetRenderAreaGranularity;
	PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges;
	PFN_vkMapMemory vkMapMemory;
	PFN_vkMergePipelineCaches vkMergePipelineCaches;
	PFN_vkQueueBindSparse vkQueueBindSparse;
	PFN_vkQueueSubmit vkQueueSubmit;
	PFN_vkQueueWaitIdle vkQueueWaitIdle;
	PFN_vkResetCommandBuffer vkResetCommandBuffer;
	PFN_vkResetCommandPool vkResetCommandPool;
	PFN_vkResetDescriptorPool vkResetDescriptorPool;
	PFN_vkResetEvent vkResetEvent;
	PFN_vkResetFences vkResetFences;
	PFN_vkSetEvent vkSetEvent;
	PFN_vkUnmapMemory vkUnmapMemory;
	PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
	PFN_vkWaitForFences vkWaitForFences;
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
	PFN_vkBindBufferMemory2 vkBindBufferMemory2;
	PFN_vkBindImageMemory2 vkBindImageMemory2;
	PFN_vkCmdDispatchBase vkCmdDispatchBase;
	PFN_vkCmdSetDeviceMask vkCmdSetDeviceMask;
	PFN_vkCreateDescriptorUpdateTemplate vkCreateDescriptorUpdateTemplate;
	PFN_vkCreateSamplerYcbcrConversion vkCreateSamplerYcbcrConversion;
	PFN_vkDestroyDescriptorUpdateTemplate vkDestroyDescriptorUpdateTemplate;
	PFN_vkDestroySamplerYcbcrConversion vkDestroySamplerYcbcrConversion;
	PFN_vkGetBufferMemoryRequirements2 vkGetBufferMemoryRequirements2;
	PFN_vkGetDescriptorSetLayoutSupport vkGetDescriptorSetLayoutSupport;
	PFN_vkGetDeviceGroupPeerMemoryFeatures vkGetDeviceGroupPeerMemoryFeatures;
	PFN_vkGetDeviceQueue2 vkGetDeviceQueue2;
	PFN_vkGetImageMemoryRequirements2 vkGetImageMemoryRequirements2;
	PFN_vkGetImageSparseMemoryRequirements2 vkGetImageSparseMemoryRequirements2;
	PFN_vkTrimCommandPool vkTrimCommandPool;
	PFN_vkUpdateDescriptorSetWithTemplate vkUpdateDescriptorSetWithTemplate;
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_AMD_buffer_marker)
	PFN_vkCmdWriteBufferMarkerAMD vkCmdWriteBufferMarkerAMD;
#endif /* defined(VK_AMD_buffer_marker) */
#if defined(VK_AMD_display_native_hdr)
	PFN_vkSetLocalDimmingAMD vkSetLocalDimmingAMD;
#endif /* defined(VK_AMD_display_native_hdr) */
#if defined(VK_AMD_draw_indirect_count)
	PFN_vkCmdDrawIndexedIndirectCountAMD vkCmdDrawIndexedIndirectCountAMD;
	PFN_vkCmdDrawIndirectCountAMD vkCmdDrawIndirectCountAMD;
#endif /* defined(VK_AMD_draw_indirect_count) */
#if defined(VK_AMD_shader_info)
	PFN_vkGetShaderInfoAMD vkGetShaderInfoAMD;
#endif /* defined(VK_AMD_shader_info) */
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
	PFN_vkGetAndroidHardwareBufferPropertiesANDROID vkGetAndroidHardwareBufferPropertiesANDROID;
	PFN_vkGetMemoryAndroidHardwareBufferANDROID vkGetMemoryAndroidHardwareBufferANDROID;
#endif /* defined(VK_ANDROID_external_memory_android_hardware_buffer) */
#if defined(VK_EXT_buffer_device_address)
	PFN_vkGetBufferDeviceAddressEXT vkGetBufferDeviceAddressEXT;
#endif /* defined(VK_EXT_buffer_device_address) */
#if defined(VK_EXT_calibrated_timestamps)
	PFN_vkGetCalibratedTimestampsEXT vkGetCalibratedTimestampsEXT;
#endif /* defined(VK_EXT_calibrated_timestamps) */
#if defined(VK_EXT_conditional_rendering)
	PFN_vkCmdBeginConditionalRenderingEXT vkCmdBeginConditionalRenderingEXT;
	PFN_vkCmdEndConditionalRenderingEXT vkCmdEndConditionalRenderingEXT;
#endif /* defined(VK_EXT_conditional_rendering) */
#if defined(VK_EXT_debug_marker)
	PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBeginEXT;
	PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEndEXT;
	PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsertEXT;
	PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT;
	PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTagEXT;
#endif /* defined(VK_EXT_debug_marker) */
#if defined(VK_EXT_debug_utils)
	PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT;
	PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT;
	PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT;
	PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXT;
	PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabelEXT;
	PFN_vkQueueInsertDebugUtilsLabelEXT vkQueueInsertDebugUtilsLabelEXT;
	PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
	PFN_vkSetDebugUtilsObjectTagEXT vkSetDebugUtilsObjectTagEXT;
#endif /* defined(VK_EXT_debug_utils) */
#if defined(VK_EXT_discard_rectangles)
	PFN_vkCmdSetDiscardRectangleEXT vkCmdSetDiscardRectangleEXT;
#endif /* defined(VK_EXT_discard_rectangles) */
#if defined(VK_EXT_display_control)
	PFN_vkDisplayPowerControlEXT vkDisplayPowerControlEXT;
	PFN_vkGetSwapchainCounterEXT vkGetSwapchainCounterEXT;
	PFN_vkRegisterDeviceEventEXT vkRegisterDeviceEventEXT;
	PFN_vkRegisterDisplayEventEXT vkRegisterDisplayEventEXT;
#endif /* defined(VK_EXT_display_control) */
#if defined(VK_EXT_external_memory_host)
	PFN_vkGetMemoryHostPointerPropertiesEXT vkGetMemoryHostPointerPropertiesEXT;
#endif /* defined(VK_EXT_external_memory_host) */
#if defined(VK_EXT_full_screen_exclusive)
	PFN_vkAcquireFullScreenExclusiveModeEXT vkAcquireFullScreenExclusiveModeEXT;
	PFN_vkReleaseFullScreenExclusiveModeEXT vkReleaseFullScreenExclusiveModeEXT;
#endif /* defined(VK_EXT_full_screen_exclusive) */
#if defined(VK_EXT_hdr_metadata)
	PFN_vkSetHdrMetadataEXT vkSetHdrMetadataEXT;
#endif /* defined(VK_EXT_hdr_metadata) */
#if defined(VK_EXT_host_query_reset)
	PFN_vkResetQueryPoolEXT vkResetQueryPoolEXT;
#endif /* defined(VK_EXT_host_query_reset) */
#if defined(VK_EXT_image_drm_format_modifier)
	PFN_vkGetImageDrmFormatModifierPropertiesEXT vkGetImageDrmFormatModifierPropertiesEXT;
#endif /* defined(VK_EXT_image_drm_format_modifier) */
#if defined(VK_EXT_sample_locations)
	PFN_vkCmdSetSampleLocationsEXT vkCmdSetSampleLocationsEXT;
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_EXT_transform_feedback)
	PFN_vkCmdBeginQueryIndexedEXT vkCmdBeginQueryIndexedEXT;
	PFN_vkCmdBeginTransformFeedbackEXT vkCmdBeginTransformFeedbackEXT;
	PFN_vkCmdBindTransformFeedbackBuffersEXT vkCmdBindTransformFeedbackBuffersEXT;
	PFN_vkCmdDrawIndirectByteCountEXT vkCmdDrawIndirectByteCountEXT;
	PFN_vkCmdEndQueryIndexedEXT vkCmdEndQueryIndexedEXT;
	PFN_vkCmdEndTransformFeedbackEXT vkCmdEndTransformFeedbackEXT;
#endif /* defined(VK_EXT_transform_feedback) */
#if defined(VK_EXT_validation_cache)
	PFN_vkCreateValidationCacheEXT vkCreateValidationCacheEXT;
	PFN_vkDestroyValidationCacheEXT vkDestroyValidationCacheEXT;
	PFN_vkGetValidationCacheDataEXT vkGetValidationCacheDataEXT;
	PFN_vkMergeValidationCachesEXT vkMergeValidationCachesEXT;
#endif /* defined(VK_EXT_validation_cache) */
#if defined(VK_GOOGLE_display_timing)
	PFN_vkGetPastPresentationTimingGOOGLE vkGetPastPresentationTimingGOOGLE;
	PFN_vkGetRefreshCycleDurationGOOGLE vkGetRefreshCycleDurationGOOGLE;
#endif /* defined(VK_GOOGLE_display_timing) */
#if defined(VK_KHR_bind_memory2)
	PFN_vkBindBufferMemory2KHR vkBindBufferMemory2KHR;
	PFN_vkBindImageMemory2KHR vkBindImageMemory2KHR;
#endif /* defined(VK_KHR_bind_memory2) */
#if defined(VK_KHR_create_renderpass2)
	PFN_vkCmdBeginRenderPass2KHR vkCmdBeginRenderPass2KHR;
	PFN_vkCmdEndRenderPass2KHR vkCmdEndRenderPass2KHR;
	PFN_vkCmdNextSubpass2KHR vkCmdNextSubpass2KHR;
	PFN_vkCreateRenderPass2KHR vkCreateRenderPass2KHR;
#endif /* defined(VK_KHR_create_renderpass2) */
#if defined(VK_KHR_descriptor_update_template)
	PFN_vkCreateDescriptorUpdateTemplateKHR vkCreateDescriptorUpdateTemplateKHR;
	PFN_vkDestroyDescriptorUpdateTemplateKHR vkDestroyDescriptorUpdateTemplateKHR;
	PFN_vkUpdateDescriptorSetWithTemplateKHR vkUpdateDescriptorSetWithTemplateKHR;
#endif /* defined(VK_KHR_descriptor_update_template) */
#if defined(VK_KHR_device_group)
	PFN_vkCmdDispatchBaseKHR vkCmdDispatchBaseKHR;
	PFN_vkCmdSetDeviceMaskKHR vkCmdSetDeviceMaskKHR;
	PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR vkGetDeviceGroupPeerMemoryFeaturesKHR;
#endif /* defined(VK_KHR_device_group) */
#if defined(VK_KHR_display_swapchain)
	PFN_vkCreateSharedSwapchainsKHR vkCreateSharedSwapchainsKHR;
#endif /* defined(VK_KHR_display_swapchain) */
#if defined(VK_KHR_draw_indirect_count)
	PFN_vkCmdDrawIndexedIndirectCountKHR vkCmdDrawIndexedIndirectCountKHR;
	PFN_vkCmdDrawIndirectCountKHR vkCmdDrawIndirectCountKHR;
#endif /* defined(VK_KHR_draw_indirect_count) */
#if defined(VK_KHR_external_fence_fd)
	PFN_vkGetFenceFdKHR vkGetFenceFdKHR;
	PFN_vkImportFenceFdKHR vkImportFenceFdKHR;
#endif /* defined(VK_KHR_external_fence_fd) */
#if defined(VK_KHR_external_fence_win32)
	PFN_vkGetFenceWin32HandleKHR vkGetFenceWin32HandleKHR;
	PFN_vkImportFenceWin32HandleKHR vkImportFenceWin32HandleKHR;
#endif /* defined(VK_KHR_external_fence_win32) */
#if defined(VK_KHR_external_memory_fd)
	PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR;
	PFN_vkGetMemoryFdPropertiesKHR vkGetMemoryFdPropertiesKHR;
#endif /* defined(VK_KHR_external_memory_fd) */
#if defined(VK_KHR_external_memory_win32)
	PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR;
	PFN_vkGetMemoryWin32HandlePropertiesKHR vkGetMemoryWin32HandlePropertiesKHR;
#endif /* defined(VK_KHR_external_memory_win32) */
#if defined(VK_KHR_external_semaphore_fd)
	PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR;
	PFN_vkImportSemaphoreFdKHR vkImportSemaphoreFdKHR;
#endif /* defined(VK_KHR_external_semaphore_fd) */
#if defined(VK_KHR_external_semaphore_win32)
	PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR;
	PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR;
#endif /* defined(VK_KHR_external_semaphore_win32) */
#if defined(VK_KHR_get_memory_requirements2)
	PFN_vkGetBufferMemoryRequirements2KHR vkGetBufferMemoryRequirements2KHR;
	PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2KHR;
	PFN_vkGetImageSparseMemoryRequirements2KHR vkGetImageSparseMemoryRequirements2KHR;
#endif /* defined(VK_KHR_get_memory_requirements2) */
#if defined(VK_KHR_maintenance1)
	PFN_vkTrimCommandPoolKHR vkTrimCommandPoolKHR;
#endif /* defined(VK_KHR_maintenance1) */
#if defined(VK_KHR_maintenance3)
	PFN_vkGetDescriptorSetLayoutSupportKHR vkGetDescriptorSetLayoutSupportKHR;
#endif /* defined(VK_KHR_maintenance3) */
#if defined(VK_KHR_push_descriptor)
	PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR;
#endif /* defined(VK_KHR_push_descriptor) */
#if defined(VK_KHR_sampler_ycbcr_conversion)
	PFN_vkCreateSamplerYcbcrConversionKHR vkCreateSamplerYcbcrConversionKHR;
	PFN_vkDestroySamplerYcbcrConversionKHR vkDestroySamplerYcbcrConversionKHR;
#endif /* defined(VK_KHR_sampler_ycbcr_conversion) */
#if defined(VK_KHR_shared_presentable_image)
	PFN_vkGetSwapchainStatusKHR vkGetSwapchainStatusKHR;
#endif /* defined(VK_KHR_shared_presentable_image) */
#if defined(VK_KHR_swapchain)
	PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
	PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
	PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
	PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
	PFN_vkQueuePresentKHR vkQueuePresentKHR;
#endif /* defined(VK_KHR_swapchain) */
#if defined(VK_NVX_device_generated_commands)
	PFN_vkCmdProcessCommandsNVX vkCmdProcessCommandsNVX;
	PFN_vkCmdReserveSpaceForCommandsNVX vkCmdReserveSpaceForCommandsNVX;
	PFN_vkCreateIndirectCommandsLayoutNVX vkCreateIndirectCommandsLayoutNVX;
	PFN_vkCreateObjectTableNVX vkCreateObjectTableNVX;
	PFN_vkDestroyIndirectCommandsLayoutNVX vkDestroyIndirectCommandsLayoutNVX;
	PFN_vkDestroyObjectTableNVX vkDestroyObjectTableNVX;
	PFN_vkRegisterObjectsNVX vkRegisterObjectsNVX;
	PFN_vkUnregisterObjectsNVX vkUnregisterObjectsNVX;
#endif /* defined(VK_NVX_device_generated_commands) */
#if defined(VK_NVX_image_view_handle)
	PFN_vkGetImageViewHandleNVX vkGetImageViewHandleNVX;
#endif /* defined(VK_NVX_image_view_handle) */
#if defined(VK_NV_clip_space_w_scaling)
	PFN_vkCmdSetViewportWScalingNV vkCmdSetViewportWScalingNV;
#endif /* defined(VK_NV_clip_space_w_scaling) */
#if defined(VK_NV_device_diagnostic_checkpoints)
	PFN_vkCmdSetCheckpointNV vkCmdSetCheckpointNV;
	PFN_vkGetQueueCheckpointDataNV vkGetQueueCheckpointDataNV;
#endif /* defined(VK_NV_device_diagnostic_checkpoints) */
#if defined(VK_NV_external_memory_win32)
	PFN_vkGetMemoryWin32HandleNV vkGetMemoryWin32HandleNV;
#endif /* defined(VK_NV_external_memory_win32) */
#if defined(VK_NV_mesh_shader)
	PFN_vkCmdDrawMeshTasksIndirectCountNV vkCmdDrawMeshTasksIndirectCountNV;
	PFN_vkCmdDrawMeshTasksIndirectNV vkCmdDrawMeshTasksIndirectNV;
	PFN_vkCmdDrawMeshTasksNV vkCmdDrawMeshTasksNV;
#endif /* defined(VK_NV_mesh_shader) */
#if defined(VK_NV_ray_tracing)
	PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV;
	PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV;
	PFN_vkCmdCopyAccelerationStructureNV vkCmdCopyAccelerationStructureNV;
	PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;
	PFN_vkCmdWriteAccelerationStructuresPropertiesNV vkCmdWriteAccelerationStructuresPropertiesNV;
	PFN_vkCompileDeferredNV vkCompileDeferredNV;
	PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV;
	PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV;
	PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV;
	PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV;
	PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV;
	PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV;
#endif /* defined(VK_NV_ray_tracing) */
#if defined(VK_NV_scissor_exclusive)
	PFN_vkCmdSetExclusiveScissorNV vkCmdSetExclusiveScissorNV;
#endif /* defined(VK_NV_scissor_exclusive) */
#if defined(VK_NV_shading_rate_image)
	PFN_vkCmdBindShadingRateImageNV vkCmdBindShadingRateImageNV;
	PFN_vkCmdSetCoarseSampleOrderNV vkCmdSetCoarseSampleOrderNV;
	PFN_vkCmdSetViewportShadingRatePaletteNV vkCmdSetViewportShadingRatePaletteNV;
#endif /* defined(VK_NV_shading_rate_image) */
#if (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_device_group) && defined(VK_EXT_full_screen_exclusive))
	PFN_vkGetDeviceGroupSurfacePresentModes2EXT vkGetDeviceGroupSurfacePresentModes2EXT;
#endif /* (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_device_group) && defined(VK_EXT_full_screen_exclusive)) */
#if (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1))
	PFN_vkCmdPushDescriptorSetWithTemplateKHR vkCmdPushDescriptorSetWithTemplateKHR;
#endif /* (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	PFN_vkGetDeviceGroupPresentCapabilitiesKHR vkGetDeviceGroupPresentCapabilitiesKHR;
	PFN_vkGetDeviceGroupSurfacePresentModesKHR vkGetDeviceGroupSurfacePresentModesKHR;
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	PFN_vkAcquireNextImage2KHR vkAcquireNextImage2KHR;
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
	/* VOLK_GENERATE_DEVICE_TABLE */
};

/* VOLK_GENERATE_PROTOTYPES_H */
#if defined(VK_VERSION_1_0)

VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(
	const VkInstanceCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkInstance* pInstance);

VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(
	VkInstance                                  instance,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(
	VkInstance                                  instance,
	uint32_t* pPhysicalDeviceCount,
	VkPhysicalDevice* pPhysicalDevices);

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures(
	VkPhysicalDevice                            physicalDevice,
	VkPhysicalDeviceFeatures* pFeatures);

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties(
	VkPhysicalDevice                            physicalDevice,
	VkFormat                                    format,
	VkFormatProperties* pFormatProperties);

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties(
	VkPhysicalDevice                            physicalDevice,
	VkFormat                                    format,
	VkImageType                                 type,
	VkImageTiling                               tiling,
	VkImageUsageFlags                           usage,
	VkImageCreateFlags                          flags,
	VkImageFormatProperties* pImageFormatProperties);

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(
	VkPhysicalDevice                            physicalDevice,
	VkPhysicalDeviceProperties* pProperties);

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
	VkPhysicalDevice                            physicalDevice,
	uint32_t* pQueueFamilyPropertyCount,
	VkQueueFamilyProperties* pQueueFamilyProperties);

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(
	VkPhysicalDevice                            physicalDevice,
	VkPhysicalDeviceMemoryProperties* pMemoryProperties);

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(
	VkInstance                                  instance,
	const char* pName);

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(
	VkDevice                                    device,
	const char* pName);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(
	VkPhysicalDevice                            physicalDevice,
	const VkDeviceCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDevice* pDevice);

VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(
	VkDevice                                    device,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
	const char* pLayerName,
	uint32_t* pPropertyCount,
	VkExtensionProperties* pProperties);

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
	VkPhysicalDevice                            physicalDevice,
	const char* pLayerName,
	uint32_t* pPropertyCount,
	VkExtensionProperties* pProperties);

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
	uint32_t* pPropertyCount,
	VkLayerProperties* pProperties);

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(
	VkPhysicalDevice                            physicalDevice,
	uint32_t* pPropertyCount,
	VkLayerProperties* pProperties);

VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(
	VkDevice                                    device,
	uint32_t                                    queueFamilyIndex,
	uint32_t                                    queueIndex,
	VkQueue* pQueue);

VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(
	VkQueue                                     queue,
	uint32_t                                    submitCount,
	const VkSubmitInfo* pSubmits,
	VkFence                                     fence);

VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle(
	VkQueue                                     queue);

VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(
	VkDevice                                    device);

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(
	VkDevice                                    device,
	const VkMemoryAllocateInfo* pAllocateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDeviceMemory* pMemory);

VKAPI_ATTR void VKAPI_CALL vkFreeMemory(
	VkDevice                                    device,
	VkDeviceMemory                              memory,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(
	VkDevice                                    device,
	VkDeviceMemory                              memory,
	VkDeviceSize                                offset,
	VkDeviceSize                                size,
	VkMemoryMapFlags                            flags,
	void** ppData);

VKAPI_ATTR void VKAPI_CALL vkUnmapMemory(
	VkDevice                                    device,
	VkDeviceMemory                              memory);

VKAPI_ATTR VkResult VKAPI_CALL vkFlushMappedMemoryRanges(
	VkDevice                                    device,
	uint32_t                                    memoryRangeCount,
	const VkMappedMemoryRange* pMemoryRanges);

VKAPI_ATTR VkResult VKAPI_CALL vkInvalidateMappedMemoryRanges(
	VkDevice                                    device,
	uint32_t                                    memoryRangeCount,
	const VkMappedMemoryRange* pMemoryRanges);

VKAPI_ATTR void VKAPI_CALL vkGetDeviceMemoryCommitment(
	VkDevice                                    device,
	VkDeviceMemory                              memory,
	VkDeviceSize* pCommittedMemoryInBytes);

VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(
	VkDevice                                    device,
	VkBuffer                                    buffer,
	VkDeviceMemory                              memory,
	VkDeviceSize                                memoryOffset);

VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory(
	VkDevice                                    device,
	VkImage                                     image,
	VkDeviceMemory                              memory,
	VkDeviceSize                                memoryOffset);

VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements(
	VkDevice                                    device,
	VkBuffer                                    buffer,
	VkMemoryRequirements* pMemoryRequirements);

VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements(
	VkDevice                                    device,
	VkImage                                     image,
	VkMemoryRequirements* pMemoryRequirements);

VKAPI_ATTR void VKAPI_CALL vkGetImageSparseMemoryRequirements(
	VkDevice                                    device,
	VkImage                                     image,
	uint32_t* pSparseMemoryRequirementCount,
	VkSparseImageMemoryRequirements* pSparseMemoryRequirements);

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties(
	VkPhysicalDevice                            physicalDevice,
	VkFormat                                    format,
	VkImageType                                 type,
	VkSampleCountFlagBits                       samples,
	VkImageUsageFlags                           usage,
	VkImageTiling                               tiling,
	uint32_t* pPropertyCount,
	VkSparseImageFormatProperties* pProperties);

VKAPI_ATTR VkResult VKAPI_CALL vkQueueBindSparse(
	VkQueue                                     queue,
	uint32_t                                    bindInfoCount,
	const VkBindSparseInfo* pBindInfo,
	VkFence                                     fence);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateFence(
	VkDevice                                    device,
	const VkFenceCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkFence* pFence);

VKAPI_ATTR void VKAPI_CALL vkDestroyFence(
	VkDevice                                    device,
	VkFence                                     fence,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkResetFences(
	VkDevice                                    device,
	uint32_t                                    fenceCount,
	const VkFence* pFences);

VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceStatus(
	VkDevice                                    device,
	VkFence                                     fence);

VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences(
	VkDevice                                    device,
	uint32_t                                    fenceCount,
	const VkFence* pFences,
	VkBool32                                    waitAll,
	uint64_t                                    timeout);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSemaphore(
	VkDevice                                    device,
	const VkSemaphoreCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkSemaphore* pSemaphore);

VKAPI_ATTR void VKAPI_CALL vkDestroySemaphore(
	VkDevice                                    device,
	VkSemaphore                                 semaphore,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateEvent(
	VkDevice                                    device,
	const VkEventCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkEvent* pEvent);

VKAPI_ATTR void VKAPI_CALL vkDestroyEvent(
	VkDevice                                    device,
	VkEvent                                     event,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkGetEventStatus(
	VkDevice                                    device,
	VkEvent                                     event);

VKAPI_ATTR VkResult VKAPI_CALL vkSetEvent(
	VkDevice                                    device,
	VkEvent                                     event);

VKAPI_ATTR VkResult VKAPI_CALL vkResetEvent(
	VkDevice                                    device,
	VkEvent                                     event);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateQueryPool(
	VkDevice                                    device,
	const VkQueryPoolCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkQueryPool* pQueryPool);

VKAPI_ATTR void VKAPI_CALL vkDestroyQueryPool(
	VkDevice                                    device,
	VkQueryPool                                 queryPool,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkGetQueryPoolResults(
	VkDevice                                    device,
	VkQueryPool                                 queryPool,
	uint32_t                                    firstQuery,
	uint32_t                                    queryCount,
	size_t                                      dataSize,
	void* pData,
	VkDeviceSize                                stride,
	VkQueryResultFlags                          flags);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(
	VkDevice                                    device,
	const VkBufferCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkBuffer* pBuffer);

VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(
	VkDevice                                    device,
	VkBuffer                                    buffer,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateBufferView(
	VkDevice                                    device,
	const VkBufferViewCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkBufferView* pView);

VKAPI_ATTR void VKAPI_CALL vkDestroyBufferView(
	VkDevice                                    device,
	VkBufferView                                bufferView,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateImage(
	VkDevice                                    device,
	const VkImageCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkImage* pImage);

VKAPI_ATTR void VKAPI_CALL vkDestroyImage(
	VkDevice                                    device,
	VkImage                                     image,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR void VKAPI_CALL vkGetImageSubresourceLayout(
	VkDevice                                    device,
	VkImage                                     image,
	const VkImageSubresource* pSubresource,
	VkSubresourceLayout* pLayout);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateImageView(
	VkDevice                                    device,
	const VkImageViewCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkImageView* pView);

VKAPI_ATTR void VKAPI_CALL vkDestroyImageView(
	VkDevice                                    device,
	VkImageView                                 imageView,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateShaderModule(
	VkDevice                                    device,
	const VkShaderModuleCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkShaderModule* pShaderModule);

VKAPI_ATTR void VKAPI_CALL vkDestroyShaderModule(
	VkDevice                                    device,
	VkShaderModule                              shaderModule,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineCache(
	VkDevice                                    device,
	const VkPipelineCacheCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkPipelineCache* pPipelineCache);

VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineCache(
	VkDevice                                    device,
	VkPipelineCache                             pipelineCache,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineCacheData(
	VkDevice                                    device,
	VkPipelineCache                             pipelineCache,
	size_t* pDataSize,
	void* pData);

VKAPI_ATTR VkResult VKAPI_CALL vkMergePipelineCaches(
	VkDevice                                    device,
	VkPipelineCache                             dstCache,
	uint32_t                                    srcCacheCount,
	const VkPipelineCache* pSrcCaches);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(
	VkDevice                                    device,
	VkPipelineCache                             pipelineCache,
	uint32_t                                    createInfoCount,
	const VkGraphicsPipelineCreateInfo* pCreateInfos,
	const VkAllocationCallbacks* pAllocator,
	VkPipeline* pPipelines);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateComputePipelines(
	VkDevice                                    device,
	VkPipelineCache                             pipelineCache,
	uint32_t                                    createInfoCount,
	const VkComputePipelineCreateInfo* pCreateInfos,
	const VkAllocationCallbacks* pAllocator,
	VkPipeline* pPipelines);

VKAPI_ATTR void VKAPI_CALL vkDestroyPipeline(
	VkDevice                                    device,
	VkPipeline                                  pipeline,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineLayout(
	VkDevice                                    device,
	const VkPipelineLayoutCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkPipelineLayout* pPipelineLayout);

VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineLayout(
	VkDevice                                    device,
	VkPipelineLayout                            pipelineLayout,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSampler(
	VkDevice                                    device,
	const VkSamplerCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkSampler* pSampler);

VKAPI_ATTR void VKAPI_CALL vkDestroySampler(
	VkDevice                                    device,
	VkSampler                                   sampler,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorSetLayout(
	VkDevice                                    device,
	const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDescriptorSetLayout* pSetLayout);

VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorSetLayout(
	VkDevice                                    device,
	VkDescriptorSetLayout                       descriptorSetLayout,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorPool(
	VkDevice                                    device,
	const VkDescriptorPoolCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDescriptorPool* pDescriptorPool);

VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorPool(
	VkDevice                                    device,
	VkDescriptorPool                            descriptorPool,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkResetDescriptorPool(
	VkDevice                                    device,
	VkDescriptorPool                            descriptorPool,
	VkDescriptorPoolResetFlags                  flags);

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateDescriptorSets(
	VkDevice                                    device,
	const VkDescriptorSetAllocateInfo* pAllocateInfo,
	VkDescriptorSet* pDescriptorSets);

VKAPI_ATTR VkResult VKAPI_CALL vkFreeDescriptorSets(
	VkDevice                                    device,
	VkDescriptorPool                            descriptorPool,
	uint32_t                                    descriptorSetCount,
	const VkDescriptorSet* pDescriptorSets);

VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets(
	VkDevice                                    device,
	uint32_t                                    descriptorWriteCount,
	const VkWriteDescriptorSet* pDescriptorWrites,
	uint32_t                                    descriptorCopyCount,
	const VkCopyDescriptorSet* pDescriptorCopies);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateFramebuffer(
	VkDevice                                    device,
	const VkFramebufferCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkFramebuffer* pFramebuffer);

VKAPI_ATTR void VKAPI_CALL vkDestroyFramebuffer(
	VkDevice                                    device,
	VkFramebuffer                               framebuffer,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateRenderPass(
	VkDevice                                    device,
	const VkRenderPassCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkRenderPass* pRenderPass);

VKAPI_ATTR void VKAPI_CALL vkDestroyRenderPass(
	VkDevice                                    device,
	VkRenderPass                                renderPass,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR void VKAPI_CALL vkGetRenderAreaGranularity(
	VkDevice                                    device,
	VkRenderPass                                renderPass,
	VkExtent2D* pGranularity);

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(
	VkDevice                                    device,
	const VkCommandBufferAllocateInfo* pAllocateInfo,
	VkCommandBuffer* pCommandBuffers);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool(
	VkDevice                                    device,
	const VkCommandPoolCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkCommandPool* pCommandPool);

VKAPI_ATTR void VKAPI_CALL vkDestroyCommandPool(
	VkDevice                                    device,
	VkCommandPool                               commandPool,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandPool(
	VkDevice                                    device,
	VkCommandPool                               commandPool,
	VkCommandPoolResetFlags                     flags);

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(
	VkDevice                                    device,
	const VkCommandBufferAllocateInfo* pAllocateInfo,
	VkCommandBuffer* pCommandBuffers);

VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(
	VkDevice                                    device,
	VkCommandPool                               commandPool,
	uint32_t                                    commandBufferCount,
	const VkCommandBuffer* pCommandBuffers);

VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(
	VkCommandBuffer                             commandBuffer,
	const VkCommandBufferBeginInfo* pBeginInfo);

VKAPI_ATTR VkResult VKAPI_CALL vkEndCommandBuffer(
	VkCommandBuffer                             commandBuffer);

VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer(
	VkCommandBuffer                             commandBuffer,
	VkCommandBufferResetFlags                   flags);

VKAPI_ATTR void VKAPI_CALL vkCmdBindPipeline(
	VkCommandBuffer                             commandBuffer,
	VkPipelineBindPoint                         pipelineBindPoint,
	VkPipeline                                  pipeline);

VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport(
	VkCommandBuffer                             commandBuffer,
	uint32_t                                    firstViewport,
	uint32_t                                    viewportCount,
	const VkViewport* pViewports);

VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor(
	VkCommandBuffer                             commandBuffer,
	uint32_t                                    firstScissor,
	uint32_t                                    scissorCount,
	const VkRect2D* pScissors);

VKAPI_ATTR void VKAPI_CALL vkCmdSetLineWidth(
	VkCommandBuffer                             commandBuffer,
	float                                       lineWidth);

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBias(
	VkCommandBuffer                             commandBuffer,
	float                                       depthBiasConstantFactor,
	float                                       depthBiasClamp,
	float                                       depthBiasSlopeFactor);

VKAPI_ATTR void VKAPI_CALL vkCmdSetBlendConstants(
	VkCommandBuffer                             commandBuffer,
	const float                                 blendConstants[4]);

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBounds(
	VkCommandBuffer                             commandBuffer,
	float                                       minDepthBounds,
	float                                       maxDepthBounds);

VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilCompareMask(
	VkCommandBuffer                             commandBuffer,
	VkStencilFaceFlags                          faceMask,
	uint32_t                                    compareMask);

VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilWriteMask(
	VkCommandBuffer                             commandBuffer,
	VkStencilFaceFlags                          faceMask,
	uint32_t                                    writeMask);

VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilReference(
	VkCommandBuffer                             commandBuffer,
	VkStencilFaceFlags                          faceMask,
	uint32_t                                    reference);

VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets(
	VkCommandBuffer                             commandBuffer,
	VkPipelineBindPoint                         pipelineBindPoint,
	VkPipelineLayout                            layout,
	uint32_t                                    firstSet,
	uint32_t                                    descriptorSetCount,
	const VkDescriptorSet* pDescriptorSets,
	uint32_t                                    dynamicOffsetCount,
	const uint32_t* pDynamicOffsets);

VKAPI_ATTR void VKAPI_CALL vkCmdBindIndexBuffer(
	VkCommandBuffer                             commandBuffer,
	VkBuffer                                    buffer,
	VkDeviceSize                                offset,
	VkIndexType                                 indexType);

VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers(
	VkCommandBuffer                             commandBuffer,
	uint32_t                                    firstBinding,
	uint32_t                                    bindingCount,
	const VkBuffer* pBuffers,
	const VkDeviceSize* pOffsets);

VKAPI_ATTR void VKAPI_CALL vkCmdDraw(
	VkCommandBuffer                             commandBuffer,
	uint32_t                                    vertexCount,
	uint32_t                                    instanceCount,
	uint32_t                                    firstVertex,
	uint32_t                                    firstInstance);

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexed(
	VkCommandBuffer                             commandBuffer,
	uint32_t                                    indexCount,
	uint32_t                                    instanceCount,
	uint32_t                                    firstIndex,
	int32_t                                     vertexOffset,
	uint32_t                                    firstInstance);

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirect(
	VkCommandBuffer                             commandBuffer,
	VkBuffer                                    buffer,
	VkDeviceSize                                offset,
	uint32_t                                    drawCount,
	uint32_t                                    stride);

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirect(
	VkCommandBuffer                             commandBuffer,
	VkBuffer                                    buffer,
	VkDeviceSize                                offset,
	uint32_t                                    drawCount,
	uint32_t                                    stride);

VKAPI_ATTR void VKAPI_CALL vkCmdDispatch(
	VkCommandBuffer                             commandBuffer,
	uint32_t                                    groupCountX,
	uint32_t                                    groupCountY,
	uint32_t                                    groupCountZ);

VKAPI_ATTR void VKAPI_CALL vkCmdDispatchIndirect(
	VkCommandBuffer                             commandBuffer,
	VkBuffer                                    buffer,
	VkDeviceSize                                offset);

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer(
	VkCommandBuffer                             commandBuffer,
	VkBuffer                                    srcBuffer,
	VkBuffer                                    dstBuffer,
	uint32_t                                    regionCount,
	const VkBufferCopy* pRegions);

VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage(
	VkCommandBuffer                             commandBuffer,
	VkImage                                     srcImage,
	VkImageLayout                               srcImageLayout,
	VkImage                                     dstImage,
	VkImageLayout                               dstImageLayout,
	uint32_t                                    regionCount,
	const VkImageCopy* pRegions);

VKAPI_ATTR void VKAPI_CALL vkCmdBlitImage(
	VkCommandBuffer                             commandBuffer,
	VkImage                                     srcImage,
	VkImageLayout                               srcImageLayout,
	VkImage                                     dstImage,
	VkImageLayout                               dstImageLayout,
	uint32_t                                    regionCount,
	const VkImageBlit* pRegions,
	VkFilter                                    filter);

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage(
	VkCommandBuffer                             commandBuffer,
	VkBuffer                                    srcBuffer,
	VkImage                                     dstImage,
	VkImageLayout                               dstImageLayout,
	uint32_t                                    regionCount,
	const VkBufferImageCopy* pRegions);

VKAPI_ATTR void VKAPI_CALL vkCmdCopyImageToBuffer(
	VkCommandBuffer                             commandBuffer,
	VkImage                                     srcImage,
	VkImageLayout                               srcImageLayout,
	VkBuffer                                    dstBuffer,
	uint32_t                                    regionCount,
	const VkBufferImageCopy* pRegions);

VKAPI_ATTR void VKAPI_CALL vkCmdUpdateBuffer(
	VkCommandBuffer                             commandBuffer,
	VkBuffer                                    dstBuffer,
	VkDeviceSize                                dstOffset,
	VkDeviceSize                                dataSize,
	const void* pData);

VKAPI_ATTR void VKAPI_CALL vkCmdFillBuffer(
	VkCommandBuffer                             commandBuffer,
	VkBuffer                                    dstBuffer,
	VkDeviceSize                                dstOffset,
	VkDeviceSize                                size,
	uint32_t                                    data);

VKAPI_ATTR void VKAPI_CALL vkCmdClearColorImage(
	VkCommandBuffer                             commandBuffer,
	VkImage                                     image,
	VkImageLayout                               imageLayout,
	const VkClearColorValue* pColor,
	uint32_t                                    rangeCount,
	const VkImageSubresourceRange* pRanges);

VKAPI_ATTR void VKAPI_CALL vkCmdClearDepthStencilImage(
	VkCommandBuffer                             commandBuffer,
	VkImage                                     image,
	VkImageLayout                               imageLayout,
	const VkClearDepthStencilValue* pDepthStencil,
	uint32_t                                    rangeCount,
	const VkImageSubresourceRange* pRanges);

VKAPI_ATTR void VKAPI_CALL vkCmdClearAttachments(
	VkCommandBuffer                             commandBuffer,
	uint32_t                                    attachmentCount,
	const VkClearAttachment* pAttachments,
	uint32_t                                    rectCount,
	const VkClearRect* pRects);

VKAPI_ATTR void VKAPI_CALL vkCmdResolveImage(
	VkCommandBuffer                             commandBuffer,
	VkImage                                     srcImage,
	VkImageLayout                               srcImageLayout,
	VkImage                                     dstImage,
	VkImageLayout                               dstImageLayout,
	uint32_t                                    regionCount,
	const VkImageResolve* pRegions);

VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent(
	VkCommandBuffer                             commandBuffer,
	VkEvent                                     event,
	VkPipelineStageFlags                        stageMask);

VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent(
	VkCommandBuffer                             commandBuffer,
	VkEvent                                     event,
	VkPipelineStageFlags                        stageMask);

VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents(
	VkCommandBuffer                             commandBuffer,
	uint32_t                                    eventCount,
	const VkEvent* pEvents,
	VkPipelineStageFlags                        srcStageMask,
	VkPipelineStageFlags                        dstStageMask,
	uint32_t                                    memoryBarrierCount,
	const VkMemoryBarrier* pMemoryBarriers,
	uint32_t                                    bufferMemoryBarrierCount,
	const VkBufferMemoryBarrier* pBufferMemoryBarriers,
	uint32_t                                    imageMemoryBarrierCount,
	const VkImageMemoryBarrier* pImageMemoryBarriers);

VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(
	VkCommandBuffer                             commandBuffer,
	VkPipelineStageFlags                        srcStageMask,
	VkPipelineStageFlags                        dstStageMask,
	VkDependencyFlags                           dependencyFlags,
	uint32_t                                    memoryBarrierCount,
	const VkMemoryBarrier* pMemoryBarriers,
	uint32_t                                    bufferMemoryBarrierCount,
	const VkBufferMemoryBarrier* pBufferMemoryBarriers,
	uint32_t                                    imageMemoryBarrierCount,
	const VkImageMemoryBarrier* pImageMemoryBarriers);

VKAPI_ATTR void VKAPI_CALL vkCmdBeginQuery(
	VkCommandBuffer                             commandBuffer,
	VkQueryPool                                 queryPool,
	uint32_t                                    query,
	VkQueryControlFlags                         flags);

VKAPI_ATTR void VKAPI_CALL vkCmdEndQuery(
	VkCommandBuffer                             commandBuffer,
	VkQueryPool                                 queryPool,
	uint32_t                                    query);

VKAPI_ATTR void VKAPI_CALL vkCmdResetQueryPool(
	VkCommandBuffer                             commandBuffer,
	VkQueryPool                                 queryPool,
	uint32_t                                    firstQuery,
	uint32_t                                    queryCount);

VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp(
	VkCommandBuffer                             commandBuffer,
	VkPipelineStageFlagBits                     pipelineStage,
	VkQueryPool                                 queryPool,
	uint32_t                                    query);

VKAPI_ATTR void VKAPI_CALL vkCmdCopyQueryPoolResults(
	VkCommandBuffer                             commandBuffer,
	VkQueryPool                                 queryPool,
	uint32_t                                    firstQuery,
	uint32_t                                    queryCount,
	VkBuffer                                    dstBuffer,
	VkDeviceSize                                dstOffset,
	VkDeviceSize                                stride,
	VkQueryResultFlags                          flags);

VKAPI_ATTR void VKAPI_CALL vkCmdPushConstants(
	VkCommandBuffer                             commandBuffer,
	VkPipelineLayout                            layout,
	VkShaderStageFlags                          stageFlags,
	uint32_t                                    offset,
	uint32_t                                    size,
	const void* pValues);

VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderPass(
	VkCommandBuffer                             commandBuffer,
	const VkRenderPassBeginInfo* pRenderPassBegin,
	VkSubpassContents                           contents);

VKAPI_ATTR void VKAPI_CALL vkCmdNextSubpass(
	VkCommandBuffer                             commandBuffer,
	VkSubpassContents                           contents);

VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass(
	VkCommandBuffer                             commandBuffer);

VKAPI_ATTR void VKAPI_CALL vkCmdExecuteCommands(
	VkCommandBuffer                             commandBuffer,
	uint32_t                                    commandBufferCount,
	const VkCommandBuffer* pCommandBuffers);
#endif /* defined(VK_VERSION_1_0) */

#if defined(VK_KHR_swapchain)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateSwapchainKHR(
	VkDevice                                    device,
	const VkSwapchainCreateInfoKHR* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkSwapchainKHR* pSwapchain);

VKAPI_ATTR void VKAPI_CALL vkDestroySwapchainKHR(
	VkDevice                                    device,
	VkSwapchainKHR                              swapchain,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainImagesKHR(
	VkDevice                                    device,
	VkSwapchainKHR                              swapchain,
	uint32_t* pSwapchainImageCount,
	VkImage* pSwapchainImages);

VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImageKHR(
	VkDevice                                    device,
	VkSwapchainKHR                              swapchain,
	uint64_t                                    timeout,
	VkSemaphore                                 semaphore,
	VkFence                                     fence,
	uint32_t* pImageIndex);

VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(
	VkQueue                                     queue,
	const VkPresentInfoKHR* pPresentInfo);
#endif /* defined(VK_KHR_swapchain) */

#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
extern PFN_vkGetDeviceGroupPresentCapabilitiesKHR vkGetDeviceGroupPresentCapabilitiesKHR;
extern PFN_vkGetDeviceGroupSurfacePresentModesKHR vkGetDeviceGroupSurfacePresentModesKHR;
extern PFN_vkGetPhysicalDevicePresentRectanglesKHR vkGetPhysicalDevicePresentRectanglesKHR;
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImage2KHR(
	VkDevice                                    device,
	const VkAcquireNextImageInfoKHR* pAcquireInfo,
	uint32_t* pImageIndex);
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */

#if defined(VK_AMD_buffer_marker)
VKAPI_ATTR void VKAPI_CALL vkCmdWriteBufferMarkerAMD(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker);
#endif /* defined(VK_AMD_buffer_marker) */

#if defined(VK_AMD_display_native_hdr)

VKAPI_ATTR void VKAPI_CALL vkSetLocalDimmingAMD(
	VkSwapchainKHR                              swapChain,
	VkBool32                                    localDimmingEnable);
#endif /* defined(VK_AMD_display_native_hdr) */

#if defined(VK_KHR_bind_memory2)
VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory2KHR(
	VkDevice                                    device,
	uint32_t                                    bindInfoCount,
	const VkBindBufferMemoryInfo* pBindInfos);

VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory2KHR(
	VkDevice                                    device,
	uint32_t                                    bindInfoCount,
	const VkBindImageMemoryInfo* pBindInfos);
#endif /* defined(VK_KHR_bind_memory2) */

#if defined(VK_VERSION_1_1)
VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceVersion(
	uint32_t* pApiVersion);

VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory2(
	VkDevice                                    device,
	uint32_t                                    bindInfoCount,
	const VkBindBufferMemoryInfo* pBindInfos);

VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory2(
	VkDevice                                    device,
	uint32_t                                    bindInfoCount,
	const VkBindImageMemoryInfo* pBindInfos);

VKAPI_ATTR void VKAPI_CALL vkGetDeviceGroupPeerMemoryFeatures(
	VkDevice                                    device,
	uint32_t                                    heapIndex,
	uint32_t                                    localDeviceIndex,
	uint32_t                                    remoteDeviceIndex,
	VkPeerMemoryFeatureFlags* pPeerMemoryFeatures);

VKAPI_ATTR void VKAPI_CALL vkCmdSetDeviceMask(
	VkCommandBuffer                             commandBuffer,
	uint32_t                                    deviceMask);

VKAPI_ATTR void VKAPI_CALL vkCmdDispatchBase(
	VkCommandBuffer                             commandBuffer,
	uint32_t                                    baseGroupX,
	uint32_t                                    baseGroupY,
	uint32_t                                    baseGroupZ,
	uint32_t                                    groupCountX,
	uint32_t                                    groupCountY,
	uint32_t                                    groupCountZ);

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDeviceGroups(
	VkInstance                                  instance,
	uint32_t* pPhysicalDeviceGroupCount,
	VkPhysicalDeviceGroupProperties* pPhysicalDeviceGroupProperties);

VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements2(
	VkDevice                                    device,
	const VkImageMemoryRequirementsInfo2* pInfo,
	VkMemoryRequirements2* pMemoryRequirements);

VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements2(
	VkDevice                                    device,
	const VkBufferMemoryRequirementsInfo2* pInfo,
	VkMemoryRequirements2* pMemoryRequirements);

VKAPI_ATTR void VKAPI_CALL vkGetImageSparseMemoryRequirements2(
	VkDevice                                    device,
	const VkImageSparseMemoryRequirementsInfo2* pInfo,
	uint32_t* pSparseMemoryRequirementCount,
	VkSparseImageMemoryRequirements2* pSparseMemoryRequirements);

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures2(
	VkPhysicalDevice                            physicalDevice,
	VkPhysicalDeviceFeatures2* pFeatures);

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2(
	VkPhysicalDevice                            physicalDevice,
	VkPhysicalDeviceProperties2* pProperties);

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties2(
	VkPhysicalDevice                            physicalDevice,
	VkFormat                                    format,
	VkFormatProperties2* pFormatProperties);

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties2(
	VkPhysicalDevice                            physicalDevice,
	const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo,
	VkImageFormatProperties2* pImageFormatProperties);

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties2(
	VkPhysicalDevice                            physicalDevice,
	uint32_t* pQueueFamilyPropertyCount,
	VkQueueFamilyProperties2* pQueueFamilyProperties);

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties2(
	VkPhysicalDevice                            physicalDevice,
	VkPhysicalDeviceMemoryProperties2* pMemoryProperties);

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties2(
	VkPhysicalDevice                            physicalDevice,
	const VkPhysicalDeviceSparseImageFormatInfo2* pFormatInfo,
	uint32_t* pPropertyCount,
	VkSparseImageFormatProperties2* pProperties);

VKAPI_ATTR void VKAPI_CALL vkTrimCommandPool(
	VkDevice                                    device,
	VkCommandPool                               commandPool,
	VkCommandPoolTrimFlags                      flags);

VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue2(
	VkDevice                                    device,
	const VkDeviceQueueInfo2* pQueueInfo,
	VkQueue* pQueue);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSamplerYcbcrConversion(
	VkDevice                                    device,
	const VkSamplerYcbcrConversionCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkSamplerYcbcrConversion* pYcbcrConversion);

VKAPI_ATTR void VKAPI_CALL vkDestroySamplerYcbcrConversion(
	VkDevice                                    device,
	VkSamplerYcbcrConversion                    ycbcrConversion,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorUpdateTemplate(
	VkDevice                                    device,
	const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate);

VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorUpdateTemplate(
	VkDevice                                    device,
	VkDescriptorUpdateTemplate                  descriptorUpdateTemplate,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSetWithTemplate(
	VkDevice                                    device,
	VkDescriptorSet                             descriptorSet,
	VkDescriptorUpdateTemplate                  descriptorUpdateTemplate,
	const void* pData);

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalBufferProperties(
	VkPhysicalDevice                            physicalDevice,
	const VkPhysicalDeviceExternalBufferInfo* pExternalBufferInfo,
	VkExternalBufferProperties* pExternalBufferProperties);

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalFenceProperties(
	VkPhysicalDevice                            physicalDevice,
	const VkPhysicalDeviceExternalFenceInfo* pExternalFenceInfo,
	VkExternalFenceProperties* pExternalFenceProperties);

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalSemaphoreProperties(
	VkPhysicalDevice                            physicalDevice,
	const VkPhysicalDeviceExternalSemaphoreInfo* pExternalSemaphoreInfo,
	VkExternalSemaphoreProperties* pExternalSemaphoreProperties);

VKAPI_ATTR void VKAPI_CALL vkGetDescriptorSetLayoutSupport(
	VkDevice                                    device,
	const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
	VkDescriptorSetLayoutSupport* pSupport);
#endif /* defined(VK_VERSION_1_1) */

#if defined(VK_EXT_debug_utils)
VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(
	VkDevice                                    device,
	const VkDebugUtilsObjectNameInfoEXT* pNameInfo);

VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectTagEXT(
	VkDevice                                    device,
	const VkDebugUtilsObjectTagInfoEXT* pTagInfo);

VKAPI_ATTR void VKAPI_CALL vkQueueBeginDebugUtilsLabelEXT(
	VkQueue                                     queue,
	const VkDebugUtilsLabelEXT* pLabelInfo);

VKAPI_ATTR void VKAPI_CALL vkQueueEndDebugUtilsLabelEXT(
	VkQueue                                     queue);

VKAPI_ATTR void VKAPI_CALL vkQueueInsertDebugUtilsLabelEXT(
	VkQueue                                     queue,
	const VkDebugUtilsLabelEXT* pLabelInfo);

VKAPI_ATTR void VKAPI_CALL vkCmdBeginDebugUtilsLabelEXT(
	VkCommandBuffer                             commandBuffer,
	const VkDebugUtilsLabelEXT* pLabelInfo);

VKAPI_ATTR void VKAPI_CALL vkCmdEndDebugUtilsLabelEXT(
	VkCommandBuffer                             commandBuffer);

VKAPI_ATTR void VKAPI_CALL vkCmdInsertDebugUtilsLabelEXT(
	VkCommandBuffer                             commandBuffer,
	const VkDebugUtilsLabelEXT* pLabelInfo);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
	VkInstance                                  instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pMessenger);

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
	VkInstance                                  instance,
	VkDebugUtilsMessengerEXT                    messenger,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR void VKAPI_CALL vkSubmitDebugUtilsMessageEXT(
	VkInstance                                  instance,
	VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData);
#endif /* defined(VK_EXT_debug_utils) */

#if defined(VK_KHR_surface)
VKAPI_ATTR void VKAPI_CALL vkDestroySurfaceKHR(
	VkInstance                                  instance,
	VkSurfaceKHR                                surface,
	const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceSupportKHR(
	VkPhysicalDevice                            physicalDevice,
	uint32_t                                    queueFamilyIndex,
	VkSurfaceKHR                                surface,
	VkBool32* pSupported);

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
	VkPhysicalDevice                            physicalDevice,
	VkSurfaceKHR                                surface,
	VkSurfaceCapabilitiesKHR* pSurfaceCapabilities);

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceFormatsKHR(
	VkPhysicalDevice                            physicalDevice,
	VkSurfaceKHR                                surface,
	uint32_t* pSurfaceFormatCount,
	VkSurfaceFormatKHR* pSurfaceFormats);

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModesKHR(
	VkPhysicalDevice                            physicalDevice,
	VkSurfaceKHR                                surface,
	uint32_t* pPresentModeCount,
	VkPresentModeKHR* pPresentModes);
#endif /* defined(VK_KHR_surface) */

#if defined(VK_AMD_draw_indirect_count)
extern PFN_vkCmdDrawIndexedIndirectCountAMD vkCmdDrawIndexedIndirectCountAMD;
extern PFN_vkCmdDrawIndirectCountAMD vkCmdDrawIndirectCountAMD;
#endif /* defined(VK_AMD_draw_indirect_count) */
#if defined(VK_AMD_shader_info)
extern PFN_vkGetShaderInfoAMD vkGetShaderInfoAMD;
#endif /* defined(VK_AMD_shader_info) */
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
extern PFN_vkGetAndroidHardwareBufferPropertiesANDROID vkGetAndroidHardwareBufferPropertiesANDROID;
extern PFN_vkGetMemoryAndroidHardwareBufferANDROID vkGetMemoryAndroidHardwareBufferANDROID;
#endif /* defined(VK_ANDROID_external_memory_android_hardware_buffer) */
#if defined(VK_EXT_acquire_xlib_display)
extern PFN_vkAcquireXlibDisplayEXT vkAcquireXlibDisplayEXT;
extern PFN_vkGetRandROutputDisplayEXT vkGetRandROutputDisplayEXT;
#endif /* defined(VK_EXT_acquire_xlib_display) */
#if defined(VK_EXT_buffer_device_address)
extern PFN_vkGetBufferDeviceAddressEXT vkGetBufferDeviceAddressEXT;
#endif /* defined(VK_EXT_buffer_device_address) */
#if defined(VK_EXT_calibrated_timestamps)
extern PFN_vkGetCalibratedTimestampsEXT vkGetCalibratedTimestampsEXT;
extern PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT vkGetPhysicalDeviceCalibrateableTimeDomainsEXT;
#endif /* defined(VK_EXT_calibrated_timestamps) */
#if defined(VK_EXT_conditional_rendering)
extern PFN_vkCmdBeginConditionalRenderingEXT vkCmdBeginConditionalRenderingEXT;
extern PFN_vkCmdEndConditionalRenderingEXT vkCmdEndConditionalRenderingEXT;
#endif /* defined(VK_EXT_conditional_rendering) */
#if defined(VK_EXT_debug_marker)
extern PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBeginEXT;
extern PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEndEXT;
extern PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsertEXT;
extern PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT;
extern PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTagEXT;
#endif /* defined(VK_EXT_debug_marker) */
#if defined(VK_EXT_debug_report)
extern PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
extern PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT;
extern PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
#endif /* defined(VK_EXT_debug_report) */
#if defined(VK_EXT_direct_mode_display)
extern PFN_vkReleaseDisplayEXT vkReleaseDisplayEXT;
#endif /* defined(VK_EXT_direct_mode_display) */
#if defined(VK_EXT_discard_rectangles)
extern PFN_vkCmdSetDiscardRectangleEXT vkCmdSetDiscardRectangleEXT;
#endif /* defined(VK_EXT_discard_rectangles) */
#if defined(VK_EXT_display_control)
extern PFN_vkDisplayPowerControlEXT vkDisplayPowerControlEXT;
extern PFN_vkGetSwapchainCounterEXT vkGetSwapchainCounterEXT;
extern PFN_vkRegisterDeviceEventEXT vkRegisterDeviceEventEXT;
extern PFN_vkRegisterDisplayEventEXT vkRegisterDisplayEventEXT;
#endif /* defined(VK_EXT_display_control) */
#if defined(VK_EXT_display_surface_counter)
extern PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT vkGetPhysicalDeviceSurfaceCapabilities2EXT;
#endif /* defined(VK_EXT_display_surface_counter) */
#if defined(VK_EXT_external_memory_host)
extern PFN_vkGetMemoryHostPointerPropertiesEXT vkGetMemoryHostPointerPropertiesEXT;
#endif /* defined(VK_EXT_external_memory_host) */
#if defined(VK_EXT_full_screen_exclusive)
extern PFN_vkAcquireFullScreenExclusiveModeEXT vkAcquireFullScreenExclusiveModeEXT;
extern PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT vkGetPhysicalDeviceSurfacePresentModes2EXT;
extern PFN_vkReleaseFullScreenExclusiveModeEXT vkReleaseFullScreenExclusiveModeEXT;
#endif /* defined(VK_EXT_full_screen_exclusive) */
#if defined(VK_EXT_hdr_metadata)
extern PFN_vkSetHdrMetadataEXT vkSetHdrMetadataEXT;
#endif /* defined(VK_EXT_hdr_metadata) */
#if defined(VK_EXT_headless_surface)
extern PFN_vkCreateHeadlessSurfaceEXT vkCreateHeadlessSurfaceEXT;
#endif /* defined(VK_EXT_headless_surface) */
#if defined(VK_EXT_host_query_reset)
extern PFN_vkResetQueryPoolEXT vkResetQueryPoolEXT;
#endif /* defined(VK_EXT_host_query_reset) */
#if defined(VK_EXT_image_drm_format_modifier)
extern PFN_vkGetImageDrmFormatModifierPropertiesEXT vkGetImageDrmFormatModifierPropertiesEXT;
#endif /* defined(VK_EXT_image_drm_format_modifier) */
#if defined(VK_EXT_metal_surface)
extern PFN_vkCreateMetalSurfaceEXT vkCreateMetalSurfaceEXT;
#endif /* defined(VK_EXT_metal_surface) */
#if defined(VK_EXT_sample_locations)
extern PFN_vkCmdSetSampleLocationsEXT vkCmdSetSampleLocationsEXT;
extern PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT vkGetPhysicalDeviceMultisamplePropertiesEXT;
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_EXT_transform_feedback)
extern PFN_vkCmdBeginQueryIndexedEXT vkCmdBeginQueryIndexedEXT;
extern PFN_vkCmdBeginTransformFeedbackEXT vkCmdBeginTransformFeedbackEXT;
extern PFN_vkCmdBindTransformFeedbackBuffersEXT vkCmdBindTransformFeedbackBuffersEXT;
extern PFN_vkCmdDrawIndirectByteCountEXT vkCmdDrawIndirectByteCountEXT;
extern PFN_vkCmdEndQueryIndexedEXT vkCmdEndQueryIndexedEXT;
extern PFN_vkCmdEndTransformFeedbackEXT vkCmdEndTransformFeedbackEXT;
#endif /* defined(VK_EXT_transform_feedback) */
#if defined(VK_EXT_validation_cache)
extern PFN_vkCreateValidationCacheEXT vkCreateValidationCacheEXT;
extern PFN_vkDestroyValidationCacheEXT vkDestroyValidationCacheEXT;
extern PFN_vkGetValidationCacheDataEXT vkGetValidationCacheDataEXT;
extern PFN_vkMergeValidationCachesEXT vkMergeValidationCachesEXT;
#endif /* defined(VK_EXT_validation_cache) */
#if defined(VK_FUCHSIA_imagepipe_surface)
extern PFN_vkCreateImagePipeSurfaceFUCHSIA vkCreateImagePipeSurfaceFUCHSIA;
#endif /* defined(VK_FUCHSIA_imagepipe_surface) */
#if defined(VK_GGP_stream_descriptor_surface)
extern PFN_vkCreateStreamDescriptorSurfaceGGP vkCreateStreamDescriptorSurfaceGGP;
#endif /* defined(VK_GGP_stream_descriptor_surface) */
#if defined(VK_GOOGLE_display_timing)
extern PFN_vkGetPastPresentationTimingGOOGLE vkGetPastPresentationTimingGOOGLE;
extern PFN_vkGetRefreshCycleDurationGOOGLE vkGetRefreshCycleDurationGOOGLE;
#endif /* defined(VK_GOOGLE_display_timing) */
#if defined(VK_KHR_android_surface)
extern PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR;
#endif /* defined(VK_KHR_android_surface) */
#if defined(VK_KHR_create_renderpass2)
extern PFN_vkCmdBeginRenderPass2KHR vkCmdBeginRenderPass2KHR;
extern PFN_vkCmdEndRenderPass2KHR vkCmdEndRenderPass2KHR;
extern PFN_vkCmdNextSubpass2KHR vkCmdNextSubpass2KHR;
extern PFN_vkCreateRenderPass2KHR vkCreateRenderPass2KHR;
#endif /* defined(VK_KHR_create_renderpass2) */
#if defined(VK_KHR_descriptor_update_template)
extern PFN_vkCreateDescriptorUpdateTemplateKHR vkCreateDescriptorUpdateTemplateKHR;
extern PFN_vkDestroyDescriptorUpdateTemplateKHR vkDestroyDescriptorUpdateTemplateKHR;
extern PFN_vkUpdateDescriptorSetWithTemplateKHR vkUpdateDescriptorSetWithTemplateKHR;
#endif /* defined(VK_KHR_descriptor_update_template) */
#if defined(VK_KHR_device_group)
extern PFN_vkCmdDispatchBaseKHR vkCmdDispatchBaseKHR;
extern PFN_vkCmdSetDeviceMaskKHR vkCmdSetDeviceMaskKHR;
extern PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR vkGetDeviceGroupPeerMemoryFeaturesKHR;
#endif /* defined(VK_KHR_device_group) */
#if defined(VK_KHR_device_group_creation)
extern PFN_vkEnumeratePhysicalDeviceGroupsKHR vkEnumeratePhysicalDeviceGroupsKHR;
#endif /* defined(VK_KHR_device_group_creation) */
#if defined(VK_KHR_display)
extern PFN_vkCreateDisplayModeKHR vkCreateDisplayModeKHR;
extern PFN_vkCreateDisplayPlaneSurfaceKHR vkCreateDisplayPlaneSurfaceKHR;
extern PFN_vkGetDisplayModePropertiesKHR vkGetDisplayModePropertiesKHR;
extern PFN_vkGetDisplayPlaneCapabilitiesKHR vkGetDisplayPlaneCapabilitiesKHR;
extern PFN_vkGetDisplayPlaneSupportedDisplaysKHR vkGetDisplayPlaneSupportedDisplaysKHR;
extern PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR vkGetPhysicalDeviceDisplayPlanePropertiesKHR;
extern PFN_vkGetPhysicalDeviceDisplayPropertiesKHR vkGetPhysicalDeviceDisplayPropertiesKHR;
#endif /* defined(VK_KHR_display) */
#if defined(VK_KHR_display_swapchain)
extern PFN_vkCreateSharedSwapchainsKHR vkCreateSharedSwapchainsKHR;
#endif /* defined(VK_KHR_display_swapchain) */
#if defined(VK_KHR_draw_indirect_count)
extern PFN_vkCmdDrawIndexedIndirectCountKHR vkCmdDrawIndexedIndirectCountKHR;
extern PFN_vkCmdDrawIndirectCountKHR vkCmdDrawIndirectCountKHR;
#endif /* defined(VK_KHR_draw_indirect_count) */
#if defined(VK_KHR_external_fence_capabilities)
extern PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR vkGetPhysicalDeviceExternalFencePropertiesKHR;
#endif /* defined(VK_KHR_external_fence_capabilities) */
#if defined(VK_KHR_external_fence_fd)
extern PFN_vkGetFenceFdKHR vkGetFenceFdKHR;
extern PFN_vkImportFenceFdKHR vkImportFenceFdKHR;
#endif /* defined(VK_KHR_external_fence_fd) */
#if defined(VK_KHR_external_fence_win32)
extern PFN_vkGetFenceWin32HandleKHR vkGetFenceWin32HandleKHR;
extern PFN_vkImportFenceWin32HandleKHR vkImportFenceWin32HandleKHR;
#endif /* defined(VK_KHR_external_fence_win32) */
#if defined(VK_KHR_external_memory_capabilities)
extern PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR vkGetPhysicalDeviceExternalBufferPropertiesKHR;
#endif /* defined(VK_KHR_external_memory_capabilities) */
#if defined(VK_KHR_external_memory_fd)
extern PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR;
extern PFN_vkGetMemoryFdPropertiesKHR vkGetMemoryFdPropertiesKHR;
#endif /* defined(VK_KHR_external_memory_fd) */
#if defined(VK_KHR_external_memory_win32)
extern PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR;
extern PFN_vkGetMemoryWin32HandlePropertiesKHR vkGetMemoryWin32HandlePropertiesKHR;
#endif /* defined(VK_KHR_external_memory_win32) */
#if defined(VK_KHR_external_semaphore_capabilities)
extern PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR vkGetPhysicalDeviceExternalSemaphorePropertiesKHR;
#endif /* defined(VK_KHR_external_semaphore_capabilities) */
#if defined(VK_KHR_external_semaphore_fd)
extern PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR;
extern PFN_vkImportSemaphoreFdKHR vkImportSemaphoreFdKHR;
#endif /* defined(VK_KHR_external_semaphore_fd) */
#if defined(VK_KHR_external_semaphore_win32)
extern PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR;
extern PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR;
#endif /* defined(VK_KHR_external_semaphore_win32) */
#if defined(VK_KHR_get_display_properties2)
extern PFN_vkGetDisplayModeProperties2KHR vkGetDisplayModeProperties2KHR;
extern PFN_vkGetDisplayPlaneCapabilities2KHR vkGetDisplayPlaneCapabilities2KHR;
extern PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR vkGetPhysicalDeviceDisplayPlaneProperties2KHR;
extern PFN_vkGetPhysicalDeviceDisplayProperties2KHR vkGetPhysicalDeviceDisplayProperties2KHR;
#endif /* defined(VK_KHR_get_display_properties2) */
#if defined(VK_KHR_get_memory_requirements2)
extern PFN_vkGetBufferMemoryRequirements2KHR vkGetBufferMemoryRequirements2KHR;
extern PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2KHR;
extern PFN_vkGetImageSparseMemoryRequirements2KHR vkGetImageSparseMemoryRequirements2KHR;
#endif /* defined(VK_KHR_get_memory_requirements2) */
#if defined(VK_KHR_get_physical_device_properties2)
extern PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2KHR;
extern PFN_vkGetPhysicalDeviceFormatProperties2KHR vkGetPhysicalDeviceFormatProperties2KHR;
extern PFN_vkGetPhysicalDeviceImageFormatProperties2KHR vkGetPhysicalDeviceImageFormatProperties2KHR;
extern PFN_vkGetPhysicalDeviceMemoryProperties2KHR vkGetPhysicalDeviceMemoryProperties2KHR;
extern PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR;
extern PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR vkGetPhysicalDeviceQueueFamilyProperties2KHR;
extern PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR vkGetPhysicalDeviceSparseImageFormatProperties2KHR;
#endif /* defined(VK_KHR_get_physical_device_properties2) */
#if defined(VK_KHR_get_surface_capabilities2)
extern PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR vkGetPhysicalDeviceSurfaceCapabilities2KHR;
extern PFN_vkGetPhysicalDeviceSurfaceFormats2KHR vkGetPhysicalDeviceSurfaceFormats2KHR;
#endif /* defined(VK_KHR_get_surface_capabilities2) */
#if defined(VK_KHR_maintenance1)
extern PFN_vkTrimCommandPoolKHR vkTrimCommandPoolKHR;
#endif /* defined(VK_KHR_maintenance1) */
#if defined(VK_KHR_maintenance3)
extern PFN_vkGetDescriptorSetLayoutSupportKHR vkGetDescriptorSetLayoutSupportKHR;
#endif /* defined(VK_KHR_maintenance3) */
#if defined(VK_KHR_push_descriptor)
extern PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR;
#endif /* defined(VK_KHR_push_descriptor) */
#if defined(VK_KHR_sampler_ycbcr_conversion)
extern PFN_vkCreateSamplerYcbcrConversionKHR vkCreateSamplerYcbcrConversionKHR;
extern PFN_vkDestroySamplerYcbcrConversionKHR vkDestroySamplerYcbcrConversionKHR;
#endif /* defined(VK_KHR_sampler_ycbcr_conversion) */
#if defined(VK_KHR_shared_presentable_image)
extern PFN_vkGetSwapchainStatusKHR vkGetSwapchainStatusKHR;
#endif /* defined(VK_KHR_shared_presentable_image) */
#if defined(VK_KHR_wayland_surface)
extern PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR;
extern PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR vkGetPhysicalDeviceWaylandPresentationSupportKHR;
#endif /* defined(VK_KHR_wayland_surface) */
#if defined(VK_KHR_win32_surface)
extern PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
extern PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR;
#endif /* defined(VK_KHR_win32_surface) */
#if defined(VK_KHR_xcb_surface)
extern PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
extern PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR vkGetPhysicalDeviceXcbPresentationSupportKHR;
#endif /* defined(VK_KHR_xcb_surface) */
#if defined(VK_KHR_xlib_surface)
extern PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
extern PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR vkGetPhysicalDeviceXlibPresentationSupportKHR;
#endif /* defined(VK_KHR_xlib_surface) */
#if defined(VK_MVK_ios_surface)
extern PFN_vkCreateIOSSurfaceMVK vkCreateIOSSurfaceMVK;
#endif /* defined(VK_MVK_ios_surface) */
#if defined(VK_MVK_macos_surface)
extern PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK;
#endif /* defined(VK_MVK_macos_surface) */
#if defined(VK_NN_vi_surface)
extern PFN_vkCreateViSurfaceNN vkCreateViSurfaceNN;
#endif /* defined(VK_NN_vi_surface) */
#if defined(VK_NVX_device_generated_commands)
extern PFN_vkCmdProcessCommandsNVX vkCmdProcessCommandsNVX;
extern PFN_vkCmdReserveSpaceForCommandsNVX vkCmdReserveSpaceForCommandsNVX;
extern PFN_vkCreateIndirectCommandsLayoutNVX vkCreateIndirectCommandsLayoutNVX;
extern PFN_vkCreateObjectTableNVX vkCreateObjectTableNVX;
extern PFN_vkDestroyIndirectCommandsLayoutNVX vkDestroyIndirectCommandsLayoutNVX;
extern PFN_vkDestroyObjectTableNVX vkDestroyObjectTableNVX;
extern PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX;
extern PFN_vkRegisterObjectsNVX vkRegisterObjectsNVX;
extern PFN_vkUnregisterObjectsNVX vkUnregisterObjectsNVX;
#endif /* defined(VK_NVX_device_generated_commands) */
#if defined(VK_NVX_image_view_handle)
extern PFN_vkGetImageViewHandleNVX vkGetImageViewHandleNVX;
#endif /* defined(VK_NVX_image_view_handle) */
#if defined(VK_NV_clip_space_w_scaling)
extern PFN_vkCmdSetViewportWScalingNV vkCmdSetViewportWScalingNV;
#endif /* defined(VK_NV_clip_space_w_scaling) */
#if defined(VK_NV_cooperative_matrix)
extern PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV vkGetPhysicalDeviceCooperativeMatrixPropertiesNV;
#endif /* defined(VK_NV_cooperative_matrix) */
#if defined(VK_NV_device_diagnostic_checkpoints)
extern PFN_vkCmdSetCheckpointNV vkCmdSetCheckpointNV;
extern PFN_vkGetQueueCheckpointDataNV vkGetQueueCheckpointDataNV;
#endif /* defined(VK_NV_device_diagnostic_checkpoints) */
#if defined(VK_NV_external_memory_capabilities)
extern PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV vkGetPhysicalDeviceExternalImageFormatPropertiesNV;
#endif /* defined(VK_NV_external_memory_capabilities) */
#if defined(VK_NV_external_memory_win32)
extern PFN_vkGetMemoryWin32HandleNV vkGetMemoryWin32HandleNV;
#endif /* defined(VK_NV_external_memory_win32) */
#if defined(VK_NV_mesh_shader)
extern PFN_vkCmdDrawMeshTasksIndirectCountNV vkCmdDrawMeshTasksIndirectCountNV;
extern PFN_vkCmdDrawMeshTasksIndirectNV vkCmdDrawMeshTasksIndirectNV;
extern PFN_vkCmdDrawMeshTasksNV vkCmdDrawMeshTasksNV;
#endif /* defined(VK_NV_mesh_shader) */
#if defined(VK_NV_ray_tracing)
extern PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV;
extern PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV;
extern PFN_vkCmdCopyAccelerationStructureNV vkCmdCopyAccelerationStructureNV;
extern PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;
extern PFN_vkCmdWriteAccelerationStructuresPropertiesNV vkCmdWriteAccelerationStructuresPropertiesNV;
extern PFN_vkCompileDeferredNV vkCompileDeferredNV;
extern PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV;
extern PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV;
extern PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV;
extern PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV;
extern PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV;
extern PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV;
#endif /* defined(VK_NV_ray_tracing) */
#if defined(VK_NV_scissor_exclusive)
extern PFN_vkCmdSetExclusiveScissorNV vkCmdSetExclusiveScissorNV;
#endif /* defined(VK_NV_scissor_exclusive) */
#if defined(VK_NV_shading_rate_image)
extern PFN_vkCmdBindShadingRateImageNV vkCmdBindShadingRateImageNV;
extern PFN_vkCmdSetCoarseSampleOrderNV vkCmdSetCoarseSampleOrderNV;
extern PFN_vkCmdSetViewportShadingRatePaletteNV vkCmdSetViewportShadingRatePaletteNV;
#endif /* defined(VK_NV_shading_rate_image) */
#if (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_device_group) && defined(VK_EXT_full_screen_exclusive))
extern PFN_vkGetDeviceGroupSurfacePresentModes2EXT vkGetDeviceGroupSurfacePresentModes2EXT;
#endif /* (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_device_group) && defined(VK_EXT_full_screen_exclusive)) */
#if (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1))
extern PFN_vkCmdPushDescriptorSetWithTemplateKHR vkCmdPushDescriptorSetWithTemplateKHR;
#endif /* (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) */
/* VOLK_GENERATE_PROTOTYPES_H */

#endif

/*
 * Copyright (c) 2018-2019 Arseny Kapoulkine
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/
