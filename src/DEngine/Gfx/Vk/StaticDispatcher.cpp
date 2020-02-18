#include "StaticDispatcher.hpp"

PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets = nullptr;
PFN_vkAllocateMemory vkAllocateMemory = nullptr;
PFN_vkBindBufferMemory vkBindBufferMemory = nullptr;
PFN_vkBindImageMemory vkBindImageMemory = nullptr;
PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets = nullptr;
PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer = nullptr;
PFN_vkCmdBindPipeline vkCmdBindPipeline = nullptr;
PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers = nullptr;
PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage = nullptr;
PFN_vkCmdDrawIndexed vkCmdDrawIndexed = nullptr;
PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = nullptr;
PFN_vkCmdPushConstants vkCmdPushConstants = nullptr;
PFN_vkCmdSetScissor vkCmdSetScissor = nullptr;
PFN_vkCmdSetViewport vkCmdSetViewport = nullptr;
PFN_vkCreateBuffer vkCreateBuffer = nullptr;
PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout = nullptr;
PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines = nullptr;
PFN_vkCreateImage vkCreateImage = nullptr;
PFN_vkCreateImageView vkCreateImageView = nullptr;
PFN_vkCreatePipelineLayout vkCreatePipelineLayout = nullptr;
PFN_vkCreateSampler vkCreateSampler = nullptr;
PFN_vkCreateShaderModule vkCreateShaderModule = nullptr;
PFN_vkDestroyBuffer vkDestroyBuffer = nullptr;
PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout = nullptr;
PFN_vkDestroyImage vkDestroyImage = nullptr;
PFN_vkDestroyImageView vkDestroyImageView = nullptr;
PFN_vkDestroyPipeline vkDestroyPipeline = nullptr;
PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout = nullptr;
PFN_vkDestroySampler vkDestroySampler = nullptr;
PFN_vkDestroyShaderModule vkDestroyShaderModule = nullptr;
PFN_vkDeviceWaitIdle vkDeviceWaitIdle = nullptr;
PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges = nullptr;
PFN_vkFreeMemory vkFreeMemory = nullptr;
PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements = nullptr;
PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements = nullptr;
PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties = nullptr;
PFN_vkMapMemory vkMapMemory = nullptr;
PFN_vkUnmapMemory vkUnmapMemory = nullptr;
PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets = nullptr;

void DEngine::Gfx::Vk::initStaticDispatcher(VkInstance instance, PFN_vkGetInstanceProcAddr getProcAddr)
{
	vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)getProcAddr(instance, "vkAllocateDescriptorSets");
	vkAllocateMemory = (PFN_vkAllocateMemory)getProcAddr(instance, "vkAllocateMemory");
	vkBindBufferMemory = (PFN_vkBindBufferMemory)getProcAddr(instance, "vkBindBufferMemory");
	vkBindImageMemory = (PFN_vkBindImageMemory)getProcAddr(instance, "vkBindImageMemory");
	vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)getProcAddr(instance, "vkCmdBindDescriptorSets");
	vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)getProcAddr(instance, "vkCmdBindIndexBuffer");
	vkCmdBindPipeline = (PFN_vkCmdBindPipeline)getProcAddr(instance, "vkCmdBindPipeline");
	vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)getProcAddr(instance, "vkCmdBindVertexBuffers");
	vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)getProcAddr(instance, "vkCmdCopyBufferToImage");
	vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)getProcAddr(instance, "vkCmdDrawIndexed");
	vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)getProcAddr(instance, "vkCmdPipelineBarrier");
	vkCmdPushConstants = (PFN_vkCmdPushConstants)getProcAddr(instance, "vkCmdPushConstants");
	vkCmdSetScissor = (PFN_vkCmdSetScissor)getProcAddr(instance, "vkCmdSetScissor");
	vkCmdSetViewport = (PFN_vkCmdSetViewport)getProcAddr(instance, "vkCmdSetViewport");
	vkCreateBuffer = (PFN_vkCreateBuffer)getProcAddr(instance, "vkCreateBuffer");
	vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)getProcAddr(instance, "vkCreateDescriptorSetLayout");
	vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)getProcAddr(instance, "vkCreateGraphicsPipelines");
	vkCreateImage = (PFN_vkCreateImage)getProcAddr(instance, "vkCreateImage");
	vkCreateImageView = (PFN_vkCreateImageView)getProcAddr(instance, "vkCreateImageView");
	vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)getProcAddr(instance, "vkCreatePipelineLayout");
	vkCreateSampler = (PFN_vkCreateSampler)getProcAddr(instance, "vkCreateSampler");
	vkCreateShaderModule = (PFN_vkCreateShaderModule)getProcAddr(instance, "vkCreateShaderModule");
	vkDestroyBuffer = (PFN_vkDestroyBuffer)getProcAddr(instance, "vkDestroyBuffer");
	vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)getProcAddr(instance, "vkDestroyDescriptorSetLayout");
	vkDestroyImage = (PFN_vkDestroyImage)getProcAddr(instance, "vkDestroyImage");
	vkDestroyImageView = (PFN_vkDestroyImageView)getProcAddr(instance, "vkDestroyImageView");
	vkDestroyPipeline = (PFN_vkDestroyPipeline)getProcAddr(instance, "vkDestroyPipeline");
	vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)getProcAddr(instance, "vkDestroyPipelineLayout");
	vkDestroySampler = (PFN_vkDestroySampler)getProcAddr(instance, "vkDestroySampler");
	vkDestroyShaderModule = (PFN_vkDestroyShaderModule)getProcAddr(instance, "vkDestroyShaderModule");
	vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)getProcAddr(instance, "vkDeviceWaitIdle");
	vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)getProcAddr(instance, "vkFlushMappedMemoryRanges");
	vkFreeMemory = (PFN_vkFreeMemory)getProcAddr(instance, "vkFreeMemory");
	vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)getProcAddr(instance, "vkGetBufferMemoryRequirements");
	vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)getProcAddr(instance, "vkGetImageMemoryRequirements");
	vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)getProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties");
	vkMapMemory = (PFN_vkMapMemory)getProcAddr(instance, "vkMapMemory");
	vkUnmapMemory = (PFN_vkUnmapMemory)getProcAddr(instance, "vkUnmapMemory");
	vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)getProcAddr(instance, "vkUpdateDescriptorSets");
}