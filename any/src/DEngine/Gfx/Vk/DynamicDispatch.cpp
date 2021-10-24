#include "DynamicDispatch.hpp"

#if defined(_WIN32)
#include <Windows.h>
#undef MemoryBarrier
#endif

using namespace DEngine;
using namespace DEngine::Gfx;
using namespace DEngine::Gfx::Vk;

PFN_vkGetInstanceProcAddr Vk::loadInstanceProcAddressPFN()
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

BaseDispatchRaw BaseDispatchRaw::Build(PFN_vkGetInstanceProcAddr getInstanceProcAddr)
{
	BaseDispatchRaw returnVal = {};
	returnVal.vkCreateInstance = (PFN_vkCreateInstance)getInstanceProcAddr(nullptr, "vkCreateInstance");
	returnVal.vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)getInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties");
	returnVal.vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)getInstanceProcAddr(nullptr, "vkEnumerateInstanceLayerProperties");
	returnVal.vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)getInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion");
	return returnVal;
}

InstanceDispatchRaw InstanceDispatchRaw::Build(
	vk::Instance inInstance, 
	PFN_vkGetInstanceProcAddr getInstanceProcAddr)
{
	VkInstance instance = static_cast<VkInstance>(inInstance);
	InstanceDispatchRaw returnVal{};
	returnVal.vkCreateDevice = (PFN_vkCreateDevice)getInstanceProcAddr(instance, "vkCreateDevice");
	returnVal.vkDestroyInstance = (PFN_vkDestroyInstance)getInstanceProcAddr(instance, "vkDestroyInstance");
	returnVal.vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)getInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
	returnVal.vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)getInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties");
	returnVal.vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)getInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures");
	returnVal.vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)getInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties");
	returnVal.vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)getInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties");
	returnVal.vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)getInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties");
	return returnVal;
}

DeviceDispatchRaw DeviceDispatchRaw::Build(
	vk::Device inDevice, 
	PFN_vkGetDeviceProcAddr getDeviceProcAddr)
{
	DeviceDispatchRaw returnVal{};
	VkDevice device = static_cast<VkDevice>(inDevice);
	returnVal.vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)getDeviceProcAddr(device, "vkAllocateCommandBuffers");
	returnVal.vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)getDeviceProcAddr(device, "vkAllocateDescriptorSets");
	returnVal.vkAllocateMemory = (PFN_vkAllocateMemory)getDeviceProcAddr(device, "vkAllocateMemory");
	returnVal.vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)getDeviceProcAddr(device, "vkBeginCommandBuffer");
	returnVal.vkBindBufferMemory = (PFN_vkBindBufferMemory)getDeviceProcAddr(device, "vkBindBufferMemory");
	returnVal.vkBindImageMemory = (PFN_vkBindImageMemory)getDeviceProcAddr(device, "vkBindImageMemory");
	returnVal.vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)getDeviceProcAddr(device, "vkCmdBeginRenderPass");
	returnVal.vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)getDeviceProcAddr(device, "vkCmdBindDescriptorSets");
	returnVal.vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)getDeviceProcAddr(device, "vkCmdBindIndexBuffer");
	returnVal.vkCmdBindPipeline = (PFN_vkCmdBindPipeline)getDeviceProcAddr(device, "vkCmdBindPipeline");
	returnVal.vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)getDeviceProcAddr(device, "vkCmdBindVertexBuffers");
	returnVal.vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)getDeviceProcAddr(device, "vkCmdCopyBuffer");
	returnVal.vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)getDeviceProcAddr(device, "vkCmdCopyBufferToImage");
	returnVal.vkCmdCopyImage = (PFN_vkCmdCopyImage)getDeviceProcAddr(device, "vkCmdCopyImage");
	returnVal.vkCmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer)getDeviceProcAddr(device, "vkCmdCopyImageToBuffer");
	returnVal.vkCmdDraw = (PFN_vkCmdDraw)getDeviceProcAddr(device, "vkCmdDraw");
	returnVal.vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)getDeviceProcAddr(device, "vkCmdDrawIndexed");
	returnVal.vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)getDeviceProcAddr(device, "vkCmdEndRenderPass");
	returnVal.vkCmdNextSubpass = (PFN_vkCmdNextSubpass)getDeviceProcAddr(device, "vkCmdNextSubpass");
	returnVal.vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)getDeviceProcAddr(device, "vkCmdPipelineBarrier");
	returnVal.vkCmdPushConstants = (PFN_vkCmdPushConstants)getDeviceProcAddr(device, "vkCmdPushConstants");
	returnVal.vkCmdSetScissor = (PFN_vkCmdSetScissor)getDeviceProcAddr(device, "vkCmdSetScissor");
	returnVal.vkCmdSetViewport = (PFN_vkCmdSetViewport)getDeviceProcAddr(device, "vkCmdSetViewport");
	returnVal.vkCreateBuffer = (PFN_vkCreateBuffer)getDeviceProcAddr(device, "vkCreateBuffer");
	returnVal.vkCreateBufferView = (PFN_vkCreateBufferView)getDeviceProcAddr(device, "vkCreateBufferView");
	returnVal.vkCreateCommandPool = (PFN_vkCreateCommandPool)getDeviceProcAddr(device, "vkCreateCommandPool");
	returnVal.vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)getDeviceProcAddr(device, "vkCreateDescriptorPool");
	returnVal.vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)getDeviceProcAddr(device, "vkCreateDescriptorSetLayout");
	returnVal.vkCreateFence = (PFN_vkCreateFence)getDeviceProcAddr(device, "vkCreateFence");
	returnVal.vkCreateFramebuffer = (PFN_vkCreateFramebuffer)getDeviceProcAddr(device, "vkCreateFramebuffer");
	returnVal.vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)getDeviceProcAddr(device, "vkCreateGraphicsPipelines");
	returnVal.vkCreateImage = (PFN_vkCreateImage)getDeviceProcAddr(device, "vkCreateImage");
	returnVal.vkCreateImageView = (PFN_vkCreateImageView)getDeviceProcAddr(device, "vkCreateImageView");
	returnVal.vkCreatePipelineCache = (PFN_vkCreatePipelineCache)getDeviceProcAddr(device, "vkCreatePipelineCache");
	returnVal.vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)getDeviceProcAddr(device, "vkCreatePipelineLayout");
	returnVal.vkCreateRenderPass = (PFN_vkCreateRenderPass)getDeviceProcAddr(device, "vkCreateRenderPass");
	returnVal.vkCreateSampler = (PFN_vkCreateSampler)getDeviceProcAddr(device, "vkCreateSampler");
	returnVal.vkCreateSemaphore = (PFN_vkCreateSemaphore)getDeviceProcAddr(device, "vkCreateSemaphore");
	returnVal.vkCreateShaderModule = (PFN_vkCreateShaderModule)getDeviceProcAddr(device, "vkCreateShaderModule");
	returnVal.vkDestroyBuffer = (PFN_vkDestroyBuffer)getDeviceProcAddr(device, "vkDestroyBuffer");
	returnVal.vkDestroyBufferView = (PFN_vkDestroyBufferView)getDeviceProcAddr(device, "vkDestroyBufferView");
	returnVal.vkDestroyCommandPool = (PFN_vkDestroyCommandPool)getDeviceProcAddr(device, "vkDestroyCommandPool");
	returnVal.vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)getDeviceProcAddr(device, "vkDestroyDescriptorPool");
	returnVal.vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)getDeviceProcAddr(device, "vkDestroyDescriptorSetLayout");
	returnVal.vkDestroyDevice = (PFN_vkDestroyDevice)getDeviceProcAddr(device, "vkDestroyDevice");
	returnVal.vkDestroyFence = (PFN_vkDestroyFence)getDeviceProcAddr(device, "vkDestroyFence");
	returnVal.vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)getDeviceProcAddr(device, "vkDestroyFramebuffer");
	returnVal.vkDestroyImage = (PFN_vkDestroyImage)getDeviceProcAddr(device, "vkDestroyImage");
	returnVal.vkDestroyImageView = (PFN_vkDestroyImageView)getDeviceProcAddr(device, "vkDestroyImageView");
	returnVal.vkDestroyPipeline = (PFN_vkDestroyPipeline)getDeviceProcAddr(device, "vkDestroyPipeline");
	returnVal.vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache)getDeviceProcAddr(device, "vkDestroyPipelineCache");
	returnVal.vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)getDeviceProcAddr(device, "vkDestroyPipelineLayout");
	returnVal.vkDestroyRenderPass = (PFN_vkDestroyRenderPass)getDeviceProcAddr(device, "vkDestroyRenderPass");
	returnVal.vkDestroySampler = (PFN_vkDestroySampler)getDeviceProcAddr(device, "vkDestroySampler");
	returnVal.vkDestroySemaphore = (PFN_vkDestroySemaphore)getDeviceProcAddr(device, "vkDestroySemaphore");
	returnVal.vkDestroyShaderModule = (PFN_vkDestroyShaderModule)getDeviceProcAddr(device, "vkDestroyShaderModule");
	returnVal.vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)getDeviceProcAddr(device, "vkDeviceWaitIdle");
	returnVal.vkEndCommandBuffer = (PFN_vkEndCommandBuffer)getDeviceProcAddr(device, "vkEndCommandBuffer");
	returnVal.vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)getDeviceProcAddr(device, "vkFlushMappedMemoryRanges");
	returnVal.vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)getDeviceProcAddr(device, "vkFreeCommandBuffers");
	returnVal.vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets)getDeviceProcAddr(device, "vkFreeDescriptorSets");
	returnVal.vkFreeMemory = (PFN_vkFreeMemory)getDeviceProcAddr(device, "vkFreeMemory");
	returnVal.vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)getDeviceProcAddr(device, "vkGetBufferMemoryRequirements");
	returnVal.vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)getDeviceProcAddr(device, "vkGetDeviceProcAddr");
	returnVal.vkGetDeviceQueue = (PFN_vkGetDeviceQueue)getDeviceProcAddr(device, "vkGetDeviceQueue");
	returnVal.vkGetFenceStatus = (PFN_vkGetFenceStatus)getDeviceProcAddr(device, "vkGetFenceStatus");
	returnVal.vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)getDeviceProcAddr(device, "vkGetImageMemoryRequirements");
	returnVal.vkInvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges)getDeviceProcAddr(device, "vkInvalidateMappedMemoryRanges");
	returnVal.vkMapMemory = (PFN_vkMapMemory)getDeviceProcAddr(device, "vkMapMemory");
	returnVal.vkResetCommandBuffer = (PFN_vkResetCommandBuffer)getDeviceProcAddr(device, "vkResetCommandBuffer");
	returnVal.vkResetCommandPool = (PFN_vkResetCommandPool)getDeviceProcAddr(device, "vkResetCommandPool");
	returnVal.vkResetDescriptorPool = (PFN_vkResetDescriptorPool)getDeviceProcAddr(device, "vkResetDescriptorPool");
	returnVal.vkResetFences = (PFN_vkResetFences)getDeviceProcAddr(device, "vkResetFences");
	returnVal.vkTrimCommandPool = (PFN_vkTrimCommandPool)getDeviceProcAddr(device, "vkTrimCommandPool");
	returnVal.vkUnmapMemory = (PFN_vkUnmapMemory)getDeviceProcAddr(device, "vkUnmapMemory");
	returnVal.vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)getDeviceProcAddr(device, "vkUpdateDescriptorSets");
	returnVal.vkWaitForFences = (PFN_vkWaitForFences)getDeviceProcAddr(device, "vkWaitForFences");
	returnVal.vkQueueSubmit = (PFN_vkQueueSubmit)getDeviceProcAddr(device, "vkQueueSubmit");
	returnVal.vkQueueWaitIdle = (PFN_vkQueueWaitIdle)getDeviceProcAddr(device, "vkQueueWaitIdle");
	return returnVal;
}

EXT_DebugUtilsDispatchRaw EXT_DebugUtilsDispatchRaw::Build(
	vk::Instance inInstance,
	PFN_vkGetInstanceProcAddr instanceProcAddr)
{
	VkInstance instance = static_cast<VkInstance>(inInstance);
	EXT_DebugUtilsDispatchRaw returnVal{};
	returnVal.vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)instanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	returnVal.vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)instanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	returnVal.vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)instanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
	return returnVal;
}

KHR_SurfaceDispatchRaw KHR_SurfaceDispatchRaw::Build(
	vk::Instance inInstance,
	PFN_vkGetInstanceProcAddr instanceProcAddr)
{
	VkInstance instance = static_cast<VkInstance>(inInstance);
	KHR_SurfaceDispatchRaw returnVal{};
	returnVal.vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)instanceProcAddr(instance, "vkDestroySurfaceKHR");
	returnVal.vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)instanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	returnVal.vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)instanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
	returnVal.vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)instanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
	returnVal.vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)instanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
	return returnVal;
}

KHR_SwapchainDispatchRaw KHR_SwapchainDispatchRaw::Build(vk::Device inDevice, PFN_vkGetDeviceProcAddr getProcAddr)
{
	VkDevice device = static_cast<VkDevice>(inDevice);
	KHR_SwapchainDispatchRaw returnVal{};
	returnVal.vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)getProcAddr(device, "vkAcquireNextImageKHR");
	returnVal.vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)getProcAddr(device, "vkCreateSwapchainKHR");
	returnVal.vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)getProcAddr(device, "vkDestroySwapchainKHR");
	returnVal.vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)getProcAddr(device, "vkGetSwapchainImagesKHR");
	returnVal.vkQueuePresentKHR = (PFN_vkQueuePresentKHR)getProcAddr(device, "vkQueuePresentKHR");
	return returnVal;
}

void BaseDispatch::BuildInPlace(
	BaseDispatch& dispatcher,
	PFN_vkGetInstanceProcAddr procAddr)
{
	dispatcher.raw = BaseDispatchRaw::Build(procAddr);
}

vk::Instance BaseDispatch::CreateInstance(
	vk::InstanceCreateInfo const& createInfo, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	vk::Instance returnVal;
	vk::Result vkResult = static_cast<vk::Result>(raw.vkCreateInstance(
		reinterpret_cast<VkInstanceCreateInfo const*>(&createInfo),
		reinterpret_cast<VkAllocationCallbacks const*>(static_cast<vk::AllocationCallbacks const*>(allocator)),
		reinterpret_cast<VkInstance*>(&returnVal)));
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create Vulkan instance.");
	return returnVal;
}

vk::Result BaseDispatch::EnumerateInstanceExtensionProperties(
	char const* pLayerName, 
	std::uint32_t* pPropertyCount,
	vk::ExtensionProperties* pProperties) const
{
	return static_cast<vk::Result>(raw.vkEnumerateInstanceExtensionProperties(
		pLayerName,
		pPropertyCount,
		reinterpret_cast<VkExtensionProperties*>(pProperties)));
}

vk::Result BaseDispatch::EnumerateInstanceLayerProperties(
	std::uint32_t* pPropertyCount, 
	vk::LayerProperties* pProperties) const
{
	return static_cast<vk::Result>(raw.vkEnumerateInstanceLayerProperties(
		pPropertyCount,
		reinterpret_cast<VkLayerProperties*>(pProperties)));
}

std::uint32_t BaseDispatch::EnumerateInstanceVersion() const
{
	vk::Result vkResult = {};
	std::uint32_t returnVal;
	if (raw.vkEnumerateInstanceVersion)
	{
		vkResult = (vk::Result)raw.vkEnumerateInstanceVersion(&returnVal);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Unable to enumerate Vulkan instance version.");
		return returnVal;
	}
	else
		return VK_MAKE_VERSION(1, 0, 0);
}

void InstanceDispatch::BuildInPlace(
	InstanceDispatch& dispatcher,
	vk::Instance instance,
	PFN_vkGetInstanceProcAddr getInstanceProcAddr)
{
	dispatcher.handle = instance;

	dispatcher.raw = InstanceDispatchRaw::Build(instance, getInstanceProcAddr);
	dispatcher.surfaceRaw = KHR_SurfaceDispatchRaw::Build(instance, getInstanceProcAddr);
}

vk::Device InstanceDispatch::createDevice(
	vk::PhysicalDevice physDevice, 
	vk::DeviceCreateInfo const& createInfo, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	vk::Device outDevice;
	vk::Result vkResult = static_cast<vk::Result>(raw.vkCreateDevice(
		static_cast<VkPhysicalDevice>(physDevice),
		reinterpret_cast<VkDeviceCreateInfo const*>(&createInfo),
		reinterpret_cast<VkAllocationCallbacks*>(static_cast<vk::AllocationCallbacks*>(allocator)),
		reinterpret_cast<VkDevice*>(&outDevice)));
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create Vulkan device.");
	return outDevice;
}

void InstanceDispatch::Destroy(vk::Optional<vk::AllocationCallbacks> allocator) const
{
	raw.vkDestroyInstance(
		static_cast<VkInstance>(handle),
		reinterpret_cast<VkAllocationCallbacks*>(static_cast<vk::AllocationCallbacks*>(allocator)));
}

vk::Result InstanceDispatch::enumeratePhysicalDeviceExtensionProperties(
	vk::PhysicalDevice physDevice,
	std::uint32_t* pPropertyCount,
	vk::ExtensionProperties* pProperties) const
{
	return static_cast<vk::Result>(raw.vkEnumerateDeviceExtensionProperties(
		static_cast<VkPhysicalDevice>(physDevice),
		nullptr,
		pPropertyCount,
		reinterpret_cast<VkExtensionProperties*>(pProperties)));
}

vk::Result InstanceDispatch::enumeratePhysicalDevices(
	std::uint32_t* pPhysicalDeviceCount,
	vk::PhysicalDevice* pPhysicalDevices) const
{
	return static_cast<vk::Result>(raw.vkEnumeratePhysicalDevices(
		static_cast<VkInstance>(handle),
		pPhysicalDeviceCount,
		reinterpret_cast<VkPhysicalDevice*>(pPhysicalDevices)));
}

vk::PhysicalDeviceFeatures InstanceDispatch::getPhysicalDeviceFeatures(vk::PhysicalDevice physDevice) const
{
	vk::PhysicalDeviceFeatures outFeatures;
	raw.vkGetPhysicalDeviceFeatures(
		static_cast<VkPhysicalDevice>(physDevice),
		reinterpret_cast<VkPhysicalDeviceFeatures*>(&outFeatures));
	return outFeatures;
}

vk::PhysicalDeviceMemoryProperties InstanceDispatch::getPhysicalDeviceMemoryProperties(vk::PhysicalDevice physDevice) const
{
	vk::PhysicalDeviceMemoryProperties outMemProperties;
	raw.vkGetPhysicalDeviceMemoryProperties(
		static_cast<VkPhysicalDevice>(physDevice),
		reinterpret_cast<VkPhysicalDeviceMemoryProperties*>(&outMemProperties));
	return outMemProperties;
}

vk::PhysicalDeviceProperties InstanceDispatch::getPhysicalDeviceProperties(vk::PhysicalDevice physDevice) const
{
	vk::PhysicalDeviceProperties outProperties;
	raw.vkGetPhysicalDeviceProperties(
		static_cast<VkPhysicalDevice>(physDevice),
		reinterpret_cast<VkPhysicalDeviceProperties*>(&outProperties));
	return outProperties;
}

void InstanceDispatch::getPhysicalDeviceQueueFamilyProperties(
	vk::PhysicalDevice physDevice, 
	std::uint32_t* pQueueFamilyPropertyCount,
	vk::QueueFamilyProperties* pQueueFamilyProperties) const
{
	raw.vkGetPhysicalDeviceQueueFamilyProperties(
		static_cast<VkPhysicalDevice>(physDevice),
		pQueueFamilyPropertyCount,
		reinterpret_cast<VkQueueFamilyProperties*>(pQueueFamilyProperties));
}

void DebugUtilsDispatch::BuildInPlace(
	DebugUtilsDispatch& dispatcher,
	vk::Instance instance,
	PFN_vkGetInstanceProcAddr instanceProcAddr)
{
	dispatcher.raw = EXT_DebugUtilsDispatchRaw::Build(instance, instanceProcAddr);
}

vk::DebugUtilsMessengerEXT DebugUtilsDispatch::createDebugUtilsMessengerEXT(
	vk::Instance instance,
	vk::DebugUtilsMessengerCreateInfoEXT const& createInfo, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	vk::DebugUtilsMessengerEXT returnVal;
	vk::Result vkResult = static_cast<vk::Result>(raw.vkCreateDebugUtilsMessengerEXT(
		static_cast<VkInstance>(instance),
		reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT const*>(&createInfo),
		reinterpret_cast<VkAllocationCallbacks*>(static_cast<vk::AllocationCallbacks*>(allocator)),
		reinterpret_cast<VkDebugUtilsMessengerEXT*>(&returnVal)));
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create Vulkan debug utils messenger.");
	return returnVal;
}

void DebugUtilsDispatch::destroyDebugUtilsMessengerEXT(
	vk::Instance instance, 
	vk::DebugUtilsMessengerEXT messenger,
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	raw.vkDestroyDebugUtilsMessengerEXT(
		static_cast<VkInstance>(instance),
		static_cast<VkDebugUtilsMessengerEXT>(messenger),
		reinterpret_cast<VkAllocationCallbacks*>(static_cast<vk::AllocationCallbacks*>(allocator)));
}

void DebugUtilsDispatch::setDebugUtilsObjectNameEXT(
	vk::Device device,
	vk::DebugUtilsObjectNameInfoEXT const& nameInfo) const
{
	raw.vkSetDebugUtilsObjectNameEXT(
		static_cast<VkDevice>(device),
		reinterpret_cast<VkDebugUtilsObjectNameInfoEXT const*>(&nameInfo));
}

void InstanceDispatch::destroy(
	vk::SurfaceKHR in, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	surfaceRaw.vkDestroySurfaceKHR(
		static_cast<VkInstance>(handle),
		static_cast<VkSurfaceKHR>(in),
		reinterpret_cast<VkAllocationCallbacks*>(static_cast<vk::AllocationCallbacks*>(allocator)));
}

vk::SurfaceCapabilitiesKHR InstanceDispatch::getPhysicalDeviceSurfaceCapabilitiesKHR(
	vk::PhysicalDevice physDevice, 
	vk::SurfaceKHR surface) const
{
	vk::SurfaceCapabilitiesKHR surfaceCaps;
	vk::Result vkResult = static_cast<vk::Result>(surfaceRaw.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		static_cast<VkPhysicalDevice>(physDevice),
		static_cast<VkSurfaceKHR>(surface),
		reinterpret_cast<VkSurfaceCapabilitiesKHR*>(&surfaceCaps)));
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to get surface capabilities.");
	return surfaceCaps;
}

vk::Result InstanceDispatch::getPhysicalDeviceSurfaceFormatsKHR(
	vk::PhysicalDevice physDevice, 
	vk::SurfaceKHR surface,
	std::uint32_t* pSurfaceFormatCount, 
	vk::SurfaceFormatKHR* pSurfaceFormats) const
{
	return static_cast<vk::Result>(surfaceRaw.vkGetPhysicalDeviceSurfaceFormatsKHR(
		static_cast<VkPhysicalDevice>(physDevice),
		static_cast<VkSurfaceKHR>(surface),
		pSurfaceFormatCount,
		reinterpret_cast<VkSurfaceFormatKHR*>(pSurfaceFormats)));
}

vk::Result InstanceDispatch::getPhysicalDeviceSurfacePresentModesKHR(
	vk::PhysicalDevice physDevice, 
	vk::SurfaceKHR surface,
	std::uint32_t* pPresentModeCount, 
	vk::PresentModeKHR* pPresentModes) const
{
	return static_cast<vk::Result>(surfaceRaw.vkGetPhysicalDeviceSurfacePresentModesKHR(
		static_cast<VkPhysicalDevice>(physDevice),
		static_cast<VkSurfaceKHR>(surface),
		pPresentModeCount,
		reinterpret_cast<VkPresentModeKHR*>(pPresentModes)));
}

bool InstanceDispatch::getPhysicalDeviceSurfaceSupportKHR(
	vk::PhysicalDevice physDevice,
	std::uint32_t queueFamilyIndex,
	vk::SurfaceKHR surface) const
{
	VkBool32 surfaceSupported;
	vk::Result vkResult = static_cast<vk::Result>(surfaceRaw.vkGetPhysicalDeviceSurfaceSupportKHR(
		static_cast<VkPhysicalDevice>(physDevice),
		queueFamilyIndex,
		static_cast<VkSurfaceKHR>(surface),
		&surfaceSupported));
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to query physical device surface support.");
	return surfaceSupported;
}

void DeviceDispatch::BuildInPlace(
	DeviceDispatch& dispatcher,
	vk::Device device,
	PFN_vkGetDeviceProcAddr getProcAddr)
{
	dispatcher.handle = device;

	dispatcher.raw = DeviceDispatchRaw::Build(device, getProcAddr);
	dispatcher.swapchainRaw = KHR_SwapchainDispatchRaw::Build(device, getProcAddr);
}

vk::Result DeviceDispatch::allocateCommandBuffers(
	vk::CommandBufferAllocateInfo const& allocateInfo,
	vk::CommandBuffer* pCommandBuffers) const noexcept
{
	return static_cast<vk::Result>(raw.vkAllocateCommandBuffers(
		static_cast<VkDevice>(handle),
		reinterpret_cast<VkCommandBufferAllocateInfo const*>(&allocateInfo),
		reinterpret_cast<VkCommandBuffer*>(pCommandBuffers)));
}

vk::Result DeviceDispatch::allocateDescriptorSets(
	vk::DescriptorSetAllocateInfo const& allocInfo,
	vk::DescriptorSet* pDescriptorSets) const noexcept
{
	return static_cast<vk::Result>(raw.vkAllocateDescriptorSets(
		static_cast<VkDevice>(handle),
		reinterpret_cast<VkDescriptorSetAllocateInfo const*>(&allocInfo),
		reinterpret_cast<VkDescriptorSet*>(pDescriptorSets)));
}

vk::DeviceMemory DeviceDispatch::allocateMemory(
	vk::MemoryAllocateInfo const& allocInfo,
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	vk::DeviceMemory temp;
	vk::Result result = static_cast<vk::Result>(raw.vkAllocateMemory(
		static_cast<VkDevice>(handle),
		reinterpret_cast<VkMemoryAllocateInfo const*>(&allocInfo),
		reinterpret_cast<VkAllocationCallbacks*>(static_cast<vk::AllocationCallbacks*>(allocator)),
		reinterpret_cast<VkDeviceMemory*>(&temp)));
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to allocate Vulkan memory.");
	return temp;
}

void DeviceDispatch::beginCommandBuffer(
	vk::CommandBuffer cmdBuffer,
	vk::CommandBufferBeginInfo const& beginInfo) const
{
	vk::Result result = static_cast<vk::Result>(raw.vkBeginCommandBuffer(
		static_cast<VkCommandBuffer>(cmdBuffer),
		reinterpret_cast<VkCommandBufferBeginInfo const*>(&beginInfo)));
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to begin vk::CommandBuffer");
}

void DeviceDispatch::cmdBeginRenderPass(
	vk::CommandBuffer commandBuffer, 
	vk::RenderPassBeginInfo const& renderPassBegin,
	vk::SubpassContents contents) const noexcept
{
	raw.vkCmdBeginRenderPass(
		static_cast<VkCommandBuffer>(commandBuffer),
		reinterpret_cast<VkRenderPassBeginInfo const*>(&renderPassBegin),
		static_cast<VkSubpassContents>(contents));
}

void DeviceDispatch::cmdBindDescriptorSets(
	vk::CommandBuffer commandBuffer, 
	vk::PipelineBindPoint pipelineBindPoint, 
	vk::PipelineLayout layout, 
	std::uint32_t firstSet, 
	vk::ArrayProxy<vk::DescriptorSet const> descriptorSets, 
	vk::ArrayProxy<uint32_t const> dynamicOffsets) const noexcept
{
	raw.vkCmdBindDescriptorSets(
		static_cast<VkCommandBuffer>(commandBuffer),
		static_cast<VkPipelineBindPoint>(pipelineBindPoint),
		static_cast<VkPipelineLayout>(layout),
		firstSet,
		descriptorSets.size(),
		reinterpret_cast<VkDescriptorSet const*>(descriptorSets.data()),
		dynamicOffsets.size(),
		dynamicOffsets.data());
}

void DeviceDispatch::cmdBindIndexBuffer(
	vk::CommandBuffer commandBuffer, 
	vk::Buffer buffer, 
	vk::DeviceSize offset, 
	vk::IndexType indexType) const noexcept
{
	raw.vkCmdBindIndexBuffer(
		static_cast<VkCommandBuffer>(commandBuffer),
		static_cast<VkBuffer>(buffer),
		offset,
		static_cast<VkIndexType>(indexType));
}

void DeviceDispatch::cmdBindPipeline(
	vk::CommandBuffer commandBuffer, 
	vk::PipelineBindPoint pipelineBindPoint, 
	vk::Pipeline pipeline) const noexcept
{
	raw.vkCmdBindPipeline(
		static_cast<VkCommandBuffer>(commandBuffer),
		static_cast<VkPipelineBindPoint>(pipelineBindPoint),
		static_cast<VkPipeline>(pipeline));
}

void DeviceDispatch::cmdBindVertexBuffers(
	vk::CommandBuffer commandBuffer,
	uint32_t firstBinding,
	vk::ArrayProxy<vk::Buffer const> buffers,
	vk::ArrayProxy<vk::DeviceSize const> offsets) const noexcept
{
	raw.vkCmdBindVertexBuffers(
		static_cast<VkCommandBuffer>(commandBuffer),
		firstBinding,
		buffers.size(),
		reinterpret_cast<VkBuffer const*>(buffers.data()),
		offsets.data());
}

void DeviceDispatch::cmdCopyBuffer(
	vk::CommandBuffer commandBuffer,
	vk::Buffer srcBuffer, 
	vk::Buffer dstBuffer, 
	vk::ArrayProxy<vk::BufferCopy const> regions) const noexcept
{
	raw.vkCmdCopyBuffer(
		static_cast<VkCommandBuffer>(commandBuffer),
		static_cast<VkBuffer>(srcBuffer),
		static_cast<VkBuffer>(dstBuffer),
		regions.size(),
		reinterpret_cast<VkBufferCopy const*>(regions.data()));
}

void DeviceDispatch::cmdCopyBufferToImage(
	vk::CommandBuffer cmdBuffer,
	vk::Buffer srcBuffer,
	vk::Image dstImage,
	vk::ImageLayout dstImageLayout,
	vk::ArrayProxy<vk::BufferImageCopy const> regions) const noexcept
{
	raw.vkCmdCopyBufferToImage(
		static_cast<VkCommandBuffer>(cmdBuffer),
		static_cast<VkBuffer>(srcBuffer),
		static_cast<VkImage>(dstImage),
		static_cast<VkImageLayout>(dstImageLayout),
		regions.size(),
		reinterpret_cast<VkBufferImageCopy const*>(regions.data()));
}

void DeviceDispatch::cmdCopyImage(
	vk::CommandBuffer commandBuffer, 
	vk::Image srcImage, 
	vk::ImageLayout srcImageLayout,
	vk::Image dstImage,
	vk::ImageLayout dstImageLayout,
	vk::ArrayProxy<vk::ImageCopy const> regions) const noexcept
{
	raw.vkCmdCopyImage(
		static_cast<VkCommandBuffer>(commandBuffer),
		static_cast<VkImage>(srcImage),
		static_cast<VkImageLayout>(srcImageLayout),
		static_cast<VkImage>(dstImage),
		static_cast<VkImageLayout>(dstImageLayout),
		regions.size(),
		reinterpret_cast<VkImageCopy const*>(regions.data()));
}

void DeviceDispatch::cmdDraw(
	vk::CommandBuffer commandBuffer,
	std::uint32_t vertexCount,
	std::uint32_t instanceCount, 
	std::uint32_t firstVertex, 
	std::uint32_t firstInstance) const noexcept
{
	raw.vkCmdDraw(
		static_cast<VkCommandBuffer>(commandBuffer),
		vertexCount,
		instanceCount,
		firstVertex,
		firstInstance);
}

void DeviceDispatch::cmdDrawIndexed(
	vk::CommandBuffer commandBuffer,
	std::uint32_t indexCount, 
	std::uint32_t instanceCount,
	std::uint32_t firstIndex, 
	std::int32_t vertexOffset, 
	std::uint32_t firstInstance) const noexcept
{
	raw.vkCmdDrawIndexed(
		static_cast<VkCommandBuffer>(commandBuffer),
		indexCount,
		instanceCount,
		firstIndex,
		vertexOffset,
		firstInstance);
}

void DeviceDispatch::cmdEndRenderPass(vk::CommandBuffer commandBuffer) const noexcept
{
	raw.vkCmdEndRenderPass(static_cast<VkCommandBuffer>(commandBuffer));
}

void DeviceDispatch::cmdPipelineBarrier(
	vk::CommandBuffer commandBuffer,
	vk::PipelineStageFlags srcStageMask,
	vk::PipelineStageFlags dstStageMask,
	vk::DependencyFlags dependencyFlags,
	vk::ArrayProxy<vk::MemoryBarrier const> memoryBarriers,
	vk::ArrayProxy<vk::BufferMemoryBarrier const> bufferMemoryBarriers, 
	vk::ArrayProxy<vk::ImageMemoryBarrier const> imageMemoryBarriers) const noexcept
{
	raw.vkCmdPipelineBarrier(
		static_cast<VkCommandBuffer>(commandBuffer),
		static_cast<VkPipelineStageFlags>(srcStageMask),
		static_cast<VkPipelineStageFlags>(dstStageMask),
		static_cast<VkDependencyFlags>(dependencyFlags),
		memoryBarriers.size(),
		reinterpret_cast<VkMemoryBarrier const*>(memoryBarriers.data()),
		bufferMemoryBarriers.size(),
		reinterpret_cast<VkBufferMemoryBarrier const*>(bufferMemoryBarriers.data()),
		imageMemoryBarriers.size(),
		reinterpret_cast<VkImageMemoryBarrier const*>(imageMemoryBarriers.data()));
}

void DeviceDispatch::cmdPushConstants(
	vk::CommandBuffer commandBuffer,
	vk::PipelineLayout layout,
	vk::ShaderStageFlags stageFlags,
	std::uint32_t offset,
	std::uint32_t size,
	void const* pValues) const noexcept
{
	raw.vkCmdPushConstants(
		static_cast<VkCommandBuffer>(commandBuffer),
		static_cast<VkPipelineLayout>(layout),
		static_cast<VkShaderStageFlags>(stageFlags),
		offset,
		size,
		pValues);
}

void DeviceDispatch::cmdSetScissor(
	vk::CommandBuffer commandBuffer,
	std::uint32_t firstScissor,
	vk::ArrayProxy<vk::Rect2D const> scissors) const noexcept
{
	raw.vkCmdSetScissor(
		static_cast<VkCommandBuffer>(commandBuffer),
		firstScissor,
		scissors.size(),
		reinterpret_cast<VkRect2D const*>(scissors.data()));
}

void DeviceDispatch::cmdSetViewport(
	vk::CommandBuffer commandBuffer, 
	std::uint32_t firstViewport, 
	vk::ArrayProxy<vk::Viewport const> viewports) const noexcept
{
	raw.vkCmdSetViewport(
		static_cast<VkCommandBuffer>(commandBuffer),
		firstViewport,
		viewports.size(),
		reinterpret_cast<VkViewport const*>(viewports.data()));
}

vk::ResultValue<vk::CommandPool> DeviceDispatch::createCommandPool(
	vk::CommandPoolCreateInfo const& createInfo,
	vk::Optional<vk::AllocationCallbacks> allocator) const noexcept
{
	vk::CommandPool temp;
	vk::Result result = static_cast<vk::Result>(raw.vkCreateCommandPool(
		static_cast<VkDevice>(handle),
		reinterpret_cast<VkCommandPoolCreateInfo const*>(&createInfo),
		reinterpret_cast<VkAllocationCallbacks*>(static_cast<vk::AllocationCallbacks*>(allocator)),
		reinterpret_cast<VkCommandPool*>(&temp)));
	return { result, temp };
}

vk::DescriptorPool DeviceDispatch::createDescriptorPool(
	vk::DescriptorPoolCreateInfo const& createInfo,
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	vk::DescriptorPool temp;
	vk::Result result = static_cast<vk::Result>(raw.vkCreateDescriptorPool(
		static_cast<VkDevice>(handle),
		reinterpret_cast<VkDescriptorPoolCreateInfo const*>(&createInfo),
		reinterpret_cast<VkAllocationCallbacks*>(static_cast<vk::AllocationCallbacks*>(allocator)),
		reinterpret_cast<VkDescriptorPool*>(&temp)));
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create Vulkan descriptor pool.");
	return temp;
}

vk::DescriptorSetLayout DeviceDispatch::createDescriptorSetLayout(
	vk::DescriptorSetLayoutCreateInfo const& createInfo,
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	vk::DescriptorSetLayout temp;
	vk::Result result = static_cast<vk::Result>(raw.vkCreateDescriptorSetLayout(
		static_cast<VkDevice>(handle),
		reinterpret_cast<VkDescriptorSetLayoutCreateInfo const*>(&createInfo),
		reinterpret_cast<VkAllocationCallbacks*>(static_cast<vk::AllocationCallbacks*>(allocator)),
		reinterpret_cast<VkDescriptorSetLayout*>(&temp)));
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create Vulkan descriptorset layout.");
	return temp;
}

vk::Fence DeviceDispatch::createFence(
	vk::FenceCreateInfo const& createInfo,
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	vk::Fence outFence;
	vk::Result result = static_cast<vk::Result>(raw.vkCreateFence(
		static_cast<VkDevice>(handle),
		reinterpret_cast<VkFenceCreateInfo const*>(&createInfo),
		reinterpret_cast<VkAllocationCallbacks*>(static_cast<vk::AllocationCallbacks*>(allocator)),
		reinterpret_cast<VkFence*>(&outFence)));
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create Vulkan fence.");
	return outFence;
}

vk::Framebuffer DeviceDispatch::createFramebuffer(
	vk::FramebufferCreateInfo const& createInfo, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	vk::Framebuffer outFramebuffer;
	vk::Result result = static_cast<vk::Result>(raw.vkCreateFramebuffer(
		static_cast<VkDevice>(handle),
		reinterpret_cast<VkFramebufferCreateInfo const*>(&createInfo),
		reinterpret_cast<VkAllocationCallbacks*>(static_cast<vk::AllocationCallbacks*>(allocator)),
		reinterpret_cast<VkFramebuffer*>(&outFramebuffer)));
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create Vulkan framebuffer.");
	return outFramebuffer;
}

vk::Image DeviceDispatch::createImage(
	vk::ImageCreateInfo const& createInfo,
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	vk::Image temp;
	vk::Result result = static_cast<vk::Result>(raw.vkCreateImage(
		static_cast<VkDevice>(handle),
		reinterpret_cast<VkImageCreateInfo const*>(&createInfo),
		reinterpret_cast<VkAllocationCallbacks const*>(static_cast<vk::AllocationCallbacks const*>(allocator)),
		reinterpret_cast<VkImage*>(&temp)));
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create Vulkan image.");
	return temp;
}

vk::ImageView DeviceDispatch::createImageView(
	vk::ImageViewCreateInfo const& createInfo,
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	vk::ImageView temp;
	vk::Result result = static_cast<vk::Result>(raw.vkCreateImageView(
		static_cast<VkDevice>(handle),
		reinterpret_cast<VkImageViewCreateInfo const*>(&createInfo),
		reinterpret_cast<VkAllocationCallbacks const*>(static_cast<vk::AllocationCallbacks const*>(allocator)),
		reinterpret_cast<VkImageView*>(&temp)));
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create Vulkan image-view.");
	return temp;
}

vk::PipelineLayout DeviceDispatch::createPipelineLayout(
	vk::PipelineLayoutCreateInfo const& createInfo, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	vk::PipelineLayout outPipelineLayout;
	vk::Result result = static_cast<vk::Result>(raw.vkCreatePipelineLayout(
		static_cast<VkDevice>(handle),
		reinterpret_cast<VkPipelineLayoutCreateInfo const*>(&createInfo),
		reinterpret_cast<VkAllocationCallbacks const*>(static_cast<vk::AllocationCallbacks const*>(allocator)),
		reinterpret_cast<VkPipelineLayout*>(&outPipelineLayout)));
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create Vulkan pipeline layout.");
	return outPipelineLayout;
}

vk::RenderPass DeviceDispatch::createRenderPass(
	vk::RenderPassCreateInfo const& createInfo, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	vk::RenderPass outRenderPass;
	vk::Result result = static_cast<vk::Result>(raw.vkCreateRenderPass(
		static_cast<VkDevice>(handle),
		reinterpret_cast<VkRenderPassCreateInfo const*>(&createInfo),
		reinterpret_cast<VkAllocationCallbacks const*>(static_cast<vk::AllocationCallbacks const*>(allocator)),
		reinterpret_cast<VkRenderPass*>(&outRenderPass)));
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create Vulkan renderpass.");
	return outRenderPass;
}

vk::Sampler DeviceDispatch::createSampler(
	vk::SamplerCreateInfo const& createInfo,
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	vk::Sampler temp;
	vk::Result result = static_cast<vk::Result>(raw.vkCreateSampler(
		static_cast<VkDevice>(handle),
		reinterpret_cast<VkSamplerCreateInfo const*>(&createInfo),
		reinterpret_cast<VkAllocationCallbacks const*>(static_cast<vk::AllocationCallbacks const*>(allocator)),
		reinterpret_cast<VkSampler*>(&temp)));
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create Vulkan sampler.");
	return temp;
}

vk::ResultValue<vk::Semaphore> DeviceDispatch::createSemaphore(
	vk::SemaphoreCreateInfo const& createInfo,
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	vk::Semaphore temp;
	vk::Result result = static_cast<vk::Result>(raw.vkCreateSemaphore(
		static_cast<VkDevice>(handle),
		reinterpret_cast<VkSemaphoreCreateInfo const*>(&createInfo),
		reinterpret_cast<VkAllocationCallbacks const*>(static_cast<vk::AllocationCallbacks const*>(allocator)),
		reinterpret_cast<VkSemaphore*>(&temp)));
	return { result, temp };
}

vk::ShaderModule DeviceDispatch::createShaderModule(
	vk::ShaderModuleCreateInfo const& createInfo, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	vk::ShaderModule outShaderModule;
	vk::Result result = static_cast<vk::Result>(raw.vkCreateShaderModule(
		static_cast<VkDevice>(handle),
		reinterpret_cast<VkShaderModuleCreateInfo const*>(&createInfo),
		reinterpret_cast<VkAllocationCallbacks const*>(static_cast<vk::AllocationCallbacks const*>(allocator)),
		reinterpret_cast<VkShaderModule*>(&outShaderModule)));
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create Vulkan shader module.");
	return outShaderModule;
}

vk::Result DeviceDispatch::createGraphicsPipelines(
	vk::PipelineCache pipelineCache,
	vk::ArrayProxy<vk::GraphicsPipelineCreateInfo const> createInfos,
	vk::Optional<vk::AllocationCallbacks> allocator,
	vk::Pipeline* pPipelines) const
{
	return static_cast<vk::Result>(raw.vkCreateGraphicsPipelines(
		static_cast<VkDevice>(handle),
		static_cast<VkPipelineCache>(pipelineCache),
		createInfos.size(),
		reinterpret_cast<VkGraphicsPipelineCreateInfo const*>(createInfos.data()),
		reinterpret_cast<VkAllocationCallbacks*>(static_cast<vk::AllocationCallbacks*>(allocator)),
		reinterpret_cast<VkPipeline*>(pPipelines)));
}

#define DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(typeName) \
	raw.vkDestroy ##typeName( \
		static_cast<VkDevice>(handle), \
		static_cast<Vk ##typeName>(in), \
		reinterpret_cast<VkAllocationCallbacks*>(static_cast<vk::AllocationCallbacks*>(allocator))); \
		

void DeviceDispatch::destroy(
	vk::CommandPool in,
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(CommandPool)
}

void DeviceDispatch::destroy(
	vk::DescriptorPool in, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(DescriptorPool)
}

void DeviceDispatch::destroy(
	vk::Fence in, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(Fence)
}

void DeviceDispatch::destroy(
	vk::Framebuffer in, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(Framebuffer)
}

void DeviceDispatch::destroy(
	vk::Image in, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(Image)
}

void DeviceDispatch::destroy(
	vk::ImageView in, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(ImageView)
}

void DeviceDispatch::destroy(
	vk::RenderPass in, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(RenderPass)
}

void DeviceDispatch::destroy(
	vk::ShaderModule in, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	DENGINE_GFX_VK_DEVICEDISPATCH_MAKEDESTROYFUNC(ShaderModule)
}

void DeviceDispatch::endCommandBuffer(vk::CommandBuffer cmdBuffer) const
{
	raw.vkEndCommandBuffer(static_cast<VkCommandBuffer>(cmdBuffer));
}

void DeviceDispatch::freeCommandBuffers(
	vk::CommandPool cmdPool, 
	vk::ArrayProxy<vk::CommandBuffer const> cmdBuffers) const
{
	raw.vkFreeCommandBuffers(
		static_cast<VkDevice>(handle),
		static_cast<VkCommandPool>(cmdPool),
		cmdBuffers.size(),
		reinterpret_cast<VkCommandBuffer const*>(cmdBuffers.data()));
}

void DeviceDispatch::freeMemory(
	vk::DeviceMemory memory, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	raw.vkFreeMemory(
		static_cast<VkDevice>(handle),
		static_cast<VkDeviceMemory>(memory),
		reinterpret_cast<VkAllocationCallbacks*>(static_cast<vk::AllocationCallbacks*>(allocator)));
}

vk::Result DeviceDispatch::getFenceStatus(vk::Fence fence) const noexcept
{
	return static_cast<vk::Result>(raw.vkGetFenceStatus(
		static_cast<VkDevice>(handle),
		static_cast<VkFence>(fence)));
}

vk::Queue DeviceDispatch::getQueue(
	std::uint32_t familyIndex,
	std::uint32_t queueIndex) const
{
	vk::Queue outQueue;
	raw.vkGetDeviceQueue(
		static_cast<VkDevice>(handle),
		familyIndex,
		queueIndex,
		reinterpret_cast<VkQueue*>(&outQueue));
	return outQueue;
}

void DeviceDispatch::resetCommandPool(
	vk::CommandPool commandPool, 
	vk::CommandPoolResetFlags flags) const
{
	raw.vkResetCommandPool(
		static_cast<VkDevice>(handle),
		static_cast<VkCommandPool>(commandPool),
		static_cast<VkCommandPoolResetFlags>(flags));
}

void DeviceDispatch::resetFences(vk::ArrayProxy<vk::Fence const> fences) const
{
	raw.vkResetFences(
		static_cast<VkDevice>(handle),
		fences.size(),
		reinterpret_cast<VkFence const*>(fences.data()));
}

void DeviceDispatch::updateDescriptorSets(
	vk::ArrayProxy<vk::WriteDescriptorSet const> descriptorWrites,
	vk::ArrayProxy<vk::CopyDescriptorSet const> descriptorCopies) const
{
	raw.vkUpdateDescriptorSets(
		static_cast<VkDevice>(handle),
		descriptorWrites.size(),
		reinterpret_cast<VkWriteDescriptorSet const*>(descriptorWrites.data()),
		descriptorCopies.size(),
		reinterpret_cast<VkCopyDescriptorSet const*>(descriptorCopies.data()));
}

vk::Result DeviceDispatch::waitForFences(
	vk::ArrayProxy<vk::Fence const> fences, 
	bool waitAll, 
	std::uint64_t timeout) const noexcept
{
	return static_cast<vk::Result>(raw.vkWaitForFences(
		static_cast<VkDevice>(handle),
		fences.size(),
		reinterpret_cast<VkFence const*>(fences.data()),
		waitAll,
		timeout));
}

#include "QueueData.hpp"
void DeviceDispatch::waitIdle() const
{
	// Go through all queues and lock their mutexes.
	std::lock_guard graphicsLock{ m_queueDataPtr->graphics.m_lock };
	std::lock_guard transferLock{ m_queueDataPtr->transfer.m_lock };
	std::lock_guard computeLock{ m_queueDataPtr->compute.m_lock };

	raw.vkDeviceWaitIdle(static_cast<VkDevice>(handle));
}

vk::ResultValue<std::uint32_t> DeviceDispatch::acquireNextImageKHR(
	vk::SwapchainKHR swapchain, 
	std::uint64_t timeout, 
	vk::Semaphore semaphore, 
	vk::Fence fence) const noexcept
{
	std::uint32_t outIndex;
	vk::Result result = static_cast<vk::Result>(swapchainRaw.vkAcquireNextImageKHR(
		static_cast<VkDevice>(handle),
		static_cast<VkSwapchainKHR>(swapchain),
		timeout,
		static_cast<VkSemaphore>(semaphore),
		static_cast<VkFence>(fence),
		&outIndex));
	return { result, outIndex };
}

vk::SwapchainKHR DeviceDispatch::createSwapchainKHR(
	vk::SwapchainCreateInfoKHR const& createInfo, 
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	vk::SwapchainKHR outSwapchain;
	vk::Result result = static_cast<vk::Result>(swapchainRaw.vkCreateSwapchainKHR(
		static_cast<VkDevice>(handle),
		reinterpret_cast<VkSwapchainCreateInfoKHR const*>(&createInfo),
		reinterpret_cast<VkAllocationCallbacks const*>(static_cast<vk::AllocationCallbacks const*>(allocator)),
		reinterpret_cast<VkSwapchainKHR*>(&outSwapchain)));
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create Vulkan swapchain");
	return outSwapchain;
}

void DeviceDispatch::destroy(
	vk::SwapchainKHR in,
	vk::Optional<vk::AllocationCallbacks> allocator) const
{
	swapchainRaw.vkDestroySwapchainKHR(
		static_cast<VkDevice>(handle),
		static_cast<VkSwapchainKHR>(in),
		reinterpret_cast<VkAllocationCallbacks*>(static_cast<vk::AllocationCallbacks*>(allocator)));
}

vk::Result DeviceDispatch::getSwapchainImagesKHR(
	vk::SwapchainKHR swapchain,
	std::uint32_t* pSwapchainImageCount,
	vk::Image* pSwapchainImages) const noexcept
{
	return static_cast<vk::Result>(swapchainRaw.vkGetSwapchainImagesKHR(
		static_cast<VkDevice>(handle),
		static_cast<VkSwapchainKHR>(swapchain),
		pSwapchainImageCount,
		reinterpret_cast<VkImage*>(pSwapchainImages)));
}

vk::Result DeviceDispatch::queuePresentKHR(
	vk::Queue queue, 
	vk::PresentInfoKHR const& presentInfo) const noexcept
{
	return static_cast<vk::Result>(swapchainRaw.vkQueuePresentKHR(
		static_cast<VkQueue>(queue),
		reinterpret_cast<VkPresentInfoKHR const*>(&presentInfo)));
}
