
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "DynamicDispatch.hpp"
#include "Vk.hpp"

namespace DEngine::Gfx::Vk
{
	static DevDispatch const* g_vmaTracker_device = nullptr;
	static DebugUtilsDispatch const* g_vmaTracker_debugUtils = nullptr;
	static std::mutex g_vmaTracker_idLock{};
	static u64 g_vmaTracker_id = 0;

}

namespace DEngine::Gfx::Vk::Init
{
	VmaAllocator InitializeVMA(
		InstanceDispatch const& instance,
		DevDispatch const& device,
		PhysDeviceInfo const& physDevice,
		DebugUtilsDispatch const* debugUtils)
	{
		g_vmaTracker_device = &device;
		g_vmaTracker_debugUtils = debugUtils;

		vk::Result vkResult{};

		VmaDeviceMemoryCallbacks callbacks{};
		callbacks.pfnAllocate = [](
			VmaAllocator      allocator,
			uint32_t          memoryType,
			VkDeviceMemory    memory,
			VkDeviceSize      size)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)memory;
			nameInfo.objectType = vk::ObjectType::eDeviceMemory;
			std::lock_guard lock(g_vmaTracker_idLock);
			std::string name = std::string("VMA DeviceMemory #") + std::to_string(g_vmaTracker_id);
			g_vmaTracker_id += 1;
			nameInfo.pObjectName = name.data();

			g_vmaTracker_debugUtils->setDebugUtilsObjectNameEXT(g_vmaTracker_device->handle, nameInfo);
		};

		VmaVulkanFunctions vmaDispatch{};
		vmaDispatch.vkAllocateMemory = device.raw.vkAllocateMemory;
		vmaDispatch.vkBindBufferMemory = device.raw.vkBindBufferMemory;
		vmaDispatch.vkBindBufferMemory2KHR = nullptr;
		vmaDispatch.vkBindImageMemory = device.raw.vkBindImageMemory;
		vmaDispatch.vkBindImageMemory2KHR = nullptr;
		vmaDispatch.vkCmdCopyBuffer = device.raw.vkCmdCopyBuffer;
		vmaDispatch.vkCreateBuffer = device.raw.vkCreateBuffer;
		vmaDispatch.vkCreateImage = device.raw.vkCreateImage;
		vmaDispatch.vkDestroyBuffer = device.raw.vkDestroyBuffer;
		vmaDispatch.vkDestroyImage = device.raw.vkDestroyImage;
		vmaDispatch.vkFlushMappedMemoryRanges = device.raw.vkFlushMappedMemoryRanges;
		vmaDispatch.vkFreeMemory = device.raw.vkFreeMemory;
		vmaDispatch.vkGetBufferMemoryRequirements = device.raw.vkGetBufferMemoryRequirements;
		vmaDispatch.vkGetBufferMemoryRequirements2KHR = nullptr;
		vmaDispatch.vkGetImageMemoryRequirements = device.raw.vkGetImageMemoryRequirements;
		vmaDispatch.vkGetImageMemoryRequirements2KHR = nullptr;
		vmaDispatch.vkGetPhysicalDeviceMemoryProperties = instance.raw.vkGetPhysicalDeviceMemoryProperties;
		vmaDispatch.vkGetPhysicalDeviceMemoryProperties2KHR = nullptr;
		vmaDispatch.vkGetPhysicalDeviceProperties = instance.raw.vkGetPhysicalDeviceProperties;
		vmaDispatch.vkInvalidateMappedMemoryRanges = device.raw.vkInvalidateMappedMemoryRanges;
		vmaDispatch.vkMapMemory = device.raw.vkMapMemory;
		vmaDispatch.vkUnmapMemory = device.raw.vkUnmapMemory;


		VmaAllocatorCreateInfo vmaInfo{};
		vmaInfo.device = (VkDevice)device.handle;
		vmaInfo.flags = 0;
		vmaInfo.frameInUseCount = 0;
		vmaInfo.instance = (VkInstance)instance.handle;
		vmaInfo.pAllocationCallbacks = nullptr;
		vmaInfo.pDeviceMemoryCallbacks = debugUtils ? &callbacks : nullptr;
		vmaInfo.pHeapSizeLimit = nullptr;
		vmaInfo.physicalDevice = (VkPhysicalDevice)physDevice.handle;
		vmaInfo.pRecordSettings = nullptr;
		vmaInfo.preferredLargeHeapBlockSize = 0; // This is default
		vmaInfo.pVulkanFunctions = &vmaDispatch;
		vmaInfo.vulkanApiVersion = VK_MAKE_VERSION(1, 0, 0);

		VmaAllocator returnVal{};

		vkResult = (vk::Result)vmaCreateAllocator(&vmaInfo, &returnVal);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Could not initialize VMA.");

		return returnVal;
	}
}
