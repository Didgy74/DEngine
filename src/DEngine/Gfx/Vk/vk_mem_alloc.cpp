#define VMA_IMPLEMENTATION

#include "VulkanIncluder.hpp"
#include "DynamicDispatch.hpp"
#include "Vk.hpp"
#include "Init.hpp"

#include "VMAIncluder.hpp"

namespace DEngine::Gfx::Vk::Init
{
	static VKAPI_ATTR void VKAPI_CALL AllocCallbackForVMA(
		VmaAllocator allocator,
		uint32_t memoryType,
		VkDeviceMemory memory,
		VkDeviceSize size,
		void* pUserData)
	{
		VMA_MemoryTrackingData& trackingData = *reinterpret_cast<VMA_MemoryTrackingData*>(pUserData);

		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64) memory;
		nameInfo.objectType = vk::ObjectType::eDeviceMemory;
		std::lock_guard _(trackingData.vma_idTracker_lock);
		std::string name = std::string("VMA DeviceMemory #") + std::to_string(trackingData.vma_idTracker);
		trackingData.vma_idTracker += 1;
		nameInfo.pObjectName = name.data();

		trackingData.debugUtils->setDebugUtilsObjectNameEXT(trackingData.deviceHandle, nameInfo);
	}
}

vk::ResultValue<VmaAllocator> DEngine::Gfx::Vk::Init::InitializeVMA(
	InstanceDispatch const& instance,
	vk::PhysicalDevice physDeviceHandle,
	DeviceDispatch const& device,
	VMA_MemoryTrackingData* vma_trackingData) 
{
	vk::Result vkResult{};

	VmaDeviceMemoryCallbacks callbacks{};
	callbacks.pfnAllocate = &AllocCallbackForVMA;
	callbacks.pUserData = vma_trackingData;

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
	vmaInfo.device = (VkDevice) device.handle;
	vmaInfo.flags = 0;
	vmaInfo.frameInUseCount = 0;
	vmaInfo.instance = (VkInstance) instance.handle;
	vmaInfo.pAllocationCallbacks = nullptr;
	vmaInfo.pDeviceMemoryCallbacks = vma_trackingData && vma_trackingData->debugUtils ? &callbacks : nullptr;
	vmaInfo.pHeapSizeLimit = nullptr;
	vmaInfo.physicalDevice = (VkPhysicalDevice)physDeviceHandle;
	vmaInfo.pRecordSettings = nullptr;
	vmaInfo.preferredLargeHeapBlockSize = 0; // This is default
	vmaInfo.pVulkanFunctions = &vmaDispatch;
	vmaInfo.vulkanApiVersion = VK_MAKE_VERSION(1, 0, 0);

	VmaAllocator returnVal{};

	vkResult = static_cast<vk::Result>(vmaCreateAllocator(&vmaInfo, &returnVal));

	return { vkResult, returnVal };
}