#include "Init.hpp"

#include "VulkanIncluder.hpp"

#include "DEngine/Gfx/Assert.hpp"

#include "ImGui/imgui_impl_vulkan.h"

#include <string>

#undef max

namespace DEngine::Gfx::Vk
{
	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverityIn,
		VkDebugUtilsMessageTypeFlagsEXT messageTypeIn,
		VkDebugUtilsMessengerCallbackDataEXT const* pCallbackDataIn,
		void* pUserData)
	{
		auto messageSeverity = static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverityIn);
		auto messageType = static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypeIn);
		auto pCallbackData = reinterpret_cast<vk::DebugUtilsMessengerCallbackDataEXT const*>(pCallbackDataIn);

		ILog* logger = static_cast<ILog*>(pUserData);

		if (logger != nullptr)
		{
			std::string msg;
			msg.reserve(512);

			if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
				msg += "[Error";
			else if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
				msg += "[Warning";
			else if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
				msg += "[Info";
			else if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose)
				msg += "[Verbose";

			if (messageType & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
				msg += ", General";
			if (messageType & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
				msg += ", Performance";
			else if (messageType & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
				msg += ", Validation";


			msg += "] - ";

			msg += pCallbackData->pMessage;

			logger->log(msg.data());
		}


		return 0;
	}
}

DEngine::Gfx::Vk::Init::CreateVkInstance_Return DEngine::Gfx::Vk::Init::CreateVkInstance(
	Std::Span<char const*> requiredExtensions,
	bool enableLayers,
	BaseDispatch const& baseDispatch,
	ILog* logger)
{
	vk::Result vkResult{};
	CreateVkInstance_Return returnValue{};

	// Build what extensions we are going to use
	std::vector<char const*> totalRequiredExtensions;
	totalRequiredExtensions.reserve(requiredExtensions.Size() + Constants::requiredInstanceExtensions.size());
	// First copy all required instance extensions
	for (uSize i = 0; i < requiredExtensions.Size(); i++)
		totalRequiredExtensions.push_back(requiredExtensions[i]);

	// Next add extensions required by renderer, don't add duplicates
	for (const char* requiredExtension : Constants::requiredInstanceExtensions)
	{
		bool extensionAlreadyPresent = false;
		for (const char* existingExtension : totalRequiredExtensions)
		{
			if (std::strcmp(requiredExtension, existingExtension) == 0)
			{
				extensionAlreadyPresent = true;
				break;
			}
		}
		if (!extensionAlreadyPresent)
			totalRequiredExtensions.push_back(requiredExtension);
	}

	// Check if all the required extensions are also available
	u32 instanceExtensionCount = 0;
	vkResult = baseDispatch.enumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
	if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
		throw std::runtime_error("Vulkan: Unable to enumerate available instance extension properties.");
	std::vector<vk::ExtensionProperties> availableExtensions(instanceExtensionCount);
	baseDispatch.enumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableExtensions.data());
	for (const char* required : totalRequiredExtensions)
	{
		bool requiredExtensionIsAvailable = false;
		for (const auto& available : availableExtensions)
		{
			if (std::strcmp(required, available.extensionName) == 0)
			{
				requiredExtensionIsAvailable = true;
				break;
			}
		}
		if (requiredExtensionIsAvailable == false)
			throw std::runtime_error("Required Vulkan instance extension is not available.");
	}

	Std::StaticVector<const char*, 5> layersToUse{};
	if constexpr (Constants::enableDebugUtils)
	{
		if (enableLayers)
		{
			// Check if debug utils is available through global list.
			bool debugUtilsIsAvailable = false;
			for (const auto& ext : availableExtensions)
			{
				if (std::strcmp(ext.extensionName, Constants::debugUtilsExtensionName) == 0)
				{
					debugUtilsIsAvailable = true;
					break;
				}
			}


			// First check if the Khronos validation layer is present
			u32 availableLayerCount = 0;
			vkResult = baseDispatch.enumerateInstanceLayerProperties(&availableLayerCount, nullptr);
			if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
				throw std::runtime_error("Failed to enumerate instance layer properties during Vulkan instance creation.");
			std::vector<vk::LayerProperties> availableLayers;
			availableLayers.resize(availableLayerCount);
			baseDispatch.enumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());
			bool layerIsAvailable = false;
			for (const auto& availableLayer : availableLayers)
			{
				char const* wantedLayerName = Constants::khronosLayerName;
				char const* availableLayerName = availableLayer.layerName;
				if (std::strcmp(wantedLayerName, availableLayerName) == 0)
				{
					layerIsAvailable = true;
					// If the layer is available, we know it implements debug utils.
					debugUtilsIsAvailable = true;
					break;
				}
			}
			if (debugUtilsIsAvailable && layerIsAvailable)
			{
				totalRequiredExtensions.push_back(Constants::debugUtilsExtensionName);
				layersToUse.PushBack(Constants::khronosLayerName);

				returnValue.debugUtilsEnabled = true;
			}
		}
	}

	vk::InstanceCreateInfo instanceInfo{};
	instanceInfo.enabledExtensionCount = (u32)totalRequiredExtensions.size();
	instanceInfo.ppEnabledExtensionNames = totalRequiredExtensions.data();
	instanceInfo.enabledLayerCount = (u32)layersToUse.Size();
	instanceInfo.ppEnabledLayerNames = layersToUse.Data();

	vk::ApplicationInfo appInfo{};
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pApplicationName = "DidgyImguiVulkanTest";
	appInfo.pEngineName = "Nothing special";
	instanceInfo.pApplicationInfo = &appInfo;

	vk::Instance instance = baseDispatch.createInstance(instanceInfo);

	returnValue.instanceHandle = instance;

	return returnValue;
}

vk::DebugUtilsMessengerEXT DEngine::Gfx::Vk::Init::CreateLayerMessenger(
	vk::Instance instanceHandle,
	DebugUtilsDispatch const* debugUtilsOpt,
	const void* userData)
{
	vk::DebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
	debugMessengerInfo.messageSeverity =
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		//vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
	debugMessengerInfo.messageType =
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
	debugMessengerInfo.pfnUserCallback = VulkanDebugCallback;
	debugMessengerInfo.pUserData = const_cast<void*>(userData);

	auto infoPtr = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT const*>(&debugMessengerInfo);

	return debugUtilsOpt->createDebugUtilsMessengerEXT(instanceHandle, debugMessengerInfo);
}

DEngine::Gfx::Vk::PhysDeviceInfo DEngine::Gfx::Vk::Init::LoadPhysDevice(
	InstanceDispatch const& instance,
	vk::SurfaceKHR surface)
{
	PhysDeviceInfo physDevice{};
	vk::Result vkResult{};

	u32 physicalDeviceCount = 0;
	vkResult = instance.enumeratePhysicalDevices(&physicalDeviceCount, nullptr);
	if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
		throw std::runtime_error("Vulkan: Unable to enumerate physical devices.");
	if (physicalDeviceCount == 0)
		throw std::runtime_error("Vulkan: Host machine has no Vulkan-capable devices.");
	std::vector<vk::PhysicalDevice> physDevices;
	physDevices.resize(physicalDeviceCount);
	instance.enumeratePhysicalDevices(&physicalDeviceCount, physDevices.data());

	// For now we just select the first physDevice we find.
	physDevice.handle = physDevices[0];

	// Find preferred queues
	u32 queueFamilyPropertyCount = 0;
	instance.getPhysicalDeviceQueueFamilyProperties(physDevice.handle, &queueFamilyPropertyCount, nullptr);
	std::vector<vk::QueueFamilyProperties> availableQueueFamilies;
	availableQueueFamilies.resize(queueFamilyPropertyCount);
	instance.getPhysicalDeviceQueueFamilyProperties(
		physDevice.handle, 
		&queueFamilyPropertyCount, 
		availableQueueFamilies.data());

	// Find graphics queue
	for (u32 i = 0; i < queueFamilyPropertyCount; i++)
	{
		const auto& queueFamily = availableQueueFamilies[i];
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			physDevice.queueIndices.graphics.familyIndex = i;
			physDevice.queueIndices.graphics.queueIndex = 0;
			break;
		}
	}
	if (physDevice.queueIndices.graphics.familyIndex == invalidIndex)
		throw std::runtime_error("DEngine - Vulkan: Unable to find a graphics queue on VkPhysicalDevice.");
	// Find transfer queue, prefer a queue on a different family than graphics. 
	for (u32 i = 0; i < queueFamilyPropertyCount; i++)
	{
		if (i == physDevice.queueIndices.graphics.familyIndex)
			continue;

		const auto& queueFamily = availableQueueFamilies[i];
		if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
		{
			physDevice.queueIndices.transfer.familyIndex = i;
			physDevice.queueIndices.transfer.queueIndex = 0;
			break;
		}
	}

	// Check presentation support
	bool presentSupport = instance.getPhysicalDeviceSurfaceSupportKHR(physDevice.handle, physDevice.queueIndices.graphics.familyIndex, surface);
	if (!presentSupport)
		throw std::runtime_error("DEngine - Vulkan: No surface present support.");

	physDevice.properties = instance.getPhysicalDeviceProperties(physDevice.handle);

	physDevice.memProperties = instance.getPhysicalDeviceMemoryProperties(physDevice.handle);

	// Find physDevice-local memory
	for (u32 i = 0; i < physDevice.memProperties.memoryTypeCount; i++)
	{
		if (physDevice.memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
		{
			physDevice.memInfo.deviceLocal = i;
			break;
		}
	}
	if (physDevice.memInfo.deviceLocal == invalidIndex)
		throw std::runtime_error("Unable to find any physDevice-local memory during Vulkan initialization.");

	// Find host-visible memory
	for (u32 i = 0; i < physDevice.memProperties.memoryTypeCount; i++)
	{
		if (physDevice.memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
		{
			physDevice.memInfo.hostVisible = i;
			break;
		}
	}
	if (physDevice.memInfo.hostVisible == invalidIndex)
		throw std::runtime_error("Unable to find any physDevice-local memory during Vulkan initialization.");

	// Find host-visible | physDevice local memory
	for (u32 i = 0; i < physDevice.memProperties.memoryTypeCount; i++)
	{
		vk::MemoryPropertyFlags searchFlags = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible;
		if ((physDevice.memProperties.memoryTypes[i].propertyFlags & searchFlags) == searchFlags)
		{
			if (physDevice.memProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
			{
				physDevice.memInfo.deviceLocalAndHostVisible = i;
				break;
			}
		}
	}

	return physDevice;
}

DEngine::Gfx::Vk::SurfaceInfo DEngine::Gfx::Vk::Init::BuildSurfaceInfo(
	InstanceDispatch const& instance,
	vk::PhysicalDevice physDevice,
	vk::SurfaceKHR surface,
	ILog* logger)
{
	vk::Result vkResult{};

	SurfaceInfo returnVal{};

	returnVal.handle = surface;

	returnVal.capabilities = instance.getPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface);

	// Grab present modes
	u32 presentModeCount = 0;
	vkResult = instance.getPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, nullptr);
	if ((vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete) || presentModeCount == 0)
	{
		logger->log("Vulkan: Unable to query present modes for Vulkan surface.");
		std::abort();
	}
	returnVal.supportedPresentModes.resize(presentModeCount);
	instance.getPhysicalDeviceSurfacePresentModesKHR(
		physDevice, 
		surface, 
		&presentModeCount, 
		returnVal.supportedPresentModes.data());

	// Grab surface formats
	u32 surfaceFormatCount = 0;
	vkResult = instance.getPhysicalDeviceSurfaceFormatsKHR(
		physDevice, 
		surface, 
		&surfaceFormatCount, 
		nullptr);
	if ((vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete) || surfaceFormatCount == 0)
	{
		logger->log("Vulkan: Unable to query surface formats for Vulkan surface.");
		std::abort();
	}
	returnVal.supportedSurfaceFormats.resize(surfaceFormatCount);
	instance.getPhysicalDeviceSurfaceFormatsKHR(
		physDevice, 
		surface, 
		&surfaceFormatCount, 
		returnVal.supportedSurfaceFormats.data());

	if (returnVal.capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque)
		returnVal.compositeAlphaFlag = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	else if (returnVal.capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)
		returnVal.compositeAlphaFlag = vk::CompositeAlphaFlagBitsKHR::eInherit;
	else
	{
		logger->log("Vulkan: Found no suitable compostive alpha flag bit for swapchain.");
		std::abort();
	}

	return returnVal;
}

void DEngine::Gfx::Vk::Init::RebuildSurfaceInfo(
	InstanceDispatch const& instance,
	PhysDeviceInfo const& physDevice,
	vk::SurfaceKHR newSurface,
	SurfaceInfo& outSurfaceInfo)
{
	outSurfaceInfo.handle = newSurface;

	outSurfaceInfo.capabilities = instance.getPhysicalDeviceSurfaceCapabilitiesKHR(physDevice.handle, outSurfaceInfo.handle);

	// Check presentation support
	bool presentSupport = instance.getPhysicalDeviceSurfaceSupportKHR(physDevice.handle, physDevice.queueIndices.graphics.familyIndex, outSurfaceInfo.handle);
	if (presentSupport == false)
		throw std::runtime_error("Vulkan: No presentation support for new surface");
}

DEngine::Gfx::Vk::SwapchainSettings DEngine::Gfx::Vk::Init::BuildSwapchainSettings(
	InstanceDispatch const& instance,
	vk::PhysicalDevice physDevice,
	SurfaceInfo const& surfaceInfo,
	ILog* logger)
{
	SwapchainSettings settings{};
	settings.surface = surfaceInfo.handle;
	settings.capabilities = surfaceInfo.capabilities;
	settings.extents = settings.capabilities.currentExtent;
	settings.compositeAlphaFlag = surfaceInfo.compositeAlphaFlag;

	// Handle swapchainData length
	u32 swapchainLength = Constants::preferredSwapchainLength;
	// If we need to, clamp the swapchainData length.
	// Upper clamp only applies if maxImageCount != 0
	if (settings.capabilities.maxImageCount != 0 && swapchainLength > settings.capabilities.maxImageCount)
		swapchainLength = settings.capabilities.maxImageCount;
	if (swapchainLength < 2)
	{
		logger->log("Vulkan: Vulkan backend doesn't support swapchainData length of 1.");
		std::abort();
	}

	settings.numImages = swapchainLength;

	// Find supported formats, find the preferred present-mode
	// If not found, fallback to FIFO, it's guaranteed to be supported.
	vk::PresentModeKHR preferredPresentMode = Constants::preferredPresentMode;
	vk::PresentModeKHR presentModeToUse{};
	bool preferredPresentModeFound = false;
	for (auto const availableMode : surfaceInfo.supportedPresentModes)
	{
		if (availableMode == preferredPresentMode)
		{
			preferredPresentModeFound = true;
			presentModeToUse = availableMode;
			break;
		}
	}
	// FIFO is guaranteed to exist, so we fallback to that one if we didn't find the one we wanted.
	if (!preferredPresentModeFound)
		presentModeToUse = vk::PresentModeKHR::eFifo;
	settings.presentMode = presentModeToUse;

	// Handle formats
	vk::SurfaceFormatKHR formatToUse = vk::SurfaceFormatKHR{};
	bool foundPreferredFormat = false;
	for (const auto& preferredFormat : Constants::preferredSurfaceFormats)
	{
		for (const auto& availableFormat : surfaceInfo.supportedSurfaceFormats)
		{
			if (availableFormat == preferredFormat)
			{
				formatToUse = preferredFormat;
				foundPreferredFormat = true;
				break;
			}
		}
		if (foundPreferredFormat)
			break;
	}
	if (!foundPreferredFormat)
	{
		logger->log("Vulkan: Found no suitable surface format when querying VkSurfaceKHR.");
		std::abort();
	}
	settings.surfaceFormat = formatToUse;

	return settings;
}

vk::Device DEngine::Gfx::Vk::Init::CreateDevice(
	InstanceDispatch const& instance,
	PhysDeviceInfo const& physDevice)
{
	vk::Result vkResult{};

	// CreateJob logical physDevice
	vk::DeviceCreateInfo createInfo{};

	// Feature configuration
	vk::PhysicalDeviceFeatures physDeviceFeatures = instance.getPhysicalDeviceFeatures(physDevice.handle);

	vk::PhysicalDeviceFeatures featuresToUse{};
	if (physDeviceFeatures.robustBufferAccess == 1)
		featuresToUse.robustBufferAccess = true;
	//if (physDeviceFeatures.sampleRateShading == 1)
			//featuresToUse.sampleRateShading = true;

	createInfo.pEnabledFeatures = &featuresToUse;

	// Queue configuration
	f32 priority[3] = { 1.f, 1.f, 1.f };
	Std::StaticVector<vk::DeviceQueueCreateInfo, 10> queueCreateInfos{};

	vk::DeviceQueueCreateInfo tempQueueCreateInfo{};

	// Add graphics queue
	tempQueueCreateInfo.pQueuePriorities = priority;
	tempQueueCreateInfo.queueCount = 1;
	tempQueueCreateInfo.queueFamilyIndex = physDevice.queueIndices.graphics.familyIndex;
	queueCreateInfos.PushBack(tempQueueCreateInfo);

	// Add transfer queue if there is a separate one from graphics queue
	if (physDevice.queueIndices.transfer.familyIndex != invalidIndex)
	{
		tempQueueCreateInfo = vk::DeviceQueueCreateInfo{};
		tempQueueCreateInfo.pQueuePriorities = priority;
		tempQueueCreateInfo.queueCount = 1;
		tempQueueCreateInfo.queueFamilyIndex = physDevice.queueIndices.transfer.familyIndex;

		queueCreateInfos.PushBack(tempQueueCreateInfo);
	}

	createInfo.queueCreateInfoCount = (u32)queueCreateInfos.Size();
	createInfo.pQueueCreateInfos = queueCreateInfos.Data();

	u32 deviceExtensionCount = 0;
	vkResult = instance.enumeratePhysicalDeviceExtensionProperties(physDevice.handle, &deviceExtensionCount, nullptr);
	if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
		throw std::runtime_error("Vulkan: Unable to enumerate device extensions.");
	std::vector<vk::ExtensionProperties> availableExtensions(deviceExtensionCount);
	instance.enumeratePhysicalDeviceExtensionProperties(physDevice.handle, &deviceExtensionCount, availableExtensions.data());
	// Check if all required extensions are present
	for (const char* required : Constants::requiredDeviceExtensions)
	{
		bool foundExtension = false;
		for (const auto& available : availableExtensions)
		{
			if (std::strcmp(required, available.extensionName) == 0)
			{
				foundExtension = true;
				break;
			}
		}
		if (foundExtension == false)
			throw std::runtime_error("Not all required physDevice extensions were available during Vulkan initialization.");
	}

	createInfo.ppEnabledExtensionNames = Constants::requiredDeviceExtensions.data();
	createInfo.enabledExtensionCount = u32(Constants::requiredDeviceExtensions.size());

	vk::Device vkDevice = instance.createDevice(physDevice.handle, createInfo);
	return vkDevice;
}

bool DEngine::Gfx::Vk::Init::InitializeViewportManager(
	ViewportManager& vpManager,
	PhysDeviceInfo const& physDevice,
	DevDispatch const& device)
{
	vk::DeviceSize elementSize = 64;
	if (physDevice.properties.limits.minUniformBufferOffsetAlignment > elementSize)
		elementSize = physDevice.properties.limits.minUniformBufferOffsetAlignment;
	vpManager.camElementSize = (uSize)elementSize;

	vk::DescriptorSetLayoutBinding binding{};
	binding.binding = 0;
	binding.descriptorCount = 1;
	binding.descriptorType = vk::DescriptorType::eUniformBuffer;
	binding.stageFlags = vk::ShaderStageFlagBits::eVertex;
	vk::DescriptorSetLayoutCreateInfo descrLayoutInfo{};
	descrLayoutInfo.bindingCount = 1;
	descrLayoutInfo.pBindings = &binding;
	vpManager.cameraDescrLayout = device.createDescriptorSetLayout(descrLayoutInfo);

	return true;
}

DEngine::Std::StaticVector<vk::Fence, DEngine::Gfx::Vk::Constants::maxResourceSets> DEngine::Gfx::Vk::Init::CreateMainFences(
	DevDispatch const& device, 
	u8 resourceSetCount,
	DebugUtilsDispatch const* debugUtils)
{
	Std::StaticVector<vk::Fence, Constants::maxResourceSets> returnVal{};
	returnVal.Resize(resourceSetCount);

	for (uSize i = 0; i < returnVal.Size(); i += 1)
	{
		vk::FenceCreateInfo info{};
		info.flags = vk::FenceCreateFlagBits::eSignaled;
		returnVal[i] = device.createFence(info);

		if (debugUtils)
		{
			std::string name = std::string("Main Fence #") + std::to_string(i);

			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkFence)returnVal[i];
			nameInfo.objectType = returnVal[i].objectType;
			nameInfo.pObjectName = name.data();

			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}
	}

	return returnVal;
}

DEngine::Gfx::Vk::SwapchainData DEngine::Gfx::Vk::Init::CreateSwapchain(
	Vk::DeviceDispatch const& device,
	QueueData const& queues,
	DeletionQueue const& deletionQueue,
	SwapchainSettings const& settings,
	DebugUtilsDispatch const* debugUtils)
{
	vk::Result vkResult{};

	SwapchainData swapchain;
	swapchain.uid = 0;
	swapchain.extents = settings.extents;
	swapchain.surfaceFormat = settings.surfaceFormat;
	swapchain.presentMode = settings.presentMode;

	vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageExtent = settings.extents;
	swapchainCreateInfo.imageFormat = settings.surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = settings.surfaceFormat.colorSpace;
	swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
	swapchainCreateInfo.presentMode = settings.presentMode;
	swapchainCreateInfo.surface = settings.surface;
	swapchainCreateInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	swapchainCreateInfo.clipped = 1;
	swapchainCreateInfo.compositeAlpha = settings.compositeAlphaFlag;
	swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment;
	swapchainCreateInfo.minImageCount = settings.numImages;
	swapchainCreateInfo.oldSwapchain = vk::SwapchainKHR{};

	swapchain.handle = device.createSwapchainKHR(swapchainCreateInfo);
	// Make name for the swapchainData
	if (debugUtils != nullptr)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkSwapchainKHR)swapchain.handle;
		nameInfo.objectType = swapchain.handle.objectType;
		std::string objectName = std::string("Swapchain #") + std::to_string(0);
		nameInfo.pObjectName = objectName.data();
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	u32 swapchainImageCount = 0;
	vkResult = device.getSwapchainImagesKHR(swapchain.handle, &swapchainImageCount, nullptr);
	if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
		throw std::runtime_error("Unable to grab swapchainData images from VkSwapchainKHR object.");
	DENGINE_GFX_ASSERT(swapchainImageCount != 0);
	DENGINE_GFX_ASSERT(swapchainImageCount == settings.numImages);
	if (swapchainImageCount > swapchain.images.Capacity())
		throw std::runtime_error("Unable to fit swapchainData image handles in allocated memory.");
	swapchain.images.Resize(swapchainImageCount);
	device.getSwapchainImagesKHR(swapchain.handle, &swapchainImageCount, swapchain.images.Data());

	// Make names for the swapchainData images
	if (debugUtils != nullptr)
	{
		for (uSize i = 0; i < swapchain.images.Size(); i++)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkImage)swapchain.images[i];
			nameInfo.objectType = swapchain.images[i].objectType;
			std::string objectName = std::string("Swapchain #0 - Image #") + std::to_string(i);
			nameInfo.pObjectName = objectName.data();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}
	}

	// Transition new swapchainData images
	TransitionSwapchainImages(device, deletionQueue, queues, swapchain.images);

	vk::CommandPoolCreateInfo cmdPoolInfo{};
	swapchain.cmdPool = device.createCommandPool(cmdPoolInfo);
	if (debugUtils != nullptr)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkCommandPool)swapchain.cmdPool;
		nameInfo.objectType = swapchain.cmdPool.objectType;
		std::string objectName = std::string("Swapchain #0 - Copy image CmdPool");
		nameInfo.pObjectName = objectName.data();
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}
	vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.commandPool = swapchain.cmdPool;
	cmdBufferAllocInfo.commandBufferCount = (u32)swapchain.images.Size();
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	swapchain.cmdBuffers.Resize(swapchain.images.Size());
	vkResult = device.allocateCommandBuffers(cmdBufferAllocInfo, swapchain.cmdBuffers.Data());
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("Failed to allocate VkCommandBuffers for copy-to-swapchainData.");
	// Give names to swapchainData command cmdBuffers
	if (debugUtils != nullptr)
	{
		for (uSize i = 0; i < swapchain.cmdBuffers.Size(); i++)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkCommandBuffer)swapchain.cmdBuffers[i];
			nameInfo.objectType = swapchain.cmdBuffers[i].objectType;
			std::string objectName = std::string("Swapchain #0 - Copy cmd buffer #0") + std::to_string(i);
			nameInfo.pObjectName = objectName.data();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}
	}

	vk::SemaphoreCreateInfo semaphoreInfo{};
	vk::ResultValue<vk::Semaphore> semaphoreResult = device.createSemaphore(semaphoreInfo);
	if (semaphoreResult.result != vk::Result::eSuccess)
		throw std::runtime_error("Unable to make semaphore.");
	swapchain.imageAvailableSemaphore = semaphoreResult.value;
	// Give name to the semaphore
	if (debugUtils != nullptr)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkSemaphore)swapchain.imageAvailableSemaphore;
		nameInfo.objectType = swapchain.imageAvailableSemaphore.objectType;
		std::string objectName = std::string("Swapchain #0 - Copy done semaphore");
		nameInfo.pObjectName = objectName.data();
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	return swapchain;
}

void DEngine::Gfx::Vk::Init::RecreateSwapchain(
	GlobUtils const& globUtils,
	SurfaceInfo& surface,
	SwapchainData& swapchain)
{
	// Query surface for new details
	vk::Result vkResult{};

	vk::SurfaceCapabilitiesKHR capabilities = globUtils.instance.getPhysicalDeviceSurfaceCapabilitiesKHR(
		globUtils.physDevice.handle, 
		surface.handle);
	surface.capabilities = capabilities;

	SwapchainSettings settings = {};
	settings.surface = surface.handle;
	settings.extents = surface.capabilities.currentExtent;
	settings.numImages = static_cast<std::uint32_t>(swapchain.images.Size());
	settings.presentMode = swapchain.presentMode;
	settings.surfaceFormat = swapchain.surfaceFormat;
	settings.capabilities = surface.capabilities;
	settings.transform = surface.capabilities.currentTransform;
	settings.compositeAlphaFlag = surface.compositeAlphaFlag;

	// We have figured out the settings to build the new swapchainData, now we actually make it

	swapchain.extents = settings.extents;
	swapchain.surfaceFormat = settings.surfaceFormat;
	swapchain.presentMode = settings.presentMode;

	vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageExtent = settings.extents;
	swapchainCreateInfo.imageFormat = settings.surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = settings.surfaceFormat.colorSpace;
	swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
	swapchainCreateInfo.presentMode = settings.presentMode;
	swapchainCreateInfo.surface = settings.surface;
	swapchainCreateInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	swapchainCreateInfo.clipped = 1;
	swapchainCreateInfo.compositeAlpha = settings.compositeAlphaFlag;
	swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment;
	swapchainCreateInfo.minImageCount = settings.numImages;
	swapchainCreateInfo.oldSwapchain = swapchain.handle;

	swapchain.handle = globUtils.device.createSwapchainKHR(swapchainCreateInfo);
	// Make name for the swapchainData
	if (globUtils.UsingDebugUtils())
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkSwapchainKHR)swapchain.handle;
		nameInfo.objectType = swapchain.handle.objectType;
		std::string objectName = std::string("Swapchain #0");
		nameInfo.pObjectName = objectName.data();
		globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
	}

	u32 swapchainImageCount = 0;
	vkResult = globUtils.device.getSwapchainImagesKHR(swapchain.handle, &swapchainImageCount, nullptr);
	if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
		throw std::runtime_error("Unable to grab swapchainData images from VkSwapchainKHR object.");
	globUtils.device.getSwapchainImagesKHR(swapchain.handle, &swapchainImageCount, swapchain.images.Data());
	// Make names for the swapchainData images
	if (globUtils.UsingDebugUtils())
	{
		for (uSize i = 0; i < swapchain.images.Size(); i++)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkImage)swapchain.images[i];
			nameInfo.objectType = swapchain.images[i].objectType;
			std::string objectName = std::string("Swapchain #0") + "- Image #0" + std::to_string(i);
			nameInfo.pObjectName = objectName.data();
			globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
		}
	}

	// Transition new swapchainData images
	Init::TransitionSwapchainImages(globUtils.device, globUtils.deletionQueue, globUtils.queues, swapchain.images);

	// Command cmdBuffers are not resettable, we deallocate the old ones and allocate new ones
	globUtils.device.FreeCommandBuffers(
		swapchain.cmdPool, 
		{ (u32)swapchain.cmdBuffers.Size(), swapchain.cmdBuffers.Data() });

	vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.commandPool = swapchain.cmdPool;
	cmdBufferAllocInfo.commandBufferCount = (u32)swapchain.images.Size();
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	vkResult = globUtils.device.allocateCommandBuffers(cmdBufferAllocInfo, swapchain.cmdBuffers.Data());
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("Vulkan: Failed to allocate VkCommandBuffers for copy-to-swapchainData.");
	// Give names to swapchainData command cmdBuffers
	if (globUtils.UsingDebugUtils())
	{
		for (uSize i = 0; i < swapchain.cmdBuffers.Size(); i++)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkCommandBuffer)swapchain.cmdBuffers[i];
			nameInfo.objectType = swapchain.cmdBuffers[i].objectType;
			std::string objectName = std::string("Swapchain #0 - Copy CmdBuffer #") + std::to_string(i);
			nameInfo.pObjectName = objectName.data();
			globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
		}
	}
}

bool DEngine::Gfx::Vk::Init::TransitionSwapchainImages(
	DeviceDispatch const& device,
	DeletionQueue const& deletionQueue,
	QueueData const& queues,
	Std::Span<const vk::Image> images)
{
	vk::Result vkResult{};

	vk::CommandPoolCreateInfo cmdPoolInfo{};
	vk::CommandPool cmdPool = device.createCommandPool(cmdPoolInfo);

	vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.commandBufferCount = 1;
	cmdBufferAllocInfo.commandPool = cmdPool;
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	vk::CommandBuffer cmdBuffer{};
	vkResult = device.allocateCommandBuffers(cmdBufferAllocInfo, &cmdBuffer);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("Vulkan: Unable to allocate command buffer when transitioning swapchainData images.");

	// Record commandbuffer
	{
		vk::CommandBufferBeginInfo cmdBeginInfo{};
		cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		device.beginCommandBuffer(cmdBuffer, cmdBeginInfo);

		for (size_t i = 0; i < images.Size(); i++)
		{
			vk::ImageMemoryBarrier imgBarrier{};
			imgBarrier.image = images[i];
			imgBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imgBarrier.subresourceRange.layerCount = 1;
			imgBarrier.subresourceRange.levelCount = 1;
			imgBarrier.oldLayout = vk::ImageLayout::eUndefined;
			imgBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
			imgBarrier.srcAccessMask = {};
			// We are going to be transferring onto the swapchain images.
			imgBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			device.cmdPipelineBarrier(
				cmdBuffer,
				vk::PipelineStageFlagBits::eTopOfPipe,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlags{},
				0, nullptr,
				0, nullptr,
				1, &imgBarrier);
		}

		device.endCommandBuffer(cmdBuffer);
	}

	vk::Fence fence = device.createFence({});

	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	queues.graphics.submit({ submitInfo }, fence);

	deletionQueue.Destroy(fence, cmdPool);

	return true;
}

vk::RenderPass DEngine::Gfx::Vk::Init::CreateGuiRenderPass(
		DeviceDispatch const& device,
		vk::Format swapchainFormat,
		DebugUtilsDispatch const* debugUtils)
{
	vk::AttachmentDescription colorAttachment{};
	colorAttachment.initialLayout = vk::ImageLayout::eTransferSrcOptimal;
	colorAttachment.finalLayout = vk::ImageLayout::eTransferSrcOptimal;
	colorAttachment.format = swapchainFormat;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	vk::AttachmentDescription attachments[1] = { colorAttachment };
	vk::AttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
	vk::SubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentRef;
	// Set up render pass
	vk::RenderPassCreateInfo createInfo{};
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = attachments;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpassDescription;

	vk::RenderPass renderPass = device.createRenderPass(createInfo);
	if (debugUtils != nullptr)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkRenderPass)renderPass;
		nameInfo.objectType = renderPass.objectType;
		nameInfo.pObjectName = "GUI RenderPass";
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	return renderPass;
}

DEngine::Gfx::Vk::GUIRenderTarget DEngine::Gfx::Vk::Init::CreateGUIRenderTarget(
	DeviceDispatch const& device,
	VmaAllocator vma,
	DeletionQueue const& deletionQueue,
	QueueData const& queues,
	vk::Extent2D swapchainDimensions,
	vk::Format swapchainFormat,
	vk::RenderPass renderPass,
	DebugUtilsDispatch const* debugUtils)
{
	vk::Result vkResult{};

	GUIRenderTarget returnVal{};
	returnVal.format = swapchainFormat;
	returnVal.extent = swapchainDimensions;

	// Allocate the rendertarget
	vk::ImageCreateInfo imgInfo{};
	imgInfo.arrayLayers = 1;
	imgInfo.extent = vk::Extent3D{ swapchainDimensions.width, swapchainDimensions.height, 1 };
	imgInfo.flags = vk::ImageCreateFlagBits{};
	imgInfo.format = swapchainFormat;
	imgInfo.imageType = vk::ImageType::e2D;
	imgInfo.initialLayout = vk::ImageLayout::eUndefined;
	imgInfo.mipLevels = 1;
	imgInfo.samples = vk::SampleCountFlagBits::e1;
	imgInfo.sharingMode = vk::SharingMode::eExclusive;
	imgInfo.tiling = vk::ImageTiling::eOptimal;
	imgInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;

	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.flags = 0;
	vmaAllocInfo.memoryTypeBits = 0;
	vmaAllocInfo.pool = 0;
	vmaAllocInfo.preferredFlags = 0;
	vmaAllocInfo.pUserData = nullptr;
	vmaAllocInfo.requiredFlags = 0;
	vmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;

	vkResult = (vk::Result)vmaCreateImage(
		vma,
		(VkImageCreateInfo const*)&imgInfo,
		&vmaAllocInfo,
		(VkImage*)&returnVal.img,
		&returnVal.vmaAllocation,
		nullptr);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: VMA was unable to allocate memory for GUI render-target.");

	if (debugUtils != nullptr)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkImage)returnVal.img;
		nameInfo.objectType = returnVal.img.objectType;
		nameInfo.pObjectName = "GUI Image";
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	vk::ImageViewCreateInfo imgViewInfo{};
	imgViewInfo.format = swapchainFormat;
	imgViewInfo.image = returnVal.img;
	imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imgViewInfo.subresourceRange.baseArrayLayer = 0;
	imgViewInfo.subresourceRange.baseMipLevel = 0;
	imgViewInfo.subresourceRange.layerCount = 1;
	imgViewInfo.subresourceRange.levelCount = 1;
	imgViewInfo.viewType = vk::ImageViewType::e2D;

	returnVal.imgView = device.createImageView(imgViewInfo);

	if (debugUtils != nullptr)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkImageView)returnVal.imgView;
		nameInfo.objectType = returnVal.imgView.objectType;
		nameInfo.pObjectName = "GUI ImageView";
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	vk::FramebufferCreateInfo fbInfo{};
	fbInfo.attachmentCount = 1;
	fbInfo.width = swapchainDimensions.width;
	fbInfo.height = swapchainDimensions.height;
	fbInfo.layers = 1;
	fbInfo.pAttachments = &returnVal.imgView;
	fbInfo.renderPass = renderPass;
	returnVal.framebuffer = device.createFramebuffer(fbInfo);
	if (debugUtils != nullptr)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkFramebuffer)returnVal.framebuffer;
		nameInfo.objectType = returnVal.framebuffer.objectType;
		nameInfo.pObjectName = "GUI Framebuffer";
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}


	// Transition the image
	vk::CommandPoolCreateInfo cmdPoolInfo{};
	cmdPoolInfo.queueFamilyIndex = 0;
	vk::CommandPool cmdPool = device.createCommandPool(cmdPoolInfo);
	vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.commandPool = cmdPool;
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	cmdBufferAllocInfo.commandBufferCount = 1;
	vk::CommandBuffer cmdBuffer{};
	vkResult = device.allocateCommandBuffers(cmdBufferAllocInfo, &cmdBuffer);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to allocate command buffer when transitioning GUI render-target.");

	vk::CommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	device.beginCommandBuffer(cmdBuffer, cmdBufferBeginInfo);

	vk::ImageMemoryBarrier imgMemoryBarrier{};
	imgMemoryBarrier.image = returnVal.img;
	imgMemoryBarrier.oldLayout = vk::ImageLayout::eUndefined;
	imgMemoryBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
	imgMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imgMemoryBarrier.subresourceRange.layerCount = 1;
	imgMemoryBarrier.subresourceRange.levelCount = 1;
	imgMemoryBarrier.srcAccessMask = vk::AccessFlagBits{};
	imgMemoryBarrier.dstAccessMask = vk::AccessFlagBits{};

	device.cmdPipelineBarrier(
		cmdBuffer,
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eBottomOfPipe,
		vk::DependencyFlags{},
		{},
		{},
		{ imgMemoryBarrier });

	device.endCommandBuffer(cmdBuffer);

	vk::FenceCreateInfo fenceInfo{};
	vk::Fence tempFence = device.createFence(fenceInfo);

	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	queues.graphics.submit({ submitInfo }, tempFence);
	
	deletionQueue.Destroy(tempFence, cmdPool);

	return returnVal;
}

DEngine::Gfx::Vk::GUIData DEngine::Gfx::Vk::Init::CreateGUIData(
	DeviceDispatch const& device,
	VmaAllocator vma,
	DeletionQueue const& deletionQueue,
	QueueData const& queues,
	vk::RenderPass guiRenderPass,
	vk::Format swapchainFormat,
	vk::Extent2D swapchainDimensions,
	u8 resourceSetCount,
	DebugUtilsDispatch const* debugUtils)
{
	vk::Result vkResult{};
	GUIData returnVal{};

	// CreateJob the commandbuffers
	{
		vk::CommandPoolCreateInfo cmdPoolInfo{};
		cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		returnVal.cmdPool = device.createCommandPool(cmdPoolInfo);
		if (debugUtils != nullptr)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkCommandPool)returnVal.cmdPool;
			nameInfo.objectType = returnVal.cmdPool.objectType;
			nameInfo.pObjectName = "GUI CmdPool";
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}

		vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
		cmdBufferAllocInfo.commandPool = returnVal.cmdPool;
		cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
		cmdBufferAllocInfo.commandBufferCount = resourceSetCount;
		returnVal.cmdBuffers.Resize(cmdBufferAllocInfo.commandBufferCount);
		vkResult = device.allocateCommandBuffers(cmdBufferAllocInfo, returnVal.cmdBuffers.Data());
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("Vulkan: Unable to allocate command cmdBuffers for GUI rendering.");
	}

	returnVal.renderTarget = CreateGUIRenderTarget(
		device, 
		vma, 
		deletionQueue,
		queues, 
		swapchainDimensions, 
		swapchainFormat, 
		guiRenderPass,
		debugUtils);

	return returnVal;
}

void DEngine::Gfx::Vk::Init::RecordSwapchainCmdBuffers(
	DeviceDispatch const& device,
	SwapchainData const& swapchainData,
	vk::Image srcImage)
{
	vk::Result vkResult{};

	for (uSize i = 0; i < swapchainData.cmdBuffers.Size(); i++)
	{
		vk::CommandBuffer cmdBuffer = swapchainData.cmdBuffers[i];
		vk::Image dstImage = swapchainData.images[i];

		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		device.beginCommandBuffer(cmdBuffer, beginInfo);

		// The renderpass handles transitioning the rendered image into transfer-src-optimal
		vk::ImageMemoryBarrier preTransfer_GuiBarrier{};
		preTransfer_GuiBarrier.image = srcImage;
		preTransfer_GuiBarrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		preTransfer_GuiBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		preTransfer_GuiBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		preTransfer_GuiBarrier.subresourceRange.layerCount = 1;
		preTransfer_GuiBarrier.subresourceRange.levelCount = 1;
		preTransfer_GuiBarrier.srcAccessMask = {};
		preTransfer_GuiBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
		device.cmdPipelineBarrier(
			cmdBuffer,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eTransfer,
			vk::DependencyFlags{},
			{},
			{},
			{ preTransfer_GuiBarrier });

		vk::ImageMemoryBarrier preTransfer_SwapchainBarrier{};
		preTransfer_SwapchainBarrier.image = dstImage;
		preTransfer_SwapchainBarrier.oldLayout = vk::ImageLayout::ePresentSrcKHR;
		preTransfer_SwapchainBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
		preTransfer_SwapchainBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		preTransfer_SwapchainBarrier.subresourceRange.layerCount = 1;
		preTransfer_SwapchainBarrier.subresourceRange.levelCount = 1;
		preTransfer_SwapchainBarrier.srcAccessMask = {};
		preTransfer_SwapchainBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
		device.cmdPipelineBarrier(
			cmdBuffer,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eTransfer,
			vk::DependencyFlags{},
			{},
			{},
			{ preTransfer_SwapchainBarrier });

		vk::ImageCopy copyRegion{};
		copyRegion.extent = vk::Extent3D{ swapchainData.extents.width, swapchainData.extents.height, 1 };
		copyRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		copyRegion.dstSubresource.layerCount = 1;
		device.cmdCopyImage(
			cmdBuffer,
			srcImage,
			vk::ImageLayout::eTransferSrcOptimal,
			dstImage,
			vk::ImageLayout::eTransferDstOptimal,
			{ copyRegion });

		vk::ImageMemoryBarrier postTransfer_SwapchainBarrier{};
		postTransfer_SwapchainBarrier.image = dstImage;
		postTransfer_SwapchainBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		postTransfer_SwapchainBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
		postTransfer_SwapchainBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		postTransfer_SwapchainBarrier.subresourceRange.layerCount = 1;
		postTransfer_SwapchainBarrier.subresourceRange.levelCount = 1;
		postTransfer_SwapchainBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		postTransfer_SwapchainBarrier.dstAccessMask = vk::AccessFlags{};
		device.cmdPipelineBarrier(
			cmdBuffer,
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::DependencyFlags{},
			{},
			{},
			{ postTransfer_SwapchainBarrier });

		// RenderPass handles transitioning rendered image back to color attachment output

		device.endCommandBuffer(cmdBuffer);
	}
}

vk::RenderPass DEngine::Gfx::Vk::Init::BuildMainGfxRenderPass(
	DeviceDispatch const& device,
	bool useEditorPipeline,
	DebugUtilsDispatch const* debugUtils)
{
	vk::AttachmentDescription colorAttach{};
	colorAttach.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttach.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttach.samples = vk::SampleCountFlagBits::e1;
	colorAttach.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttach.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttach.format = vk::Format::eR8G8B8A8Srgb;
	if (useEditorPipeline)
	{
		// We want to sample from the finalized image into the editor GUI
		colorAttach.initialLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		colorAttach.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	}
	else
	{
		// We want to transfer it to a swapchain image later
		colorAttach.initialLayout = vk::ImageLayout::eTransferSrcOptimal;
		colorAttach.finalLayout = vk::ImageLayout::eTransferSrcOptimal;
	}

	vk::AttachmentReference colorAttachRef{};
	colorAttachRef.attachment = 0;
	colorAttachRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpass{};
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachRef;

	vk::RenderPassCreateInfo rpInfo{};
	rpInfo.attachmentCount = 1;
	rpInfo.pAttachments = &colorAttach;
	rpInfo.subpassCount = 1;
	rpInfo.pSubpasses = &subpass;

	vk::RenderPass renderPass = device.createRenderPass(rpInfo);
	if (debugUtils)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkRenderPass)renderPass;
		nameInfo.objectType = renderPass.objectType;
		nameInfo.pObjectName = "Main Rendering RenderPass";
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	return renderPass;
}

void DEngine::Gfx::Vk::Init::TransitionGfxImage(
	DeviceDispatch const& device,
	DeletionQueue const& deletionQueue,
	QueueData const& queues,
	vk::Image img,
	bool useEditorPipeline)
{
	vk::Result vkResult{};

	vk::CommandPoolCreateInfo cmdPoolInfo{};
	vk::CommandPool cmdPool = device.createCommandPool(cmdPoolInfo);

	vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.commandBufferCount = 1;
	cmdBufferAllocInfo.commandPool = cmdPool;
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	vk::CommandBuffer cmdBuffer{};
	vkResult = device.allocateCommandBuffers(cmdBufferAllocInfo, &cmdBuffer);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("Unable to allocate command buffer when transitioning swapchainData images.");

	// Record commandbuffer
	{
		vk::CommandBufferBeginInfo cmdBeginInfo{};
		cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		device.beginCommandBuffer(cmdBuffer, cmdBeginInfo);
		vk::ImageMemoryBarrier imgBarrier{};
		imgBarrier.image = img;
		imgBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imgBarrier.subresourceRange.layerCount = 1;
		imgBarrier.subresourceRange.levelCount = 1;
		imgBarrier.oldLayout = vk::ImageLayout::eUndefined;
		// If we're in editor mode, we want to sample from the graphics viewport
		// into the editor's GUI pass.
		// If we're not in editor mode, we use a render-pass where this
		// is the image that gets copied onto the swapchain. That render-pass
		// requires the image to be in transferSrcOptimal layout.
		if (useEditorPipeline)
			imgBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		else
			imgBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		imgBarrier.srcAccessMask = {};
		// We want to write to the image as a render-target
		imgBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

		vk::PipelineStageFlags srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
		vk::PipelineStageFlags dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		device.cmdPipelineBarrier(
			cmdBuffer,
			srcStage,
			dstStage,
			vk::DependencyFlags{},
			0, nullptr,
			0, nullptr,
			1, &imgBarrier);

		device.endCommandBuffer(cmdBuffer);
	}

	vk::FenceCreateInfo fenceInfo{};
	vk::Fence fence = device.createFence(fenceInfo);

	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	queues.graphics.submit({ submitInfo }, fence);

	deletionQueue.Destroy(fence, cmdPool);
}

DEngine::Gfx::Vk::GfxRenderTarget DEngine::Gfx::Vk::Init::InitializeGfxViewportRenderTarget(
	GlobUtils const& globUtils,
	uSize viewportID,
	vk::Extent2D viewportSize)
{
	vk::Result vkResult{};

	GfxRenderTarget returnVal{};
	returnVal.extent = viewportSize;

	vk::ImageCreateInfo imageInfo{};
	imageInfo.arrayLayers = 1;
	imageInfo.extent = vk::Extent3D{ viewportSize.width, viewportSize.height, 1 };
	imageInfo.format = vk::Format::eR8G8B8A8Srgb;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.mipLevels = 1;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;
	imageInfo.tiling = vk::ImageTiling::eOptimal;
	imageInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
	if (globUtils.useEditorPipeline)
	{
		// We want to sample from the image to show it in the editor.
		imageInfo.usage |= vk::ImageUsageFlagBits::eSampled;
	}

	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	vmaAllocInfo.memoryTypeBits = 0;
	vmaAllocInfo.pool = 0;
	vmaAllocInfo.preferredFlags = 0;
	vmaAllocInfo.pUserData = nullptr;
	vmaAllocInfo.requiredFlags = 0;
	vmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;

	vkResult = (vk::Result)vmaCreateImage(
		globUtils.vma,
		(VkImageCreateInfo*)&imageInfo,
		&vmaAllocInfo,
		(VkImage*)&returnVal.img,
		&returnVal.vmaAllocation,
		nullptr);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Could not make VkImage through VMA when initializing virtual viewport.");

	if (globUtils.UsingDebugUtils())
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkImage)returnVal.img;
		nameInfo.objectType = returnVal.img.objectType;
		std::string name = std::string("Graphics viewport #") + std::to_string(viewportID) + " - Image";
		nameInfo.pObjectName = name.data();
		globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
	}

	// We have to transition this image
	TransitionGfxImage(
		globUtils.device, 
		globUtils.deletionQueue, 
		globUtils.queues, 
		returnVal.img, 
		globUtils.useEditorPipeline);

	// Make the image view
	vk::ImageViewCreateInfo imgViewInfo{};
	imgViewInfo.components.r = vk::ComponentSwizzle::eIdentity;
	imgViewInfo.components.g = vk::ComponentSwizzle::eIdentity;
	imgViewInfo.components.b = vk::ComponentSwizzle::eIdentity;
	imgViewInfo.components.a = vk::ComponentSwizzle::eIdentity;
	imgViewInfo.format = vk::Format::eR8G8B8A8Srgb;
	imgViewInfo.image = returnVal.img;
	imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imgViewInfo.subresourceRange.baseArrayLayer = 0;
	imgViewInfo.subresourceRange.baseMipLevel = 0;
	imgViewInfo.subresourceRange.layerCount = 1;
	imgViewInfo.subresourceRange.levelCount = 1;
	imgViewInfo.viewType = vk::ImageViewType::e2D;

	returnVal.imgView = globUtils.device.createImageView(imgViewInfo);
	if (globUtils.UsingDebugUtils())
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkImageView)returnVal.imgView;
		nameInfo.objectType = returnVal.imgView.objectType;
		std::string name = std::string("Graphics viewport #") + std::to_string(viewportID) + " - Image View";
		nameInfo.pObjectName = name.data();
		globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
	}

	vk::FramebufferCreateInfo fbInfo{};
	fbInfo.attachmentCount = 1;
	fbInfo.pAttachments = &returnVal.imgView;
	fbInfo.height = viewportSize.height;
	fbInfo.layers = 1;
	fbInfo.renderPass = globUtils.gfxRenderPass;
	fbInfo.width = viewportSize.width;
	returnVal.framebuffer = globUtils.device.createFramebuffer(fbInfo);
	if (globUtils.UsingDebugUtils())
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkFramebuffer)returnVal.framebuffer;
		nameInfo.objectType = returnVal.framebuffer.objectType;
		std::string name = std::string("Graphics viewport #") + std::to_string(viewportID) + " - Framebuffer";
		nameInfo.pObjectName = name.data();
		globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
	}

	return returnVal;
}

namespace DEngine::Gfx::Vk
{
	void imguiCheckVkResult(VkResult result)
	{
		if (static_cast<vk::Result>(result) != vk::Result::eSuccess)
			throw std::runtime_error("");
	}
}

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_vulkan.h"

void DEngine::Gfx::Vk::Init::InitializeImGui(
	APIData& apiData,
	DevDispatch const& device,
	PFN_vkGetInstanceProcAddr instanceProcAddr,
	DebugUtilsDispatch const* debugUtils)
{
	vk::Result vkResult{};

	// CreateJob the descriptor pool for ImGui to use
	VkDescriptorPoolSize pool_sizes[] =
	{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};
	vk::DescriptorPoolCreateInfo poolInfo{};
	poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	poolInfo.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
	poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
	poolInfo.pPoolSizes = (vk::DescriptorPoolSize const*)pool_sizes;
	vk::DescriptorPool imguiDescrPool = device.createDescriptorPool(poolInfo);
	if (debugUtils)
	{
		char const* name = "Dear ImGui - Descriptor Pool";

		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkDescriptorPool)imguiDescrPool;
		nameInfo.objectType = imguiDescrPool.objectType;
		nameInfo.pObjectName = name;
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	ImGui_ImplVulkan_InitInfo imguiInfo = {};
	imguiInfo.Instance = static_cast<VkInstance>(apiData.globUtils.instance.handle);
	imguiInfo.PhysicalDevice = static_cast<VkPhysicalDevice>(apiData.globUtils.physDevice.handle);
	imguiInfo.Device = static_cast<VkDevice>(apiData.globUtils.device.handle);
	imguiInfo.QueueFamily = 0;
	imguiInfo.Queue = static_cast<VkQueue>(apiData.globUtils.queues.graphics.Handle());
	imguiInfo.PipelineCache = 0;
	imguiInfo.DescriptorPool = static_cast<VkDescriptorPool>(imguiDescrPool);
	imguiInfo.Allocator = 0;
	imguiInfo.MinImageCount = 2;
	imguiInfo.ImageCount = 2;
	imguiInfo.CheckVkResultFn = &Vk::imguiCheckVkResult;
	imguiInfo.MSAASamples = static_cast<VkSampleCountFlagBits>(vk::SampleCountFlagBits::e1);
	imguiInfo.pfnVkGetInstanceProcAddr = instanceProcAddr;
	if (debugUtils)
		imguiInfo.useDebugUtils = true;

	bool imguiInitSuccess = ImGui_ImplVulkan_Init(&imguiInfo, static_cast<VkRenderPass>(apiData.globUtils.guiRenderPass));
	if (!imguiInitSuccess)
		throw std::runtime_error("Could not initialize the ImGui Vulkan stuff.");

	vk::CommandPoolCreateInfo cmdPoolInfo{};
	vk::CommandPool cmdPool = device.createCommandPool(cmdPoolInfo);

	vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.commandBufferCount = 1;
	cmdBufferAllocInfo.commandPool = cmdPool;
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	vk::CommandBuffer cmdBuffer{};
	vkResult = device.allocateCommandBuffers(cmdBufferAllocInfo, &cmdBuffer);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("Unable to allocate command buffer when initializing ImGui font texture");

	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	device.beginCommandBuffer(cmdBuffer, beginInfo);

	ImGui_ImplVulkan_CreateFontsTexture(static_cast<VkCommandBuffer>(cmdBuffer));

	device.endCommandBuffer(cmdBuffer);

	vk::Fence tempFence = apiData.globUtils.device.createFence({});
	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	apiData.globUtils.queues.graphics.submit({ submitInfo }, tempFence);
	
	vkResult = apiData.globUtils.device.waitForFences({ tempFence }, true, std::numeric_limits<std::uint64_t>::max());
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("Vulkan: Could not wait for fence after submitting ImGui create-fonts cmdBuffer.");
	device.Destroy(tempFence);
	device.Destroy(cmdPool);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}