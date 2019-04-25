/* This file is part of volk library; see volk.h for version/license details */
#include "volk/volk.hpp"

#include <memory>

#ifdef _WIN32
#	include <windows.h>
#else
#	include <dlfcn.h>
#endif

namespace Volk
{
	struct FunctionPointers
	{
#ifdef __GNUC__
#	pragma GCC visibility push(hidden)
#endif

		/* VOLK_GENERATE_PROTOTYPES_C */

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
		PFN_vkCreateDevice vkCreateDevice;
		PFN_vkCreateEvent vkCreateEvent;
		PFN_vkCreateFence vkCreateFence;
		PFN_vkCreateFramebuffer vkCreateFramebuffer;
		PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
		PFN_vkCreateImage vkCreateImage;
		PFN_vkCreateImageView vkCreateImageView;
		PFN_vkCreateInstance vkCreateInstance;
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
		PFN_vkDestroyInstance vkDestroyInstance;
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
		PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
		PFN_vkEnumerateDeviceLayerProperties vkEnumerateDeviceLayerProperties;
		PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
		PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
		PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
		PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
		PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
		PFN_vkFreeDescriptorSets vkFreeDescriptorSets;
		PFN_vkFreeMemory vkFreeMemory;
		PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
		PFN_vkGetDeviceMemoryCommitment vkGetDeviceMemoryCommitment;
		PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
		PFN_vkGetDeviceQueue vkGetDeviceQueue;
		PFN_vkGetEventStatus vkGetEventStatus;
		PFN_vkGetFenceStatus vkGetFenceStatus;
		PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
		PFN_vkGetImageSparseMemoryRequirements vkGetImageSparseMemoryRequirements;
		PFN_vkGetImageSubresourceLayout vkGetImageSubresourceLayout;
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
		PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
		PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;
		PFN_vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceImageFormatProperties;
		PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
		PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
		PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
		PFN_vkGetPhysicalDeviceSparseImageFormatProperties vkGetPhysicalDeviceSparseImageFormatProperties;
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
		PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion;
		PFN_vkEnumeratePhysicalDeviceGroups vkEnumeratePhysicalDeviceGroups;
		PFN_vkGetBufferMemoryRequirements2 vkGetBufferMemoryRequirements2;
		PFN_vkGetDescriptorSetLayoutSupport vkGetDescriptorSetLayoutSupport;
		PFN_vkGetDeviceGroupPeerMemoryFeatures vkGetDeviceGroupPeerMemoryFeatures;
		PFN_vkGetDeviceQueue2 vkGetDeviceQueue2;
		PFN_vkGetImageMemoryRequirements2 vkGetImageMemoryRequirements2;
		PFN_vkGetImageSparseMemoryRequirements2 vkGetImageSparseMemoryRequirements2;
		PFN_vkGetPhysicalDeviceExternalBufferProperties vkGetPhysicalDeviceExternalBufferProperties;
		PFN_vkGetPhysicalDeviceExternalFenceProperties vkGetPhysicalDeviceExternalFenceProperties;
		PFN_vkGetPhysicalDeviceExternalSemaphoreProperties vkGetPhysicalDeviceExternalSemaphoreProperties;
		PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2;
		PFN_vkGetPhysicalDeviceFormatProperties2 vkGetPhysicalDeviceFormatProperties2;
		PFN_vkGetPhysicalDeviceImageFormatProperties2 vkGetPhysicalDeviceImageFormatProperties2;
		PFN_vkGetPhysicalDeviceMemoryProperties2 vkGetPhysicalDeviceMemoryProperties2;
		PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2;
		PFN_vkGetPhysicalDeviceQueueFamilyProperties2 vkGetPhysicalDeviceQueueFamilyProperties2;
		PFN_vkGetPhysicalDeviceSparseImageFormatProperties2 vkGetPhysicalDeviceSparseImageFormatProperties2;
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
#if defined(VK_EXT_acquire_xlib_display)
		PFN_vkAcquireXlibDisplayEXT vkAcquireXlibDisplayEXT;
		PFN_vkGetRandROutputDisplayEXT vkGetRandROutputDisplayEXT;
#endif /* defined(VK_EXT_acquire_xlib_display) */
#if defined(VK_EXT_buffer_device_address)
		PFN_vkGetBufferDeviceAddressEXT vkGetBufferDeviceAddressEXT;
#endif /* defined(VK_EXT_buffer_device_address) */
#if defined(VK_EXT_calibrated_timestamps)
		PFN_vkGetCalibratedTimestampsEXT vkGetCalibratedTimestampsEXT;
		PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT vkGetPhysicalDeviceCalibrateableTimeDomainsEXT;
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
#if defined(VK_EXT_debug_report)
		PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
		PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT;
		PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
#endif /* defined(VK_EXT_debug_report) */
#if defined(VK_EXT_debug_utils)
		PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT;
		PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT;
		PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT;
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
		PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXT;
		PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabelEXT;
		PFN_vkQueueInsertDebugUtilsLabelEXT vkQueueInsertDebugUtilsLabelEXT;
		PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
		PFN_vkSetDebugUtilsObjectTagEXT vkSetDebugUtilsObjectTagEXT;
		PFN_vkSubmitDebugUtilsMessageEXT vkSubmitDebugUtilsMessageEXT;
#endif /* defined(VK_EXT_debug_utils) */
#if defined(VK_EXT_direct_mode_display)
		PFN_vkReleaseDisplayEXT vkReleaseDisplayEXT;
#endif /* defined(VK_EXT_direct_mode_display) */
#if defined(VK_EXT_discard_rectangles)
		PFN_vkCmdSetDiscardRectangleEXT vkCmdSetDiscardRectangleEXT;
#endif /* defined(VK_EXT_discard_rectangles) */
#if defined(VK_EXT_display_control)
		PFN_vkDisplayPowerControlEXT vkDisplayPowerControlEXT;
		PFN_vkGetSwapchainCounterEXT vkGetSwapchainCounterEXT;
		PFN_vkRegisterDeviceEventEXT vkRegisterDeviceEventEXT;
		PFN_vkRegisterDisplayEventEXT vkRegisterDisplayEventEXT;
#endif /* defined(VK_EXT_display_control) */
#if defined(VK_EXT_display_surface_counter)
		PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT vkGetPhysicalDeviceSurfaceCapabilities2EXT;
#endif /* defined(VK_EXT_display_surface_counter) */
#if defined(VK_EXT_external_memory_host)
		PFN_vkGetMemoryHostPointerPropertiesEXT vkGetMemoryHostPointerPropertiesEXT;
#endif /* defined(VK_EXT_external_memory_host) */
#if defined(VK_EXT_full_screen_exclusive)
		PFN_vkAcquireFullScreenExclusiveModeEXT vkAcquireFullScreenExclusiveModeEXT;
		PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT vkGetPhysicalDeviceSurfacePresentModes2EXT;
		PFN_vkReleaseFullScreenExclusiveModeEXT vkReleaseFullScreenExclusiveModeEXT;
#endif /* defined(VK_EXT_full_screen_exclusive) */
#if defined(VK_EXT_hdr_metadata)
		PFN_vkSetHdrMetadataEXT vkSetHdrMetadataEXT;
#endif /* defined(VK_EXT_hdr_metadata) */
#if defined(VK_EXT_headless_surface)
		PFN_vkCreateHeadlessSurfaceEXT vkCreateHeadlessSurfaceEXT;
#endif /* defined(VK_EXT_headless_surface) */
#if defined(VK_EXT_host_query_reset)
		PFN_vkResetQueryPoolEXT vkResetQueryPoolEXT;
#endif /* defined(VK_EXT_host_query_reset) */
#if defined(VK_EXT_image_drm_format_modifier)
		PFN_vkGetImageDrmFormatModifierPropertiesEXT vkGetImageDrmFormatModifierPropertiesEXT;
#endif /* defined(VK_EXT_image_drm_format_modifier) */
#if defined(VK_EXT_metal_surface)
		PFN_vkCreateMetalSurfaceEXT vkCreateMetalSurfaceEXT;
#endif /* defined(VK_EXT_metal_surface) */
#if defined(VK_EXT_sample_locations)
		PFN_vkCmdSetSampleLocationsEXT vkCmdSetSampleLocationsEXT;
		PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT vkGetPhysicalDeviceMultisamplePropertiesEXT;
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
#if defined(VK_FUCHSIA_imagepipe_surface)
		PFN_vkCreateImagePipeSurfaceFUCHSIA vkCreateImagePipeSurfaceFUCHSIA;
#endif /* defined(VK_FUCHSIA_imagepipe_surface) */
#if defined(VK_GGP_stream_descriptor_surface)
		PFN_vkCreateStreamDescriptorSurfaceGGP vkCreateStreamDescriptorSurfaceGGP;
#endif /* defined(VK_GGP_stream_descriptor_surface) */
#if defined(VK_GOOGLE_display_timing)
		PFN_vkGetPastPresentationTimingGOOGLE vkGetPastPresentationTimingGOOGLE;
		PFN_vkGetRefreshCycleDurationGOOGLE vkGetRefreshCycleDurationGOOGLE;
#endif /* defined(VK_GOOGLE_display_timing) */
#if defined(VK_KHR_android_surface)
		PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR;
#endif /* defined(VK_KHR_android_surface) */
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
#if defined(VK_KHR_device_group_creation)
		PFN_vkEnumeratePhysicalDeviceGroupsKHR vkEnumeratePhysicalDeviceGroupsKHR;
#endif /* defined(VK_KHR_device_group_creation) */
#if defined(VK_KHR_display)
		PFN_vkCreateDisplayModeKHR vkCreateDisplayModeKHR;
		PFN_vkCreateDisplayPlaneSurfaceKHR vkCreateDisplayPlaneSurfaceKHR;
		PFN_vkGetDisplayModePropertiesKHR vkGetDisplayModePropertiesKHR;
		PFN_vkGetDisplayPlaneCapabilitiesKHR vkGetDisplayPlaneCapabilitiesKHR;
		PFN_vkGetDisplayPlaneSupportedDisplaysKHR vkGetDisplayPlaneSupportedDisplaysKHR;
		PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR vkGetPhysicalDeviceDisplayPlanePropertiesKHR;
		PFN_vkGetPhysicalDeviceDisplayPropertiesKHR vkGetPhysicalDeviceDisplayPropertiesKHR;
#endif /* defined(VK_KHR_display) */
#if defined(VK_KHR_display_swapchain)
		PFN_vkCreateSharedSwapchainsKHR vkCreateSharedSwapchainsKHR;
#endif /* defined(VK_KHR_display_swapchain) */
#if defined(VK_KHR_draw_indirect_count)
		PFN_vkCmdDrawIndexedIndirectCountKHR vkCmdDrawIndexedIndirectCountKHR;
		PFN_vkCmdDrawIndirectCountKHR vkCmdDrawIndirectCountKHR;
#endif /* defined(VK_KHR_draw_indirect_count) */
#if defined(VK_KHR_external_fence_capabilities)
		PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR vkGetPhysicalDeviceExternalFencePropertiesKHR;
#endif /* defined(VK_KHR_external_fence_capabilities) */
#if defined(VK_KHR_external_fence_fd)
		PFN_vkGetFenceFdKHR vkGetFenceFdKHR;
		PFN_vkImportFenceFdKHR vkImportFenceFdKHR;
#endif /* defined(VK_KHR_external_fence_fd) */
#if defined(VK_KHR_external_fence_win32)
		PFN_vkGetFenceWin32HandleKHR vkGetFenceWin32HandleKHR;
		PFN_vkImportFenceWin32HandleKHR vkImportFenceWin32HandleKHR;
#endif /* defined(VK_KHR_external_fence_win32) */
#if defined(VK_KHR_external_memory_capabilities)
		PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR vkGetPhysicalDeviceExternalBufferPropertiesKHR;
#endif /* defined(VK_KHR_external_memory_capabilities) */
#if defined(VK_KHR_external_memory_fd)
		PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR;
		PFN_vkGetMemoryFdPropertiesKHR vkGetMemoryFdPropertiesKHR;
#endif /* defined(VK_KHR_external_memory_fd) */
#if defined(VK_KHR_external_memory_win32)
		PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR;
		PFN_vkGetMemoryWin32HandlePropertiesKHR vkGetMemoryWin32HandlePropertiesKHR;
#endif /* defined(VK_KHR_external_memory_win32) */
#if defined(VK_KHR_external_semaphore_capabilities)
		PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR vkGetPhysicalDeviceExternalSemaphorePropertiesKHR;
#endif /* defined(VK_KHR_external_semaphore_capabilities) */
#if defined(VK_KHR_external_semaphore_fd)
		PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR;
		PFN_vkImportSemaphoreFdKHR vkImportSemaphoreFdKHR;
#endif /* defined(VK_KHR_external_semaphore_fd) */
#if defined(VK_KHR_external_semaphore_win32)
		PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR;
		PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR;
#endif /* defined(VK_KHR_external_semaphore_win32) */
#if defined(VK_KHR_get_display_properties2)
		PFN_vkGetDisplayModeProperties2KHR vkGetDisplayModeProperties2KHR;
		PFN_vkGetDisplayPlaneCapabilities2KHR vkGetDisplayPlaneCapabilities2KHR;
		PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR vkGetPhysicalDeviceDisplayPlaneProperties2KHR;
		PFN_vkGetPhysicalDeviceDisplayProperties2KHR vkGetPhysicalDeviceDisplayProperties2KHR;
#endif /* defined(VK_KHR_get_display_properties2) */
#if defined(VK_KHR_get_memory_requirements2)
		PFN_vkGetBufferMemoryRequirements2KHR vkGetBufferMemoryRequirements2KHR;
		PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2KHR;
		PFN_vkGetImageSparseMemoryRequirements2KHR vkGetImageSparseMemoryRequirements2KHR;
#endif /* defined(VK_KHR_get_memory_requirements2) */
#if defined(VK_KHR_get_physical_device_properties2)
		PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2KHR;
		PFN_vkGetPhysicalDeviceFormatProperties2KHR vkGetPhysicalDeviceFormatProperties2KHR;
		PFN_vkGetPhysicalDeviceImageFormatProperties2KHR vkGetPhysicalDeviceImageFormatProperties2KHR;
		PFN_vkGetPhysicalDeviceMemoryProperties2KHR vkGetPhysicalDeviceMemoryProperties2KHR;
		PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR;
		PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR vkGetPhysicalDeviceQueueFamilyProperties2KHR;
		PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR vkGetPhysicalDeviceSparseImageFormatProperties2KHR;
#endif /* defined(VK_KHR_get_physical_device_properties2) */
#if defined(VK_KHR_get_surface_capabilities2)
		PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR vkGetPhysicalDeviceSurfaceCapabilities2KHR;
		PFN_vkGetPhysicalDeviceSurfaceFormats2KHR vkGetPhysicalDeviceSurfaceFormats2KHR;
#endif /* defined(VK_KHR_get_surface_capabilities2) */
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
#if defined(VK_KHR_surface)
		PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
		PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
		PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
		PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
		PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
#endif /* defined(VK_KHR_surface) */
#if defined(VK_KHR_swapchain)
		PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
		PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
		PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
		PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
		PFN_vkQueuePresentKHR vkQueuePresentKHR;
#endif /* defined(VK_KHR_swapchain) */
#if defined(VK_KHR_wayland_surface)
		PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR;
		PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR vkGetPhysicalDeviceWaylandPresentationSupportKHR;
#endif /* defined(VK_KHR_wayland_surface) */
#if defined(VK_KHR_win32_surface)
		PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
		PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR;
#endif /* defined(VK_KHR_win32_surface) */
#if defined(VK_KHR_xcb_surface)
		PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
		PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR vkGetPhysicalDeviceXcbPresentationSupportKHR;
#endif /* defined(VK_KHR_xcb_surface) */
#if defined(VK_KHR_xlib_surface)
		PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
		PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR vkGetPhysicalDeviceXlibPresentationSupportKHR;
#endif /* defined(VK_KHR_xlib_surface) */
#if defined(VK_MVK_ios_surface)
		PFN_vkCreateIOSSurfaceMVK vkCreateIOSSurfaceMVK;
#endif /* defined(VK_MVK_ios_surface) */
#if defined(VK_MVK_macos_surface)
		PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK;
#endif /* defined(VK_MVK_macos_surface) */
#if defined(VK_NN_vi_surface)
		PFN_vkCreateViSurfaceNN vkCreateViSurfaceNN;
#endif /* defined(VK_NN_vi_surface) */
#if defined(VK_NVX_device_generated_commands)
		PFN_vkCmdProcessCommandsNVX vkCmdProcessCommandsNVX;
		PFN_vkCmdReserveSpaceForCommandsNVX vkCmdReserveSpaceForCommandsNVX;
		PFN_vkCreateIndirectCommandsLayoutNVX vkCreateIndirectCommandsLayoutNVX;
		PFN_vkCreateObjectTableNVX vkCreateObjectTableNVX;
		PFN_vkDestroyIndirectCommandsLayoutNVX vkDestroyIndirectCommandsLayoutNVX;
		PFN_vkDestroyObjectTableNVX vkDestroyObjectTableNVX;
		PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX;
		PFN_vkRegisterObjectsNVX vkRegisterObjectsNVX;
		PFN_vkUnregisterObjectsNVX vkUnregisterObjectsNVX;
#endif /* defined(VK_NVX_device_generated_commands) */
#if defined(VK_NVX_image_view_handle)
		PFN_vkGetImageViewHandleNVX vkGetImageViewHandleNVX;
#endif /* defined(VK_NVX_image_view_handle) */
#if defined(VK_NV_clip_space_w_scaling)
		PFN_vkCmdSetViewportWScalingNV vkCmdSetViewportWScalingNV;
#endif /* defined(VK_NV_clip_space_w_scaling) */
#if defined(VK_NV_cooperative_matrix)
		PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV vkGetPhysicalDeviceCooperativeMatrixPropertiesNV;
#endif /* defined(VK_NV_cooperative_matrix) */
#if defined(VK_NV_device_diagnostic_checkpoints)
		PFN_vkCmdSetCheckpointNV vkCmdSetCheckpointNV;
		PFN_vkGetQueueCheckpointDataNV vkGetQueueCheckpointDataNV;
#endif /* defined(VK_NV_device_diagnostic_checkpoints) */
#if defined(VK_NV_external_memory_capabilities)
		PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV vkGetPhysicalDeviceExternalImageFormatPropertiesNV;
#endif /* defined(VK_NV_external_memory_capabilities) */
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
		PFN_vkGetPhysicalDevicePresentRectanglesKHR vkGetPhysicalDevicePresentRectanglesKHR;
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
		PFN_vkAcquireNextImage2KHR vkAcquireNextImage2KHR;
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
		/* VOLK_GENERATE_PROTOTYPES_C */

#ifdef __GNUC__
#	pragma GCC visibility pop
#endif
	};

	static std::unique_ptr<FunctionPointers> funcPtrs = nullptr;
}

namespace Volk
{
	static void volkGenLoadLoader(void* context, PFN_vkVoidFunction(*load)(void*, const char*));
	static void volkGenLoadInstance(void* context, PFN_vkVoidFunction(*load)(void*, const char*));
	static void volkGenLoadDevice(void* context, PFN_vkVoidFunction(*load)(void*, const char*));
	static void volkGenLoadDeviceTable(Volk::DeviceTable* table, void* context, PFN_vkVoidFunction(*load)(void*, const char*));

	static PFN_vkVoidFunction vkGetInstanceProcAddrStub(void* context, const char* name)
	{
		return funcPtrs->vkGetInstanceProcAddr((VkInstance)context, name);
	}

	static PFN_vkVoidFunction vkGetDeviceProcAddrStub(void* context, const char* name)
	{
		return funcPtrs->vkGetDeviceProcAddr((VkDevice)context, name);
	}
}

VkResult Volk::Initialize(void)
{
#if defined(_WIN32)
	HMODULE module = LoadLibraryA("vulkan-1.dll");
	if (!module)
		return VK_ERROR_INITIALIZATION_FAILED;

	funcPtrs = std::make_unique<FunctionPointers>();

	// note: function pointer is cast through void function pointer to silence cast-function-type warning on gcc8
	funcPtrs->vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)(void(*)(void))GetProcAddress(module, "vkGetInstanceProcAddr");
#elif defined(__APPLE__)
	void* module = dlopen("libvulkan.dylib", RTLD_NOW | RTLD_LOCAL);
	if (!module)
		module = dlopen("libvulkan.dylib.1", RTLD_NOW | RTLD_LOCAL);
	if (!module)
		module = dlopen("libMoltenVK.dylib", RTLD_NOW | RTLD_LOCAL);
	if (!module)
		return VK_ERROR_INITIALIZATION_FAILED;

	vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(module, "vkGetInstanceProcAddr");
#else
	void* module = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
	if (!module)
		module = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
	if (!module)
		return VK_ERROR_INITIALIZATION_FAILED;

	vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(module, "vkGetInstanceProcAddr");
#endif

	volkGenLoadLoader(NULL, vkGetInstanceProcAddrStub);

	return VK_SUCCESS;
}

void Volk::InitializeCustom(PFN_vkGetInstanceProcAddr handler)
{
	funcPtrs = std::make_unique<FunctionPointers>();

	funcPtrs->vkGetInstanceProcAddr = handler;

	volkGenLoadLoader(NULL, vkGetInstanceProcAddrStub);
}

uint32_t Volk::GetInstanceVersion(void)
{
#if defined(VK_VERSION_1_1)
	uint32_t apiVersion = 0;
	if (funcPtrs->vkEnumerateInstanceVersion && funcPtrs->vkEnumerateInstanceVersion(&apiVersion) == VK_SUCCESS)
		return apiVersion;
#endif

	if (funcPtrs->vkCreateInstance)
		return VK_API_VERSION_1_0;

	return 0;
}

void Volk::LoadInstance(VkInstance instance)
{
	volkGenLoadInstance(instance, vkGetInstanceProcAddrStub);
	volkGenLoadDevice(instance, vkGetInstanceProcAddrStub);
}

void Volk::LoadDevice(VkDevice device)
{
	volkGenLoadDevice(device, vkGetDeviceProcAddrStub);
}

namespace Volk
{
	void LoadDeviceTable(Volk::DeviceTable* table, VkDevice device)
	{
		volkGenLoadDeviceTable(table, device, vkGetDeviceProcAddrStub);
	}
}


static void Volk::volkGenLoadLoader(void* context, PFN_vkVoidFunction (*load)(void*, const char*))
{
	/* VOLK_GENERATE_LOAD_LOADER */
#if defined(VK_VERSION_1_0)
	funcPtrs->vkCreateInstance = (PFN_vkCreateInstance)load(context, "vkCreateInstance");
	funcPtrs->vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)load(context, "vkEnumerateInstanceExtensionProperties");
	funcPtrs->vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)load(context, "vkEnumerateInstanceLayerProperties");
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
	funcPtrs->vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)load(context, "vkEnumerateInstanceVersion");
#endif /* defined(VK_VERSION_1_1) */
	/* VOLK_GENERATE_LOAD_LOADER */
}

static void Volk::volkGenLoadInstance(void* context, PFN_vkVoidFunction (*load)(void*, const char*))
{
	/* VOLK_GENERATE_LOAD_INSTANCE */
#if defined(VK_VERSION_1_0)
	funcPtrs->vkCreateDevice = (PFN_vkCreateDevice)load(context, "vkCreateDevice");
	funcPtrs->vkDestroyInstance = (PFN_vkDestroyInstance)load(context, "vkDestroyInstance");
	funcPtrs->vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)load(context, "vkEnumerateDeviceExtensionProperties");
	funcPtrs->vkEnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties)load(context, "vkEnumerateDeviceLayerProperties");
	funcPtrs->vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)load(context, "vkEnumeratePhysicalDevices");
	funcPtrs->vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)load(context, "vkGetDeviceProcAddr");
	funcPtrs->vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)load(context, "vkGetPhysicalDeviceFeatures");
	funcPtrs->vkGetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties)load(context, "vkGetPhysicalDeviceFormatProperties");
	funcPtrs->vkGetPhysicalDeviceImageFormatProperties = (PFN_vkGetPhysicalDeviceImageFormatProperties)load(context, "vkGetPhysicalDeviceImageFormatProperties");
	funcPtrs->vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)load(context, "vkGetPhysicalDeviceMemoryProperties");
	funcPtrs->vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)load(context, "vkGetPhysicalDeviceProperties");
	funcPtrs->vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)load(context, "vkGetPhysicalDeviceQueueFamilyProperties");
	funcPtrs->vkGetPhysicalDeviceSparseImageFormatProperties = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties)load(context, "vkGetPhysicalDeviceSparseImageFormatProperties");
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
	funcPtrs->vkEnumeratePhysicalDeviceGroups = (PFN_vkEnumeratePhysicalDeviceGroups)load(context, "vkEnumeratePhysicalDeviceGroups");
	funcPtrs->vkGetPhysicalDeviceExternalBufferProperties = (PFN_vkGetPhysicalDeviceExternalBufferProperties)load(context, "vkGetPhysicalDeviceExternalBufferProperties");
	funcPtrs->vkGetPhysicalDeviceExternalFenceProperties = (PFN_vkGetPhysicalDeviceExternalFenceProperties)load(context, "vkGetPhysicalDeviceExternalFenceProperties");
	funcPtrs->vkGetPhysicalDeviceExternalSemaphoreProperties = (PFN_vkGetPhysicalDeviceExternalSemaphoreProperties)load(context, "vkGetPhysicalDeviceExternalSemaphoreProperties");
	funcPtrs->vkGetPhysicalDeviceFeatures2 = (PFN_vkGetPhysicalDeviceFeatures2)load(context, "vkGetPhysicalDeviceFeatures2");
	funcPtrs->vkGetPhysicalDeviceFormatProperties2 = (PFN_vkGetPhysicalDeviceFormatProperties2)load(context, "vkGetPhysicalDeviceFormatProperties2");
	funcPtrs->vkGetPhysicalDeviceImageFormatProperties2 = (PFN_vkGetPhysicalDeviceImageFormatProperties2)load(context, "vkGetPhysicalDeviceImageFormatProperties2");
	funcPtrs->vkGetPhysicalDeviceMemoryProperties2 = (PFN_vkGetPhysicalDeviceMemoryProperties2)load(context, "vkGetPhysicalDeviceMemoryProperties2");
	funcPtrs->vkGetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2)load(context, "vkGetPhysicalDeviceProperties2");
	funcPtrs->vkGetPhysicalDeviceQueueFamilyProperties2 = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2)load(context, "vkGetPhysicalDeviceQueueFamilyProperties2");
	funcPtrs->vkGetPhysicalDeviceSparseImageFormatProperties2 = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties2)load(context, "vkGetPhysicalDeviceSparseImageFormatProperties2");
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_EXT_acquire_xlib_display)
	vkAcquireXlibDisplayEXT = (PFN_vkAcquireXlibDisplayEXT)load(context, "vkAcquireXlibDisplayEXT");
	vkGetRandROutputDisplayEXT = (PFN_vkGetRandROutputDisplayEXT)load(context, "vkGetRandROutputDisplayEXT");
#endif /* defined(VK_EXT_acquire_xlib_display) */
#if defined(VK_EXT_calibrated_timestamps)
	vkGetPhysicalDeviceCalibrateableTimeDomainsEXT = (PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)load(context, "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT");
#endif /* defined(VK_EXT_calibrated_timestamps) */
#if defined(VK_EXT_debug_report)
	vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)load(context, "vkCreateDebugReportCallbackEXT");
	vkDebugReportMessageEXT = (PFN_vkDebugReportMessageEXT)load(context, "vkDebugReportMessageEXT");
	vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)load(context, "vkDestroyDebugReportCallbackEXT");
#endif /* defined(VK_EXT_debug_report) */
#if defined(VK_EXT_debug_utils)
	funcPtrs->vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)load(context, "vkCreateDebugUtilsMessengerEXT");
	funcPtrs->vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)load(context, "vkDestroyDebugUtilsMessengerEXT");
	funcPtrs->vkSubmitDebugUtilsMessageEXT = (PFN_vkSubmitDebugUtilsMessageEXT)load(context, "vkSubmitDebugUtilsMessageEXT");
#endif /* defined(VK_EXT_debug_utils) */
#if defined(VK_EXT_direct_mode_display)
	vkReleaseDisplayEXT = (PFN_vkReleaseDisplayEXT)load(context, "vkReleaseDisplayEXT");
#endif /* defined(VK_EXT_direct_mode_display) */
#if defined(VK_EXT_display_surface_counter)
	vkGetPhysicalDeviceSurfaceCapabilities2EXT = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT)load(context, "vkGetPhysicalDeviceSurfaceCapabilities2EXT");
#endif /* defined(VK_EXT_display_surface_counter) */
#if defined(VK_EXT_full_screen_exclusive)
	vkGetPhysicalDeviceSurfacePresentModes2EXT = (PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT)load(context, "vkGetPhysicalDeviceSurfacePresentModes2EXT");
#endif /* defined(VK_EXT_full_screen_exclusive) */
#if defined(VK_EXT_headless_surface)
	vkCreateHeadlessSurfaceEXT = (PFN_vkCreateHeadlessSurfaceEXT)load(context, "vkCreateHeadlessSurfaceEXT");
#endif /* defined(VK_EXT_headless_surface) */
#if defined(VK_EXT_metal_surface)
	vkCreateMetalSurfaceEXT = (PFN_vkCreateMetalSurfaceEXT)load(context, "vkCreateMetalSurfaceEXT");
#endif /* defined(VK_EXT_metal_surface) */
#if defined(VK_EXT_sample_locations)
	vkGetPhysicalDeviceMultisamplePropertiesEXT = (PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT)load(context, "vkGetPhysicalDeviceMultisamplePropertiesEXT");
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_FUCHSIA_imagepipe_surface)
	vkCreateImagePipeSurfaceFUCHSIA = (PFN_vkCreateImagePipeSurfaceFUCHSIA)load(context, "vkCreateImagePipeSurfaceFUCHSIA");
#endif /* defined(VK_FUCHSIA_imagepipe_surface) */
#if defined(VK_GGP_stream_descriptor_surface)
	vkCreateStreamDescriptorSurfaceGGP = (PFN_vkCreateStreamDescriptorSurfaceGGP)load(context, "vkCreateStreamDescriptorSurfaceGGP");
#endif /* defined(VK_GGP_stream_descriptor_surface) */
#if defined(VK_KHR_android_surface)
	vkCreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR)load(context, "vkCreateAndroidSurfaceKHR");
#endif /* defined(VK_KHR_android_surface) */
#if defined(VK_KHR_device_group_creation)
	vkEnumeratePhysicalDeviceGroupsKHR = (PFN_vkEnumeratePhysicalDeviceGroupsKHR)load(context, "vkEnumeratePhysicalDeviceGroupsKHR");
#endif /* defined(VK_KHR_device_group_creation) */
#if defined(VK_KHR_display)
	funcPtrs->vkCreateDisplayModeKHR = (PFN_vkCreateDisplayModeKHR)load(context, "vkCreateDisplayModeKHR");
	funcPtrs->vkCreateDisplayPlaneSurfaceKHR = (PFN_vkCreateDisplayPlaneSurfaceKHR)load(context, "vkCreateDisplayPlaneSurfaceKHR");
	funcPtrs->vkGetDisplayModePropertiesKHR = (PFN_vkGetDisplayModePropertiesKHR)load(context, "vkGetDisplayModePropertiesKHR");
	funcPtrs->vkGetDisplayPlaneCapabilitiesKHR = (PFN_vkGetDisplayPlaneCapabilitiesKHR)load(context, "vkGetDisplayPlaneCapabilitiesKHR");
	funcPtrs->vkGetDisplayPlaneSupportedDisplaysKHR = (PFN_vkGetDisplayPlaneSupportedDisplaysKHR)load(context, "vkGetDisplayPlaneSupportedDisplaysKHR");
	funcPtrs->vkGetPhysicalDeviceDisplayPlanePropertiesKHR = (PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR)load(context, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR");
	funcPtrs->vkGetPhysicalDeviceDisplayPropertiesKHR = (PFN_vkGetPhysicalDeviceDisplayPropertiesKHR)load(context, "vkGetPhysicalDeviceDisplayPropertiesKHR");
#endif /* defined(VK_KHR_display) */
#if defined(VK_KHR_external_fence_capabilities)
	vkGetPhysicalDeviceExternalFencePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR)load(context, "vkGetPhysicalDeviceExternalFencePropertiesKHR");
#endif /* defined(VK_KHR_external_fence_capabilities) */
#if defined(VK_KHR_external_memory_capabilities)
	vkGetPhysicalDeviceExternalBufferPropertiesKHR = (PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR)load(context, "vkGetPhysicalDeviceExternalBufferPropertiesKHR");
#endif /* defined(VK_KHR_external_memory_capabilities) */
#if defined(VK_KHR_external_semaphore_capabilities)
	vkGetPhysicalDeviceExternalSemaphorePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR)load(context, "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR");
#endif /* defined(VK_KHR_external_semaphore_capabilities) */
#if defined(VK_KHR_get_display_properties2)
	vkGetDisplayModeProperties2KHR = (PFN_vkGetDisplayModeProperties2KHR)load(context, "vkGetDisplayModeProperties2KHR");
	vkGetDisplayPlaneCapabilities2KHR = (PFN_vkGetDisplayPlaneCapabilities2KHR)load(context, "vkGetDisplayPlaneCapabilities2KHR");
	vkGetPhysicalDeviceDisplayPlaneProperties2KHR = (PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR)load(context, "vkGetPhysicalDeviceDisplayPlaneProperties2KHR");
	vkGetPhysicalDeviceDisplayProperties2KHR = (PFN_vkGetPhysicalDeviceDisplayProperties2KHR)load(context, "vkGetPhysicalDeviceDisplayProperties2KHR");
#endif /* defined(VK_KHR_get_display_properties2) */
#if defined(VK_KHR_get_physical_device_properties2)
	vkGetPhysicalDeviceFeatures2KHR = (PFN_vkGetPhysicalDeviceFeatures2KHR)load(context, "vkGetPhysicalDeviceFeatures2KHR");
	vkGetPhysicalDeviceFormatProperties2KHR = (PFN_vkGetPhysicalDeviceFormatProperties2KHR)load(context, "vkGetPhysicalDeviceFormatProperties2KHR");
	vkGetPhysicalDeviceImageFormatProperties2KHR = (PFN_vkGetPhysicalDeviceImageFormatProperties2KHR)load(context, "vkGetPhysicalDeviceImageFormatProperties2KHR");
	vkGetPhysicalDeviceMemoryProperties2KHR = (PFN_vkGetPhysicalDeviceMemoryProperties2KHR)load(context, "vkGetPhysicalDeviceMemoryProperties2KHR");
	vkGetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR)load(context, "vkGetPhysicalDeviceProperties2KHR");
	vkGetPhysicalDeviceQueueFamilyProperties2KHR = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR)load(context, "vkGetPhysicalDeviceQueueFamilyProperties2KHR");
	vkGetPhysicalDeviceSparseImageFormatProperties2KHR = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR)load(context, "vkGetPhysicalDeviceSparseImageFormatProperties2KHR");
#endif /* defined(VK_KHR_get_physical_device_properties2) */
#if defined(VK_KHR_get_surface_capabilities2)
	vkGetPhysicalDeviceSurfaceCapabilities2KHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR)load(context, "vkGetPhysicalDeviceSurfaceCapabilities2KHR");
	vkGetPhysicalDeviceSurfaceFormats2KHR = (PFN_vkGetPhysicalDeviceSurfaceFormats2KHR)load(context, "vkGetPhysicalDeviceSurfaceFormats2KHR");
#endif /* defined(VK_KHR_get_surface_capabilities2) */
#if defined(VK_KHR_surface)
	funcPtrs->vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)load(context, "vkDestroySurfaceKHR");
	funcPtrs->vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)load(context, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	funcPtrs->vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)load(context, "vkGetPhysicalDeviceSurfaceFormatsKHR");
	funcPtrs->vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)load(context, "vkGetPhysicalDeviceSurfacePresentModesKHR");
	funcPtrs->vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)load(context, "vkGetPhysicalDeviceSurfaceSupportKHR");
#endif /* defined(VK_KHR_surface) */
#if defined(VK_KHR_wayland_surface)
	vkCreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)load(context, "vkCreateWaylandSurfaceKHR");
	vkGetPhysicalDeviceWaylandPresentationSupportKHR = (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR)load(context, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
#endif /* defined(VK_KHR_wayland_surface) */
#if defined(VK_KHR_win32_surface)
	vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)load(context, "vkCreateWin32SurfaceKHR");
	vkGetPhysicalDeviceWin32PresentationSupportKHR = (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR)load(context, "vkGetPhysicalDeviceWin32PresentationSupportKHR");
#endif /* defined(VK_KHR_win32_surface) */
#if defined(VK_KHR_xcb_surface)
	vkCreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)load(context, "vkCreateXcbSurfaceKHR");
	vkGetPhysicalDeviceXcbPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR)load(context, "vkGetPhysicalDeviceXcbPresentationSupportKHR");
#endif /* defined(VK_KHR_xcb_surface) */
#if defined(VK_KHR_xlib_surface)
	vkCreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)load(context, "vkCreateXlibSurfaceKHR");
	vkGetPhysicalDeviceXlibPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)load(context, "vkGetPhysicalDeviceXlibPresentationSupportKHR");
#endif /* defined(VK_KHR_xlib_surface) */
#if defined(VK_MVK_ios_surface)
	vkCreateIOSSurfaceMVK = (PFN_vkCreateIOSSurfaceMVK)load(context, "vkCreateIOSSurfaceMVK");
#endif /* defined(VK_MVK_ios_surface) */
#if defined(VK_MVK_macos_surface)
	vkCreateMacOSSurfaceMVK = (PFN_vkCreateMacOSSurfaceMVK)load(context, "vkCreateMacOSSurfaceMVK");
#endif /* defined(VK_MVK_macos_surface) */
#if defined(VK_NN_vi_surface)
	vkCreateViSurfaceNN = (PFN_vkCreateViSurfaceNN)load(context, "vkCreateViSurfaceNN");
#endif /* defined(VK_NN_vi_surface) */
#if defined(VK_NVX_device_generated_commands)
	funcPtrs->vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX = (PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX)load(context, "vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX");
#endif /* defined(VK_NVX_device_generated_commands) */
#if defined(VK_NV_cooperative_matrix)
	vkGetPhysicalDeviceCooperativeMatrixPropertiesNV = (PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV)load(context, "vkGetPhysicalDeviceCooperativeMatrixPropertiesNV");
#endif /* defined(VK_NV_cooperative_matrix) */
#if defined(VK_NV_external_memory_capabilities)
	vkGetPhysicalDeviceExternalImageFormatPropertiesNV = (PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV)load(context, "vkGetPhysicalDeviceExternalImageFormatPropertiesNV");
#endif /* defined(VK_NV_external_memory_capabilities) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	funcPtrs->vkGetPhysicalDevicePresentRectanglesKHR = (PFN_vkGetPhysicalDevicePresentRectanglesKHR)load(context, "vkGetPhysicalDevicePresentRectanglesKHR");
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
	/* VOLK_GENERATE_LOAD_INSTANCE */
}

static void Volk::volkGenLoadDevice(void* context, PFN_vkVoidFunction (*load)(void*, const char*))
{
	/* VOLK_GENERATE_LOAD_DEVICE */
#if defined(VK_VERSION_1_0)
	funcPtrs->vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)load(context, "vkAllocateCommandBuffers");
	funcPtrs->vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)load(context, "vkAllocateDescriptorSets");
	funcPtrs->vkAllocateMemory = (PFN_vkAllocateMemory)load(context, "vkAllocateMemory");
	funcPtrs->vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)load(context, "vkBeginCommandBuffer");
	funcPtrs->vkBindBufferMemory = (PFN_vkBindBufferMemory)load(context, "vkBindBufferMemory");
	funcPtrs->vkBindImageMemory = (PFN_vkBindImageMemory)load(context, "vkBindImageMemory");
	funcPtrs->vkCmdBeginQuery = (PFN_vkCmdBeginQuery)load(context, "vkCmdBeginQuery");
	funcPtrs->vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)load(context, "vkCmdBeginRenderPass");
	funcPtrs->vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)load(context, "vkCmdBindDescriptorSets");
	funcPtrs->vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)load(context, "vkCmdBindIndexBuffer");
	funcPtrs->vkCmdBindPipeline = (PFN_vkCmdBindPipeline)load(context, "vkCmdBindPipeline");
	funcPtrs->vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)load(context, "vkCmdBindVertexBuffers");
	funcPtrs->vkCmdBlitImage = (PFN_vkCmdBlitImage)load(context, "vkCmdBlitImage");
	funcPtrs->vkCmdClearAttachments = (PFN_vkCmdClearAttachments)load(context, "vkCmdClearAttachments");
	funcPtrs->vkCmdClearColorImage = (PFN_vkCmdClearColorImage)load(context, "vkCmdClearColorImage");
	funcPtrs->vkCmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage)load(context, "vkCmdClearDepthStencilImage");
	funcPtrs->vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)load(context, "vkCmdCopyBuffer");
	funcPtrs->vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)load(context, "vkCmdCopyBufferToImage");
	funcPtrs->vkCmdCopyImage = (PFN_vkCmdCopyImage)load(context, "vkCmdCopyImage");
	funcPtrs->vkCmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer)load(context, "vkCmdCopyImageToBuffer");
	funcPtrs->vkCmdCopyQueryPoolResults = (PFN_vkCmdCopyQueryPoolResults)load(context, "vkCmdCopyQueryPoolResults");
	funcPtrs->vkCmdDispatch = (PFN_vkCmdDispatch)load(context, "vkCmdDispatch");
	funcPtrs->vkCmdDispatchIndirect = (PFN_vkCmdDispatchIndirect)load(context, "vkCmdDispatchIndirect");
	funcPtrs->vkCmdDraw = (PFN_vkCmdDraw)load(context, "vkCmdDraw");
	funcPtrs->vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)load(context, "vkCmdDrawIndexed");
	funcPtrs->vkCmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect)load(context, "vkCmdDrawIndexedIndirect");
	funcPtrs->vkCmdDrawIndirect = (PFN_vkCmdDrawIndirect)load(context, "vkCmdDrawIndirect");
	funcPtrs->vkCmdEndQuery = (PFN_vkCmdEndQuery)load(context, "vkCmdEndQuery");
	funcPtrs->vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)load(context, "vkCmdEndRenderPass");
	funcPtrs->vkCmdExecuteCommands = (PFN_vkCmdExecuteCommands)load(context, "vkCmdExecuteCommands");
	funcPtrs->vkCmdFillBuffer = (PFN_vkCmdFillBuffer)load(context, "vkCmdFillBuffer");
	funcPtrs->vkCmdNextSubpass = (PFN_vkCmdNextSubpass)load(context, "vkCmdNextSubpass");
	funcPtrs->vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)load(context, "vkCmdPipelineBarrier");
	funcPtrs->vkCmdPushConstants = (PFN_vkCmdPushConstants)load(context, "vkCmdPushConstants");
	funcPtrs->vkCmdResetEvent = (PFN_vkCmdResetEvent)load(context, "vkCmdResetEvent");
	funcPtrs->vkCmdResetQueryPool = (PFN_vkCmdResetQueryPool)load(context, "vkCmdResetQueryPool");
	funcPtrs->vkCmdResolveImage = (PFN_vkCmdResolveImage)load(context, "vkCmdResolveImage");
	funcPtrs->vkCmdSetBlendConstants = (PFN_vkCmdSetBlendConstants)load(context, "vkCmdSetBlendConstants");
	funcPtrs->vkCmdSetDepthBias = (PFN_vkCmdSetDepthBias)load(context, "vkCmdSetDepthBias");
	funcPtrs->vkCmdSetDepthBounds = (PFN_vkCmdSetDepthBounds)load(context, "vkCmdSetDepthBounds");
	funcPtrs->vkCmdSetEvent = (PFN_vkCmdSetEvent)load(context, "vkCmdSetEvent");
	funcPtrs->vkCmdSetLineWidth = (PFN_vkCmdSetLineWidth)load(context, "vkCmdSetLineWidth");
	funcPtrs->vkCmdSetScissor = (PFN_vkCmdSetScissor)load(context, "vkCmdSetScissor");
	funcPtrs->vkCmdSetStencilCompareMask = (PFN_vkCmdSetStencilCompareMask)load(context, "vkCmdSetStencilCompareMask");
	funcPtrs->vkCmdSetStencilReference = (PFN_vkCmdSetStencilReference)load(context, "vkCmdSetStencilReference");
	funcPtrs->vkCmdSetStencilWriteMask = (PFN_vkCmdSetStencilWriteMask)load(context, "vkCmdSetStencilWriteMask");
	funcPtrs->vkCmdSetViewport = (PFN_vkCmdSetViewport)load(context, "vkCmdSetViewport");
	funcPtrs->vkCmdSetViewport = (PFN_vkCmdSetViewport)load(context, "vkCmdSetViewport");
	funcPtrs->vkCmdUpdateBuffer = (PFN_vkCmdUpdateBuffer)load(context, "vkCmdUpdateBuffer");
	funcPtrs->vkCmdWaitEvents = (PFN_vkCmdWaitEvents)load(context, "vkCmdWaitEvents");
	funcPtrs->vkCmdWriteTimestamp = (PFN_vkCmdWriteTimestamp)load(context, "vkCmdWriteTimestamp");
	funcPtrs->vkCreateBuffer = (PFN_vkCreateBuffer)load(context, "vkCreateBuffer");
	funcPtrs->vkCreateBufferView = (PFN_vkCreateBufferView)load(context, "vkCreateBufferView");
	funcPtrs->vkCreateCommandPool = (PFN_vkCreateCommandPool)load(context, "vkCreateCommandPool");
	funcPtrs->vkCreateComputePipelines = (PFN_vkCreateComputePipelines)load(context, "vkCreateComputePipelines");
	funcPtrs->vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)load(context, "vkCreateDescriptorPool");
	funcPtrs->vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)load(context, "vkCreateDescriptorSetLayout");
	funcPtrs->vkCreateEvent = (PFN_vkCreateEvent)load(context, "vkCreateEvent");
	funcPtrs->vkCreateFence = (PFN_vkCreateFence)load(context, "vkCreateFence");
	funcPtrs->vkCreateFramebuffer = (PFN_vkCreateFramebuffer)load(context, "vkCreateFramebuffer");
	funcPtrs->vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)load(context, "vkCreateGraphicsPipelines");
	funcPtrs->vkCreateImage = (PFN_vkCreateImage)load(context, "vkCreateImage");
	funcPtrs->vkCreateImageView = (PFN_vkCreateImageView)load(context, "vkCreateImageView");
	funcPtrs->vkCreatePipelineCache = (PFN_vkCreatePipelineCache)load(context, "vkCreatePipelineCache");
	funcPtrs->vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)load(context, "vkCreatePipelineLayout");
	funcPtrs->vkCreateQueryPool = (PFN_vkCreateQueryPool)load(context, "vkCreateQueryPool");
	funcPtrs->vkCreateRenderPass = (PFN_vkCreateRenderPass)load(context, "vkCreateRenderPass");
	funcPtrs->vkCreateSampler = (PFN_vkCreateSampler)load(context, "vkCreateSampler");
	funcPtrs->vkCreateSemaphore = (PFN_vkCreateSemaphore)load(context, "vkCreateSemaphore");
	funcPtrs->vkCreateShaderModule = (PFN_vkCreateShaderModule)load(context, "vkCreateShaderModule");
	funcPtrs->vkDestroyBuffer = (PFN_vkDestroyBuffer)load(context, "vkDestroyBuffer");
	funcPtrs->vkDestroyBufferView = (PFN_vkDestroyBufferView)load(context, "vkDestroyBufferView");
	funcPtrs->vkDestroyCommandPool = (PFN_vkDestroyCommandPool)load(context, "vkDestroyCommandPool");
	funcPtrs->vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)load(context, "vkDestroyDescriptorPool");
	funcPtrs->vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)load(context, "vkDestroyDescriptorSetLayout");
	funcPtrs->vkDestroyDevice = (PFN_vkDestroyDevice)load(context, "vkDestroyDevice");
	funcPtrs->vkDestroyEvent = (PFN_vkDestroyEvent)load(context, "vkDestroyEvent");
	funcPtrs->vkDestroyFence = (PFN_vkDestroyFence)load(context, "vkDestroyFence");
	funcPtrs->vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)load(context, "vkDestroyFramebuffer");
	funcPtrs->vkDestroyImage = (PFN_vkDestroyImage)load(context, "vkDestroyImage");
	funcPtrs->vkDestroyImageView = (PFN_vkDestroyImageView)load(context, "vkDestroyImageView");
	funcPtrs->vkDestroyPipeline = (PFN_vkDestroyPipeline)load(context, "vkDestroyPipeline");
	funcPtrs->vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache)load(context, "vkDestroyPipelineCache");
	funcPtrs->vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)load(context, "vkDestroyPipelineLayout");
	funcPtrs->vkDestroyQueryPool = (PFN_vkDestroyQueryPool)load(context, "vkDestroyQueryPool");
	funcPtrs->vkDestroyRenderPass = (PFN_vkDestroyRenderPass)load(context, "vkDestroyRenderPass");
	funcPtrs->vkDestroySampler = (PFN_vkDestroySampler)load(context, "vkDestroySampler");
	funcPtrs->vkDestroySemaphore = (PFN_vkDestroySemaphore)load(context, "vkDestroySemaphore");
	funcPtrs->vkDestroyShaderModule = (PFN_vkDestroyShaderModule)load(context, "vkDestroyShaderModule");
	funcPtrs->vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)load(context, "vkDeviceWaitIdle");
	funcPtrs->vkEndCommandBuffer = (PFN_vkEndCommandBuffer)load(context, "vkEndCommandBuffer");
	funcPtrs->vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)load(context, "vkFlushMappedMemoryRanges");
	funcPtrs->vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)load(context, "vkFreeCommandBuffers");
	funcPtrs->vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets)load(context, "vkFreeDescriptorSets");
	funcPtrs->vkFreeMemory = (PFN_vkFreeMemory)load(context, "vkFreeMemory");
	funcPtrs->vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)load(context, "vkGetBufferMemoryRequirements");
	funcPtrs->vkGetDeviceMemoryCommitment = (PFN_vkGetDeviceMemoryCommitment)load(context, "vkGetDeviceMemoryCommitment");
	funcPtrs->vkGetDeviceQueue = (PFN_vkGetDeviceQueue)load(context, "vkGetDeviceQueue");
	funcPtrs->vkGetEventStatus = (PFN_vkGetEventStatus)load(context, "vkGetEventStatus");
	funcPtrs->vkGetFenceStatus = (PFN_vkGetFenceStatus)load(context, "vkGetFenceStatus");
	funcPtrs->vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)load(context, "vkGetImageMemoryRequirements");
	funcPtrs->vkGetImageSparseMemoryRequirements = (PFN_vkGetImageSparseMemoryRequirements)load(context, "vkGetImageSparseMemoryRequirements");
	funcPtrs->vkGetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout)load(context, "vkGetImageSubresourceLayout");
	funcPtrs->vkGetPipelineCacheData = (PFN_vkGetPipelineCacheData)load(context, "vkGetPipelineCacheData");
	funcPtrs->vkGetQueryPoolResults = (PFN_vkGetQueryPoolResults)load(context, "vkGetQueryPoolResults");
	funcPtrs->vkGetRenderAreaGranularity = (PFN_vkGetRenderAreaGranularity)load(context, "vkGetRenderAreaGranularity");
	funcPtrs->vkInvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges)load(context, "vkInvalidateMappedMemoryRanges");
	funcPtrs->vkMapMemory = (PFN_vkMapMemory)load(context, "vkMapMemory");
	funcPtrs->vkMergePipelineCaches = (PFN_vkMergePipelineCaches)load(context, "vkMergePipelineCaches");
	funcPtrs->vkQueueBindSparse = (PFN_vkQueueBindSparse)load(context, "vkQueueBindSparse");
	funcPtrs->vkQueueSubmit = (PFN_vkQueueSubmit)load(context, "vkQueueSubmit");
	funcPtrs->vkQueueWaitIdle = (PFN_vkQueueWaitIdle)load(context, "vkQueueWaitIdle");
	funcPtrs->vkResetCommandBuffer = (PFN_vkResetCommandBuffer)load(context, "vkResetCommandBuffer");
	funcPtrs->vkResetCommandPool = (PFN_vkResetCommandPool)load(context, "vkResetCommandPool");
	funcPtrs->vkResetDescriptorPool = (PFN_vkResetDescriptorPool)load(context, "vkResetDescriptorPool");
	funcPtrs->vkResetEvent = (PFN_vkResetEvent)load(context, "vkResetEvent");
	funcPtrs->vkResetFences = (PFN_vkResetFences)load(context, "vkResetFences");
	funcPtrs->vkSetEvent = (PFN_vkSetEvent)load(context, "vkSetEvent");
	funcPtrs->vkUnmapMemory = (PFN_vkUnmapMemory)load(context, "vkUnmapMemory");
	funcPtrs->vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)load(context, "vkUpdateDescriptorSets");
	funcPtrs->vkWaitForFences = (PFN_vkWaitForFences)load(context, "vkWaitForFences");
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
	funcPtrs->vkBindBufferMemory2 = (PFN_vkBindBufferMemory2)load(context, "vkBindBufferMemory2");
	funcPtrs->vkBindImageMemory2 = (PFN_vkBindImageMemory2)load(context, "vkBindImageMemory2");
	funcPtrs->vkCmdDispatchBase = (PFN_vkCmdDispatchBase)load(context, "vkCmdDispatchBase");
	funcPtrs->vkCmdSetDeviceMask = (PFN_vkCmdSetDeviceMask)load(context, "vkCmdSetDeviceMask");
	funcPtrs->vkCreateDescriptorUpdateTemplate = (PFN_vkCreateDescriptorUpdateTemplate)load(context, "vkCreateDescriptorUpdateTemplate");
	funcPtrs->vkCreateSamplerYcbcrConversion = (PFN_vkCreateSamplerYcbcrConversion)load(context, "vkCreateSamplerYcbcrConversion");
	funcPtrs->vkDestroyDescriptorUpdateTemplate = (PFN_vkDestroyDescriptorUpdateTemplate)load(context, "vkDestroyDescriptorUpdateTemplate");
	funcPtrs->vkDestroySamplerYcbcrConversion = (PFN_vkDestroySamplerYcbcrConversion)load(context, "vkDestroySamplerYcbcrConversion");
	funcPtrs->vkGetBufferMemoryRequirements2 = (PFN_vkGetBufferMemoryRequirements2)load(context, "vkGetBufferMemoryRequirements2");
	funcPtrs->vkGetDescriptorSetLayoutSupport = (PFN_vkGetDescriptorSetLayoutSupport)load(context, "vkGetDescriptorSetLayoutSupport");
	funcPtrs->vkGetDeviceGroupPeerMemoryFeatures = (PFN_vkGetDeviceGroupPeerMemoryFeatures)load(context, "vkGetDeviceGroupPeerMemoryFeatures");
	funcPtrs->vkGetDeviceQueue2 = (PFN_vkGetDeviceQueue2)load(context, "vkGetDeviceQueue2");
	funcPtrs->vkGetImageMemoryRequirements2 = (PFN_vkGetImageMemoryRequirements2)load(context, "vkGetImageMemoryRequirements2");
	funcPtrs->vkGetImageSparseMemoryRequirements2 = (PFN_vkGetImageSparseMemoryRequirements2)load(context, "vkGetImageSparseMemoryRequirements2");
	funcPtrs->vkTrimCommandPool = (PFN_vkTrimCommandPool)load(context, "vkTrimCommandPool");
	funcPtrs->vkUpdateDescriptorSetWithTemplate = (PFN_vkUpdateDescriptorSetWithTemplate)load(context, "vkUpdateDescriptorSetWithTemplate");
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_AMD_buffer_marker)
	funcPtrs->vkCmdWriteBufferMarkerAMD = (PFN_vkCmdWriteBufferMarkerAMD)load(context, "vkCmdWriteBufferMarkerAMD");
#endif /* defined(VK_AMD_buffer_marker) */
#if defined(VK_AMD_display_native_hdr)
	funcPtrs->vkSetLocalDimmingAMD = (PFN_vkSetLocalDimmingAMD)load(context, "vkSetLocalDimmingAMD");
#endif /* defined(VK_AMD_display_native_hdr) */
#if defined(VK_AMD_draw_indirect_count)
	vkCmdDrawIndexedIndirectCountAMD = (PFN_vkCmdDrawIndexedIndirectCountAMD)load(context, "vkCmdDrawIndexedIndirectCountAMD");
	vkCmdDrawIndirectCountAMD = (PFN_vkCmdDrawIndirectCountAMD)load(context, "vkCmdDrawIndirectCountAMD");
#endif /* defined(VK_AMD_draw_indirect_count) */
#if defined(VK_AMD_shader_info)
	vkGetShaderInfoAMD = (PFN_vkGetShaderInfoAMD)load(context, "vkGetShaderInfoAMD");
#endif /* defined(VK_AMD_shader_info) */
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
	vkGetAndroidHardwareBufferPropertiesANDROID = (PFN_vkGetAndroidHardwareBufferPropertiesANDROID)load(context, "vkGetAndroidHardwareBufferPropertiesANDROID");
	vkGetMemoryAndroidHardwareBufferANDROID = (PFN_vkGetMemoryAndroidHardwareBufferANDROID)load(context, "vkGetMemoryAndroidHardwareBufferANDROID");
#endif /* defined(VK_ANDROID_external_memory_android_hardware_buffer) */
#if defined(VK_EXT_buffer_device_address)
	vkGetBufferDeviceAddressEXT = (PFN_vkGetBufferDeviceAddressEXT)load(context, "vkGetBufferDeviceAddressEXT");
#endif /* defined(VK_EXT_buffer_device_address) */
#if defined(VK_EXT_calibrated_timestamps)
	vkGetCalibratedTimestampsEXT = (PFN_vkGetCalibratedTimestampsEXT)load(context, "vkGetCalibratedTimestampsEXT");
#endif /* defined(VK_EXT_calibrated_timestamps) */
#if defined(VK_EXT_conditional_rendering)
	vkCmdBeginConditionalRenderingEXT = (PFN_vkCmdBeginConditionalRenderingEXT)load(context, "vkCmdBeginConditionalRenderingEXT");
	vkCmdEndConditionalRenderingEXT = (PFN_vkCmdEndConditionalRenderingEXT)load(context, "vkCmdEndConditionalRenderingEXT");
#endif /* defined(VK_EXT_conditional_rendering) */
#if defined(VK_EXT_debug_marker)
	vkCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)load(context, "vkCmdDebugMarkerBeginEXT");
	vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)load(context, "vkCmdDebugMarkerEndEXT");
	vkCmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)load(context, "vkCmdDebugMarkerInsertEXT");
	vkDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)load(context, "vkDebugMarkerSetObjectNameEXT");
	vkDebugMarkerSetObjectTagEXT = (PFN_vkDebugMarkerSetObjectTagEXT)load(context, "vkDebugMarkerSetObjectTagEXT");
#endif /* defined(VK_EXT_debug_marker) */
#if defined(VK_EXT_debug_utils)
	funcPtrs->vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)load(context, "vkCmdBeginDebugUtilsLabelEXT");
	funcPtrs->vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)load(context, "vkCmdEndDebugUtilsLabelEXT");
	funcPtrs->vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)load(context, "vkCmdInsertDebugUtilsLabelEXT");
	funcPtrs->vkQueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT)load(context, "vkQueueBeginDebugUtilsLabelEXT");
	funcPtrs->vkQueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT)load(context, "vkQueueEndDebugUtilsLabelEXT");
	funcPtrs->vkQueueInsertDebugUtilsLabelEXT = (PFN_vkQueueInsertDebugUtilsLabelEXT)load(context, "vkQueueInsertDebugUtilsLabelEXT");
	funcPtrs->vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)load(context, "vkSetDebugUtilsObjectNameEXT");
	funcPtrs->vkSetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)load(context, "vkSetDebugUtilsObjectTagEXT");
#endif /* defined(VK_EXT_debug_utils) */
#if defined(VK_EXT_discard_rectangles)
	vkCmdSetDiscardRectangleEXT = (PFN_vkCmdSetDiscardRectangleEXT)load(context, "vkCmdSetDiscardRectangleEXT");
#endif /* defined(VK_EXT_discard_rectangles) */
#if defined(VK_EXT_display_control)
	vkDisplayPowerControlEXT = (PFN_vkDisplayPowerControlEXT)load(context, "vkDisplayPowerControlEXT");
	vkGetSwapchainCounterEXT = (PFN_vkGetSwapchainCounterEXT)load(context, "vkGetSwapchainCounterEXT");
	vkRegisterDeviceEventEXT = (PFN_vkRegisterDeviceEventEXT)load(context, "vkRegisterDeviceEventEXT");
	vkRegisterDisplayEventEXT = (PFN_vkRegisterDisplayEventEXT)load(context, "vkRegisterDisplayEventEXT");
#endif /* defined(VK_EXT_display_control) */
#if defined(VK_EXT_external_memory_host)
	vkGetMemoryHostPointerPropertiesEXT = (PFN_vkGetMemoryHostPointerPropertiesEXT)load(context, "vkGetMemoryHostPointerPropertiesEXT");
#endif /* defined(VK_EXT_external_memory_host) */
#if defined(VK_EXT_full_screen_exclusive)
	vkAcquireFullScreenExclusiveModeEXT = (PFN_vkAcquireFullScreenExclusiveModeEXT)load(context, "vkAcquireFullScreenExclusiveModeEXT");
	vkReleaseFullScreenExclusiveModeEXT = (PFN_vkReleaseFullScreenExclusiveModeEXT)load(context, "vkReleaseFullScreenExclusiveModeEXT");
#endif /* defined(VK_EXT_full_screen_exclusive) */
#if defined(VK_EXT_hdr_metadata)
	vkSetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT)load(context, "vkSetHdrMetadataEXT");
#endif /* defined(VK_EXT_hdr_metadata) */
#if defined(VK_EXT_host_query_reset)
	vkResetQueryPoolEXT = (PFN_vkResetQueryPoolEXT)load(context, "vkResetQueryPoolEXT");
#endif /* defined(VK_EXT_host_query_reset) */
#if defined(VK_EXT_image_drm_format_modifier)
	vkGetImageDrmFormatModifierPropertiesEXT = (PFN_vkGetImageDrmFormatModifierPropertiesEXT)load(context, "vkGetImageDrmFormatModifierPropertiesEXT");
#endif /* defined(VK_EXT_image_drm_format_modifier) */
#if defined(VK_EXT_sample_locations)
	vkCmdSetSampleLocationsEXT = (PFN_vkCmdSetSampleLocationsEXT)load(context, "vkCmdSetSampleLocationsEXT");
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_EXT_transform_feedback)
	vkCmdBeginQueryIndexedEXT = (PFN_vkCmdBeginQueryIndexedEXT)load(context, "vkCmdBeginQueryIndexedEXT");
	vkCmdBeginTransformFeedbackEXT = (PFN_vkCmdBeginTransformFeedbackEXT)load(context, "vkCmdBeginTransformFeedbackEXT");
	vkCmdBindTransformFeedbackBuffersEXT = (PFN_vkCmdBindTransformFeedbackBuffersEXT)load(context, "vkCmdBindTransformFeedbackBuffersEXT");
	vkCmdDrawIndirectByteCountEXT = (PFN_vkCmdDrawIndirectByteCountEXT)load(context, "vkCmdDrawIndirectByteCountEXT");
	vkCmdEndQueryIndexedEXT = (PFN_vkCmdEndQueryIndexedEXT)load(context, "vkCmdEndQueryIndexedEXT");
	vkCmdEndTransformFeedbackEXT = (PFN_vkCmdEndTransformFeedbackEXT)load(context, "vkCmdEndTransformFeedbackEXT");
#endif /* defined(VK_EXT_transform_feedback) */
#if defined(VK_EXT_validation_cache)
	vkCreateValidationCacheEXT = (PFN_vkCreateValidationCacheEXT)load(context, "vkCreateValidationCacheEXT");
	vkDestroyValidationCacheEXT = (PFN_vkDestroyValidationCacheEXT)load(context, "vkDestroyValidationCacheEXT");
	vkGetValidationCacheDataEXT = (PFN_vkGetValidationCacheDataEXT)load(context, "vkGetValidationCacheDataEXT");
	vkMergeValidationCachesEXT = (PFN_vkMergeValidationCachesEXT)load(context, "vkMergeValidationCachesEXT");
#endif /* defined(VK_EXT_validation_cache) */
#if defined(VK_GOOGLE_display_timing)
	vkGetPastPresentationTimingGOOGLE = (PFN_vkGetPastPresentationTimingGOOGLE)load(context, "vkGetPastPresentationTimingGOOGLE");
	vkGetRefreshCycleDurationGOOGLE = (PFN_vkGetRefreshCycleDurationGOOGLE)load(context, "vkGetRefreshCycleDurationGOOGLE");
#endif /* defined(VK_GOOGLE_display_timing) */
#if defined(VK_KHR_bind_memory2)
	Volk::funcPtrs->vkBindBufferMemory2KHR = (PFN_vkBindBufferMemory2KHR)load(context, "vkBindBufferMemory2KHR");
	Volk::funcPtrs->vkBindImageMemory2KHR = (PFN_vkBindImageMemory2KHR)load(context, "vkBindImageMemory2KHR");
#endif /* defined(VK_KHR_bind_memory2) */
#if defined(VK_KHR_create_renderpass2)
	vkCmdBeginRenderPass2KHR = (PFN_vkCmdBeginRenderPass2KHR)load(context, "vkCmdBeginRenderPass2KHR");
	vkCmdEndRenderPass2KHR = (PFN_vkCmdEndRenderPass2KHR)load(context, "vkCmdEndRenderPass2KHR");
	vkCmdNextSubpass2KHR = (PFN_vkCmdNextSubpass2KHR)load(context, "vkCmdNextSubpass2KHR");
	vkCreateRenderPass2KHR = (PFN_vkCreateRenderPass2KHR)load(context, "vkCreateRenderPass2KHR");
#endif /* defined(VK_KHR_create_renderpass2) */
#if defined(VK_KHR_descriptor_update_template)
	vkCreateDescriptorUpdateTemplateKHR = (PFN_vkCreateDescriptorUpdateTemplateKHR)load(context, "vkCreateDescriptorUpdateTemplateKHR");
	vkDestroyDescriptorUpdateTemplateKHR = (PFN_vkDestroyDescriptorUpdateTemplateKHR)load(context, "vkDestroyDescriptorUpdateTemplateKHR");
	vkUpdateDescriptorSetWithTemplateKHR = (PFN_vkUpdateDescriptorSetWithTemplateKHR)load(context, "vkUpdateDescriptorSetWithTemplateKHR");
#endif /* defined(VK_KHR_descriptor_update_template) */
#if defined(VK_KHR_device_group)
	vkCmdDispatchBaseKHR = (PFN_vkCmdDispatchBaseKHR)load(context, "vkCmdDispatchBaseKHR");
	vkCmdSetDeviceMaskKHR = (PFN_vkCmdSetDeviceMaskKHR)load(context, "vkCmdSetDeviceMaskKHR");
	vkGetDeviceGroupPeerMemoryFeaturesKHR = (PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR)load(context, "vkGetDeviceGroupPeerMemoryFeaturesKHR");
#endif /* defined(VK_KHR_device_group) */
#if defined(VK_KHR_display_swapchain)
	vkCreateSharedSwapchainsKHR = (PFN_vkCreateSharedSwapchainsKHR)load(context, "vkCreateSharedSwapchainsKHR");
#endif /* defined(VK_KHR_display_swapchain) */
#if defined(VK_KHR_draw_indirect_count)
	vkCmdDrawIndexedIndirectCountKHR = (PFN_vkCmdDrawIndexedIndirectCountKHR)load(context, "vkCmdDrawIndexedIndirectCountKHR");
	vkCmdDrawIndirectCountKHR = (PFN_vkCmdDrawIndirectCountKHR)load(context, "vkCmdDrawIndirectCountKHR");
#endif /* defined(VK_KHR_draw_indirect_count) */
#if defined(VK_KHR_external_fence_fd)
	vkGetFenceFdKHR = (PFN_vkGetFenceFdKHR)load(context, "vkGetFenceFdKHR");
	vkImportFenceFdKHR = (PFN_vkImportFenceFdKHR)load(context, "vkImportFenceFdKHR");
#endif /* defined(VK_KHR_external_fence_fd) */
#if defined(VK_KHR_external_fence_win32)
	vkGetFenceWin32HandleKHR = (PFN_vkGetFenceWin32HandleKHR)load(context, "vkGetFenceWin32HandleKHR");
	vkImportFenceWin32HandleKHR = (PFN_vkImportFenceWin32HandleKHR)load(context, "vkImportFenceWin32HandleKHR");
#endif /* defined(VK_KHR_external_fence_win32) */
#if defined(VK_KHR_external_memory_fd)
	vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)load(context, "vkGetMemoryFdKHR");
	vkGetMemoryFdPropertiesKHR = (PFN_vkGetMemoryFdPropertiesKHR)load(context, "vkGetMemoryFdPropertiesKHR");
#endif /* defined(VK_KHR_external_memory_fd) */
#if defined(VK_KHR_external_memory_win32)
	vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)load(context, "vkGetMemoryWin32HandleKHR");
	vkGetMemoryWin32HandlePropertiesKHR = (PFN_vkGetMemoryWin32HandlePropertiesKHR)load(context, "vkGetMemoryWin32HandlePropertiesKHR");
#endif /* defined(VK_KHR_external_memory_win32) */
#if defined(VK_KHR_external_semaphore_fd)
	vkGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)load(context, "vkGetSemaphoreFdKHR");
	vkImportSemaphoreFdKHR = (PFN_vkImportSemaphoreFdKHR)load(context, "vkImportSemaphoreFdKHR");
#endif /* defined(VK_KHR_external_semaphore_fd) */
#if defined(VK_KHR_external_semaphore_win32)
	vkGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)load(context, "vkGetSemaphoreWin32HandleKHR");
	vkImportSemaphoreWin32HandleKHR = (PFN_vkImportSemaphoreWin32HandleKHR)load(context, "vkImportSemaphoreWin32HandleKHR");
#endif /* defined(VK_KHR_external_semaphore_win32) */
#if defined(VK_KHR_get_memory_requirements2)
	vkGetBufferMemoryRequirements2KHR = (PFN_vkGetBufferMemoryRequirements2KHR)load(context, "vkGetBufferMemoryRequirements2KHR");
	vkGetImageMemoryRequirements2KHR = (PFN_vkGetImageMemoryRequirements2KHR)load(context, "vkGetImageMemoryRequirements2KHR");
	vkGetImageSparseMemoryRequirements2KHR = (PFN_vkGetImageSparseMemoryRequirements2KHR)load(context, "vkGetImageSparseMemoryRequirements2KHR");
#endif /* defined(VK_KHR_get_memory_requirements2) */
#if defined(VK_KHR_maintenance1)
	vkTrimCommandPoolKHR = (PFN_vkTrimCommandPoolKHR)load(context, "vkTrimCommandPoolKHR");
#endif /* defined(VK_KHR_maintenance1) */
#if defined(VK_KHR_maintenance3)
	vkGetDescriptorSetLayoutSupportKHR = (PFN_vkGetDescriptorSetLayoutSupportKHR)load(context, "vkGetDescriptorSetLayoutSupportKHR");
#endif /* defined(VK_KHR_maintenance3) */
#if defined(VK_KHR_push_descriptor)
	vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)load(context, "vkCmdPushDescriptorSetKHR");
#endif /* defined(VK_KHR_push_descriptor) */
#if defined(VK_KHR_sampler_ycbcr_conversion)
	vkCreateSamplerYcbcrConversionKHR = (PFN_vkCreateSamplerYcbcrConversionKHR)load(context, "vkCreateSamplerYcbcrConversionKHR");
	vkDestroySamplerYcbcrConversionKHR = (PFN_vkDestroySamplerYcbcrConversionKHR)load(context, "vkDestroySamplerYcbcrConversionKHR");
#endif /* defined(VK_KHR_sampler_ycbcr_conversion) */
#if defined(VK_KHR_shared_presentable_image)
	vkGetSwapchainStatusKHR = (PFN_vkGetSwapchainStatusKHR)load(context, "vkGetSwapchainStatusKHR");
#endif /* defined(VK_KHR_shared_presentable_image) */
#if defined(VK_KHR_swapchain)
	funcPtrs->vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)load(context, "vkAcquireNextImageKHR");
	funcPtrs->vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)load(context, "vkCreateSwapchainKHR");
	funcPtrs->vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)load(context, "vkDestroySwapchainKHR");
	funcPtrs->vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)load(context, "vkGetSwapchainImagesKHR");
	funcPtrs->vkQueuePresentKHR = (PFN_vkQueuePresentKHR)load(context, "vkQueuePresentKHR");
#endif /* defined(VK_KHR_swapchain) */
#if defined(VK_NVX_device_generated_commands)
	funcPtrs->vkCmdProcessCommandsNVX = (PFN_vkCmdProcessCommandsNVX)load(context, "vkCmdProcessCommandsNVX");
	funcPtrs->vkCmdReserveSpaceForCommandsNVX = (PFN_vkCmdReserveSpaceForCommandsNVX)load(context, "vkCmdReserveSpaceForCommandsNVX");
	funcPtrs->vkCreateIndirectCommandsLayoutNVX = (PFN_vkCreateIndirectCommandsLayoutNVX)load(context, "vkCreateIndirectCommandsLayoutNVX");
	funcPtrs->vkCreateObjectTableNVX = (PFN_vkCreateObjectTableNVX)load(context, "vkCreateObjectTableNVX");
	funcPtrs->vkDestroyIndirectCommandsLayoutNVX = (PFN_vkDestroyIndirectCommandsLayoutNVX)load(context, "vkDestroyIndirectCommandsLayoutNVX");
	funcPtrs->vkDestroyObjectTableNVX = (PFN_vkDestroyObjectTableNVX)load(context, "vkDestroyObjectTableNVX");
	funcPtrs->vkRegisterObjectsNVX = (PFN_vkRegisterObjectsNVX)load(context, "vkRegisterObjectsNVX");
	funcPtrs->vkUnregisterObjectsNVX = (PFN_vkUnregisterObjectsNVX)load(context, "vkUnregisterObjectsNVX");
#endif /* defined(VK_NVX_device_generated_commands) */
#if defined(VK_NVX_image_view_handle)
	vkGetImageViewHandleNVX = (PFN_vkGetImageViewHandleNVX)load(context, "vkGetImageViewHandleNVX");
#endif /* defined(VK_NVX_image_view_handle) */
#if defined(VK_NV_clip_space_w_scaling)
	vkCmdSetViewportWScalingNV = (PFN_vkCmdSetViewportWScalingNV)load(context, "vkCmdSetViewportWScalingNV");
#endif /* defined(VK_NV_clip_space_w_scaling) */
#if defined(VK_NV_device_diagnostic_checkpoints)
	vkCmdSetCheckpointNV = (PFN_vkCmdSetCheckpointNV)load(context, "vkCmdSetCheckpointNV");
	vkGetQueueCheckpointDataNV = (PFN_vkGetQueueCheckpointDataNV)load(context, "vkGetQueueCheckpointDataNV");
#endif /* defined(VK_NV_device_diagnostic_checkpoints) */
#if defined(VK_NV_external_memory_win32)
	vkGetMemoryWin32HandleNV = (PFN_vkGetMemoryWin32HandleNV)load(context, "vkGetMemoryWin32HandleNV");
#endif /* defined(VK_NV_external_memory_win32) */
#if defined(VK_NV_mesh_shader)
	vkCmdDrawMeshTasksIndirectCountNV = (PFN_vkCmdDrawMeshTasksIndirectCountNV)load(context, "vkCmdDrawMeshTasksIndirectCountNV");
	vkCmdDrawMeshTasksIndirectNV = (PFN_vkCmdDrawMeshTasksIndirectNV)load(context, "vkCmdDrawMeshTasksIndirectNV");
	vkCmdDrawMeshTasksNV = (PFN_vkCmdDrawMeshTasksNV)load(context, "vkCmdDrawMeshTasksNV");
#endif /* defined(VK_NV_mesh_shader) */
#if defined(VK_NV_ray_tracing)
	funcPtrs->vkBindAccelerationStructureMemoryNV = (PFN_vkBindAccelerationStructureMemoryNV)load(context, "vkBindAccelerationStructureMemoryNV");
	funcPtrs->vkCmdBuildAccelerationStructureNV = (PFN_vkCmdBuildAccelerationStructureNV)load(context, "vkCmdBuildAccelerationStructureNV");
	funcPtrs->vkCmdCopyAccelerationStructureNV = (PFN_vkCmdCopyAccelerationStructureNV)load(context, "vkCmdCopyAccelerationStructureNV");
	funcPtrs->vkCmdTraceRaysNV = (PFN_vkCmdTraceRaysNV)load(context, "vkCmdTraceRaysNV");
	funcPtrs->vkCmdWriteAccelerationStructuresPropertiesNV = (PFN_vkCmdWriteAccelerationStructuresPropertiesNV)load(context, "vkCmdWriteAccelerationStructuresPropertiesNV");
	funcPtrs->vkCompileDeferredNV = (PFN_vkCompileDeferredNV)load(context, "vkCompileDeferredNV");
	funcPtrs->vkCreateAccelerationStructureNV = (PFN_vkCreateAccelerationStructureNV)load(context, "vkCreateAccelerationStructureNV");
	funcPtrs->vkCreateRayTracingPipelinesNV = (PFN_vkCreateRayTracingPipelinesNV)load(context, "vkCreateRayTracingPipelinesNV");
	funcPtrs->vkDestroyAccelerationStructureNV = (PFN_vkDestroyAccelerationStructureNV)load(context, "vkDestroyAccelerationStructureNV");
	funcPtrs->vkGetAccelerationStructureHandleNV = (PFN_vkGetAccelerationStructureHandleNV)load(context, "vkGetAccelerationStructureHandleNV");
	funcPtrs->vkGetAccelerationStructureMemoryRequirementsNV = (PFN_vkGetAccelerationStructureMemoryRequirementsNV)load(context, "vkGetAccelerationStructureMemoryRequirementsNV");
	funcPtrs->vkGetRayTracingShaderGroupHandlesNV = (PFN_vkGetRayTracingShaderGroupHandlesNV)load(context, "vkGetRayTracingShaderGroupHandlesNV");
#endif /* defined(VK_NV_ray_tracing) */
#if defined(VK_NV_scissor_exclusive)
	vkCmdSetExclusiveScissorNV = (PFN_vkCmdSetExclusiveScissorNV)load(context, "vkCmdSetExclusiveScissorNV");
#endif /* defined(VK_NV_scissor_exclusive) */
#if defined(VK_NV_shading_rate_image)
	vkCmdBindShadingRateImageNV = (PFN_vkCmdBindShadingRateImageNV)load(context, "vkCmdBindShadingRateImageNV");
	vkCmdSetCoarseSampleOrderNV = (PFN_vkCmdSetCoarseSampleOrderNV)load(context, "vkCmdSetCoarseSampleOrderNV");
	vkCmdSetViewportShadingRatePaletteNV = (PFN_vkCmdSetViewportShadingRatePaletteNV)load(context, "vkCmdSetViewportShadingRatePaletteNV");
#endif /* defined(VK_NV_shading_rate_image) */
#if (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_device_group) && defined(VK_EXT_full_screen_exclusive))
	vkGetDeviceGroupSurfacePresentModes2EXT = (PFN_vkGetDeviceGroupSurfacePresentModes2EXT)load(context, "vkGetDeviceGroupSurfacePresentModes2EXT");
#endif /* (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_device_group) && defined(VK_EXT_full_screen_exclusive)) */
#if (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1))
	vkCmdPushDescriptorSetWithTemplateKHR = (PFN_vkCmdPushDescriptorSetWithTemplateKHR)load(context, "vkCmdPushDescriptorSetWithTemplateKHR");
#endif /* (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	funcPtrs->vkGetDeviceGroupPresentCapabilitiesKHR = (PFN_vkGetDeviceGroupPresentCapabilitiesKHR)load(context, "vkGetDeviceGroupPresentCapabilitiesKHR");
	funcPtrs->vkGetDeviceGroupSurfacePresentModesKHR = (PFN_vkGetDeviceGroupSurfacePresentModesKHR)load(context, "vkGetDeviceGroupSurfacePresentModesKHR");
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	funcPtrs->vkAcquireNextImage2KHR = (PFN_vkAcquireNextImage2KHR)load(context, "vkAcquireNextImage2KHR");
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
	/* VOLK_GENERATE_LOAD_DEVICE */
}

static void Volk::volkGenLoadDeviceTable(Volk::DeviceTable* table, void* context, PFN_vkVoidFunction (*load)(void*, const char*))
{
	/* VOLK_GENERATE_LOAD_DEVICE_TABLE */
#if defined(VK_VERSION_1_0)
	table->vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)load(context, "vkAllocateCommandBuffers");
	table->vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)load(context, "vkAllocateDescriptorSets");
	table->vkAllocateMemory = (PFN_vkAllocateMemory)load(context, "vkAllocateMemory");
	table->vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)load(context, "vkBeginCommandBuffer");
	table->vkBindBufferMemory = (PFN_vkBindBufferMemory)load(context, "vkBindBufferMemory");
	table->vkBindImageMemory = (PFN_vkBindImageMemory)load(context, "vkBindImageMemory");
	table->vkCmdBeginQuery = (PFN_vkCmdBeginQuery)load(context, "vkCmdBeginQuery");
	table->vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)load(context, "vkCmdBeginRenderPass");
	table->vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)load(context, "vkCmdBindDescriptorSets");
	table->vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)load(context, "vkCmdBindIndexBuffer");
	table->vkCmdBindPipeline = (PFN_vkCmdBindPipeline)load(context, "vkCmdBindPipeline");
	table->vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)load(context, "vkCmdBindVertexBuffers");
	table->vkCmdBlitImage = (PFN_vkCmdBlitImage)load(context, "vkCmdBlitImage");
	table->vkCmdClearAttachments = (PFN_vkCmdClearAttachments)load(context, "vkCmdClearAttachments");
	table->vkCmdClearColorImage = (PFN_vkCmdClearColorImage)load(context, "vkCmdClearColorImage");
	table->vkCmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage)load(context, "vkCmdClearDepthStencilImage");
	table->vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)load(context, "vkCmdCopyBuffer");
	table->vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)load(context, "vkCmdCopyBufferToImage");
	table->vkCmdCopyImage = (PFN_vkCmdCopyImage)load(context, "vkCmdCopyImage");
	table->vkCmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer)load(context, "vkCmdCopyImageToBuffer");
	table->vkCmdCopyQueryPoolResults = (PFN_vkCmdCopyQueryPoolResults)load(context, "vkCmdCopyQueryPoolResults");
	table->vkCmdDispatch = (PFN_vkCmdDispatch)load(context, "vkCmdDispatch");
	table->vkCmdDispatchIndirect = (PFN_vkCmdDispatchIndirect)load(context, "vkCmdDispatchIndirect");
	table->vkCmdDraw = (PFN_vkCmdDraw)load(context, "vkCmdDraw");
	table->vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)load(context, "vkCmdDrawIndexed");
	table->vkCmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect)load(context, "vkCmdDrawIndexedIndirect");
	table->vkCmdDrawIndirect = (PFN_vkCmdDrawIndirect)load(context, "vkCmdDrawIndirect");
	table->vkCmdEndQuery = (PFN_vkCmdEndQuery)load(context, "vkCmdEndQuery");
	table->vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)load(context, "vkCmdEndRenderPass");
	table->vkCmdExecuteCommands = (PFN_vkCmdExecuteCommands)load(context, "vkCmdExecuteCommands");
	table->vkCmdFillBuffer = (PFN_vkCmdFillBuffer)load(context, "vkCmdFillBuffer");
	table->vkCmdNextSubpass = (PFN_vkCmdNextSubpass)load(context, "vkCmdNextSubpass");
	table->vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)load(context, "vkCmdPipelineBarrier");
	table->vkCmdPushConstants = (PFN_vkCmdPushConstants)load(context, "vkCmdPushConstants");
	table->vkCmdResetEvent = (PFN_vkCmdResetEvent)load(context, "vkCmdResetEvent");
	table->vkCmdResetQueryPool = (PFN_vkCmdResetQueryPool)load(context, "vkCmdResetQueryPool");
	table->vkCmdResolveImage = (PFN_vkCmdResolveImage)load(context, "vkCmdResolveImage");
	table->vkCmdSetBlendConstants = (PFN_vkCmdSetBlendConstants)load(context, "vkCmdSetBlendConstants");
	table->vkCmdSetDepthBias = (PFN_vkCmdSetDepthBias)load(context, "vkCmdSetDepthBias");
	table->vkCmdSetDepthBounds = (PFN_vkCmdSetDepthBounds)load(context, "vkCmdSetDepthBounds");
	table->vkCmdSetEvent = (PFN_vkCmdSetEvent)load(context, "vkCmdSetEvent");
	table->vkCmdSetLineWidth = (PFN_vkCmdSetLineWidth)load(context, "vkCmdSetLineWidth");
	table->vkCmdSetScissor = (PFN_vkCmdSetScissor)load(context, "vkCmdSetScissor");
	table->vkCmdSetStencilCompareMask = (PFN_vkCmdSetStencilCompareMask)load(context, "vkCmdSetStencilCompareMask");
	table->vkCmdSetStencilReference = (PFN_vkCmdSetStencilReference)load(context, "vkCmdSetStencilReference");
	table->vkCmdSetStencilWriteMask = (PFN_vkCmdSetStencilWriteMask)load(context, "vkCmdSetStencilWriteMask");
	table->vkCmdSetViewport = (PFN_vkCmdSetViewport)load(context, "vkCmdSetViewport");
	table->vkCmdUpdateBuffer = (PFN_vkCmdUpdateBuffer)load(context, "vkCmdUpdateBuffer");
	table->vkCmdWaitEvents = (PFN_vkCmdWaitEvents)load(context, "vkCmdWaitEvents");
	table->vkCmdWriteTimestamp = (PFN_vkCmdWriteTimestamp)load(context, "vkCmdWriteTimestamp");
	table->vkCreateBuffer = (PFN_vkCreateBuffer)load(context, "vkCreateBuffer");
	table->vkCreateBufferView = (PFN_vkCreateBufferView)load(context, "vkCreateBufferView");
	table->vkCreateCommandPool = (PFN_vkCreateCommandPool)load(context, "vkCreateCommandPool");
	table->vkCreateComputePipelines = (PFN_vkCreateComputePipelines)load(context, "vkCreateComputePipelines");
	table->vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)load(context, "vkCreateDescriptorPool");
	table->vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)load(context, "vkCreateDescriptorSetLayout");
	table->vkCreateEvent = (PFN_vkCreateEvent)load(context, "vkCreateEvent");
	table->vkCreateFence = (PFN_vkCreateFence)load(context, "vkCreateFence");
	table->vkCreateFramebuffer = (PFN_vkCreateFramebuffer)load(context, "vkCreateFramebuffer");
	table->vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)load(context, "vkCreateGraphicsPipelines");
	table->vkCreateImage = (PFN_vkCreateImage)load(context, "vkCreateImage");
	table->vkCreateImageView = (PFN_vkCreateImageView)load(context, "vkCreateImageView");
	table->vkCreatePipelineCache = (PFN_vkCreatePipelineCache)load(context, "vkCreatePipelineCache");
	table->vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)load(context, "vkCreatePipelineLayout");
	table->vkCreateQueryPool = (PFN_vkCreateQueryPool)load(context, "vkCreateQueryPool");
	table->vkCreateRenderPass = (PFN_vkCreateRenderPass)load(context, "vkCreateRenderPass");
	table->vkCreateSampler = (PFN_vkCreateSampler)load(context, "vkCreateSampler");
	table->vkCreateSemaphore = (PFN_vkCreateSemaphore)load(context, "vkCreateSemaphore");
	table->vkCreateShaderModule = (PFN_vkCreateShaderModule)load(context, "vkCreateShaderModule");
	table->vkDestroyBuffer = (PFN_vkDestroyBuffer)load(context, "vkDestroyBuffer");
	table->vkDestroyBufferView = (PFN_vkDestroyBufferView)load(context, "vkDestroyBufferView");
	table->vkDestroyCommandPool = (PFN_vkDestroyCommandPool)load(context, "vkDestroyCommandPool");
	table->vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)load(context, "vkDestroyDescriptorPool");
	table->vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)load(context, "vkDestroyDescriptorSetLayout");
	table->vkDestroyDevice = (PFN_vkDestroyDevice)load(context, "vkDestroyDevice");
	table->vkDestroyEvent = (PFN_vkDestroyEvent)load(context, "vkDestroyEvent");
	table->vkDestroyFence = (PFN_vkDestroyFence)load(context, "vkDestroyFence");
	table->vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)load(context, "vkDestroyFramebuffer");
	table->vkDestroyImage = (PFN_vkDestroyImage)load(context, "vkDestroyImage");
	table->vkDestroyImageView = (PFN_vkDestroyImageView)load(context, "vkDestroyImageView");
	table->vkDestroyPipeline = (PFN_vkDestroyPipeline)load(context, "vkDestroyPipeline");
	table->vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache)load(context, "vkDestroyPipelineCache");
	table->vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)load(context, "vkDestroyPipelineLayout");
	table->vkDestroyQueryPool = (PFN_vkDestroyQueryPool)load(context, "vkDestroyQueryPool");
	table->vkDestroyRenderPass = (PFN_vkDestroyRenderPass)load(context, "vkDestroyRenderPass");
	table->vkDestroySampler = (PFN_vkDestroySampler)load(context, "vkDestroySampler");
	table->vkDestroySemaphore = (PFN_vkDestroySemaphore)load(context, "vkDestroySemaphore");
	table->vkDestroyShaderModule = (PFN_vkDestroyShaderModule)load(context, "vkDestroyShaderModule");
	table->vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)load(context, "vkDeviceWaitIdle");
	table->vkEndCommandBuffer = (PFN_vkEndCommandBuffer)load(context, "vkEndCommandBuffer");
	table->vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)load(context, "vkFlushMappedMemoryRanges");
	table->vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)load(context, "vkFreeCommandBuffers");
	table->vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets)load(context, "vkFreeDescriptorSets");
	table->vkFreeMemory = (PFN_vkFreeMemory)load(context, "vkFreeMemory");
	table->vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)load(context, "vkGetBufferMemoryRequirements");
	table->vkGetDeviceMemoryCommitment = (PFN_vkGetDeviceMemoryCommitment)load(context, "vkGetDeviceMemoryCommitment");
	table->vkGetDeviceQueue = (PFN_vkGetDeviceQueue)load(context, "vkGetDeviceQueue");
	table->vkGetEventStatus = (PFN_vkGetEventStatus)load(context, "vkGetEventStatus");
	table->vkGetFenceStatus = (PFN_vkGetFenceStatus)load(context, "vkGetFenceStatus");
	table->vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)load(context, "vkGetImageMemoryRequirements");
	table->vkGetImageSparseMemoryRequirements = (PFN_vkGetImageSparseMemoryRequirements)load(context, "vkGetImageSparseMemoryRequirements");
	table->vkGetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout)load(context, "vkGetImageSubresourceLayout");
	table->vkGetPipelineCacheData = (PFN_vkGetPipelineCacheData)load(context, "vkGetPipelineCacheData");
	table->vkGetQueryPoolResults = (PFN_vkGetQueryPoolResults)load(context, "vkGetQueryPoolResults");
	table->vkGetRenderAreaGranularity = (PFN_vkGetRenderAreaGranularity)load(context, "vkGetRenderAreaGranularity");
	table->vkInvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges)load(context, "vkInvalidateMappedMemoryRanges");
	table->vkMapMemory = (PFN_vkMapMemory)load(context, "vkMapMemory");
	table->vkMergePipelineCaches = (PFN_vkMergePipelineCaches)load(context, "vkMergePipelineCaches");
	table->vkQueueBindSparse = (PFN_vkQueueBindSparse)load(context, "vkQueueBindSparse");
	table->vkQueueSubmit = (PFN_vkQueueSubmit)load(context, "vkQueueSubmit");
	table->vkQueueWaitIdle = (PFN_vkQueueWaitIdle)load(context, "vkQueueWaitIdle");
	table->vkResetCommandBuffer = (PFN_vkResetCommandBuffer)load(context, "vkResetCommandBuffer");
	table->vkResetCommandPool = (PFN_vkResetCommandPool)load(context, "vkResetCommandPool");
	table->vkResetDescriptorPool = (PFN_vkResetDescriptorPool)load(context, "vkResetDescriptorPool");
	table->vkResetEvent = (PFN_vkResetEvent)load(context, "vkResetEvent");
	table->vkResetFences = (PFN_vkResetFences)load(context, "vkResetFences");
	table->vkSetEvent = (PFN_vkSetEvent)load(context, "vkSetEvent");
	table->vkUnmapMemory = (PFN_vkUnmapMemory)load(context, "vkUnmapMemory");
	table->vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)load(context, "vkUpdateDescriptorSets");
	table->vkWaitForFences = (PFN_vkWaitForFences)load(context, "vkWaitForFences");
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
	table->vkBindBufferMemory2 = (PFN_vkBindBufferMemory2)load(context, "vkBindBufferMemory2");
	table->vkBindImageMemory2 = (PFN_vkBindImageMemory2)load(context, "vkBindImageMemory2");
	table->vkCmdDispatchBase = (PFN_vkCmdDispatchBase)load(context, "vkCmdDispatchBase");
	table->vkCmdSetDeviceMask = (PFN_vkCmdSetDeviceMask)load(context, "vkCmdSetDeviceMask");
	table->vkCreateDescriptorUpdateTemplate = (PFN_vkCreateDescriptorUpdateTemplate)load(context, "vkCreateDescriptorUpdateTemplate");
	table->vkCreateSamplerYcbcrConversion = (PFN_vkCreateSamplerYcbcrConversion)load(context, "vkCreateSamplerYcbcrConversion");
	table->vkDestroyDescriptorUpdateTemplate = (PFN_vkDestroyDescriptorUpdateTemplate)load(context, "vkDestroyDescriptorUpdateTemplate");
	table->vkDestroySamplerYcbcrConversion = (PFN_vkDestroySamplerYcbcrConversion)load(context, "vkDestroySamplerYcbcrConversion");
	table->vkGetBufferMemoryRequirements2 = (PFN_vkGetBufferMemoryRequirements2)load(context, "vkGetBufferMemoryRequirements2");
	table->vkGetDescriptorSetLayoutSupport = (PFN_vkGetDescriptorSetLayoutSupport)load(context, "vkGetDescriptorSetLayoutSupport");
	table->vkGetDeviceGroupPeerMemoryFeatures = (PFN_vkGetDeviceGroupPeerMemoryFeatures)load(context, "vkGetDeviceGroupPeerMemoryFeatures");
	table->vkGetDeviceQueue2 = (PFN_vkGetDeviceQueue2)load(context, "vkGetDeviceQueue2");
	table->vkGetImageMemoryRequirements2 = (PFN_vkGetImageMemoryRequirements2)load(context, "vkGetImageMemoryRequirements2");
	table->vkGetImageSparseMemoryRequirements2 = (PFN_vkGetImageSparseMemoryRequirements2)load(context, "vkGetImageSparseMemoryRequirements2");
	table->vkTrimCommandPool = (PFN_vkTrimCommandPool)load(context, "vkTrimCommandPool");
	table->vkUpdateDescriptorSetWithTemplate = (PFN_vkUpdateDescriptorSetWithTemplate)load(context, "vkUpdateDescriptorSetWithTemplate");
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_AMD_buffer_marker)
	table->vkCmdWriteBufferMarkerAMD = (PFN_vkCmdWriteBufferMarkerAMD)load(context, "vkCmdWriteBufferMarkerAMD");
#endif /* defined(VK_AMD_buffer_marker) */
#if defined(VK_AMD_display_native_hdr)
	table->vkSetLocalDimmingAMD = (PFN_vkSetLocalDimmingAMD)load(context, "vkSetLocalDimmingAMD");
#endif /* defined(VK_AMD_display_native_hdr) */
#if defined(VK_AMD_draw_indirect_count)
	table->vkCmdDrawIndexedIndirectCountAMD = (PFN_vkCmdDrawIndexedIndirectCountAMD)load(context, "vkCmdDrawIndexedIndirectCountAMD");
	table->vkCmdDrawIndirectCountAMD = (PFN_vkCmdDrawIndirectCountAMD)load(context, "vkCmdDrawIndirectCountAMD");
#endif /* defined(VK_AMD_draw_indirect_count) */
#if defined(VK_AMD_shader_info)
	table->vkGetShaderInfoAMD = (PFN_vkGetShaderInfoAMD)load(context, "vkGetShaderInfoAMD");
#endif /* defined(VK_AMD_shader_info) */
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
	table->vkGetAndroidHardwareBufferPropertiesANDROID = (PFN_vkGetAndroidHardwareBufferPropertiesANDROID)load(context, "vkGetAndroidHardwareBufferPropertiesANDROID");
	table->vkGetMemoryAndroidHardwareBufferANDROID = (PFN_vkGetMemoryAndroidHardwareBufferANDROID)load(context, "vkGetMemoryAndroidHardwareBufferANDROID");
#endif /* defined(VK_ANDROID_external_memory_android_hardware_buffer) */
#if defined(VK_EXT_buffer_device_address)
	table->vkGetBufferDeviceAddressEXT = (PFN_vkGetBufferDeviceAddressEXT)load(context, "vkGetBufferDeviceAddressEXT");
#endif /* defined(VK_EXT_buffer_device_address) */
#if defined(VK_EXT_calibrated_timestamps)
	table->vkGetCalibratedTimestampsEXT = (PFN_vkGetCalibratedTimestampsEXT)load(context, "vkGetCalibratedTimestampsEXT");
#endif /* defined(VK_EXT_calibrated_timestamps) */
#if defined(VK_EXT_conditional_rendering)
	table->vkCmdBeginConditionalRenderingEXT = (PFN_vkCmdBeginConditionalRenderingEXT)load(context, "vkCmdBeginConditionalRenderingEXT");
	table->vkCmdEndConditionalRenderingEXT = (PFN_vkCmdEndConditionalRenderingEXT)load(context, "vkCmdEndConditionalRenderingEXT");
#endif /* defined(VK_EXT_conditional_rendering) */
#if defined(VK_EXT_debug_marker)
	table->vkCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)load(context, "vkCmdDebugMarkerBeginEXT");
	table->vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)load(context, "vkCmdDebugMarkerEndEXT");
	table->vkCmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)load(context, "vkCmdDebugMarkerInsertEXT");
	table->vkDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)load(context, "vkDebugMarkerSetObjectNameEXT");
	table->vkDebugMarkerSetObjectTagEXT = (PFN_vkDebugMarkerSetObjectTagEXT)load(context, "vkDebugMarkerSetObjectTagEXT");
#endif /* defined(VK_EXT_debug_marker) */
#if defined(VK_EXT_debug_utils)
	table->vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)load(context, "vkCmdBeginDebugUtilsLabelEXT");
	table->vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)load(context, "vkCmdEndDebugUtilsLabelEXT");
	table->vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)load(context, "vkCmdInsertDebugUtilsLabelEXT");
	table->vkQueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT)load(context, "vkQueueBeginDebugUtilsLabelEXT");
	table->vkQueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT)load(context, "vkQueueEndDebugUtilsLabelEXT");
	table->vkQueueInsertDebugUtilsLabelEXT = (PFN_vkQueueInsertDebugUtilsLabelEXT)load(context, "vkQueueInsertDebugUtilsLabelEXT");
	table->vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)load(context, "vkSetDebugUtilsObjectNameEXT");
	table->vkSetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)load(context, "vkSetDebugUtilsObjectTagEXT");
#endif /* defined(VK_EXT_debug_utils) */
#if defined(VK_EXT_discard_rectangles)
	table->vkCmdSetDiscardRectangleEXT = (PFN_vkCmdSetDiscardRectangleEXT)load(context, "vkCmdSetDiscardRectangleEXT");
#endif /* defined(VK_EXT_discard_rectangles) */
#if defined(VK_EXT_display_control)
	table->vkDisplayPowerControlEXT = (PFN_vkDisplayPowerControlEXT)load(context, "vkDisplayPowerControlEXT");
	table->vkGetSwapchainCounterEXT = (PFN_vkGetSwapchainCounterEXT)load(context, "vkGetSwapchainCounterEXT");
	table->vkRegisterDeviceEventEXT = (PFN_vkRegisterDeviceEventEXT)load(context, "vkRegisterDeviceEventEXT");
	table->vkRegisterDisplayEventEXT = (PFN_vkRegisterDisplayEventEXT)load(context, "vkRegisterDisplayEventEXT");
#endif /* defined(VK_EXT_display_control) */
#if defined(VK_EXT_external_memory_host)
	table->vkGetMemoryHostPointerPropertiesEXT = (PFN_vkGetMemoryHostPointerPropertiesEXT)load(context, "vkGetMemoryHostPointerPropertiesEXT");
#endif /* defined(VK_EXT_external_memory_host) */
#if defined(VK_EXT_full_screen_exclusive)
	table->vkAcquireFullScreenExclusiveModeEXT = (PFN_vkAcquireFullScreenExclusiveModeEXT)load(context, "vkAcquireFullScreenExclusiveModeEXT");
	table->vkReleaseFullScreenExclusiveModeEXT = (PFN_vkReleaseFullScreenExclusiveModeEXT)load(context, "vkReleaseFullScreenExclusiveModeEXT");
#endif /* defined(VK_EXT_full_screen_exclusive) */
#if defined(VK_EXT_hdr_metadata)
	table->vkSetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT)load(context, "vkSetHdrMetadataEXT");
#endif /* defined(VK_EXT_hdr_metadata) */
#if defined(VK_EXT_host_query_reset)
	table->vkResetQueryPoolEXT = (PFN_vkResetQueryPoolEXT)load(context, "vkResetQueryPoolEXT");
#endif /* defined(VK_EXT_host_query_reset) */
#if defined(VK_EXT_image_drm_format_modifier)
	table->vkGetImageDrmFormatModifierPropertiesEXT = (PFN_vkGetImageDrmFormatModifierPropertiesEXT)load(context, "vkGetImageDrmFormatModifierPropertiesEXT");
#endif /* defined(VK_EXT_image_drm_format_modifier) */
#if defined(VK_EXT_sample_locations)
	table->vkCmdSetSampleLocationsEXT = (PFN_vkCmdSetSampleLocationsEXT)load(context, "vkCmdSetSampleLocationsEXT");
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_EXT_transform_feedback)
	table->vkCmdBeginQueryIndexedEXT = (PFN_vkCmdBeginQueryIndexedEXT)load(context, "vkCmdBeginQueryIndexedEXT");
	table->vkCmdBeginTransformFeedbackEXT = (PFN_vkCmdBeginTransformFeedbackEXT)load(context, "vkCmdBeginTransformFeedbackEXT");
	table->vkCmdBindTransformFeedbackBuffersEXT = (PFN_vkCmdBindTransformFeedbackBuffersEXT)load(context, "vkCmdBindTransformFeedbackBuffersEXT");
	table->vkCmdDrawIndirectByteCountEXT = (PFN_vkCmdDrawIndirectByteCountEXT)load(context, "vkCmdDrawIndirectByteCountEXT");
	table->vkCmdEndQueryIndexedEXT = (PFN_vkCmdEndQueryIndexedEXT)load(context, "vkCmdEndQueryIndexedEXT");
	table->vkCmdEndTransformFeedbackEXT = (PFN_vkCmdEndTransformFeedbackEXT)load(context, "vkCmdEndTransformFeedbackEXT");
#endif /* defined(VK_EXT_transform_feedback) */
#if defined(VK_EXT_validation_cache)
	table->vkCreateValidationCacheEXT = (PFN_vkCreateValidationCacheEXT)load(context, "vkCreateValidationCacheEXT");
	table->vkDestroyValidationCacheEXT = (PFN_vkDestroyValidationCacheEXT)load(context, "vkDestroyValidationCacheEXT");
	table->vkGetValidationCacheDataEXT = (PFN_vkGetValidationCacheDataEXT)load(context, "vkGetValidationCacheDataEXT");
	table->vkMergeValidationCachesEXT = (PFN_vkMergeValidationCachesEXT)load(context, "vkMergeValidationCachesEXT");
#endif /* defined(VK_EXT_validation_cache) */
#if defined(VK_GOOGLE_display_timing)
	table->vkGetPastPresentationTimingGOOGLE = (PFN_vkGetPastPresentationTimingGOOGLE)load(context, "vkGetPastPresentationTimingGOOGLE");
	table->vkGetRefreshCycleDurationGOOGLE = (PFN_vkGetRefreshCycleDurationGOOGLE)load(context, "vkGetRefreshCycleDurationGOOGLE");
#endif /* defined(VK_GOOGLE_display_timing) */
#if defined(VK_KHR_bind_memory2)
	table->vkBindBufferMemory2KHR = (PFN_vkBindBufferMemory2KHR)load(context, "vkBindBufferMemory2KHR");
	table->vkBindImageMemory2KHR = (PFN_vkBindImageMemory2KHR)load(context, "vkBindImageMemory2KHR");
#endif /* defined(VK_KHR_bind_memory2) */
#if defined(VK_KHR_create_renderpass2)
	table->vkCmdBeginRenderPass2KHR = (PFN_vkCmdBeginRenderPass2KHR)load(context, "vkCmdBeginRenderPass2KHR");
	table->vkCmdEndRenderPass2KHR = (PFN_vkCmdEndRenderPass2KHR)load(context, "vkCmdEndRenderPass2KHR");
	table->vkCmdNextSubpass2KHR = (PFN_vkCmdNextSubpass2KHR)load(context, "vkCmdNextSubpass2KHR");
	table->vkCreateRenderPass2KHR = (PFN_vkCreateRenderPass2KHR)load(context, "vkCreateRenderPass2KHR");
#endif /* defined(VK_KHR_create_renderpass2) */
#if defined(VK_KHR_descriptor_update_template)
	table->vkCreateDescriptorUpdateTemplateKHR = (PFN_vkCreateDescriptorUpdateTemplateKHR)load(context, "vkCreateDescriptorUpdateTemplateKHR");
	table->vkDestroyDescriptorUpdateTemplateKHR = (PFN_vkDestroyDescriptorUpdateTemplateKHR)load(context, "vkDestroyDescriptorUpdateTemplateKHR");
	table->vkUpdateDescriptorSetWithTemplateKHR = (PFN_vkUpdateDescriptorSetWithTemplateKHR)load(context, "vkUpdateDescriptorSetWithTemplateKHR");
#endif /* defined(VK_KHR_descriptor_update_template) */
#if defined(VK_KHR_device_group)
	table->vkCmdDispatchBaseKHR = (PFN_vkCmdDispatchBaseKHR)load(context, "vkCmdDispatchBaseKHR");
	table->vkCmdSetDeviceMaskKHR = (PFN_vkCmdSetDeviceMaskKHR)load(context, "vkCmdSetDeviceMaskKHR");
	table->vkGetDeviceGroupPeerMemoryFeaturesKHR = (PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR)load(context, "vkGetDeviceGroupPeerMemoryFeaturesKHR");
#endif /* defined(VK_KHR_device_group) */
#if defined(VK_KHR_display_swapchain)
	table->vkCreateSharedSwapchainsKHR = (PFN_vkCreateSharedSwapchainsKHR)load(context, "vkCreateSharedSwapchainsKHR");
#endif /* defined(VK_KHR_display_swapchain) */
#if defined(VK_KHR_draw_indirect_count)
	table->vkCmdDrawIndexedIndirectCountKHR = (PFN_vkCmdDrawIndexedIndirectCountKHR)load(context, "vkCmdDrawIndexedIndirectCountKHR");
	table->vkCmdDrawIndirectCountKHR = (PFN_vkCmdDrawIndirectCountKHR)load(context, "vkCmdDrawIndirectCountKHR");
#endif /* defined(VK_KHR_draw_indirect_count) */
#if defined(VK_KHR_external_fence_fd)
	table->vkGetFenceFdKHR = (PFN_vkGetFenceFdKHR)load(context, "vkGetFenceFdKHR");
	table->vkImportFenceFdKHR = (PFN_vkImportFenceFdKHR)load(context, "vkImportFenceFdKHR");
#endif /* defined(VK_KHR_external_fence_fd) */
#if defined(VK_KHR_external_fence_win32)
	table->vkGetFenceWin32HandleKHR = (PFN_vkGetFenceWin32HandleKHR)load(context, "vkGetFenceWin32HandleKHR");
	table->vkImportFenceWin32HandleKHR = (PFN_vkImportFenceWin32HandleKHR)load(context, "vkImportFenceWin32HandleKHR");
#endif /* defined(VK_KHR_external_fence_win32) */
#if defined(VK_KHR_external_memory_fd)
	table->vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)load(context, "vkGetMemoryFdKHR");
	table->vkGetMemoryFdPropertiesKHR = (PFN_vkGetMemoryFdPropertiesKHR)load(context, "vkGetMemoryFdPropertiesKHR");
#endif /* defined(VK_KHR_external_memory_fd) */
#if defined(VK_KHR_external_memory_win32)
	table->vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)load(context, "vkGetMemoryWin32HandleKHR");
	table->vkGetMemoryWin32HandlePropertiesKHR = (PFN_vkGetMemoryWin32HandlePropertiesKHR)load(context, "vkGetMemoryWin32HandlePropertiesKHR");
#endif /* defined(VK_KHR_external_memory_win32) */
#if defined(VK_KHR_external_semaphore_fd)
	table->vkGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)load(context, "vkGetSemaphoreFdKHR");
	table->vkImportSemaphoreFdKHR = (PFN_vkImportSemaphoreFdKHR)load(context, "vkImportSemaphoreFdKHR");
#endif /* defined(VK_KHR_external_semaphore_fd) */
#if defined(VK_KHR_external_semaphore_win32)
	table->vkGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)load(context, "vkGetSemaphoreWin32HandleKHR");
	table->vkImportSemaphoreWin32HandleKHR = (PFN_vkImportSemaphoreWin32HandleKHR)load(context, "vkImportSemaphoreWin32HandleKHR");
#endif /* defined(VK_KHR_external_semaphore_win32) */
#if defined(VK_KHR_get_memory_requirements2)
	table->vkGetBufferMemoryRequirements2KHR = (PFN_vkGetBufferMemoryRequirements2KHR)load(context, "vkGetBufferMemoryRequirements2KHR");
	table->vkGetImageMemoryRequirements2KHR = (PFN_vkGetImageMemoryRequirements2KHR)load(context, "vkGetImageMemoryRequirements2KHR");
	table->vkGetImageSparseMemoryRequirements2KHR = (PFN_vkGetImageSparseMemoryRequirements2KHR)load(context, "vkGetImageSparseMemoryRequirements2KHR");
#endif /* defined(VK_KHR_get_memory_requirements2) */
#if defined(VK_KHR_maintenance1)
	table->vkTrimCommandPoolKHR = (PFN_vkTrimCommandPoolKHR)load(context, "vkTrimCommandPoolKHR");
#endif /* defined(VK_KHR_maintenance1) */
#if defined(VK_KHR_maintenance3)
	table->vkGetDescriptorSetLayoutSupportKHR = (PFN_vkGetDescriptorSetLayoutSupportKHR)load(context, "vkGetDescriptorSetLayoutSupportKHR");
#endif /* defined(VK_KHR_maintenance3) */
#if defined(VK_KHR_push_descriptor)
	table->vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)load(context, "vkCmdPushDescriptorSetKHR");
#endif /* defined(VK_KHR_push_descriptor) */
#if defined(VK_KHR_sampler_ycbcr_conversion)
	table->vkCreateSamplerYcbcrConversionKHR = (PFN_vkCreateSamplerYcbcrConversionKHR)load(context, "vkCreateSamplerYcbcrConversionKHR");
	table->vkDestroySamplerYcbcrConversionKHR = (PFN_vkDestroySamplerYcbcrConversionKHR)load(context, "vkDestroySamplerYcbcrConversionKHR");
#endif /* defined(VK_KHR_sampler_ycbcr_conversion) */
#if defined(VK_KHR_shared_presentable_image)
	table->vkGetSwapchainStatusKHR = (PFN_vkGetSwapchainStatusKHR)load(context, "vkGetSwapchainStatusKHR");
#endif /* defined(VK_KHR_shared_presentable_image) */
#if defined(VK_KHR_swapchain)
	table->vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)load(context, "vkAcquireNextImageKHR");
	table->vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)load(context, "vkCreateSwapchainKHR");
	table->vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)load(context, "vkDestroySwapchainKHR");
	table->vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)load(context, "vkGetSwapchainImagesKHR");
	table->vkQueuePresentKHR = (PFN_vkQueuePresentKHR)load(context, "vkQueuePresentKHR");
#endif /* defined(VK_KHR_swapchain) */
#if defined(VK_NVX_device_generated_commands)
	table->vkCmdProcessCommandsNVX = (PFN_vkCmdProcessCommandsNVX)load(context, "vkCmdProcessCommandsNVX");
	table->vkCmdReserveSpaceForCommandsNVX = (PFN_vkCmdReserveSpaceForCommandsNVX)load(context, "vkCmdReserveSpaceForCommandsNVX");
	table->vkCreateIndirectCommandsLayoutNVX = (PFN_vkCreateIndirectCommandsLayoutNVX)load(context, "vkCreateIndirectCommandsLayoutNVX");
	table->vkCreateObjectTableNVX = (PFN_vkCreateObjectTableNVX)load(context, "vkCreateObjectTableNVX");
	table->vkDestroyIndirectCommandsLayoutNVX = (PFN_vkDestroyIndirectCommandsLayoutNVX)load(context, "vkDestroyIndirectCommandsLayoutNVX");
	table->vkDestroyObjectTableNVX = (PFN_vkDestroyObjectTableNVX)load(context, "vkDestroyObjectTableNVX");
	table->vkRegisterObjectsNVX = (PFN_vkRegisterObjectsNVX)load(context, "vkRegisterObjectsNVX");
	table->vkUnregisterObjectsNVX = (PFN_vkUnregisterObjectsNVX)load(context, "vkUnregisterObjectsNVX");
#endif /* defined(VK_NVX_device_generated_commands) */
#if defined(VK_NVX_image_view_handle)
	table->vkGetImageViewHandleNVX = (PFN_vkGetImageViewHandleNVX)load(context, "vkGetImageViewHandleNVX");
#endif /* defined(VK_NVX_image_view_handle) */
#if defined(VK_NV_clip_space_w_scaling)
	table->vkCmdSetViewportWScalingNV = (PFN_vkCmdSetViewportWScalingNV)load(context, "vkCmdSetViewportWScalingNV");
#endif /* defined(VK_NV_clip_space_w_scaling) */
#if defined(VK_NV_device_diagnostic_checkpoints)
	table->vkCmdSetCheckpointNV = (PFN_vkCmdSetCheckpointNV)load(context, "vkCmdSetCheckpointNV");
	table->vkGetQueueCheckpointDataNV = (PFN_vkGetQueueCheckpointDataNV)load(context, "vkGetQueueCheckpointDataNV");
#endif /* defined(VK_NV_device_diagnostic_checkpoints) */
#if defined(VK_NV_external_memory_win32)
	table->vkGetMemoryWin32HandleNV = (PFN_vkGetMemoryWin32HandleNV)load(context, "vkGetMemoryWin32HandleNV");
#endif /* defined(VK_NV_external_memory_win32) */
#if defined(VK_NV_mesh_shader)
	table->vkCmdDrawMeshTasksIndirectCountNV = (PFN_vkCmdDrawMeshTasksIndirectCountNV)load(context, "vkCmdDrawMeshTasksIndirectCountNV");
	table->vkCmdDrawMeshTasksIndirectNV = (PFN_vkCmdDrawMeshTasksIndirectNV)load(context, "vkCmdDrawMeshTasksIndirectNV");
	table->vkCmdDrawMeshTasksNV = (PFN_vkCmdDrawMeshTasksNV)load(context, "vkCmdDrawMeshTasksNV");
#endif /* defined(VK_NV_mesh_shader) */
#if defined(VK_NV_ray_tracing)
	table->vkBindAccelerationStructureMemoryNV = (PFN_vkBindAccelerationStructureMemoryNV)load(context, "vkBindAccelerationStructureMemoryNV");
	table->vkCmdBuildAccelerationStructureNV = (PFN_vkCmdBuildAccelerationStructureNV)load(context, "vkCmdBuildAccelerationStructureNV");
	table->vkCmdCopyAccelerationStructureNV = (PFN_vkCmdCopyAccelerationStructureNV)load(context, "vkCmdCopyAccelerationStructureNV");
	table->vkCmdTraceRaysNV = (PFN_vkCmdTraceRaysNV)load(context, "vkCmdTraceRaysNV");
	table->vkCmdWriteAccelerationStructuresPropertiesNV = (PFN_vkCmdWriteAccelerationStructuresPropertiesNV)load(context, "vkCmdWriteAccelerationStructuresPropertiesNV");
	table->vkCompileDeferredNV = (PFN_vkCompileDeferredNV)load(context, "vkCompileDeferredNV");
	table->vkCreateAccelerationStructureNV = (PFN_vkCreateAccelerationStructureNV)load(context, "vkCreateAccelerationStructureNV");
	table->vkCreateRayTracingPipelinesNV = (PFN_vkCreateRayTracingPipelinesNV)load(context, "vkCreateRayTracingPipelinesNV");
	table->vkDestroyAccelerationStructureNV = (PFN_vkDestroyAccelerationStructureNV)load(context, "vkDestroyAccelerationStructureNV");
	table->vkGetAccelerationStructureHandleNV = (PFN_vkGetAccelerationStructureHandleNV)load(context, "vkGetAccelerationStructureHandleNV");
	table->vkGetAccelerationStructureMemoryRequirementsNV = (PFN_vkGetAccelerationStructureMemoryRequirementsNV)load(context, "vkGetAccelerationStructureMemoryRequirementsNV");
	table->vkGetRayTracingShaderGroupHandlesNV = (PFN_vkGetRayTracingShaderGroupHandlesNV)load(context, "vkGetRayTracingShaderGroupHandlesNV");
#endif /* defined(VK_NV_ray_tracing) */
#if defined(VK_NV_scissor_exclusive)
	table->vkCmdSetExclusiveScissorNV = (PFN_vkCmdSetExclusiveScissorNV)load(context, "vkCmdSetExclusiveScissorNV");
#endif /* defined(VK_NV_scissor_exclusive) */
#if defined(VK_NV_shading_rate_image)
	table->vkCmdBindShadingRateImageNV = (PFN_vkCmdBindShadingRateImageNV)load(context, "vkCmdBindShadingRateImageNV");
	table->vkCmdSetCoarseSampleOrderNV = (PFN_vkCmdSetCoarseSampleOrderNV)load(context, "vkCmdSetCoarseSampleOrderNV");
	table->vkCmdSetViewportShadingRatePaletteNV = (PFN_vkCmdSetViewportShadingRatePaletteNV)load(context, "vkCmdSetViewportShadingRatePaletteNV");
#endif /* defined(VK_NV_shading_rate_image) */
#if (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_device_group) && defined(VK_EXT_full_screen_exclusive))
	table->vkGetDeviceGroupSurfacePresentModes2EXT = (PFN_vkGetDeviceGroupSurfacePresentModes2EXT)load(context, "vkGetDeviceGroupSurfacePresentModes2EXT");
#endif /* (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_device_group) && defined(VK_EXT_full_screen_exclusive)) */
#if (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1))
	table->vkCmdPushDescriptorSetWithTemplateKHR = (PFN_vkCmdPushDescriptorSetWithTemplateKHR)load(context, "vkCmdPushDescriptorSetWithTemplateKHR");
#endif /* (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	table->vkGetDeviceGroupPresentCapabilitiesKHR = (PFN_vkGetDeviceGroupPresentCapabilitiesKHR)load(context, "vkGetDeviceGroupPresentCapabilitiesKHR");
	table->vkGetDeviceGroupSurfacePresentModesKHR = (PFN_vkGetDeviceGroupSurfacePresentModesKHR)load(context, "vkGetDeviceGroupSurfacePresentModesKHR");
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	table->vkAcquireNextImage2KHR = (PFN_vkAcquireNextImage2KHR)load(context, "vkAcquireNextImage2KHR");
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
	/* VOLK_GENERATE_LOAD_DEVICE_TABLE */
}

#if defined(VK_VERSION_1_0)

VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(
	const VkInstanceCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkInstance* pInstance)
{
	return Volk::funcPtrs->vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(
	VkInstance                                  instance,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyInstance(instance, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(
	VkInstance                                  instance,
	uint32_t* pPhysicalDeviceCount,
	VkPhysicalDevice* pPhysicalDevices)
{
	return Volk::funcPtrs->vkEnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures(
	VkPhysicalDevice                            physicalDevice,
	VkPhysicalDeviceFeatures* pFeatures)
{
	return Volk::funcPtrs->vkGetPhysicalDeviceFeatures(physicalDevice, pFeatures);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties(
	VkPhysicalDevice                            physicalDevice,
	VkFormat                                    format,
	VkFormatProperties* pFormatProperties)
{
	return Volk::funcPtrs->vkGetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties(
	VkPhysicalDevice                            physicalDevice,
	VkFormat                                    format,
	VkImageType                                 type,
	VkImageTiling                               tiling,
	VkImageUsageFlags                           usage,
	VkImageCreateFlags                          flags,
	VkImageFormatProperties* pImageFormatProperties)
{
	return Volk::funcPtrs->vkGetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(
	VkPhysicalDevice                            physicalDevice,
	VkPhysicalDeviceProperties* pProperties)
{
	return Volk::funcPtrs->vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
	VkPhysicalDevice                            physicalDevice,
	uint32_t* pQueueFamilyPropertyCount,
	VkQueueFamilyProperties* pQueueFamilyProperties)
{
	return Volk::funcPtrs->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(
	VkPhysicalDevice                            physicalDevice,
	VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{
	return Volk::funcPtrs->vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(
	VkInstance                                  instance,
	const char* pName)
{
	return Volk::funcPtrs->vkGetInstanceProcAddr(instance, pName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(
	VkDevice                                    device,
	const char* pName)
{
	return Volk::funcPtrs->vkGetDeviceProcAddr(device, pName);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(
	VkPhysicalDevice                            physicalDevice,
	const VkDeviceCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDevice* pDevice)
{
	return Volk::funcPtrs->vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(
	VkDevice                                    device,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyDevice(device, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
	const char* pLayerName,
	uint32_t* pPropertyCount,
	VkExtensionProperties* pProperties)
{
	return Volk::funcPtrs->vkEnumerateInstanceExtensionProperties(pLayerName, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
	VkPhysicalDevice                            physicalDevice,
	const char* pLayerName,
	uint32_t* pPropertyCount,
	VkExtensionProperties* pProperties)
{
	return Volk::funcPtrs->vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
	uint32_t* pPropertyCount,
	VkLayerProperties* pProperties)
{
	return Volk::funcPtrs->vkEnumerateInstanceLayerProperties(pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(
	VkPhysicalDevice                            physicalDevice,
	uint32_t* pPropertyCount,
	VkLayerProperties* pProperties)
{
	return Volk::funcPtrs->vkEnumerateDeviceLayerProperties(physicalDevice, pPropertyCount, pProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(
	VkDevice                                    device,
	uint32_t                                    queueFamilyIndex,
	uint32_t                                    queueIndex,
	VkQueue* pQueue)
{
	return Volk::funcPtrs->vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(
	VkQueue                                     queue,
	uint32_t                                    submitCount,
	const VkSubmitInfo* pSubmits,
	VkFence                                     fence)
{
	return Volk::funcPtrs->vkQueueSubmit(queue, submitCount, pSubmits, fence);
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle(
	VkQueue                                     queue)
{
	return Volk::funcPtrs->vkQueueWaitIdle(queue);
}

VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(
	VkDevice                                    device)
{
	return Volk::funcPtrs->vkDeviceWaitIdle(device);
}

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(
	VkDevice                                    device,
	const VkMemoryAllocateInfo* pAllocateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDeviceMemory* pMemory)
{
	return Volk::funcPtrs->vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
}

VKAPI_ATTR void VKAPI_CALL vkFreeMemory(
	VkDevice                                    device,
	VkDeviceMemory                              memory,
	const VkAllocationCallbacks* pAllocator)
{
	Volk::funcPtrs->vkFreeMemory(device, memory, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(
	VkDevice                                    device,
	VkDeviceMemory                              memory,
	VkDeviceSize                                offset,
	VkDeviceSize                                size,
	VkMemoryMapFlags                            flags,
	void** ppData)
{
	return Volk::funcPtrs->vkMapMemory(device, memory, offset, size, flags, ppData);
}

VKAPI_ATTR void VKAPI_CALL vkUnmapMemory(
	VkDevice                                    device,
	VkDeviceMemory                              memory)
{
	return Volk::funcPtrs->vkUnmapMemory(device, memory);
}

VKAPI_ATTR VkResult VKAPI_CALL vkFlushMappedMemoryRanges(
	VkDevice                                    device,
	uint32_t                                    memoryRangeCount,
	const VkMappedMemoryRange* pMemoryRanges)
{
	return Volk::funcPtrs->vkFlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

VKAPI_ATTR VkResult VKAPI_CALL vkInvalidateMappedMemoryRanges(
	VkDevice                                    device,
	uint32_t                                    memoryRangeCount,
	const VkMappedMemoryRange* pMemoryRanges)
{
	return Volk::funcPtrs->vkInvalidateMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceMemoryCommitment(
	VkDevice                                    device,
	VkDeviceMemory                              memory,
	VkDeviceSize* pCommittedMemoryInBytes)
{
	return Volk::funcPtrs->vkGetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(
	VkDevice                                    device,
	VkBuffer                                    buffer,
	VkDeviceMemory                              memory,
	VkDeviceSize                                memoryOffset)
{
	return Volk::funcPtrs->vkBindBufferMemory(device, buffer, memory, memoryOffset);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory(
	VkDevice                                    device,
	VkImage                                     image,
	VkDeviceMemory                              memory,
	VkDeviceSize                                memoryOffset)
{
	return Volk::funcPtrs->vkBindImageMemory(device, image, memory, memoryOffset);
}

VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements(
	VkDevice                                    device,
	VkBuffer                                    buffer,
	VkMemoryRequirements* pMemoryRequirements)
{
	return Volk::funcPtrs->vkGetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements(
	VkDevice                                    device,
	VkImage                                     image,
	VkMemoryRequirements* pMemoryRequirements)
{
	return Volk::funcPtrs->vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetImageSparseMemoryRequirements(
	VkDevice                                    device,
	VkImage                                     image,
	uint32_t* pSparseMemoryRequirementCount,
	VkSparseImageMemoryRequirements* pSparseMemoryRequirements)
{
	return Volk::funcPtrs->vkGetImageSparseMemoryRequirements(device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties(
	VkPhysicalDevice                            physicalDevice,
	VkFormat                                    format,
	VkImageType                                 type,
	VkSampleCountFlagBits                       samples,
	VkImageUsageFlags                           usage,
	VkImageTiling                               tiling,
	uint32_t* pPropertyCount,
	VkSparseImageFormatProperties* pProperties)
{
	return Volk::funcPtrs->vkGetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueBindSparse(
	VkQueue                                     queue,
	uint32_t                                    bindInfoCount,
	const VkBindSparseInfo* pBindInfo,
	VkFence                                     fence)
{
	return Volk::funcPtrs->vkQueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateFence(
	VkDevice                                    device,
	const VkFenceCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkFence* pFence)
{
	return Volk::funcPtrs->vkCreateFence(device, pCreateInfo, pAllocator, pFence);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyFence(
	VkDevice                                    device,
	VkFence                                     fence,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyFence(device, fence, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetFences(
	VkDevice                                    device,
	uint32_t                                    fenceCount,
	const VkFence* pFences)
{
	return Volk::funcPtrs->vkResetFences(device, fenceCount, pFences);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceStatus(
	VkDevice                                    device,
	VkFence                                     fence)
{
	return Volk::funcPtrs->vkGetFenceStatus(device, fence);
}

VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences(
	VkDevice                                    device,
	uint32_t                                    fenceCount,
	const VkFence* pFences,
	VkBool32                                    waitAll,
	uint64_t                                    timeout)
{
	return Volk::funcPtrs->vkWaitForFences(device, fenceCount, pFences, waitAll, timeout);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSemaphore(
	VkDevice                                    device,
	const VkSemaphoreCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkSemaphore* pSemaphore)
{
	return Volk::funcPtrs->vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
}

VKAPI_ATTR void VKAPI_CALL vkDestroySemaphore(
	VkDevice                                    device,
	VkSemaphore                                 semaphore,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroySemaphore(device, semaphore, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateEvent(
	VkDevice                                    device,
	const VkEventCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkEvent* pEvent)
{
	return Volk::funcPtrs->vkCreateEvent(device, pCreateInfo, pAllocator, pEvent);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyEvent(
	VkDevice                                    device,
	VkEvent                                     event,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyEvent(device, event, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetEventStatus(
	VkDevice                                    device,
	VkEvent                                     event)
{
	return Volk::funcPtrs->vkGetEventStatus(device, event);
}

VKAPI_ATTR VkResult VKAPI_CALL vkSetEvent(
	VkDevice                                    device,
	VkEvent                                     event)
{
	return Volk::funcPtrs->vkSetEvent(device, event);
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetEvent(
	VkDevice                                    device,
	VkEvent                                     event)
{
	return Volk::funcPtrs->vkResetEvent(device, event);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateQueryPool(
	VkDevice                                    device,
	const VkQueryPoolCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkQueryPool* pQueryPool)
{
	return Volk::funcPtrs->vkCreateQueryPool(device, pCreateInfo, pAllocator, pQueryPool);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyQueryPool(
	VkDevice                                    device,
	VkQueryPool                                 queryPool,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyQueryPool(device, queryPool, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetQueryPoolResults(
	VkDevice                                    device,
	VkQueryPool                                 queryPool,
	uint32_t                                    firstQuery,
	uint32_t                                    queryCount,
	size_t                                      dataSize,
	void* pData,
	VkDeviceSize                                stride,
	VkQueryResultFlags                          flags)
{
	return Volk::funcPtrs->vkGetQueryPoolResults(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(
	VkDevice                                    device,
	const VkBufferCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkBuffer* pBuffer)
{
	return Volk::funcPtrs->vkCreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(
	VkDevice                                    device,
	VkBuffer                                    buffer,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyBuffer(device, buffer, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateBufferView(
	VkDevice                                    device,
	const VkBufferViewCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkBufferView* pView)
{
	return Volk::funcPtrs->vkCreateBufferView(device, pCreateInfo, pAllocator, pView);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyBufferView(
	VkDevice                                    device,
	VkBufferView                                bufferView,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyBufferView(device, bufferView, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateImage(
	VkDevice                                    device,
	const VkImageCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkImage* pImage)
{
	return Volk::funcPtrs->vkCreateImage(device, pCreateInfo, pAllocator, pImage);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyImage(
	VkDevice                                    device,
	VkImage                                     image,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyImage(device, image, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkGetImageSubresourceLayout(
	VkDevice                                    device,
	VkImage                                     image,
	const VkImageSubresource* pSubresource,
	VkSubresourceLayout* pLayout)
{
	return Volk::funcPtrs->vkGetImageSubresourceLayout(device, image, pSubresource, pLayout);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateImageView(
	VkDevice                                    device,
	const VkImageViewCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkImageView* pView)
{
	return Volk::funcPtrs->vkCreateImageView(device, pCreateInfo, pAllocator, pView);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyImageView(
	VkDevice                                    device,
	VkImageView                                 imageView,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyImageView(device, imageView, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateShaderModule(
	VkDevice                                    device,
	const VkShaderModuleCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkShaderModule* pShaderModule)
{
	return Volk::funcPtrs->vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyShaderModule(
	VkDevice                                    device,
	VkShaderModule                              shaderModule,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyShaderModule(device, shaderModule, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineCache(
	VkDevice                                    device,
	const VkPipelineCacheCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkPipelineCache* pPipelineCache)
{
	return Volk::funcPtrs->vkCreatePipelineCache(device, pCreateInfo, pAllocator, pPipelineCache);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineCache(
	VkDevice                                    device,
	VkPipelineCache                             pipelineCache,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyPipelineCache(device, pipelineCache, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineCacheData(
	VkDevice                                    device,
	VkPipelineCache                             pipelineCache,
	size_t* pDataSize,
	void* pData)
{
	return Volk::funcPtrs->vkGetPipelineCacheData(device, pipelineCache, pDataSize, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkMergePipelineCaches(
	VkDevice                                    device,
	VkPipelineCache                             dstCache,
	uint32_t                                    srcCacheCount,
	const VkPipelineCache* pSrcCaches)
{
	return Volk::funcPtrs->vkMergePipelineCaches(device, dstCache, srcCacheCount, pSrcCaches);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(
	VkDevice                                    device,
	VkPipelineCache                             pipelineCache,
	uint32_t                                    createInfoCount,
	const VkGraphicsPipelineCreateInfo* pCreateInfos,
	const VkAllocationCallbacks* pAllocator,
	VkPipeline* pPipelines)
{
	return Volk::funcPtrs->vkCreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateComputePipelines(
	VkDevice                                    device,
	VkPipelineCache                             pipelineCache,
	uint32_t                                    createInfoCount,
	const VkComputePipelineCreateInfo* pCreateInfos,
	const VkAllocationCallbacks* pAllocator,
	VkPipeline* pPipelines)
{
	return Volk::funcPtrs->vkCreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyPipeline(
	VkDevice                                    device,
	VkPipeline                                  pipeline,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyPipeline(device, pipeline, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineLayout(
	VkDevice                                    device,
	const VkPipelineLayoutCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkPipelineLayout* pPipelineLayout)
{
	return Volk::funcPtrs->vkCreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineLayout(
	VkDevice                                    device,
	VkPipelineLayout                            pipelineLayout,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyPipelineLayout(device, pipelineLayout, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSampler(
	VkDevice                                    device,
	const VkSamplerCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkSampler* pSampler)
{
	return Volk::funcPtrs->vkCreateSampler(device, pCreateInfo, pAllocator, pSampler);
}

VKAPI_ATTR void VKAPI_CALL vkDestroySampler(
	VkDevice                                    device,
	VkSampler                                   sampler,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroySampler(device, sampler, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorSetLayout(
	VkDevice                                    device,
	const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDescriptorSetLayout* pSetLayout)
{
	return Volk::funcPtrs->vkCreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorSetLayout(
	VkDevice                                    device,
	VkDescriptorSetLayout                       descriptorSetLayout,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorPool(
	VkDevice                                    device,
	const VkDescriptorPoolCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDescriptorPool* pDescriptorPool)
{
	return Volk::funcPtrs->vkCreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorPool(
	VkDevice                                    device,
	VkDescriptorPool                            descriptorPool,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyDescriptorPool(device, descriptorPool, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetDescriptorPool(
	VkDevice                                    device,
	VkDescriptorPool                            descriptorPool,
	VkDescriptorPoolResetFlags                  flags)
{
	return Volk::funcPtrs->vkResetDescriptorPool(device, descriptorPool, flags);
}

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateDescriptorSets(
	VkDevice                                    device,
	const VkDescriptorSetAllocateInfo* pAllocateInfo,
	VkDescriptorSet* pDescriptorSets)
{
	return Volk::funcPtrs->vkAllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
}

VKAPI_ATTR VkResult VKAPI_CALL vkFreeDescriptorSets(
	VkDevice                                    device,
	VkDescriptorPool                            descriptorPool,
	uint32_t                                    descriptorSetCount,
	const VkDescriptorSet* pDescriptorSets)
{
	return Volk::funcPtrs->vkFreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets(
	VkDevice                                    device,
	uint32_t                                    descriptorWriteCount,
	const VkWriteDescriptorSet* pDescriptorWrites,
	uint32_t                                    descriptorCopyCount,
	const VkCopyDescriptorSet* pDescriptorCopies)
{
	return Volk::funcPtrs->vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateFramebuffer(
	VkDevice                                    device,
	const VkFramebufferCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkFramebuffer* pFramebuffer)
{
	return Volk::funcPtrs->vkCreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyFramebuffer(
	VkDevice                                    device,
	VkFramebuffer                               framebuffer,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyFramebuffer(device, framebuffer, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateRenderPass(
	VkDevice                                    device,
	const VkRenderPassCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkRenderPass* pRenderPass)
{
	return Volk::funcPtrs->vkCreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyRenderPass(
	VkDevice                                    device,
	VkRenderPass                                renderPass,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyRenderPass(device, renderPass, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkGetRenderAreaGranularity(
	VkDevice                                    device,
	VkRenderPass                                renderPass,
	VkExtent2D* pGranularity)
{
	return Volk::funcPtrs->vkGetRenderAreaGranularity(device, renderPass, pGranularity);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool(
	VkDevice                                    device,
	const VkCommandPoolCreateInfo* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkCommandPool* pCommandPool)
{
	return Volk::funcPtrs->vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyCommandPool(
	VkDevice                                    device,
	VkCommandPool                               commandPool,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyCommandPool(device, commandPool, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandPool(
	VkDevice                                    device,
	VkCommandPool                               commandPool,
	VkCommandPoolResetFlags                     flags)
{
	return Volk::funcPtrs->vkResetCommandPool(device, commandPool, flags);
}

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(
	VkDevice                                    device,
	const VkCommandBufferAllocateInfo* pAllocateInfo,
	VkCommandBuffer* pCommandBuffers)
{
	return Volk::funcPtrs->vkAllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
}

VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(
	VkDevice                                    device,
	VkCommandPool                               commandPool,
	uint32_t                                    commandBufferCount,
	const VkCommandBuffer* pCommandBuffers)
{
	return Volk::funcPtrs->vkFreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(
	VkCommandBuffer                             commandBuffer,
	const VkCommandBufferBeginInfo* pBeginInfo)
{
	return Volk::funcPtrs->vkBeginCommandBuffer(commandBuffer, pBeginInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEndCommandBuffer(
	VkCommandBuffer                             commandBuffer)
{
	return Volk::funcPtrs->vkEndCommandBuffer(commandBuffer);
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer(
	VkCommandBuffer                             commandBuffer,
	VkCommandBufferResetFlags                   flags)
{
	return Volk::funcPtrs->vkResetCommandBuffer(commandBuffer, flags);
}

#endif /* defined(VK_VERSION_1_0) */

#if defined(VK_KHR_surface)
VKAPI_ATTR void VKAPI_CALL vkDestroySurfaceKHR(
	VkInstance                                  instance,
	VkSurfaceKHR                                surface,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroySurfaceKHR(instance, surface, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceSupportKHR(
	VkPhysicalDevice                            physicalDevice,
	uint32_t                                    queueFamilyIndex,
	VkSurfaceKHR                                surface,
	VkBool32* pSupported)
{
	return Volk::funcPtrs->vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, pSupported);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
	VkPhysicalDevice                            physicalDevice,
	VkSurfaceKHR                                surface,
	VkSurfaceCapabilitiesKHR* pSurfaceCapabilities)
{
	return Volk::funcPtrs->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabilities);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceFormatsKHR(
	VkPhysicalDevice                            physicalDevice,
	VkSurfaceKHR                                surface,
	uint32_t* pSurfaceFormatCount,
	VkSurfaceFormatKHR* pSurfaceFormats)
{
	return Volk::funcPtrs->vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModesKHR(
	VkPhysicalDevice                            physicalDevice,
	VkSurfaceKHR                                surface,
	uint32_t* pPresentModeCount,
	VkPresentModeKHR* pPresentModes)
{
	return Volk::funcPtrs->vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, pPresentModeCount, pPresentModes);
}
#endif /* defined(VK_KHR_surface) */

#if defined(VK_KHR_swapchain)
VKAPI_ATTR VkResult VKAPI_CALL vkCreateSwapchainKHR(
	VkDevice                                    device,
	const VkSwapchainCreateInfoKHR* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkSwapchainKHR* pSwapchain)
{
	return Volk::funcPtrs->vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
}

VKAPI_ATTR void VKAPI_CALL vkDestroySwapchainKHR(
	VkDevice                                    device,
	VkSwapchainKHR                              swapchain,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroySwapchainKHR(device, swapchain, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainImagesKHR(
	VkDevice                                    device,
	VkSwapchainKHR                              swapchain,
	uint32_t* pSwapchainImageCount,
	VkImage* pSwapchainImages)
{
	return Volk::funcPtrs->vkGetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
}

VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImageKHR(
	VkDevice                                    device,
	VkSwapchainKHR                              swapchain,
	uint64_t                                    timeout,
	VkSemaphore                                 semaphore,
	VkFence                                     fence,
	uint32_t* pImageIndex)
{
	return Volk::funcPtrs->vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(
	VkQueue                                     queue,
	const VkPresentInfoKHR* pPresentInfo)
{
	return Volk::funcPtrs->vkQueuePresentKHR(queue, pPresentInfo);
}
#endif /* defined(VK_KHR_swapchain) */

#if (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImage2KHR(
	VkDevice                                    device,
	const VkAcquireNextImageInfoKHR* pAcquireInfo,
	uint32_t* pImageIndex)
{
	return Volk::funcPtrs->vkAcquireNextImage2KHR(device, pAcquireInfo, pImageIndex);
}
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */

#if defined(VK_VERSION_1_1)
VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceVersion(
	uint32_t* pApiVersion)
{
	return Volk::funcPtrs->vkEnumerateInstanceVersion(pApiVersion);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory2(
	VkDevice                                    device,
	uint32_t                                    bindInfoCount,
	const VkBindBufferMemoryInfo* pBindInfos)
{
	return Volk::funcPtrs->vkBindBufferMemory2(device, bindInfoCount, pBindInfos);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory2(
	VkDevice                                    device,
	uint32_t                                    bindInfoCount,
	const VkBindImageMemoryInfo* pBindInfos)
{
	return Volk::funcPtrs->vkBindImageMemory2(device, bindInfoCount, pBindInfos);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceGroupPeerMemoryFeatures(
	VkDevice                                    device,
	uint32_t                                    heapIndex,
	uint32_t                                    localDeviceIndex,
	uint32_t                                    remoteDeviceIndex,
	VkPeerMemoryFeatureFlags* pPeerMemoryFeatures)
{
	return Volk::funcPtrs->vkGetDeviceGroupPeerMemoryFeatures(device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures);
}

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
	const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
	return Volk::funcPtrs->vkSetDebugUtilsObjectNameEXT(device, pNameInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectTagEXT(
	VkDevice                                    device,
	const VkDebugUtilsObjectTagInfoEXT* pTagInfo)
{
	return Volk::funcPtrs->vkSetDebugUtilsObjectTagEXT(device, pTagInfo);
}

VKAPI_ATTR void VKAPI_CALL vkQueueBeginDebugUtilsLabelEXT(
	VkQueue                                     queue,
	const VkDebugUtilsLabelEXT* pLabelInfo)
{
	return Volk::funcPtrs->vkQueueBeginDebugUtilsLabelEXT(queue, pLabelInfo);
}

VKAPI_ATTR void VKAPI_CALL vkQueueEndDebugUtilsLabelEXT(
	VkQueue                                     queue)
{
	return Volk::funcPtrs->vkQueueEndDebugUtilsLabelEXT(queue);
}

VKAPI_ATTR void VKAPI_CALL vkQueueInsertDebugUtilsLabelEXT(
	VkQueue                                     queue,
	const VkDebugUtilsLabelEXT* pLabelInfo)
{
	return Volk::funcPtrs->vkQueueInsertDebugUtilsLabelEXT(queue, pLabelInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginDebugUtilsLabelEXT(
	VkCommandBuffer                             commandBuffer,
	const VkDebugUtilsLabelEXT* pLabelInfo)
{
	return Volk::funcPtrs->vkCmdBeginDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndDebugUtilsLabelEXT(
	VkCommandBuffer                             commandBuffer)
{
	return Volk::funcPtrs->vkCmdEndDebugUtilsLabelEXT(commandBuffer);
}

VKAPI_ATTR void VKAPI_CALL vkCmdInsertDebugUtilsLabelEXT(
	VkCommandBuffer                             commandBuffer,
	const VkDebugUtilsLabelEXT* pLabelInfo)
{
	return Volk::funcPtrs->vkCmdInsertDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
	VkInstance                                  instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pMessenger)
{
	return Volk::funcPtrs->vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
	VkInstance                                  instance,
	VkDebugUtilsMessengerEXT                    messenger,
	const VkAllocationCallbacks* pAllocator)
{
	return Volk::funcPtrs->vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkSubmitDebugUtilsMessageEXT(
	VkInstance                                  instance,
	VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData)
{
	return Volk::funcPtrs->vkSubmitDebugUtilsMessageEXT(instance, messageSeverity, messageTypes, pCallbackData);
}
#endif /* defined(VK_EXT_debug_utils) */



#ifdef __GNUC__
#	pragma GCC visibility push(hidden)
#endif

/* VOLK_GENERATE_PROTOTYPES_C */
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
#if defined(VK_EXT_acquire_xlib_display)
PFN_vkAcquireXlibDisplayEXT vkAcquireXlibDisplayEXT;
PFN_vkGetRandROutputDisplayEXT vkGetRandROutputDisplayEXT;
#endif /* defined(VK_EXT_acquire_xlib_display) */
#if defined(VK_EXT_buffer_device_address)
PFN_vkGetBufferDeviceAddressEXT vkGetBufferDeviceAddressEXT;
#endif /* defined(VK_EXT_buffer_device_address) */
#if defined(VK_EXT_calibrated_timestamps)
PFN_vkGetCalibratedTimestampsEXT vkGetCalibratedTimestampsEXT;
PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT vkGetPhysicalDeviceCalibrateableTimeDomainsEXT;
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
#if defined(VK_EXT_debug_report)
PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT;
PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
#endif /* defined(VK_EXT_debug_report) */
#if defined(VK_EXT_direct_mode_display)
PFN_vkReleaseDisplayEXT vkReleaseDisplayEXT;
#endif /* defined(VK_EXT_direct_mode_display) */
#if defined(VK_EXT_discard_rectangles)
PFN_vkCmdSetDiscardRectangleEXT vkCmdSetDiscardRectangleEXT;
#endif /* defined(VK_EXT_discard_rectangles) */
#if defined(VK_EXT_display_control)
PFN_vkDisplayPowerControlEXT vkDisplayPowerControlEXT;
PFN_vkGetSwapchainCounterEXT vkGetSwapchainCounterEXT;
PFN_vkRegisterDeviceEventEXT vkRegisterDeviceEventEXT;
PFN_vkRegisterDisplayEventEXT vkRegisterDisplayEventEXT;
#endif /* defined(VK_EXT_display_control) */
#if defined(VK_EXT_display_surface_counter)
PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT vkGetPhysicalDeviceSurfaceCapabilities2EXT;
#endif /* defined(VK_EXT_display_surface_counter) */
#if defined(VK_EXT_external_memory_host)
PFN_vkGetMemoryHostPointerPropertiesEXT vkGetMemoryHostPointerPropertiesEXT;
#endif /* defined(VK_EXT_external_memory_host) */
#if defined(VK_EXT_full_screen_exclusive)
PFN_vkAcquireFullScreenExclusiveModeEXT vkAcquireFullScreenExclusiveModeEXT;
PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT vkGetPhysicalDeviceSurfacePresentModes2EXT;
PFN_vkReleaseFullScreenExclusiveModeEXT vkReleaseFullScreenExclusiveModeEXT;
#endif /* defined(VK_EXT_full_screen_exclusive) */
#if defined(VK_EXT_hdr_metadata)
PFN_vkSetHdrMetadataEXT vkSetHdrMetadataEXT;
#endif /* defined(VK_EXT_hdr_metadata) */
#if defined(VK_EXT_headless_surface)
PFN_vkCreateHeadlessSurfaceEXT vkCreateHeadlessSurfaceEXT;
#endif /* defined(VK_EXT_headless_surface) */
#if defined(VK_EXT_host_query_reset)
PFN_vkResetQueryPoolEXT vkResetQueryPoolEXT;
#endif /* defined(VK_EXT_host_query_reset) */
#if defined(VK_EXT_image_drm_format_modifier)
PFN_vkGetImageDrmFormatModifierPropertiesEXT vkGetImageDrmFormatModifierPropertiesEXT;
#endif /* defined(VK_EXT_image_drm_format_modifier) */
#if defined(VK_EXT_metal_surface)
PFN_vkCreateMetalSurfaceEXT vkCreateMetalSurfaceEXT;
#endif /* defined(VK_EXT_metal_surface) */
#if defined(VK_EXT_sample_locations)
PFN_vkCmdSetSampleLocationsEXT vkCmdSetSampleLocationsEXT;
PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT vkGetPhysicalDeviceMultisamplePropertiesEXT;
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
#if defined(VK_FUCHSIA_imagepipe_surface)
PFN_vkCreateImagePipeSurfaceFUCHSIA vkCreateImagePipeSurfaceFUCHSIA;
#endif /* defined(VK_FUCHSIA_imagepipe_surface) */
#if defined(VK_GGP_stream_descriptor_surface)
PFN_vkCreateStreamDescriptorSurfaceGGP vkCreateStreamDescriptorSurfaceGGP;
#endif /* defined(VK_GGP_stream_descriptor_surface) */
#if defined(VK_GOOGLE_display_timing)
PFN_vkGetPastPresentationTimingGOOGLE vkGetPastPresentationTimingGOOGLE;
PFN_vkGetRefreshCycleDurationGOOGLE vkGetRefreshCycleDurationGOOGLE;
#endif /* defined(VK_GOOGLE_display_timing) */
#if defined(VK_KHR_android_surface)
PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR;
#endif /* defined(VK_KHR_android_surface) */
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
#if defined(VK_KHR_device_group_creation)
PFN_vkEnumeratePhysicalDeviceGroupsKHR vkEnumeratePhysicalDeviceGroupsKHR;
#endif /* defined(VK_KHR_device_group_creation) */
#if defined(VK_KHR_display)
PFN_vkCreateDisplayModeKHR vkCreateDisplayModeKHR;
PFN_vkCreateDisplayPlaneSurfaceKHR vkCreateDisplayPlaneSurfaceKHR;
PFN_vkGetDisplayModePropertiesKHR vkGetDisplayModePropertiesKHR;
PFN_vkGetDisplayPlaneCapabilitiesKHR vkGetDisplayPlaneCapabilitiesKHR;
PFN_vkGetDisplayPlaneSupportedDisplaysKHR vkGetDisplayPlaneSupportedDisplaysKHR;
PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR vkGetPhysicalDeviceDisplayPlanePropertiesKHR;
PFN_vkGetPhysicalDeviceDisplayPropertiesKHR vkGetPhysicalDeviceDisplayPropertiesKHR;
#endif /* defined(VK_KHR_display) */
#if defined(VK_KHR_display_swapchain)
PFN_vkCreateSharedSwapchainsKHR vkCreateSharedSwapchainsKHR;
#endif /* defined(VK_KHR_display_swapchain) */
#if defined(VK_KHR_draw_indirect_count)
PFN_vkCmdDrawIndexedIndirectCountKHR vkCmdDrawIndexedIndirectCountKHR;
PFN_vkCmdDrawIndirectCountKHR vkCmdDrawIndirectCountKHR;
#endif /* defined(VK_KHR_draw_indirect_count) */
#if defined(VK_KHR_external_fence_capabilities)
PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR vkGetPhysicalDeviceExternalFencePropertiesKHR;
#endif /* defined(VK_KHR_external_fence_capabilities) */
#if defined(VK_KHR_external_fence_fd)
PFN_vkGetFenceFdKHR vkGetFenceFdKHR;
PFN_vkImportFenceFdKHR vkImportFenceFdKHR;
#endif /* defined(VK_KHR_external_fence_fd) */
#if defined(VK_KHR_external_fence_win32)
PFN_vkGetFenceWin32HandleKHR vkGetFenceWin32HandleKHR;
PFN_vkImportFenceWin32HandleKHR vkImportFenceWin32HandleKHR;
#endif /* defined(VK_KHR_external_fence_win32) */
#if defined(VK_KHR_external_memory_capabilities)
PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR vkGetPhysicalDeviceExternalBufferPropertiesKHR;
#endif /* defined(VK_KHR_external_memory_capabilities) */
#if defined(VK_KHR_external_memory_fd)
PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR;
PFN_vkGetMemoryFdPropertiesKHR vkGetMemoryFdPropertiesKHR;
#endif /* defined(VK_KHR_external_memory_fd) */
#if defined(VK_KHR_external_memory_win32)
PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR;
PFN_vkGetMemoryWin32HandlePropertiesKHR vkGetMemoryWin32HandlePropertiesKHR;
#endif /* defined(VK_KHR_external_memory_win32) */
#if defined(VK_KHR_external_semaphore_capabilities)
PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR vkGetPhysicalDeviceExternalSemaphorePropertiesKHR;
#endif /* defined(VK_KHR_external_semaphore_capabilities) */
#if defined(VK_KHR_external_semaphore_fd)
PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR;
PFN_vkImportSemaphoreFdKHR vkImportSemaphoreFdKHR;
#endif /* defined(VK_KHR_external_semaphore_fd) */
#if defined(VK_KHR_external_semaphore_win32)
PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR;
PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR;
#endif /* defined(VK_KHR_external_semaphore_win32) */
#if defined(VK_KHR_get_display_properties2)
PFN_vkGetDisplayModeProperties2KHR vkGetDisplayModeProperties2KHR;
PFN_vkGetDisplayPlaneCapabilities2KHR vkGetDisplayPlaneCapabilities2KHR;
PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR vkGetPhysicalDeviceDisplayPlaneProperties2KHR;
PFN_vkGetPhysicalDeviceDisplayProperties2KHR vkGetPhysicalDeviceDisplayProperties2KHR;
#endif /* defined(VK_KHR_get_display_properties2) */
#if defined(VK_KHR_get_memory_requirements2)
PFN_vkGetBufferMemoryRequirements2KHR vkGetBufferMemoryRequirements2KHR;
PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2KHR;
PFN_vkGetImageSparseMemoryRequirements2KHR vkGetImageSparseMemoryRequirements2KHR;
#endif /* defined(VK_KHR_get_memory_requirements2) */
#if defined(VK_KHR_get_physical_device_properties2)
PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2KHR;
PFN_vkGetPhysicalDeviceFormatProperties2KHR vkGetPhysicalDeviceFormatProperties2KHR;
PFN_vkGetPhysicalDeviceImageFormatProperties2KHR vkGetPhysicalDeviceImageFormatProperties2KHR;
PFN_vkGetPhysicalDeviceMemoryProperties2KHR vkGetPhysicalDeviceMemoryProperties2KHR;
PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR;
PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR vkGetPhysicalDeviceQueueFamilyProperties2KHR;
PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR vkGetPhysicalDeviceSparseImageFormatProperties2KHR;
#endif /* defined(VK_KHR_get_physical_device_properties2) */
#if defined(VK_KHR_get_surface_capabilities2)
PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR vkGetPhysicalDeviceSurfaceCapabilities2KHR;
PFN_vkGetPhysicalDeviceSurfaceFormats2KHR vkGetPhysicalDeviceSurfaceFormats2KHR;
#endif /* defined(VK_KHR_get_surface_capabilities2) */
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
#if defined(VK_KHR_wayland_surface)
PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR;
PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR vkGetPhysicalDeviceWaylandPresentationSupportKHR;
#endif /* defined(VK_KHR_wayland_surface) */
#if defined(VK_KHR_win32_surface)
PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR;
#endif /* defined(VK_KHR_win32_surface) */
#if defined(VK_KHR_xcb_surface)
PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR vkGetPhysicalDeviceXcbPresentationSupportKHR;
#endif /* defined(VK_KHR_xcb_surface) */
#if defined(VK_KHR_xlib_surface)
PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR vkGetPhysicalDeviceXlibPresentationSupportKHR;
#endif /* defined(VK_KHR_xlib_surface) */
#if defined(VK_MVK_ios_surface)
PFN_vkCreateIOSSurfaceMVK vkCreateIOSSurfaceMVK;
#endif /* defined(VK_MVK_ios_surface) */
#if defined(VK_MVK_macos_surface)
PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK;
#endif /* defined(VK_MVK_macos_surface) */
#if defined(VK_NN_vi_surface)
PFN_vkCreateViSurfaceNN vkCreateViSurfaceNN;
#endif /* defined(VK_NN_vi_surface) */
#if defined(VK_NVX_device_generated_commands)
PFN_vkCmdProcessCommandsNVX vkCmdProcessCommandsNVX;
PFN_vkCmdReserveSpaceForCommandsNVX vkCmdReserveSpaceForCommandsNVX;
PFN_vkCreateIndirectCommandsLayoutNVX vkCreateIndirectCommandsLayoutNVX;
PFN_vkCreateObjectTableNVX vkCreateObjectTableNVX;
PFN_vkDestroyIndirectCommandsLayoutNVX vkDestroyIndirectCommandsLayoutNVX;
PFN_vkDestroyObjectTableNVX vkDestroyObjectTableNVX;
PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX;
PFN_vkRegisterObjectsNVX vkRegisterObjectsNVX;
PFN_vkUnregisterObjectsNVX vkUnregisterObjectsNVX;
#endif /* defined(VK_NVX_device_generated_commands) */
#if defined(VK_NVX_image_view_handle)
PFN_vkGetImageViewHandleNVX vkGetImageViewHandleNVX;
#endif /* defined(VK_NVX_image_view_handle) */
#if defined(VK_NV_clip_space_w_scaling)
PFN_vkCmdSetViewportWScalingNV vkCmdSetViewportWScalingNV;
#endif /* defined(VK_NV_clip_space_w_scaling) */
#if defined(VK_NV_cooperative_matrix)
PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV vkGetPhysicalDeviceCooperativeMatrixPropertiesNV;
#endif /* defined(VK_NV_cooperative_matrix) */
#if defined(VK_NV_device_diagnostic_checkpoints)
PFN_vkCmdSetCheckpointNV vkCmdSetCheckpointNV;
PFN_vkGetQueueCheckpointDataNV vkGetQueueCheckpointDataNV;
#endif /* defined(VK_NV_device_diagnostic_checkpoints) */
#if defined(VK_NV_external_memory_capabilities)
PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV vkGetPhysicalDeviceExternalImageFormatPropertiesNV;
#endif /* defined(VK_NV_external_memory_capabilities) */
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
PFN_vkGetPhysicalDevicePresentRectanglesKHR vkGetPhysicalDevicePresentRectanglesKHR;
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
/* VOLK_GENERATE_PROTOTYPES_C */

#ifdef __GNUC__
#	pragma GCC visibility pop
#endif